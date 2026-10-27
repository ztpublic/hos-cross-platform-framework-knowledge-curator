/**
 * @file ohosProcess.cpp
 * @author MiKTeX HarmonyOS Port
 * @brief Process handling (HarmonyOS) - uses posix_spawn instead of fork/exec
 *
 * @copyright Copyright © 1996-2024 Christian Schenk
 *
 * This file is part of the MiKTeX Core Library.
 *
 * The MiKTeX Core Library is licensed under GNU General Public License version
 * 2 or any later version.
 */

#include "config.h"

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <thread>
#include <tuple>

#include <miktex/Core/AutoResource>
#include <miktex/Core/Directory>
#include <miktex/Core/Environment>
#include <miktex/Core/CommandLineBuilder>
#include <miktex/Core/StreamReader>
#include <miktex/Trace/Trace>
#include <miktex/Trace/TraceStream>

#include <miktex/Util/PathNameUtil>
#include <miktex/Util/Tokenizer>

#include "internal.h"
#include "ohosProcess.h"
#include "Session/SessionImpl.h"

using namespace std;
using namespace MiKTeX::Core;
using namespace MiKTeX::Trace;
using namespace MiKTeX::Util;

const int filenoStdin = 0;
const int filenoStdout = 1;
const int filenoStderr = 2;

// Helper: duplicate fd, throw on error
static int OhosDup(int fd)
{
    int dupfd = dup(fd);
    if (dupfd < 0)
    {
        MIKTEX_FATAL_CRT_ERROR("dup");
    }
    return dupfd;
}

// Helper: close fd, throw on error
static void OhosClose(int fd)
{
    if (close(fd) < 0)
    {
        MIKTEX_FATAL_CRT_ERROR("close");
    }
}

// ---- Pipe helper --------------------------------------------------------

class OhosPipe
{
public:
    ~OhosPipe() noexcept
    {
        try { Dispose(); } catch (...) {}
    }

    void Create()
    {
        if (pipe(twofd) < 0)
        {
            MIKTEX_FATAL_CRT_ERROR("pipe");
        }
        // Set FD_CLOEXEC on both ends so they are NOT inherited by posix_spawn'd children.
        // The child's stdin/stdout/stderr are set up via file actions (dup2), which clears
        // FD_CLOEXEC. All other pipe ends must be closed in the child to avoid deadlocks
        // (e.g. dvipdfmx inheriting its own stdin pipe write-end and blocking on read).
        if (fcntl(twofd[0], F_SETFD, FD_CLOEXEC) < 0)
            MIKTEX_FATAL_CRT_ERROR("fcntl");
        if (fcntl(twofd[1], F_SETFD, FD_CLOEXEC) < 0)
            MIKTEX_FATAL_CRT_ERROR("fcntl");
    }

    int GetReadEnd()  const { return twofd[0]; }
    int GetWriteEnd() const { return twofd[1]; }

    int StealReadEnd()
    {
        int fd = twofd[0]; twofd[0] = -1; return fd;
    }

    int StealWriteEnd()
    {
        int fd = twofd[1]; twofd[1] = -1; return fd;
    }

    void CloseReadEnd()
    {
        if (twofd[0] >= 0) { OhosClose(twofd[0]); twofd[0] = -1; }
    }

    void CloseWriteEnd()
    {
        if (twofd[1] >= 0) { OhosClose(twofd[1]); twofd[1] = -1; }
    }

    void Dispose()
    {
        CloseReadEnd();
        CloseWriteEnd();
    }

private:
    int twofd[2] = { -1, -1 };
};

// ---- CreateEnvironmentBlock (shared with unxProcess) --------------------

static tuple<char*, char**> CreateEnvironmentBlock(const unordered_map<string, string>& envMap)
{
    size_t envSize = 0;
    for (const auto& p : envMap)
    {
        envSize += p.first.length() + 1 + p.second.length() + 1;
    }
    char* environmentStrings = new char[envSize + 1];
    char** environmentPointers = new char*[envMap.size() + 1];
    size_t stringIdx = 0;
    size_t pointerIdx = 0;
    for (const auto& p : envMap)
    {
        string s = fmt::format("{}={}", p.first, p.second);
        strcpy(environmentStrings + stringIdx, s.c_str());
        environmentPointers[pointerIdx] = environmentStrings + stringIdx;
        stringIdx += s.length() + 1;
        ++pointerIdx;
    }
    environmentPointers[pointerIdx] = nullptr;
    return make_tuple(environmentStrings, environmentPointers);
}

// ---- Process::Start factory ---------------------------------------------

unique_ptr<Process> Process::Start(const ProcessStartInfo& startinfo)
{
    return make_unique<ohosProcess>(startinfo);
}

// ---- ohosProcess::Create ------------------------------------------------

void ohosProcess::Create()
{
    MIKTEX_EXPECT(!startinfo.FileName.empty());

    auto trace_process = TraceStream::Open(MIKTEX_TRACE_PROCESS);

    shared_ptr<SessionImpl> session = SESSION_IMPL();

    PathName fileName;
    if (PathNameUtil::IsAbsolutePath(startinfo.FileName) ||
        !session->FindFile(startinfo.FileName, FileType::EXE, fileName))
    {
        fileName = startinfo.FileName;
    }

    Argv argv(startinfo.Arguments.empty()
        ? vector<string>{ PathName(startinfo.FileName).GetFileNameWithoutExtension().ToString() }
        : startinfo.Arguments);

    OhosPipe pipeStdout;
    OhosPipe pipeStderr;
    OhosPipe pipeStdin;

    int fdChildStderr = -1;

    if (startinfo.RedirectStandardOutput)
    {
        pipeStdout.Create();
        if (!startinfo.RedirectStandardError)
        {
            fdChildStderr = OhosDup(pipeStdout.GetWriteEnd());
        }
    }

    if (startinfo.RedirectStandardError)
    {
        pipeStderr.Create();
    }

    int fdChildStdin = -1;
    if (startinfo.StandardInput != nullptr)
    {
        fdChildStdin = fileno(startinfo.StandardInput);
        if (fdChildStdin < 0)
        {
            MIKTEX_FATAL_CRT_ERROR("fileno");
        }
    }
    else if (startinfo.RedirectStandardInput)
    {
        pipeStdin.Create();
    }

    tmpFile = TemporaryFile::Create();

    unordered_map<string, string> envMap =
        session->CreateChildEnvironment(!startinfo.WorkingDirectory.empty());
    envMap[MIKTEX_ENV_EXCEPTION_PATH] = tmpFile->GetPathName().ToString();
    char* environmentStrings;
    char** environmentPointers;
    tie(environmentStrings, environmentPointers) = CreateEnvironmentBlock(envMap);
    MIKTEX_AUTO(delete[] environmentStrings);
    MIKTEX_AUTO(delete[] environmentPointers);

    session->UnloadFilenameDatabase();

    // ---- posix_spawn attrs (kept for reference) --------------------------
    // NOTE: We use fork+exec instead of posix_spawn because OHOS musl's
    // posix_spawn implementation does NOT honour FD_CLOEXEC nor
    // file_actions_addclose: all parent fds are inherited by the child with
    // cloexec=0. This causes dvipdfmx to inherit the write-end of its own
    // stdin pipe, preventing EOF and causing a deadlock in dvi_close().

    // ---- chdir if requested (before fork) --------------------------------
    char savedCwd[PATH_MAX] = {};
    bool didChdir = false;
    if (!startinfo.WorkingDirectory.empty())
    {
        if (getcwd(savedCwd, sizeof(savedCwd)) != nullptr)
        {
            if (chdir(startinfo.WorkingDirectory.c_str()) == 0)
            {
                didChdir = true;
            }
        }
    }

    trace_process->WriteLine("core", TraceLevel::Info,
        fmt::format("fork+exec: {}", fileName.ToDisplayString()));

    pid = fork();
    if (pid < 0)
    {
        MIKTEX_FATAL_CRT_ERROR("fork");
    }

    if (pid == 0)
    {
        // ---- Child process -----------------------------------------------
        // Step 1: Set up stdio redirections
        if (pipeStdout.GetWriteEnd() >= 0)
        {
            dup2(pipeStdout.GetWriteEnd(), filenoStdout);
        }
        if (pipeStderr.GetWriteEnd() >= 0)
        {
            dup2(pipeStderr.GetWriteEnd(), filenoStderr);
        }
        else if (fdChildStderr >= 0)
        {
            dup2(fdChildStderr, filenoStderr);
        }
        if (pipeStdin.GetReadEnd() >= 0)
        {
            dup2(pipeStdin.GetReadEnd(), filenoStdin);
        }
        else if (fdChildStdin >= 0)
        {
            dup2(fdChildStdin, filenoStdin);
        }

        // Step 2: Close ALL inherited fds > 2 using /proc/self/fd enumeration.
        // This is the correct fix for OHOS musl posix_spawn's FD_CLOEXEC bug.
        {
            DIR* d = opendir("/proc/self/fd");
            if (d)
            {
                int dfd = dirfd(d);
                struct dirent* ent;
                // Collect fds first, then close them.
                // Closing fds during readdir iteration can cause issues because
                // opendir("/proc/self/fd") itself uses a fd.
                int fdsToClose[4096];
                int numFds = 0;
                while ((ent = readdir(d)) != NULL)
                {
                    char* endp;
                    long fd = strtol(ent->d_name, &endp, 10);
                    if (*endp != '\0') continue; // skip ".", ".."
                    if (fd > 2 && fd != (long)dfd && numFds < 4096)
                        fdsToClose[numFds++] = (int)fd;
                }
                closedir(d);
                for (int i = 0; i < numFds; i++)
                    close(fdsToClose[i]);
            }
            else
            {
                // Fallback: close a generous range
                for (int fd = 3; fd < 4096; fd++)
                    close(fd);
            }
        }

        // Step 3: exec the target binary
        execve(fileName.GetData(),
               const_cast<char* const*>(argv.GetArgv()),
               const_cast<char* const*>(environmentPointers));

        // exec failed — write error to stderr and exit
        _exit(127);
    }

    // ---- Parent process --------------------------------------------------
    // Restore working directory if we changed it
    if (didChdir && savedCwd[0] != '\0')
    {
        (void)chdir(savedCwd);
    }

    if (fdChildStderr >= 0)
    {
        OhosClose(fdChildStderr);
    }

    if (startinfo.RedirectStandardOutput)
    {
        fdStandardOutput = pipeStdout.StealReadEnd();
    }
    if (startinfo.RedirectStandardError)
    {
        fdStandardError = pipeStderr.StealReadEnd();
    }
    if (startinfo.RedirectStandardInput)
    {
        fdStandardInput = pipeStdin.StealWriteEnd();
    }

    pipeStdout.Dispose();
    pipeStderr.Dispose();
    pipeStdin.Dispose();
}

// ---- Constructor / Destructor -------------------------------------------

ohosProcess::ohosProcess(const ProcessStartInfo& startinfo)
    : startinfo(startinfo)
{
    Create();
}

ohosProcess::~ohosProcess()
{
    try { Close(); } catch (...) {}
}

// ---- I/O accessors -------------------------------------------------------

void ohosProcess::Close()
{
    if (fdStandardOutput >= 0) { close(fdStandardOutput); fdStandardOutput = -1; }
    if (fdStandardError  >= 0) { close(fdStandardError);  fdStandardError  = -1; }
    if (fdStandardInput  >= 0) { close(fdStandardInput);  fdStandardInput  = -1; }
    pid = -1;
    if (tmpFile != nullptr) { tmpFile->Delete(); tmpFile = nullptr; }
}

FILE* ohosProcess::get_StandardInput()
{
    if (pFileStandardInput != nullptr) return pFileStandardInput;
    if (fdStandardInput < 0)           return nullptr;
    pFileStandardInput = FdOpen(fdStandardInput, "wb");
    fdStandardInput = -1;
    return pFileStandardInput;
}

FILE* ohosProcess::get_StandardOutput()
{
    if (pFileStandardOutput != nullptr) return pFileStandardOutput;
    if (fdStandardOutput < 0)           return nullptr;
    pFileStandardOutput = FdOpen(fdStandardOutput, "rb");
    fdStandardOutput = -1;
    return pFileStandardOutput;
}

FILE* ohosProcess::get_StandardError()
{
    if (pFileStandardError != nullptr) return pFileStandardError;
    if (fdStandardError < 0)           return nullptr;
    pFileStandardError = FdOpen(fdStandardError, "rb");
    fdStandardError = -1;
    return pFileStandardError;
}

// ---- Wait / exit status -------------------------------------------------

void ohosProcess::WaitForExit()
{
    if (pid > 0)
    {
        auto trace_process = TraceStream::Open(MIKTEX_TRACE_PROCESS);
        trace_process->WriteLine("core", fmt::format("waiting for process {}", pid));
        pid_t saved = pid;
        pid = -1;
        while (waitpid(saved, &status, 0) <= 0)
        {
            if (errno != EINTR)
            {
                MIKTEX_FATAL_CRT_ERROR("waitpid");
            }
        }
    }
}

bool ohosProcess::WaitForExit(int milliseconds)
{
    if (pid <= 0) return true;
    do
    {
        pid_t p = waitpid(pid, &status, WNOHANG);
        if (p == pid)       { pid = -1; return true; }
        else if (p < 0)     { pid = -1; MIKTEX_FATAL_CRT_ERROR("waitpid"); }
        this_thread::sleep_for(chrono::milliseconds(1));
        --milliseconds;
    } while (milliseconds > 0);
    return false;
}

ProcessExitStatus ohosProcess::get_ExitStatus() const
{
    if (WIFEXITED(status)  != 0) return ProcessExitStatus::Exited;
    if (WIFSIGNALED(status)!= 0) return ProcessExitStatus::Signaled;
    return ProcessExitStatus::Other;
}

int ohosProcess::get_ExitCode() const
{
    if (WIFEXITED(status) != 0)
    {
        return WEXITSTATUS(status);
    }
    MIKTEX_FATAL_ERROR_2(T_("Process terminated unexpectedly."),
        "fileName", startinfo.FileName, "exitStatus", std::to_string(status));
}

bool ohosProcess::get_Exception(MiKTeXException& ex) const
{
    return MiKTeXException::Load(tmpFile->GetPathName().ToString(), ex);
}

// ---- Process identity ---------------------------------------------------

int ohosProcess::GetSystemId()
{
    return pid;
}

unique_ptr<Process> ohosProcess::get_Parent()
{
    ProcessInfo info = GetProcessInfo();
    auto parent = make_unique<ohosProcess>();
    parent->pid = info.parent;
    return parent;
}

string ohosProcess::get_ProcessName()
{
    // On OHOS (Linux kernel): read /proc/<pid>/comm
    PathName path("/proc");
    path /= std::to_string(pid);
    path /= "comm";
    if (!File::Exists(path)) return "";
    StreamReader reader(path);
    string line;
    while (reader.ReadLine(line)) return line;
    return "";
}

ProcessInfo ohosProcess::GetProcessInfo()
{
    ProcessInfo info;
    info.name = get_ProcessName();
    // Read /proc/<pid>/stat for status and parent PID
    PathName path("/proc");
    path /= std::to_string(pid);
    path /= "stat";
    if (!File::Exists(path)) return info;
    StreamReader reader(path);
    string line;
    while (reader.ReadLine(line))
    {
        Tokenizer tok(line, " ");
        ++tok; // pid
        ++tok; // comm
        string state = *tok; ++tok; // state
        if (!state.empty())
        {
            switch (state[0])
            {
            case 'R': info.status = ProcessStatus::Runnable;  break;
            case 'S': case 'D': info.status = ProcessStatus::Sleeping;  break;
            case 'T': info.status = ProcessStatus::Stopped;   break;
            case 'Z': info.status = ProcessStatus::Zombie;    break;
            default:  info.status = ProcessStatus::Other;     break;
            }
        }
        info.parent = std::stoi(*tok);
    }
    return info;
}

// ---- Static helpers (system command, overlay, current/get process) ------

static vector<string> WrapCommand(const string& commandLine)
{
    // On OHOS there is no /bin/sh; fall back to returning an error wrapper
    // Callers using shell commands will fail gracefully.
    return vector<string>{ "/system/bin/sh", "-c", commandLine };
}

unique_ptr<Process> Process::StartSystemCommand(const string& commandLine,
    FILE** ppFileStandardInput, FILE** ppFileStandardOutput)
{
    vector<string> arguments = WrapCommand(commandLine);
    ProcessStartInfo startinfo;
    startinfo.FileName = arguments[0];
    startinfo.Arguments = arguments;
    startinfo.RedirectStandardInput  = (ppFileStandardInput  != nullptr);
    startinfo.RedirectStandardOutput = (ppFileStandardOutput != nullptr);
    unique_ptr<Process> process(Process::Start(startinfo));
    if (ppFileStandardInput  != nullptr) *ppFileStandardInput  = process->get_StandardInput();
    if (ppFileStandardOutput != nullptr) *ppFileStandardOutput = process->get_StandardOutput();
    return process;
}

bool Process::ExecuteSystemCommand(const string& commandLine, int* exitCode,
    IRunProcessCallback* callback, const char* directory)
{
    vector<string> arguments = WrapCommand(commandLine);
    return Process::Run(PathName(arguments[0]), arguments, callback, exitCode, directory);
}

unique_ptr<Process> Process::GetCurrentProcess()
{
    auto p = make_unique<ohosProcess>();
    p->pid = getpid();
    return p;
}

unique_ptr<Process> Process::GetProcess(int systemId)
{
    if (kill(systemId, 0) != 0)
    {
        if (errno == ESRCH) return nullptr;
        MIKTEX_FATAL_CRT_ERROR("kill");
    }
    auto p = make_unique<ohosProcess>();
    p->pid = systemId;
    return p;
}

void Process::Overlay(const PathName& fileName, const vector<string>& arguments)
{
    MIKTEX_EXPECT(!fileName.Empty());
    Argv argv(arguments.empty()
        ? vector<string>{ PathName(fileName).GetFileNameWithoutExtension().ToString() }
        : arguments);
    auto trace_process = TraceStream::Open(MIKTEX_TRACE_PROCESS);
    trace_process->WriteLine("core", TraceLevel::Info,
        fmt::format("execve: {}", Q_(fileName.ToDisplayString())));
    shared_ptr<SessionImpl> session = SESSION_IMPL();
    unordered_map<string, string> envMap = session->CreateChildEnvironment(false);
    char* environmentStrings;
    char** environmentPointers;
    tie(environmentStrings, environmentPointers) = CreateEnvironmentBlock(envMap);
    MIKTEX_AUTO(delete[] environmentStrings);
    MIKTEX_AUTO(delete[] environmentPointers);
    session->UnloadFilenameDatabase();
    execve(fileName.GetData(),
           const_cast<char* const*>(argv.GetArgv()),
           const_cast<char* const*>(environmentPointers));
    MIKTEX_UNEXPECTED();
}
