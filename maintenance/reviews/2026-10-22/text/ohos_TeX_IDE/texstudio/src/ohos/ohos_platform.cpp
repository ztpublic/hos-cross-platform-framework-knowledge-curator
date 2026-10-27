#include "ohos_platform.h"
#include <cstdlib>
#include <cstdio>
#include <cerrno>
#include <string>
#include <climits>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <QDir>
#ifdef OHOS_FILE_LOG
#include "ohos_logger.h"
#endif
#include <QDebug>
#include <QProcess>
#include <QEventLoop>
#include <QTimer>
#include <QElapsedTimer>

namespace ohos {

static QString s_bundleCodeDir;
static QString s_filesDir;
static QString s_cacheDir;
static QString s_hnpBinDir;  // Discovered HNP bin path
static bool s_gitAvailable = false;     // Git availability flag
static QString s_gitBinPath;            // Detected git binary path
static QString s_bundleName;            // App bundle name for path conversion

// Forward declaration - defined after setupMiktexEnvironment()
static void detectGit();

void initPaths(const char* bundleCodeDir, const char* filesDir, const char* cacheDir)
{
    s_bundleCodeDir = QString::fromUtf8(bundleCodeDir);
    s_filesDir = QString::fromUtf8(filesDir);
    s_cacheDir = QString::fromUtf8(cacheDir);
    qDebug() << "[OHOS] bundleCodeDir:" << s_bundleCodeDir;
    qDebug() << "[OHOS] filesDir:" << s_filesDir;
    qDebug() << "[OHOS] cacheDir:" << s_cacheDir;

    // Ensure dlopen() can find OpenSSL and other native libraries in the app's libs directory.
    // Qt5's QNetworkAccessManager uses dlopen("libssl.so") at runtime for TLS.
    QString libPath = s_bundleCodeDir + "/libs/arm64";
    const char* oldLdPath = getenv("LD_LIBRARY_PATH");
    std::string newLdPath = libPath.toStdString();
    if (oldLdPath && oldLdPath[0] != '\0')
        newLdPath = newLdPath + ":" + oldLdPath;
    setenv("LD_LIBRARY_PATH", newLdPath.c_str(), 1);
    qDebug() << "[OHOS] LD_LIBRARY_PATH set:" << newLdPath.c_str();
}

QString filesDir()
{
    return s_filesDir;
}

QString cacheDir()
{
    return s_cacheDir;
}

QString userDataRoot()
{
    // This path (/data/storage/el2/base/files/texmfs/data) is the MiKTeX userData
    // root where stat() works (unlike filesDir — see P-04).
    // Defined in setupMiktexEnvironment() as well:
    //   userDataRoot = /data/storage/el2/base/files/texmfs/data
    // Using a hardcoded string since it's a well-known stable OHOS path.
    return QStringLiteral("/data/storage/el2/base/files/texmfs/data");
}

// ---- miktex HNP path prefix helpers (single source of truth, HNPPRIV-005) ----
// P-82: private layout reverse-engineered on-device; P-83: private-first
// discovery, public paths are last-resort fallback (WARNING when hit).
// These helpers only assemble path strings — no stat()/realpath() (P-04).

QStringList hnpLinkBinDirs()
{
    return QStringList{
        QStringLiteral("/data/app/bin"),         // private install.links (probed first, P-83)
        QStringLiteral("/data/service/hnp/bin"), // public, last-resort fallback
    };
}

QStringList hnpOrgBaseDirs()
{
    return QStringList{
        QStringLiteral("/data/app/miktex.org"), // private physical layout (T0-1)
        s_bundleCodeDir + "/hnp/miktex.org",
        QStringLiteral("/data/service/hnp/miktex.org"), // public, last resort
    };
}

QString hnpFallbackRoot()
{
    // Version pinned by P-47 — do not bump here.
    return QStringLiteral("/data/app/miktex.org/miktex_24.1.0");
}

QStringList hnpPersistedPathPrefixes()
{
    return QStringList{
        QStringLiteral("/data/service/hnp/"), // public wide prefix
        QStringLiteral("/data/app/"),         // private wide prefix
    };
}

QString hnpPublicSysBinDir()
{
    return QStringLiteral("/data/service/hnp/bin");
}

QString miktexBinPath()
{
    if (!s_hnpBinDir.isEmpty())
        return s_hnpBinDir;
    // Private HNP physical layout (T0-1). Version dir pinned by P-47.
    return hnpFallbackRoot() + "/bin";
}

QString miktexTexmfsPath()
{
    if (!s_hnpBinDir.isEmpty()) {
        // HNP bin is .../bin, texmfs is sibling
        QString parent = s_hnpBinDir;
        parent.chop(4); // remove "/bin"
        return parent + "/texmfs";
    }
    return hnpFallbackRoot() + "/texmfs";
}

// Helper: scan a directory using opendir/readdir (bypasses QDir stat issues)
static QStringList rawListDir(const char* path) {
    QStringList result;
    DIR* dir = opendir(path);
    if (!dir) return result;
    struct dirent* ent;
    while ((ent = readdir(dir)) != nullptr) {
        if (ent->d_name[0] == '.') continue;
        result.append(QString::fromUtf8(ent->d_name));
    }
    closedir(dir);
    return result;
}

// Normalize a symlink target without realpath() — realpath()'s internal
// stat() can fail in the OHOS sandbox (P-04). A relative target is resolved
// against the directory containing the link; ".." and duplicate slashes are
// collapsed lexically.
// T0-1 measured: readlink /data/app/bin/pdflatex returns the RELATIVE target
// "../miktex.org/miktex_24.1.0//bin/miktex-pdftex" which normalizes to
// /data/app/miktex.org/miktex_24.1.0/bin/miktex-pdftex
static QString normalizeHnpLinkTarget(const QString& linkDir, const QString& target) {
    QString joined = target.startsWith('/') ? target : linkDir + "/" + target;
    QStringList out;
    const QStringList parts = joined.split('/', QString::SkipEmptyParts);
    for (const QString& part : parts) {
        if (part == ".") continue;
        if (part == "..") {
            if (!out.isEmpty()) out.removeLast();
            continue;
        }
        out.append(part);
    }
    return "/" + out.join('/');
}

// Resolve HNP root by following symlink from /data/app/bin/<cmd> (private HNP,
// probed first) or /data/service/hnp/bin/<cmd> (public HNP, fallback only).
// Returns the HNP root (e.g. /data/app/miktex.org/miktex_24.1.0)
// or empty string if resolution fails.
static QString resolveHnpRootFromSymlink(FILE* diagFile) {
    const QStringList binDirs = hnpLinkBinDirs();
    const char* probes[] = {"pdflatex", "miktex-pdftex", "xelatex", "bibtex", nullptr};
    for (int d = 0; d < binDirs.size(); d++) {
        const QByteArray binDir = binDirs[d].toUtf8();
        for (int i = 0; probes[i]; i++) {
            char linkPath[512];
            snprintf(linkPath, sizeof(linkPath), "%s/%s", binDir.constData(), probes[i]);
            char resolved[PATH_MAX];
            ssize_t n = readlink(linkPath, resolved, sizeof(resolved) - 1);
            if (n > 0) {
                resolved[n] = '\0';
                if (diagFile) fprintf(diagFile, "  readlink(%s) = %s\n", linkPath, resolved);
                // resolved is like <hnpRoot>/bin/miktex-pdftex, but may be a
                // relative target (T0-1) — normalize it against the link dir.
                QString fullPath = normalizeHnpLinkTarget(binDirs[d],
                                                          QString::fromUtf8(resolved));
                if (fullPath != QString::fromUtf8(resolved) && diagFile)
                    fprintf(diagFile, "  normalized: %s\n", fullPath.toUtf8().constData());
                // Strip last two components to get HNP root
                int binIdx = fullPath.lastIndexOf("/bin/");
                if (binIdx > 0) {
                    QString hnpRoot = fullPath.left(binIdx);
                    QString hnpBin = hnpRoot + "/bin";
                    // Verify the bin dir via fopen() (P-04): readdir on the
                    // physical path returns 0 entries in the sandbox even when
                    // the files are accessible, so rawListDir is diag-only.
                    QStringList entries = rawListDir(hnpBin.toUtf8().constData());
                    QString probePath = hnpBin + "/miktex-pdftex";
                    FILE* probe = fopen(probePath.toUtf8().constData(), "rb");
                    if (diagFile) fprintf(diagFile, "  HNP root from symlink: %s (bin has %d entries, fopen(miktex-pdftex)=%s)\n",
                                          hnpRoot.toUtf8().constData(), entries.count(),
                                          probe ? "OK" : "FAIL");
                    if (probe) {
                        fclose(probe);
                        if (d > 0 && diagFile)
                            fprintf(diagFile, "  WARNING: fell back to public HNP root via %s\n", binDir.constData());
                        return hnpRoot;
                    }
                }
            }
        }
    }
    return QString();
}

// Scan miktex.org base dirs for version directories.
// Private HNP physical base /data/app/miktex.org (T0-1 measured layout) first;
// /data/service/hnp/miktex.org (public) is kept as the last-resort fallback
// and emits a WARNING diag line when hit (R1 anti-false-success).
// The sandbox returns 0 readdir entries on physical paths even when files are
// accessible (T0-1), so all validation uses fopen() (P-04); when enumeration
// is empty we directly probe the pinned miktex_24.1.0 version dir (P-47).
static QString scanMiktexOrgDir(FILE* diagFile) {
    const QStringList bases = hnpOrgBaseDirs();
    const QString publicBase = bases.last(); // public, last resort
    for (const auto& base : bases) {
        const bool isPublic = (base == publicBase);
        QStringList entries = rawListDir(base.toUtf8().constData());
        if (diagFile) fprintf(diagFile, "  Scanning %s: %d entries\n",
                              base.toUtf8().constData(), entries.count());
        for (const auto& entry : entries) {
            if (entry.startsWith("miktex_")) {
                QString candidate = QString("%1/%2").arg(base).arg(entry);
                QString binProbe = candidate + "/bin/miktex-pdftex";
                QStringList binEntries = rawListDir((candidate + "/bin").toUtf8().constData());
                FILE* probe = fopen(binProbe.toUtf8().constData(), "rb");
                if (diagFile) fprintf(diagFile, "    %s -> bin has %d entries, fopen(miktex-pdftex)=%s\n",
                                      entry.toUtf8().constData(), binEntries.count(),
                                      probe ? "OK" : "FAIL");
                if (probe) {
                    fclose(probe);
                    if (isPublic && diagFile)
                        fprintf(diagFile, "  WARNING: fell back to public HNP root via %s\n",
                                base.toUtf8().constData());
                    return candidate;
                }
            }
        }
        // readdir empty does NOT mean absent (sandbox restriction) — directly
        // probe the pinned version dir via fopen() (version dir name comes from
        // hnpFallbackRoot() so P-47 pinning stays single-point).
        if (entries.isEmpty()) {
            QString candidate = base + "/" + hnpFallbackRoot().section('/', -1);
            QString binProbe = candidate + "/bin/miktex-pdftex";
            FILE* probe = fopen(binProbe.toUtf8().constData(), "rb");
            if (diagFile) fprintf(diagFile, "    readdir empty, direct probe %s -> %s\n",
                                  binProbe.toUtf8().constData(), probe ? "OK" : "FAIL");
            if (probe) {
                fclose(probe);
                if (isPublic && diagFile)
                    fprintf(diagFile, "  WARNING: fell back to public HNP root via %s\n",
                            base.toUtf8().constData());
                return candidate;
            }
        }
    }
    return QString();
}

// 首启后台初始化进程（initexmf --dump=latex / --mkmaps）启动辅助：
// 注入 MIKTEX_OHOS_QUIET_PKG_EVENTS=1，让这些进程触发的宏包安装不写
// miktex_events.log —— 首启的格式构建依赖安装是一次性环境准备，
// 不应在日志区弹出一串"宏包下载"提示（用户主动编译触发的安装不受影响）。
static void ohosStartDetachedQuiet(const QString &program, const QStringList &args)
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("MIKTEX_OHOS_QUIET_PKG_EVENTS"), QStringLiteral("1"));
    QProcess proc;
    proc.setProcessEnvironment(env);
    proc.setProgram(program);
    proc.setArguments(args);
    proc.startDetached();
}

void setupMiktexEnvironment()
{
    QElapsedTimer envTimer; // [PERF] 启动耗时细分观测
    envTimer.start();
    // Write diagnostics to file (hilog drops messages due to rate limiting)
    QString diagPath = s_filesDir + "/hnp_diag.txt";
    FILE* diagFile = fopen(diagPath.toUtf8().constData(), "w");

    if (diagFile) fprintf(diagFile, "=== HNP Diagnostic ===\nbundleCodeDir: %s\nfilesDir: %s\n\n",
                          s_bundleCodeDir.toUtf8().constData(), s_filesDir.toUtf8().constData());

    // ---- T0 probe: private HNP layout observation ----
    if (diagFile) {
        const QByteArray linkBin0 = hnpLinkBinDirs().first().toUtf8();
        fprintf(diagFile, "=== T0 probe: %s listing ===\n", linkBin0.constData());
        QStringList appBinEntries = rawListDir(linkBin0.constData());
        fprintf(diagFile, "  %s: %d entries\n", linkBin0.constData(), appBinEntries.count());
        for (int i = 0; i < qMin(appBinEntries.count(), 30); i++)
            fprintf(diagFile, "    %s\n", appBinEntries[i].toUtf8().constData());

        fprintf(diagFile, "\n=== T0 probe: readlink %s/<cmd> ===\n", linkBin0.constData());
        const char* probeCmds[] = {"pdflatex", "miktex-pdftex", "xelatex", "bibtex", nullptr};
        for (int i = 0; probeCmds[i]; i++) {
            char linkPath[512];
            snprintf(linkPath, sizeof(linkPath), "%s/%s", linkBin0.constData(), probeCmds[i]);
            char resolved[PATH_MAX];
            errno = 0;
            ssize_t n = readlink(linkPath, resolved, sizeof(resolved) - 1);
            if (n > 0) {
                resolved[n] = '\0';
                fprintf(diagFile, "  readlink(%s) = %s\n", linkPath, resolved);
            } else {
                fprintf(diagFile, "  readlink(%s) failed: errno=%d\n", linkPath, errno);
            }
        }

        fprintf(diagFile, "\n=== T0 probe: environment variables ===\n");
        const char* envNames[] = {"HNP_PRIVATE_HOME", "HNP_PUBLIC_HOME", "PATH", nullptr};
        for (int i = 0; envNames[i]; i++) {
            const char* val = getenv(envNames[i]);
            fprintf(diagFile, "  %s = %s\n", envNames[i], val ? val : "(null)");
        }

        fprintf(diagFile, "\n=== T0 probe: private physical path candidates ===\n");
        const QStringList orgBases = hnpOrgBaseDirs();
        const QStringList probeCandidates = {
            orgBases.value(0), // private physical layout (T0-1 measured)
            QStringLiteral("/data/app/el1/bundle/com.develop.opensource.texide/hnp/miktex.org"),
            orgBases.value(1),
            s_bundleCodeDir + "/hnp",
            orgBases.value(2),
        };
        for (const auto& candidate : probeCandidates) {
            QStringList entries = rawListDir(candidate.toUtf8().constData());
            fprintf(diagFile, "  %s -> %d entries\n", candidate.toUtf8().constData(), entries.count());
        }

        const QByteArray publicOrgBase = hnpOrgBaseDirs().last().toUtf8();
        fprintf(diagFile, "\n=== T0 probe: public residue %s ===\n", publicOrgBase.constData());
        QStringList residue = rawListDir(publicOrgBase.constData());
        fprintf(diagFile, "  %s: %d entries\n", publicOrgBase.constData(), residue.count());
        for (const auto& entry : residue)
            fprintf(diagFile, "    %s\n", entry.toUtf8().constData());
        fprintf(diagFile, "\n");
    }

    // ---- Step 1: Discover HNP root via symlink resolution ----
    QString hnpRoot;
    QString hnpBinDir;

    if (diagFile) fprintf(diagFile, "=== HNP root discovery via symlink ===\n");
    hnpRoot = resolveHnpRootFromSymlink(diagFile);

    // Fallback: scan miktex.org base dirs (private candidates first) for version dirs
    if (hnpRoot.isEmpty()) {
        if (diagFile) fprintf(diagFile, "\n=== Fallback: scan miktex.org dir ===\n");
        hnpRoot = scanMiktexOrgDir(diagFile);
    }

    // Fallback: try static candidate list
    if (hnpRoot.isEmpty()) {
        if (diagFile) fprintf(diagFile, "\n=== Fallback: static candidate list ===\n");
        // Single measured private-layout candidate (T0-1); version pinned (P-47).
        // The old .hnp/miktex guesses never matched any real layout (dead code, R9).
        // Validation via fopen() (P-04) since sandbox readdir returns 0 entries.
        const QString candidate = hnpFallbackRoot();
        const QString probePath = candidate + "/bin/miktex-pdftex";
        FILE* probe = fopen(probePath.toUtf8().constData(), "rb");
        if (diagFile) fprintf(diagFile, "  %s -> fopen=%s\n",
                              probePath.toUtf8().constData(), probe ? "OK" : "FAIL");
        if (probe) {
            fclose(probe);
            hnpRoot = candidate;
        }
    }

    if (!hnpRoot.isEmpty()) {
        hnpBinDir = hnpRoot + "/bin";
        if (diagFile) fprintf(diagFile, "\n>>> HNP root: %s\n>>> HNP bin: %s\n",
                              hnpRoot.toUtf8().constData(), hnpBinDir.toUtf8().constData());
    } else {
        hnpBinDir = hnpFallbackRoot() + "/bin"; // last-resort fallback
        hnpRoot = hnpFallbackRoot();
        if (diagFile) fprintf(diagFile, "\nWARNING: No HNP root found, using fallback: %s\n",
                              hnpRoot.toUtf8().constData());
    }

    // Store discovered path for miktexBinPath() / miktexTexmfsPath()
    s_hnpBinDir = hnpBinDir;

    // ---- R2: hnpRoot change detection -> FNDB refresh ----
    // FNDB filename = MD5(hnpRoot absolute path): a changed root orphans the
    // old FNDB, so refresh it once (async/detached, quiet). Also clear the
    // fontconfig cache (stale dir records from the old root). Every step is
    // warn-only: failures must not break the main startup flow.
    {
        const QString recordPath = s_filesDir + "/hnp_root.txt";
        QString lastRoot;
        FILE* rf = fopen(recordPath.toUtf8().constData(), "r");
        if (rf) {
            char buf[512];
            size_t n = fread(buf, 1, sizeof(buf) - 1, rf);
            fclose(rf);
            buf[n] = '\0';
            lastRoot = QString::fromUtf8(buf).trimmed();
        }
        if (lastRoot != hnpRoot) {
            const QString lastShown = lastRoot.isEmpty() ? QStringLiteral("(none)") : lastRoot;
            ohos::log("Startup", QStringLiteral("hnpRoot changed: %1 -> %2, refreshing FNDB")
                      .arg(lastShown, hnpRoot));
            if (diagFile) fprintf(diagFile, "\n>>> hnpRoot changed: %s -> %s (refreshing FNDB)\n",
                                  lastShown.toUtf8().constData(), hnpRoot.toUtf8().constData());
            // a) async FNDB refresh (MiKTeX install lock guards concurrency)
            ohosStartDetachedQuiet(hnpBinDir + "/initexmf", QStringList() << "--update-fndb");
            // b) clear fontconfig cache files (userDataRoot: stat works there)
            QDir fcCacheDir(userDataRoot() + "/fontconfig/cache");
            const QStringList cacheFiles = fcCacheDir.entryList(QDir::Files | QDir::NoDotAndDotDot);
            int removedCount = 0;
            for (const QString& fn : cacheFiles) {
                if (QFile::remove(fcCacheDir.absoluteFilePath(fn)))
                    removedCount++;
            }
            ohos::log("Startup", QStringLiteral("fontconfig cache cleared on hnpRoot change: %1/%2 files removed")
                      .arg(removedCount).arg(cacheFiles.count()));
            // c) persist the new root record
            FILE* wf = fopen(recordPath.toUtf8().constData(), "w");
            if (wf) {
                QByteArray data = hnpRoot.toUtf8();
                size_t written = fwrite(data.constData(), 1, data.size(), wf);
                fclose(wf);
                if (static_cast<size_t>(written) != static_cast<size_t>(data.size()))
                    ohos::logWarn("Startup", QStringLiteral("hnp_root.txt write incomplete: %1").arg(recordPath));
            } else {
                ohos::logWarn("Startup", QStringLiteral("hnp_root.txt write failed: %1").arg(recordPath));
            }
        }
    }

    ohos::logDebug("Startup", QStringLiteral("[PERF] miktexEnv: HNP probing: %1 ms").arg(envTimer.elapsed()));

    // Scan /data/app/bin/ (private HNP install.links symlinks) as the primary
    // system bin; also scan public /data/service/hnp/bin for comparison (diag only).
    QString hnpSystemBin = hnpLinkBinDirs().first();
    QStringList hnpSysBinEntries = rawListDir(hnpSystemBin.toUtf8().constData());
    const QByteArray publicSysBin = hnpPublicSysBinDir().toUtf8();
    QStringList hnpPublicSysBinEntries = rawListDir(publicSysBin.constData());
    if (diagFile) {
        fprintf(diagFile, "\n=== HNP system bin: %s (%d entries) ===\n",
                hnpSystemBin.toUtf8().constData(), hnpSysBinEntries.count());
        for (int i = 0; i < qMin(hnpSysBinEntries.count(), 30); i++)
            fprintf(diagFile, "    %s\n", hnpSysBinEntries[i].toUtf8().constData());

        fprintf(diagFile, "\n=== HNP public system bin (comparison): %s (%d entries) ===\n",
                publicSysBin.constData(), hnpPublicSysBinEntries.count());
        for (int i = 0; i < qMin(hnpPublicSysBinEntries.count(), 30); i++)
            fprintf(diagFile, "    %s\n", hnpPublicSysBinEntries[i].toUtf8().constData());

        fprintf(diagFile, "\n=== Environment setup ===\n");
        fprintf(diagFile, "MIKTEX_HNP_BIN = %s\n", hnpBinDir.toUtf8().constData());
        fprintf(diagFile, "texmfsPath = %s\n", miktexTexmfsPath().toUtf8().constData());

        // Test buildmanager resolution for key commands
        fprintf(diagFile, "\n=== BuildManager resolution test ===\n");
        const char* cmds[] = {"pdflatex", "lualatex", "bibtex", "makeindex", "miktex-pdftex", "xelatex", "miktex-gs", nullptr};
        for (int i = 0; cmds[i]; i++) {
            QString cmd = QString::fromUtf8(cmds[i]);
            QString p1 = hnpBinDir + "/" + cmd;
            bool f1 = fileExists(p1);
            QString p2 = hnpSystemBin + "/" + cmd;
            bool f2 = fileExists(p2);
            fprintf(diagFile, "  %s: hnpBin=%d sysBin=%d\n", cmds[i], f1 ? 1 : 0, f2 ? 1 : 0);
            if (f1) fprintf(diagFile, "    -> %s\n", p1.toUtf8().constData());
            else if (f2) fprintf(diagFile, "    -> %s\n", p2.toUtf8().constData());
        }

        fprintf(diagFile, "\n=== Done ===\n");
        fclose(diagFile);
    }

    qDebug() << "[OHOS] HNP root discovered:" << hnpRoot;
    qDebug() << "[OHOS] HNP bin:" << hnpBinDir;
    ohos::logDebug("Startup", QStringLiteral("[PERF] miktexEnv: diag+sysbin scan: %1 ms").arg(envTimer.elapsed()));

    // PATH: HNP bin dir + private/public system HNP bin (install.links symlinks)
    std::string hnpBin = hnpBinDir.toStdString();
    std::string sysBin = hnpSystemBin.toStdString();
    const char* oldPath = getenv("PATH");
    std::string newPath = hnpBin + ":" + sysBin + ":" + hnpPublicSysBinDir().toStdString()
                        + (oldPath ? std::string(":") + oldPath : "");
    setenv("PATH", newPath.c_str(), 1);

    // MIKTEX_HNP_BIN: point to actual HNP bin where miktex-pdftex etc. live
    setenv("MIKTEX_HNP_BIN", hnpBin.c_str(), 1);

    // HOME: point to sandbox files dir (QStandardPaths depends on this)
    setenv("HOME", s_filesDir.toStdString().c_str(), 1);

    // XDG directories for Qt5 QStandardPaths
    setenv("XDG_DATA_HOME", s_filesDir.toStdString().c_str(), 1);
    setenv("XDG_CONFIG_HOME", s_filesDir.toStdString().c_str(), 1);
    setenv("XDG_CACHE_HOME", s_cacheDir.toStdString().c_str(), 1);

    // TMPDIR: MiKTeX needs writable temp directory
    setenv("TMPDIR", s_filesDir.toStdString().c_str(), 1);

    // Ghostscript resources live inside the embedded MiKTeX HNP.
    QString gsRoot = hnpRoot + "/ghostscript";
    QString gsLib = gsRoot + "/Resource/Init:" + gsRoot + "/lib:" + gsRoot + "/Resource/Font";
    setenv("GS_LIB", gsLib.toStdString().c_str(), 1);
    setenv("GS_FONTPATH", (gsRoot + "/Resource/Font").toStdString().c_str(), 1);

    // ---- FONTCONFIG SETUP ----
    ohos::logDebug("Startup", QStringLiteral("[PERF] miktexEnv: setenv block: %1 ms").arg(envTimer.elapsed()));
    // MiKTeX's bundled fontconfig ignores FONTCONFIG_FILE/FONTCONFIG_PATH env vars.
    // It searches for fonts.conf in TEXMF root directories + ConfigRoot.
    // From ohosStartupConfig.cpp:
    //   userConfigRoot  = /data/storage/el2/base/files/texmfs/config
    //   userDataRoot    = /data/storage/el2/base/files/texmfs/data
    //   commonInstall   = <hnpRoot>/texmfs/install (has fonts.conf.in template only)
    //
    // We must write fonts.conf to: <userConfigRoot>/fontconfig/config/fonts.conf
    // so that MiKTeX's fontconfig finds it during FcConfigGetPath() search.

    static const char* OHOS_USER_STORAGE = "/data/storage/el2/base/files";
    QString userConfigRoot = QString("%1/texmfs/config").arg(OHOS_USER_STORAGE);
    QString userDataRoot   = QString("%1/texmfs/data").arg(OHOS_USER_STORAGE);
    QString commonInstall  = hnpRoot + "/texmfs/install";

    // Target: MiKTeX ConfigRoot fontconfig directory
    QString fcDir = userConfigRoot + "/fontconfig/config";
    QString fcConfFile = fcDir + "/fonts.conf";
    QString localFontsConf = fcDir + "/localfonts.conf";
    QString fcTemplate = commonInstall + "/fontconfig/config/fonts.conf.in";
    QString fcCacheDir = userDataRoot + "/fontconfig/cache";

    // Font directories in HNP common install
    QString otfDir = commonInstall + "/fonts/opentype";
    QString ttfDir = commonInstall + "/fonts/truetype";
    // Also check user install root (packages downloaded at runtime)
    QString userInstallRoot = QString("%1/texmfs/install").arg(OHOS_USER_STORAGE);
    QString userOtfDir = userInstallRoot + "/texmf/fonts/opentype";
    QString userTtfDir = userInstallRoot + "/texmf/fonts/truetype";

    QDir().mkpath(fcDir);
    QDir().mkpath(fcCacheDir);

    fprintf(stderr, "[OHOS] fontconfig: target dir=%s\n", fcDir.toUtf8().constData());
    fprintf(stderr, "[OHOS] fontconfig: template=%s\n", fcTemplate.toUtf8().constData());
    // ohos::log diagnostic — confirm fontconfig setup reached
    ohos::log("fontconfig", QString("setup: fcDir=%1 fontsDir=%2/fonts")
              .arg(fcDir).arg(s_filesDir));

    // Always regenerate fonts.conf (paths may change between HNP versions)
    {
        bool generated = false;
        // Try from template first
        FILE *tf = fopen(fcTemplate.toUtf8().constData(), "r");
        if (tf) {
            fseek(tf, 0, SEEK_END);
            long sz = ftell(tf);
            fseek(tf, 0, SEEK_SET);
            QByteArray tmpl(sz, '\0');
            fread(tmpl.data(), 1, sz, tf);
            fclose(tf);

            QString content = QString::fromUtf8(tmpl);
            // Replace template placeholders with actual OHOS paths
            content.replace("@MIKTEX_CONFIG@", userConfigRoot);
            content.replace("@MIKTEX_INSTALL@", commonInstall);
            content.replace("@MIKTEX_DATA@", userDataRoot);

            FILE *of = fopen(fcConfFile.toUtf8().constData(), "w");
            if (of) {
                QByteArray out = content.toUtf8();
                size_t written = fwrite(out.constData(), 1, out.size(), of);
                fclose(of);
                if (static_cast<ssize_t>(written) == out.size()) {
                    generated = true;
                    fprintf(stderr, "[OHOS] fontconfig: generated from template (%ld bytes -> %d bytes)\n",
                            sz, out.size());
                } else {
                    // Incomplete write (e.g. disk full), clean up partial file
                    QFile::remove(fcConfFile);
                    fprintf(stderr, "[OHOS] fontconfig: incomplete write (%zu/%d bytes)\n",
                            written, out.size());
                }
            }
        } else {
            fprintf(stderr, "[OHOS] fontconfig: template not found at %s (errno=%d)\n",
                    fcTemplate.toUtf8().constData(), errno);
        }

        // Fallback: generate minimal fonts.conf directly
        if (!generated) {
            FILE *of = fopen(fcConfFile.toUtf8().constData(), "w");
            if (of) {
                fprintf(of, "<?xml version=\"1.0\"?>\n");
                fprintf(of, "<!DOCTYPE fontconfig SYSTEM \"fonts.dtd\">\n");
                fprintf(of, "<fontconfig>\n");
                fprintf(of, "  <include>%s</include>\n", localFontsConf.toUtf8().constData());
                fprintf(of, "  <cachedir>%s</cachedir>\n", fcCacheDir.toUtf8().constData());
                fprintf(of, "  <config><rescan><int>30</int></rescan></config>\n");
                fprintf(of, "</fontconfig>\n");
                fclose(of);
                generated = true;
                fprintf(stderr, "[OHOS] fontconfig: generated fallback minimal fonts.conf\n");
            }
        }
        fprintf(stderr, "[OHOS] fontconfig: fonts.conf %s at %s\n",
                generated ? "OK" : "FAILED", fcConfFile.toUtf8().constData());
    }

    // Generate localfonts.conf with correct OHOS font paths
    {
        FILE *wf = fopen(localFontsConf.toUtf8().constData(), "w");
        if (wf) {
            fprintf(wf, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
            fprintf(wf, "<!-- [FIX-A Bug #2448] System fonts first for CJK priority -->\n");
            fprintf(wf, "<fontconfig>\n");
            // System fonts first — highest priority for CJK glyph resolution (HarmonyOS Sans SC etc.)
            fprintf(wf, "<dir>/system/fonts</dir>\n");
            fprintf(wf, "<dir>%s</dir>\n", otfDir.toUtf8().constData());
            fprintf(wf, "<dir>%s</dir>\n", ttfDir.toUtf8().constData());
            fprintf(wf, "<dir>%s</dir>\n", userOtfDir.toUtf8().constData());
            fprintf(wf, "<dir>%s</dir>\n", userTtfDir.toUtf8().constData());
            // Fandol Chinese fonts extracted from HAP rawfile for ctex support
            fprintf(wf, "<dir>%s/fonts</dir>\n", s_filesDir.toUtf8().constData());
            fprintf(wf, "</fontconfig>\n");
            fclose(wf);
            fprintf(stderr, "[OHOS] fontconfig: localfonts.conf generated at %s\n",
                    localFontsConf.toUtf8().constData());
            ohos::log("fontconfig", QString("localfonts.conf written with s_filesDir=%1/fonts")
                      .arg(s_filesDir));
        }
    }

    // Ensure config directory exists
    QString configDir = s_filesDir + "/config";
    QDir().mkpath(configDir);

    qDebug() << "[OHOS] PATH set, HNP bin:" << hnpBin.c_str() << ", sys bin:" << sysBin.c_str();
    qDebug() << "[OHOS] HOME:" << s_filesDir;
    qDebug() << "[OHOS] fontconfig target:" << fcDir;

    // Detect git availability (e.g. from GitNext HNP public installation)
    ohos::logDebug("Startup", QStringLiteral("[PERF] miktexEnv: fontconfig rewrite: %1 ms").arg(envTimer.elapsed()));
    detectGit();
    ohos::logDebug("Startup", QStringLiteral("[PERF] miktexEnv: detectGit: %1 ms").arg(envTimer.elapsed()));
}

// 首帧后执行的 MiKTeX 维护检查（启动白屏优化：实测两检查块在 GUI 线程花 ~45ms，
// 其中 startDetached 的 fork+exec 是大头；均为一次性维护任务，无需在 QApplication 前完成）
void miktexPostStartupMaintenance()
{
    static const char* OHOS_USER_STORAGE = "/data/storage/el2/base/files";
    QString userInstallRoot = QString("%1/texmfs/install").arg(OHOS_USER_STORAGE);

    // Build latex.fmt format if not present (needed for MetaPost btex...etex labels).
    // initexmf --dump=latex takes ~10s on first run, skip if format already exists.
    {
        QString latexFmtPath = QString("%1/texmfs/data/miktex/data/le/pdftex/latex.fmt").arg(OHOS_USER_STORAGE);
        FILE* checkFmt = fopen(latexFmtPath.toUtf8().constData(), "rb");
        if (!checkFmt) {
            qDebug() << "[OHOS] latex.fmt not found at" << latexFmtPath << ", building...";
            ohosStartDetachedQuiet("initexmf", QStringList() << "--dump=latex");
        } else {
            fclose(checkFmt);
            qDebug() << "[OHOS] latex.fmt already exists, skipping build";
        }
    }

    // Auto-generate font map files (pdftex.map, etc.) via initexmf --mkmaps
    // Uses fopen() to check existence (P-04 compliance)
    {
        QString mapPath = userInstallRoot + "/texmf/fonts/map/pdftex/pdftex.map";
        FILE* mapFile = fopen(mapPath.toUtf8().constData(), "rb");
        if (!mapFile) {
            qDebug() << "[OHOS] pdftex.map not found, running initexmf --mkmaps...";
            ohosStartDetachedQuiet("initexmf", QStringList() << "--mkmaps");
            qDebug() << "[OHOS] initexmf --mkmaps started (async)";
        } else {
            fclose(mapFile);
            qDebug() << "[OHOS] pdftex.map exists, skipping font map generation";
        }
    }
}

// gitNext bundle name
static const char* GITNEXT_BUNDLE_NAME = "com.develop.opensource.gitnext";

// Known paths where gitNext may install its git binary
static const QStringList gitNextCandidatePaths = {
    "/data/service/hnp/bin/git",
    QString("/data/app/el1/bundle/public/%1/libs/arm64-v8a/git").arg(GITNEXT_BUNDLE_NAME),
    QString("/data/service/hnp/%1/git.hnp/bin/git").arg(GITNEXT_BUNDLE_NAME),
};

static void detectGit()
{
    s_gitAvailable = false;
    s_gitBinPath.clear();

    // On OHOS, HNP binaries (like git from GitNext) live under /data/service/hnp/
    // which is not accessible via fopen()/access() from the app process (sandbox
    // restrictions). However, QProcess-spawned children CAN execute them because
    // PATH includes /data/service/hnp/bin. So we detect git by checking if "git"
    // appears in PATH directories, without verifying file existence via fopen.

    // Strategy 1: Check well-known HNP paths (may work for some paths)
    for (const QString& path : gitNextCandidatePaths) {
        if (fileExists(path)) {
            s_gitAvailable = true;
            s_gitBinPath = path;
            break;
        }
    }

    // Strategy 2: Check if "git" is in any PATH directory
    if (!s_gitAvailable) {
        const char* pathEnv = getenv("PATH");
        if (pathEnv) {
            QStringList dirs = QString::fromUtf8(pathEnv).split(':');
            for (const QString& dir : dirs) {
                // Only trust the system HNP bin directory (/data/service/hnp/bin)
                // where GitNext installs its git binary. Do NOT match MiKTeX or
                // other app-specific HNP directories (they don't contain git).
                if (dir == "/data/service/hnp/bin") {
                    QString candidate = dir + "/git";
                    // Trust the PATH entry: if /data/service/hnp/bin is in PATH,
                    // HNP manager installed git there. fopen can't verify but
                    // QProcess will find it.
                    s_gitAvailable = true;
                    s_gitBinPath = candidate;
                    break;
                }
            }
        }
    }

    // Strategy 3: Check ArkTS signal file
    if (!s_gitAvailable) {
        QString signalFile = s_filesDir + "/gitnext_path";
        FILE* f = fopen(signalFile.toUtf8().constData(), "r");
        if (f) {
            char buf[512];
            size_t n = fread(buf, 1, sizeof(buf) - 1, f);
            fclose(f);
            buf[n] = '\0';
            QString path = QString::fromUtf8(buf).trimmed();
            if (!path.isEmpty() && fileExists(path)) {
                s_gitAvailable = true;
                s_gitBinPath = path;
            }
        }
    }

    // ---- Git config strategy ----
    // After ArkTS activates fileShare permission for /storage/Users/currentUser,
    // this process can access .ohos_git/.gitconfig. On second+ launch the permission
    // is activated before C++ starts (via persisted URI). On first launch, ArkTS
    // writes git_user_info signal file after picker authorization.
    if (s_gitAvailable) {
        const char* realGitconfig = "/storage/Users/currentUser/.ohos_git/.gitconfig";

        // Check if we can access the real .gitconfig (fileShare activated)
        FILE* probe = fopen(realGitconfig, "r");
        if (probe) {
            fclose(probe);
            // Real .gitconfig accessible — use it directly!
            // Write a merged config: safe.directory + include the real config
            QString gitconfigPath = s_filesDir + "/.gitconfig";
            FILE* gcf = fopen(gitconfigPath.toUtf8().constData(), "w");
            if (gcf) {
                fprintf(gcf, "[safe]\n\tdirectory = *\n");
                fprintf(gcf, "[include]\n\tpath = %s\n", realGitconfig);
                fclose(gcf);
                setenv("GIT_CONFIG_GLOBAL", gitconfigPath.toUtf8().constData(), 1);
                qDebug() << "[OHOS] .gitconfig with include written, real config accessible";
            }
        } else {
            // Real .gitconfig not accessible yet (first launch, fileShare not yet activated)
            // Write safe-only .gitconfig; user identity will come from -c injection via git_user_info
            QString gitconfigPath = s_filesDir + "/.gitconfig";
            FILE* gcf = fopen(gitconfigPath.toUtf8().constData(), "w");
            if (gcf) {
                fprintf(gcf, "[safe]\n\tdirectory = *\n");
                fclose(gcf);
                setenv("GIT_CONFIG_GLOBAL", gitconfigPath.toUtf8().constData(), 1);
                qDebug() << "[OHOS] .gitconfig (safe only) - real config not yet accessible";
            }
        }
    }

    qDebug() << "[OHOS] Git detection:" << (s_gitAvailable ? "available" : "not available")
             << "path:" << s_gitBinPath;

    // ---- SSH client detection ----
    // openssh.hnp installs ssh to /data/service/hnp/bin/ssh.
    // Verify via rawListDir (opendir works when fopen can't on HNP paths).
    if (s_gitAvailable) {
        // Debug: list HNP bin directory from app process
        QStringList hnpBinEntries = rawListDir("/data/service/hnp/bin");

        QStringList gitnextLibs = rawListDir(
            QString("/data/app/el1/bundle/public/%1/libs/arm64").arg(GITNEXT_BUNDLE_NAME).toUtf8().constData());
        // Resolve git symlink
        char gitResolved[PATH_MAX];
        ssize_t gitN = readlink("/data/service/hnp/bin/git", gitResolved, sizeof(gitResolved) - 1);
        QString gitRealPath;
        if (gitN > 0) {
            gitResolved[gitN] = '\0';
            gitRealPath = QString::fromUtf8(gitResolved);
            if (!gitRealPath.startsWith('/')) {
                gitRealPath = "/data/service/hnp/bin/" + gitRealPath;
            }
            // git symlink resolved, use its directory for ssh search
        }

        QString sshPath;
        QStringList sshCandidates;

        // 1. Check git's resolved directory
        if (!gitRealPath.isEmpty()) {
            QFileInfo fi(gitRealPath);
            QString gitDir = fi.absolutePath();
            sshCandidates << gitDir + "/ssh" << gitDir + "/ssh.sh";
        }

        // 2. Check HNP bin (if entries listed via rawListDir)
        for (const QString& entry : hnpBinEntries) {
            if (entry == "ssh" || entry == "ssh.sh") {
                sshCandidates << QString("/data/service/hnp/bin/") + entry;
            }
        }

        // 3. Check GitNext native libs
        for (const QString& entry : gitnextLibs) {
            if (entry == "ssh" || entry == "ssh.sh") {
                sshCandidates << QString("/data/app/el1/bundle/public/%1/libs/arm64/%2")
                    .arg(GITNEXT_BUNDLE_NAME, entry);
            }
        }

        // 4. Check known GitNext HNP paths
        sshCandidates
            << QString("/data/service/hnp/%1/git.hnp/bin/ssh").arg(GITNEXT_BUNDLE_NAME)
            << QString("/data/service/hnp/%1/git.hnp/bin/ssh.sh").arg(GITNEXT_BUNDLE_NAME);

        // 5. Check same directory as detected git binary
        if (!s_gitBinPath.isEmpty()) {
            QFileInfo fi(s_gitBinPath);
            sshCandidates << fi.absolutePath() + "/ssh" << fi.absolutePath() + "/ssh.sh";
        }

        for (const QString& p : sshCandidates) {
            char sshResolved[PATH_MAX];
            ssize_t sshN = readlink(p.toUtf8().constData(), sshResolved, sizeof(sshResolved) - 1);
            if (sshN > 0 || fileExists(p)) {
                sshPath = p;
                break;
            }
        }

        if (!sshPath.isEmpty()) {
            setenv("GIT_SSH", sshPath.toUtf8().constData(), 1);
            qDebug() << "[OHOS] GIT_SSH set to:" << sshPath;
        } else {
            qDebug() << "[OHOS] GIT_SSH: ssh not found anywhere, SSH push will fail";
        }
    }
}

bool isGitAvailable()
{
    return s_gitAvailable;
}

QString gitBinPath()
{
    return s_gitBinPath;
}

bool fileExists(const QString& path)
{
    // OHOS sandbox: stat() may return EACCES even for accessible files.
    // Use fopen() as a reliable existence check.
    FILE* f = fopen(path.toUtf8().constData(), "rb");
    if (f) {
        fclose(f);
        return true;
    }
    return false;
}

bool openUrl(const QString& url)
{
    if (url.isEmpty())
        return false;
    QString requestFile = s_filesDir + "/open_url_request";
    FILE* f = fopen(requestFile.toUtf8().constData(), "w");
    if (!f)
        return false;
    QByteArray data = url.toUtf8();
    size_t written = fwrite(data.constData(), 1, data.size(), f);
    fclose(f);
    if (static_cast<ssize_t>(written) != data.size()) {
        QFile::remove(requestFile);
        return false;
    }
    return true;
}

bool openExternalFile(const QString& filePath)
{
    if (filePath.isEmpty())
        return false;
    // Convert sandbox path to physical path for ArkTS fileUri.getUriFromPath()
    QString physicalPath = sandboxPathToPhysical(filePath);
    QString requestFile = s_filesDir + "/open_external_file_request";
    FILE* f = fopen(requestFile.toUtf8().constData(), "w");
    if (!f)
        return false;
    QByteArray data = physicalPath.toUtf8();
    size_t written = fwrite(data.constData(), 1, data.size(), f);
    fclose(f);
    if (static_cast<ssize_t>(written) != data.size()) {
        QFile::remove(requestFile);
        return false;
    }
    return true;
}
// ---- gitNext detection and override ----

static QString s_gitOverridePath;

// Helper: search for git in PATH directories
static QString findGitInPath() {
    const char* pathEnv = getenv("PATH");
    if (!pathEnv) return QString();

    QString pathStr = QString::fromUtf8(pathEnv);
    QStringList dirs = pathStr.split(':');
    for (const QString& dir : dirs) {
        QString candidate = dir + "/git";
        // Use fileExists() (fopen-based) — access() fails in OHOS sandbox (P-04)
        if (fileExists(candidate)) {
            return candidate;
        }
    }
    return QString();
}

QString getGitNextPath()
{
    // Check known file paths first
    // Use fileExists() (fopen-based) — access() fails in OHOS sandbox (P-04)
    for (const QString& path : gitNextCandidatePaths) {
        if (fileExists(path)) {
            qDebug() << "[OHOS] gitNext found at:" << path;
            return path;
        }
    }

    // Check PATH environment variable directories
    QString gitInPath = findGitInPath();
    if (!gitInPath.isEmpty()) {
        qDebug() << "[OHOS] git found in PATH:" << gitInPath;
        return gitInPath;
    }

    // Trust /data/service/hnp/bin even if fopen can't verify (HNP binaries
    // are sandbox-protected). QProcess-spawned children CAN execute them.
    const char* pathEnv = getenv("PATH");
    if (pathEnv) {
        QStringList dirs = QString::fromUtf8(pathEnv).split(':');
        for (const QString& dir : dirs) {
            if (dir == "/data/service/hnp/bin") {
                QString candidate = dir + "/git";
                qDebug() << "[OHOS] git found via PATH trust:" << candidate;
                return candidate;
            }
        }
    }

    // Check if ArkTS wrote a path signal file
    QString signalFile = s_filesDir + "/gitnext_path";
    FILE* f = fopen(signalFile.toUtf8().constData(), "r");
    if (f) {
        char buf[512];
        size_t n = fread(buf, 1, sizeof(buf) - 1, f);
        fclose(f);
        buf[n] = '\0';
        QString path = QString::fromUtf8(buf).trimmed();
        if (!path.isEmpty() && fileExists(path)) {
            qDebug() << "[OHOS] gitNext path from ArkTS signal:" << path;
            return path;
        }
    }

    qDebug() << "[OHOS] gitNext not found";
    return QString();
}

void setGitOverridePath(const QString& path)
{
    s_gitOverridePath = path;
}

QString getGitOverridePath()
{
    return s_gitOverridePath;
}

bool showGitNextInstallPrompt(const QString& message)
{
    if (message.isEmpty()) return false;
    QString requestFile = s_filesDir + "/gitnext_prompt_request";
    FILE* f = fopen(requestFile.toUtf8().constData(), "w");
    if (!f) return false;
    QByteArray data = message.toUtf8();
    size_t written = fwrite(data.constData(), 1, data.size(), f);
    fclose(f);
    if (static_cast<ssize_t>(written) != data.size()) {
        QFile::remove(requestFile);
        qDebug() << "[OHOS] gitNext install prompt: incomplete write";
        return false;
    }
    qDebug() << "[OHOS] gitNext install prompt signal written";
    return true;
}

bool openDirectory(const QString& dirPath)
{
    if (dirPath.isEmpty())
        return false;
    // Convert sandbox path to physical path for ArkTS fileUri.getUriFromPath()
    QString physicalPath = sandboxPathToPhysical(dirPath);
    qDebug() << "[OHOS] openDirectory: sandbox path =" << dirPath;
    qDebug() << "[OHOS] openDirectory: physical path =" << physicalPath;
    QString requestFile = s_filesDir + "/open_directory_request";
    FILE* f = fopen(requestFile.toUtf8().constData(), "w");
    if (!f)
        return false;
    QByteArray data = physicalPath.toUtf8();
    size_t written = fwrite(data.constData(), 1, data.size(), f);
    fclose(f);
    if (static_cast<ssize_t>(written) != data.size()) {
        QFile::remove(requestFile);
        return false;
    }
    return true;
}

// Helper: try to parse user.name/email from a .gitconfig-format file
static bool parseGitconfigFile(const char* path, QString& outName, QString& outEmail)
{
    FILE* f = fopen(path, "r");
    if (!f) return false;
    char line[512];
    bool inUserSection = false;
    while (fgets(line, sizeof(line), f)) {
        QString l = QString::fromUtf8(line).trimmed();
        if (l.startsWith('[') && l.endsWith(']')) {
            inUserSection = (l == "[user]");
            continue;
        }
        if (!inUserSection) continue;
        int eq = l.indexOf('=');
        if (eq < 0) continue;
        QString key = l.left(eq).trimmed();
        QString val = l.mid(eq + 1).trimmed();
        if (val.startsWith('"') && val.endsWith('"'))
            val = val.mid(1, val.length() - 2);
        if (key == "name") outName = val;
        else if (key == "email") outEmail = val;
    }
    fclose(f);
    return !outName.isEmpty() || !outEmail.isEmpty();
}

// Helper: try to read simple "name\nemail\n" format
static bool parseSimpleIdentityFile(const char* path, QString& outName, QString& outEmail)
{
    FILE* f = fopen(path, "r");
    if (!f) return false;
    char buf[256] = {};
    if (fgets(buf, sizeof(buf), f))
        outName = QString::fromUtf8(buf).trimmed();
    buf[0] = '\0';
    if (fgets(buf, sizeof(buf), f))
        outEmail = QString::fromUtf8(buf).trimmed();
    fclose(f);
    return !outName.isEmpty() || !outEmail.isEmpty();
}

// Lazy git config setup: ensures GIT_CONFIG_GLOBAL points to a merged config
// that includes the real .ohos_git/.gitconfig. Only needed when detectGit()
// ran before gitnext was available (s_gitAvailable was false at startup).
static void ensureGitConfigGlobal()
{
    if (getenv("GIT_CONFIG_GLOBAL"))
        return; // Already set up

    const char* realGitconfig = "/storage/Users/currentUser/.ohos_git/.gitconfig";
    // Use fopen() per P-04 (no stat())
    FILE* probe = fopen(realGitconfig, "r");
    if (!probe)
        return; // .ohos_git not accessible (fileShare not active or gitnext not logged in)
    fclose(probe);

    QString gitconfigPath = s_filesDir + "/.gitconfig";
    FILE* gcf = fopen(gitconfigPath.toUtf8().constData(), "w");
    if (gcf) {
        fprintf(gcf, "[safe]\n\tdirectory = *\n");
        fprintf(gcf, "[include]\n\tpath = %s\n", realGitconfig);
        fclose(gcf);
        setenv("GIT_CONFIG_GLOBAL", gitconfigPath.toUtf8().constData(), 1);
    }
}

// Last-resort fallback: ask git directly for user.name / user.email.
// Only called when all file-based candidates have failed and GIT_CONFIG_GLOBAL
// has been set up (so git knows where to read global config).
// Uses a short timeout to avoid UI freeze.
static bool tryGitConfigViaProcess(QString& outName, QString& outEmail)
{
    QProcess proc;
    proc.start("git", QStringList() << "-c" << "safe.directory=*"
                << "config" << "--global" << "user.name");
    if (proc.waitForFinished(2000) && proc.exitCode() == 0) {
        outName = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
    }
    proc.start("git", QStringList() << "-c" << "safe.directory=*"
                << "config" << "--global" << "user.email");
    if (proc.waitForFinished(2000) && proc.exitCode() == 0) {
        outEmail = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
    }
    return !outName.isEmpty() || !outEmail.isEmpty();
}

// Read git user identity from multiple candidate locations.
// Called on every git command — no caching, no startup timing issue.
// Search order:
//   1. .ohos_git/.gitconfig (gitnext real config — highest priority, handles
//      "texstudio started before gitnext logged in" scenario)
//   2. sandbox/git_user_info (written by ArkTS if it can read .ohos_git)
//   3. Desktop/git_identity.txt (simple format, user can create manually)
//   4. Documents/TexStudio/git_identity.txt
//   5. Desktop/.gitconfig (standard git format)
//   6. Documents/TexStudio/.gitconfig
//   7. Lazy setup + git config --global via QProcess (ultimate fallback)
static void readGitUserInfoFile(QString& outName, QString& outEmail)
{
    outName.clear();
    outEmail.clear();

    // 1. GitNext real config (handles late-login after texstudio startup)
    if (parseGitconfigFile("/storage/Users/currentUser/.ohos_git/.gitconfig",
                           outName, outEmail))
        return;

    // 2. Sandbox signal file (from ArkTS)
    QString signalPath = s_filesDir + "/git_user_info";
    if (parseSimpleIdentityFile(signalPath.toUtf8().constData(), outName, outEmail))
        return;

    // 3-6. Try accessible user directories
    const char* candidates[] = {
        "/storage/Users/currentUser/Desktop/git_identity.txt",
        "/storage/Users/currentUser/Documents/TexStudio/git_identity.txt",
        "/storage/Users/currentUser/Desktop/.gitconfig",
        "/storage/Users/currentUser/Documents/TexStudio/.gitconfig",
    };
    for (const char* path : candidates) {
        if (strstr(path, "git_identity")) {
            if (parseSimpleIdentityFile(path, outName, outEmail))
                return;
        } else {
            if (parseGitconfigFile(path, outName, outEmail))
                return;
        }
    }

    // 7. Ultimate fallback: lazy config setup + git config --global
    ensureGitConfigGlobal();
    tryGitConfigViaProcess(outName, outEmail);
}

QString getGitUserName()
{
    QString name, email;
    readGitUserInfoFile(name, email);
    return name;
}

QString getGitUserEmail()
{
    QString name, email;
    readGitUserInfoFile(name, email);
    return email;
}

QString dictionariesDir()
{
    return s_filesDir + "/dictionaries";
}

bool isDictionariesExtracted()
{
    // Check for marker file from ArkTS (P-04: use fopen not stat)
    QString markerPath = s_filesDir + "/.dictionaries_extracted";
    FILE* f = fopen(markerPath.toUtf8().constData(), "rb");
    if (f) {
        fclose(f);
        return true;
    }
    // Also check dictionaries_ready signal file
    QString signalPath = s_filesDir + "/dictionaries_ready";
    f = fopen(signalPath.toUtf8().constData(), "rb");
    if (f) {
        fclose(f);
        return true;
    }
    return false;
}

// Bundle name for sandbox↔physical path conversion.
// Matches bundleName in texstudio-hap/AppScope/app.json5.
static const char* APP_BUNDLE_NAME = "com.develop.opensource.texide";

bool isHomeAccessGranted()
{
    FILE* f = fopen("/storage/Users/currentUser/.ohos_git/.gitconfig", "r");
    if (f) {
        fclose(f);
        return true;
    }
    // Fallback: check home_path signal file (written by ArkTS after successful auth)
    QString signalPath = s_filesDir + "/home_path";
    f = fopen(signalPath.toUtf8().constData(), "r");
    if (f) {
        fclose(f);
        return true;
    }
    return false;
}

bool requestHomeReauth()
{
    QString requestFile = s_filesDir + "/request_home_auth";
    FILE* f = fopen(requestFile.toUtf8().constData(), "w");
    if (!f) return false;
    fwrite("1", 1, 1, f);
    fclose(f);
    return true;
}

// Public wrapper: re-check whether .ohos_git/.gitconfig has become accessible
// since startup and, if so, rewrite the merged GIT_CONFIG_GLOBAL to include it.
// Called from GIT::makeCmd() before every git command so the startup race
// (TexStudio started before GitNext activated fileShare) is healed on the next
// git operation after the user opens GitNext.
void ensureGitConfig()
{
    const char* realGitconfig = "/storage/Users/currentUser/.ohos_git/.gitconfig";
    FILE* probe = fopen(realGitconfig, "r");
    if (!probe)
        return; // fileShare not yet active — leave config as-is
    fclose(probe);

    // Switch HOME to /storage/Users/currentUser so git can find ~/.ssh/ keys
    // (matches GitNext behavior). XDG_* env vars still point to sandbox so
    // QStandardPaths and Qt internals are unaffected.
    const char* currentHome = getenv("HOME");
    const char* userHome = "/storage/Users/currentUser";
    if (!currentHome || strcmp(currentHome, userHome) != 0) {
        setenv("HOME", userHome, 1);
        qDebug() << "[OHOS] HOME switched to" << userHome << "for SSH key access";
        // Copy existing .git-credentials from sandbox to new HOME
        QString oldCred = s_filesDir + "/.git-credentials";
        QString newCred = QString(userHome) + "/.git-credentials";
        QFile oldFile(oldCred);
        if (oldFile.exists() && !QFile::exists(newCred)) {
            QFile::copy(oldCred, newCred);
        }
    }

    // Check if current GIT_CONFIG_GLOBAL already includes the real config
    const char* current = getenv("GIT_CONFIG_GLOBAL");
    if (current) {
        FILE* f = fopen(current, "r");
        if (f) {
            char buf[1024] = {};
            fread(buf, 1, sizeof(buf) - 1, f);
            fclose(f);
            if (strstr(buf, ".ohos_git/.gitconfig"))
                return; // Already merged — nothing to do
        }
    }

    // Rewrite merged config to include the real .gitconfig (credential helper etc.)
    QString gitconfigPath = s_filesDir + "/.gitconfig";
    FILE* gcf = fopen(gitconfigPath.toUtf8().constData(), "w");
    if (gcf) {
        fprintf(gcf, "[safe]\n\tdirectory = *\n");
        fprintf(gcf, "[include]\n\tpath = %s\n", realGitconfig);
        fclose(gcf);
        setenv("GIT_CONFIG_GLOBAL", gitconfigPath.toUtf8().constData(), 1);
    }
}

// Convert sandbox path (e.g. /data/storage/el2/base/haps/entry/files/xxx)
// to physical path (e.g. /storage/Users/currentUser/appdata/el2/base/<bundle>/haps/entry/files/xxx)
// that ArkTS fileUri.getUriFromPath() can process.
// This is the reverse of ohosFixSandboxPath() in filedialog.cpp.
QString sandboxPathToPhysical(const QString &sandboxPath)
{
    static const QString sandboxPrefix = QStringLiteral("/data/storage/el2/base/");
    static const QString physPrefix = QStringLiteral("/storage/Users/currentUser/appdata/el2/base/");

    if (sandboxPath.startsWith(sandboxPrefix)) {
        // sandboxPrefix + "haps/entry/files/xxx"
        // → physPrefix + "<bundleName>/haps/entry/files/xxx"
        QString relative = sandboxPath.mid(sandboxPrefix.length());
        QString bundleName = QString::fromUtf8(APP_BUNDLE_NAME);
        QString physicalPath = physPrefix + bundleName + "/" + relative;
        return physicalPath;
    }
    return sandboxPath;
}

} // namespace ohos
