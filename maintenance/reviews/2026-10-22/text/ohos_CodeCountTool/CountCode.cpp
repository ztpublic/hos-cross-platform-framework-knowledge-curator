#pragma execution_character_set("utf-8")

#include "CountCode.h"
#include "ui_CountCode.h"

#include<QFileDialog>
#include<QApplication>
#include<QTextStream>
#include<QFile>
#include<QFileInfo>
#include<QDir>
#include<QDebug>
#include<QToolButton>
#include<QPainter>
#include<QProgressBar>
#include<QStandardPaths>
#include<QHeaderView>
#include<QLineEdit>
#include<QLabel>
#include<QGuiApplication>
#include<QScreen>
#include<QUrl>
#include<QThread>
#include<QElapsedTimer>
#include<QFrame>
#include<QHBoxLayout>
#include<QVBoxLayout>
#include<QResizeEvent>

#if defined(Q_OS_OPENHARMONY) || defined(OPENHARMONY)
#include <QtCore/qopenharmony.h>
#endif

namespace {

QString defaultCodeFileFilter()
{
#if defined(Q_OS_OPENHARMONY) || defined(OPENHARMONY)
    return QStringLiteral("*.h *.hpp *.c *.cc *.cpp *.cxx *.ui *.qrc *.css *.qml *.js *.ts *.ets *.json *.json5 *.java *.kt *.xml *.gradle *.properties *.py *.sh *.cmake CMakeLists.txt *.md");
#else
    return QStringLiteral("*.h *.cpp *.css");
#endif
}

QStringList splitFilter(const QString &filter)
{
    return filter.split(QLatin1Char(' '), Qt::SkipEmptyParts);
}

bool matchesFileFilters(const QString &baseName, const QStringList &filters)
{
    if (baseName.startsWith("moc_") || baseName.startsWith("mocs_") || baseName.startsWith("ui_")
        || baseName.startsWith("qrc_")) {
        return false;
    }
    return !filters.isEmpty() && QDir::match(filters, baseName);
}

bool isExcludedDir(const QString &dirName, const QStringList &filters)
{
    if (dirName.startsWith("build") || dirName.startsWith("bin")
        || dirName.startsWith("debug") || dirName.startsWith("release")) {
        return true;
    }

    foreach (const QString &dirFilter, filters) {
        if (dirFilter.isEmpty()) {
            continue;
        }
        if (dirName.contains(dirFilter)) {
            return true;
        }
    }
    return false;
}

QString defaultDialogPath()
{
#if defined(Q_OS_OPENHARMONY) || defined(OPENHARMONY)
    const QList<QStandardPaths::StandardLocation> locations = {
        QStandardPaths::DocumentsLocation,
        QStandardPaths::DownloadLocation,
        QStandardPaths::HomeLocation
    };
    for (QStandardPaths::StandardLocation location : locations) {
        const QString path = QStandardPaths::writableLocation(location);
        if (!path.isEmpty()) {
            return path;
        }
    }
#endif
    return QDir::homePath();
}

void setProgressPercent(QProgressBar *bar, quint64 part, quint64 total)
{
    const double percent = total == 0 ? 0.0 : static_cast<double>(part) / static_cast<double>(total) * 100.0;
    bar->setFormat(QString("%1%").arg(percent, 5, 'f', 2, QChar(' ')));
    bar->setValue(static_cast<int>(percent));
}

#if defined(Q_OS_OPENHARMONY) || defined(OPENHARMONY)
double harmonyUiScale()
{
    double scale = 1.0;
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        const QSize availableSize = screen->availableGeometry().size();
        if (availableSize.width() > 0 && availableSize.height() > 0) {
            const double widthScale = static_cast<double>(availableSize.width()) / 1440.0;
            const double heightScale = static_cast<double>(availableSize.height()) / 900.0;
            scale = qMin(widthScale, heightScale);
        }
    }
    return qBound(1.15, scale, 1.65);
}

int harmonyScaled(int value)
{
    return static_cast<int>(value * harmonyUiScale() + 0.5);
}

void setFixedScaled(QWidget *widget, int width, int height)
{
    widget->setMinimumSize(harmonyScaled(width), harmonyScaled(height));
    widget->setMaximumSize(harmonyScaled(width), harmonyScaled(height));
}

void setScaledHeight(QWidget *widget, int height)
{
    widget->setMinimumHeight(harmonyScaled(height));
    widget->setMaximumHeight(harmonyScaled(height));
}

QString normalizeSelectedPath(const QString &path)
{
    QString normalized = path.trimmed();
    if (normalized.isEmpty()) {
        return normalized;
    }

    if (normalized.startsWith(QLatin1String("file://"))
            || normalized.startsWith(QLatin1String("content://"))) {
        const QString resolved = QtOh::pathFromUri(normalized);
        if (!resolved.isEmpty()) {
            return resolved;
        }
    }

    const QUrl url(normalized);
    if (url.isLocalFile()) {
        return url.toLocalFile();
    }

    if (normalized.startsWith(QLatin1String("file://docs"))) {
        normalized.remove(0, QStringLiteral("file://docs").size());
        if (!normalized.startsWith(QLatin1Char('/'))) {
            normalized.prepend(QLatin1Char('/'));
        }
    }
    return QDir::cleanPath(normalized);
}

QStringList normalizeSelectedPaths(const QStringList &paths)
{
    QStringList normalizedPaths;
    for (const QString &path : paths) {
        const QString normalizedPath = normalizeSelectedPath(path);
        if (!normalizedPath.isEmpty()) {
            normalizedPaths << normalizedPath;
        }
    }
    return normalizedPaths;
}

void logPathDiagnostic(const QString &kind, const QString &rawPath, const QString &path)
{
    const QFileInfo info(path);
    qInfo().noquote() << "CodeCountTool Harmony selected" << kind
                      << "raw=" << rawPath
                      << "path=" << path
                      << "exists=" << info.exists()
                      << "dir=" << info.isDir()
                      << "file=" << info.isFile()
                      << "readable=" << info.isReadable();
}
#else
QString normalizeSelectedPath(const QString &path)
{
    return path;
}

QStringList normalizeSelectedPaths(const QStringList &paths)
{
    return paths;
}

void logPathDiagnostic(const QString &, const QString &, const QString &)
{
}
#endif

} // namespace

void CountCodeWorker::requestCancel()
{
    m_cancelRequested = true;
}

void CountCodeWorker::countDirectory(const QString &rootPath, const QStringList &fileFilters, const QStringList &dirFilters)
{
    m_cancelRequested = false;
    const QStringList files = collectFiles(rootPath, fileFilters, dirFilters);
    QList<CodeFileStat> stats;
    if (!m_cancelRequested) {
        stats = countFileList(files, fileFilters);
    }
    emit finished(stats, m_cancelRequested);
}

void CountCodeWorker::countFiles(const QStringList &files, const QStringList &fileFilters)
{
    m_cancelRequested = false;
    const QList<CodeFileStat> stats = countFileList(files, fileFilters);
    emit finished(stats, m_cancelRequested);
}

QStringList CountCodeWorker::collectFiles(const QString &rootPath, const QStringList &fileFilters, const QStringList &dirFilters)
{
    QStringList files;
    QDir root(rootPath);
    if (!root.exists()) {
        qWarning().noquote() << "CodeCountTool directory is not accessible:" << rootPath;
        return files;
    }

    //用户显式选择的根目录不参与排除规则，排除规则只作用于子目录
    QStringList pending;
    pending << root.absolutePath();

    QElapsedTimer progressTimer;
    progressTimer.start();

    while (!pending.isEmpty()) {
        if (m_cancelRequested) {
            return files;
        }
        const QDir dir(pending.takeLast());
        const QFileInfoList fileInfos = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Files);
        foreach (const QFileInfo &fileInfo, fileInfos) {
            if (m_cancelRequested) {
                return files;
            }
            if (fileInfo.isFile()) {
                if (matchesFileFilters(fileInfo.fileName(), fileFilters)) {
                    files << fileInfo.filePath();
                }
            } else if (!isExcludedDir(fileInfo.fileName(), dirFilters)) {
                pending << fileInfo.absoluteFilePath();
            }
        }
        if (progressTimer.elapsed() >= 100) {
            emit progressChanged(static_cast<int>(files.count()), 0, -1);
            progressTimer.restart();
        }
    }
    return files;
}

QList<CodeFileStat> CountCodeWorker::countFileList(const QStringList &files, const QStringList &fileFilters)
{
    QList<CodeFileStat> stats;
    const int total = static_cast<int>(files.count());
    stats.reserve(total);

    QElapsedTimer progressTimer;
    progressTimer.start();

    for (int i = 0; i < total; i++) {
        if (m_cancelRequested) {
            break;
        }
        const QFileInfo fileInfo(files.at(i));
        if (!matchesFileFilters(fileInfo.fileName(), fileFilters)) {
            continue;
        }

        CodeFileStat stat;
        stat.filePath = fileInfo.filePath();
        stat.fileName = fileInfo.fileName();
        stat.suffix = fileInfo.suffix();
        stat.bytes = fileInfo.size();
        if (!countLines(stat.filePath, stat.lineCode, stat.lineBlank, stat.lineNote)) {
            break;
        }
        stats << stat;

        if (progressTimer.elapsed() >= 100) {
            emit progressChanged(total, i + 1, total);
            progressTimer.restart();
        }
    }
    return stats;
}

bool CountCodeWorker::countLines(const QString &fileName, int &lineCode, int &lineBlank, int &lineNotes) const
{
    lineCode = lineBlank = lineNotes = 0;
    QFile file(fileName);
    if (file.open(QFile::ReadOnly)) {
        QTextStream out(&file);
        QString line;
        bool isNote = false;
        int linesRead = 0;
        while (!out.atEnd()) {
            //超大文件也要能及时响应取消
            if ((++linesRead & 0x3FFF) == 0 && m_cancelRequested) {
                return false;
            }
            line = out.readLine();

            line = line.trimmed();

            //判断当前行是否是注释
            if (line.startsWith("/*")) {
                isNote = true;
            }

            //注释部分
            if (isNote) {
                lineNotes++;
            } else {
                if (line.startsWith("//")) {    //注释行
                    lineNotes++;
                } else if (line.isEmpty()) {    //空白行
                    lineBlank++;
                } else {                        //代码行
                    lineCode++;
                }
            }

            //注释结束
            if (line.endsWith("*/")) {
                isNote = false;
            }
        }
    }
    return true;
}

CountCode::CountCode(QWidget *parent) : QWidget(parent), ui(new Ui::CountCode)
{
    ui->setupUi(this);
    {
        QFile file(":/Res/style.css");
        if (file.open(QIODevice::ReadOnly)) {
            this->setStyleSheet(file.readAll());
        } else {
            qDebug() << "样式表打开失败QAQ";
            return;
        }
    }
    this->initForm();

    qRegisterMetaType<QList<CodeFileStat>>("QList<CodeFileStat>");
    m_workerThread = new QThread(this);
    m_worker = new CountCodeWorker;
    m_worker->moveToThread(m_workerThread);
    connect(m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(this, &CountCode::countDirectoryRequested, m_worker, &CountCodeWorker::countDirectory);
    connect(this, &CountCode::countFilesRequested, m_worker, &CountCodeWorker::countFiles);
    connect(m_worker, &CountCodeWorker::progressChanged, this, &CountCode::onCountProgress);
    connect(m_worker, &CountCodeWorker::finished, this, &CountCode::onCountFinished);
    m_workerThread->start();

    connect(ui->btnOpenFile,&QToolButton::clicked,this,&CountCode::onBtnOpenFileClicked);
    connect(ui->btnOpenPath,&QToolButton::clicked,this,&CountCode::onBtnOpenPathClicked);
    connect(ui->btnClear,&QToolButton::clicked,this,&CountCode::onBtnClearClicked);
    connect(ui->txtFilter, &QLineEdit::editingFinished, this, &CountCode::onEditingFinished);
    connect(ui->dirFilter, &QLineEdit::editingFinished, this, &CountCode::onEditingFinished);
    onBtnClearClicked();
}

CountCode::~CountCode()
{
    m_worker->requestCancel();
    m_workerThread->quit();
    m_workerThread->wait();
    delete ui;
}

void CountCode::initForm() {
    this->setWindowIcon(QIcon(QStringLiteral(":/Res/icon.ico")));

    auto *header = new QFrame(this);
    header->setObjectName(QStringLiteral("appHeader"));
    auto *headerLayout = new QVBoxLayout(header);
    headerLayout->setContentsMargins(18, 10, 18, 10);
    headerLayout->setSpacing(2);

    auto *title = new QLabel(QStringLiteral("代码统计"), header);
    title->setObjectName(QStringLiteral("appTitle"));
    auto *description = new QLabel(QStringLiteral("选择文件或目录，查看代码、注释与空白行分布"), header);
    description->setObjectName(QStringLiteral("appDescription"));
    headerLayout->addWidget(title);
    headerLayout->addWidget(description);
    ui->verticalLayout_2->insertWidget(0, header);

    QStringList headText;
    headText << "文件名" << "类型" << "大小" << "总行数" << "代码行数" << "注释行数" << "空白行数" << "路径";
    QList<int> columnWidth;
    columnWidth << 130 << 50 << 70 << 80 << 70 << 70 << 70 << 150;

    int columnCount = static_cast<int>(headText.count());
    ui->tableWidget->setColumnCount(columnCount);
    ui->tableWidget->setHorizontalHeaderLabels(headText);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableWidget->verticalHeader()->setVisible(false);
    ui->tableWidget->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidget->horizontalHeader()->setHighlightSections(false);
    ui->tableWidget->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->tableWidget->verticalHeader()->setDefaultSectionSize(20);
    ui->tableWidget->verticalHeader()->setHighlightSections(false);
    ui->tableWidget->setAlternatingRowColors(true);
    ui->tableWidget->setShowGrid(true);

    m_emptyStateLabel = new QLabel(QStringLiteral("选择文件或目录开始统计\n支持按后缀筛选，并可排除指定子目录"),
                                   ui->tableWidget->viewport());
    m_emptyStateLabel->setObjectName(QStringLiteral("emptyState"));
    m_emptyStateLabel->setAlignment(Qt::AlignCenter);
    m_emptyStateLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_emptyStateLabel->show();

    for (int i = 0; i < columnCount; i++) {
        ui->tableWidget->setColumnWidth(i, columnWidth.at(i));
    }

    //设置字体加粗
    QFont font;
    font.setBold(true);
    if (font.pointSize() > 0) {
        font.setPointSize(font.pointSize() + 1);
    } else {
        font.setPixelSize(font.pixelSize() + 2);
    }

    ui->txtCount->setFont(font);
    ui->txtSize->setFont(font);
    ui->txtRow->setFont(font);
    ui->txtCode->setFont(font);
    ui->txtNote->setFont(font);
    ui->txtBlank->setFont(font);

#if (QT_VERSION > QT_VERSION_CHECK(4,7,0))
    ui->txtFilter->setPlaceholderText("中间空格隔开,例如 *.h *.cpp *.c");
#endif
    ui->txtFilter->setText(defaultCodeFileFilter());
    ui->txtFilter->setCursorPosition(0);

    //设置按钮
    ui->btnOpenFile->setRadius(2);
    ui->btnOpenPath->setRadius(2);
    ui->btnClear->setRadius(2);
    ui->btnOpenFile->setBlurRadius(0);
    ui->btnOpenPath->setBlurRadius(0);
    ui->btnClear->setBlurRadius(0);
    ui->btnOpenFile->setFillColor(QColor(QStringLiteral("#002FA7")));
    ui->btnOpenPath->setFillColor(QColor(QStringLiteral("#002FA7")));
    ui->btnClear->setFillColor(QColor(QStringLiteral("#002FA7")));

    applyPlatformUiTuning();
}

void CountCode::applyPlatformUiTuning()
{
#if defined(Q_OS_OPENHARMONY) || defined(OPENHARMONY)
    QFont font = this->font();
    font.setPixelSize(harmonyScaled(13));
    font.setBold(false);
    this->setFont(font);
    qApp->setFont(font);
    qInfo() << "CodeCountTool Harmony UI scale:" << harmonyUiScale();

    const QList<QWidget *> allWidgets = findChildren<QWidget *>();
    for (QWidget *widget : allWidgets) {
        widget->setFont(font);
    }

    const QList<QLineEdit *> summaryFields = {
        ui->txtCount,
        ui->txtSize,
        ui->txtRow,
        ui->txtCode,
        ui->txtNote,
        ui->txtBlank
    };
    QFont summaryFont = font;
    summaryFont.setPixelSize(harmonyScaled(17));
    summaryFont.setBold(true);
    for (QLineEdit *field : summaryFields) {
        setFixedScaled(field, 100, 25);
        field->setFont(summaryFont);
    }

    setScaledHeight(ui->txtFile, 32);
    setScaledHeight(ui->txtPath, 32);
    setScaledHeight(ui->txtFilter, 32);
    setScaledHeight(ui->dirFilter, 32);
    ui->txtFilter->setMinimumWidth(harmonyScaled(260));
    ui->dirFilter->setMinimumWidth(harmonyScaled(160));
    setScaledHeight(ui->code_progressBar, 28);
    setScaledHeight(ui->note_progressBar, 28);
    setScaledHeight(ui->blank_progressBar, 28);

    const QList<RippleButton *> buttons = {
        ui->btnOpenFile,
        ui->btnOpenPath,
        ui->btnClear
    };
    for (RippleButton *button : buttons) {
        setFixedScaled(button, 105, 34);
        QFont buttonFont = font;
        buttonFont.setBold(true);
        button->setFont(buttonFont);
        button->setRadius(harmonyScaled(2));
        button->setBlurRadius(0);
    }

    const QList<int> columnWidth = {
        130, 50, 70, 80, 70, 70, 70, 150
    };
    for (int i = 0; i < columnWidth.count(); ++i) {
        ui->tableWidget->setColumnWidth(i, harmonyScaled(columnWidth.at(i)));
    }

    ui->tableWidget->verticalHeader()->setDefaultSectionSize(harmonyScaled(34));
    ui->tableWidget->horizontalHeader()->setMinimumHeight(harmonyScaled(38));

    QFont tableFont = ui->tableWidget->font();
    tableFont.setPixelSize(harmonyScaled(12));
    tableFont.setBold(false);
    ui->tableWidget->setFont(tableFont);
    QFont headerFont = tableFont;
    headerFont.setBold(true);
    ui->tableWidget->horizontalHeader()->setFont(headerFont);

    QFont titleFont = font;
    titleFont.setPixelSize(harmonyScaled(22));
    titleFont.setBold(true);
    findChild<QLabel *>(QStringLiteral("appTitle"))->setFont(titleFont);

    QFont descriptionFont = font;
    descriptionFont.setPixelSize(harmonyScaled(11));
    findChild<QLabel *>(QStringLiteral("appDescription"))->setFont(descriptionFont);

    ui->verticalLayout_2->setContentsMargins(harmonyScaled(14), harmonyScaled(14),
                                             harmonyScaled(14), harmonyScaled(14));
    ui->verticalLayout_2->setSpacing(harmonyScaled(10));
    ui->horizontalLayout_3->setContentsMargins(harmonyScaled(14), harmonyScaled(12),
                                               harmonyScaled(14), harmonyScaled(12));
    ui->horizontalLayout_3->setSpacing(harmonyScaled(12));
    ui->gridLayout->setHorizontalSpacing(harmonyScaled(8));
    ui->gridLayout->setVerticalSpacing(harmonyScaled(8));

    this->setMinimumSize(1024, 680);
#endif
}

void CountCode::showMessageRow(const QString &message)
{
    ui->tableWidget->clearSpans();
    ui->tableWidget->setRowCount(1);
    for (int column = 0; column < ui->tableWidget->columnCount(); ++column) {
        ui->tableWidget->setItem(0, column, new QTableWidgetItem);
    }
    ui->tableWidget->setSpan(0, 0, 1, ui->tableWidget->columnCount());
    QTableWidgetItem *item = ui->tableWidget->item(0, 0);
    item->setText(message);
    item->setTextAlignment(Qt::AlignCenter);
}

QStringList CountCode::currentFileFilters() const
{
    return splitFilter(ui->txtFilter->text().trimmed());
}

QStringList CountCode::currentDirFilters() const
{
    return splitFilter(ui->dirFilter->text().trimmed());
}

void CountCode::startDirectoryCount(const QString &path)
{
    m_emptyStateLabel->hide();
    setCountingState(true);
    showMessageRow(QStringLiteral("正在扫描目录..."));
    emit countDirectoryRequested(path, currentFileFilters(), currentDirFilters());
}

void CountCode::startFilesCount(const QStringList &files)
{
    m_emptyStateLabel->hide();
    setCountingState(true);
    showMessageRow(QStringLiteral("正在统计..."));
    emit countFilesRequested(files, currentFileFilters());
}

void CountCode::setCountingState(bool counting)
{
    m_counting = counting;
    ui->btnOpenFile->setEnabled(!counting);
    ui->btnOpenPath->setEnabled(!counting);
    ui->txtFilter->setEnabled(!counting);
    ui->dirFilter->setEnabled(!counting);
    ui->btnClear->setText(counting ? QStringLiteral("取消统计") : QStringLiteral("清空结果"));
}

void CountCode::populateResults(const QList<CodeFileStat> &stats)
{
    m_emptyStateLabel->hide();
    quint64 totalLines = 0;
    quint64 totalBytes = 0;
    quint64 totalCodes = 0;
    quint64 totalNotes = 0;
    quint64 totalBlanks = 0;

    const int count = static_cast<int>(stats.count());
    ui->tableWidget->setUpdatesEnabled(false);
    ui->tableWidget->clearSpans();
    ui->tableWidget->setRowCount(count);

    for (int i = 0; i < count; i++) {
        const CodeFileStat &stat = stats.at(i);
        const int lineAll = stat.lineCode + stat.lineNote + stat.lineBlank;

        const auto itemName = new QTableWidgetItem;
        itemName->setText(stat.fileName);

        const auto itemSuffix = new QTableWidgetItem;
        itemSuffix->setText(stat.suffix);

        const auto itemSize = new QTableWidgetItem;
        itemSize->setText(QString::number(stat.bytes));

        const auto itemLine = new QTableWidgetItem;
        itemLine->setText(QString::number(lineAll));

        const auto itemCode = new QTableWidgetItem;
        itemCode->setText(QString::number(stat.lineCode));

        const auto itemNote = new QTableWidgetItem;
        itemNote->setText(QString::number(stat.lineNote));

        const auto itemBlank = new QTableWidgetItem;
        itemBlank->setText(QString::number(stat.lineBlank));

        const auto itemPath = new QTableWidgetItem;
        itemPath->setText(stat.filePath);

        itemSuffix->setTextAlignment(Qt::AlignCenter);
        itemSize->setTextAlignment(Qt::AlignCenter);
        itemLine->setTextAlignment(Qt::AlignCenter);
        itemCode->setTextAlignment(Qt::AlignCenter);
        itemNote->setTextAlignment(Qt::AlignCenter);
        itemBlank->setTextAlignment(Qt::AlignCenter);

        ui->tableWidget->setItem(i, 0, itemName);
        ui->tableWidget->setItem(i, 1, itemSuffix);
        ui->tableWidget->setItem(i, 2, itemSize);
        ui->tableWidget->setItem(i, 3, itemLine);
        ui->tableWidget->setItem(i, 4, itemCode);
        ui->tableWidget->setItem(i, 5, itemNote);
        ui->tableWidget->setItem(i, 6, itemBlank);
        ui->tableWidget->setItem(i, 7, itemPath);

        totalBytes  += stat.bytes;
        totalLines  += lineAll;
        totalCodes  += stat.lineCode;
        totalNotes  += stat.lineNote;
        totalBlanks += stat.lineBlank;
    }
    ui->tableWidget->setUpdatesEnabled(true);

    //显示统计结果
    ui->txtCount->setText(QString::number(count));
    ui->txtSize->setText(QString::number(totalBytes));
    ui->txtRow->setText(QString::number(totalLines));
    ui->txtCode->setText(QString::number(totalCodes));
    ui->txtNote->setText(QString::number(totalNotes));
    ui->txtBlank->setText(QString::number(totalBlanks));

    //计算百分比
    setProgressPercent(ui->code_progressBar, totalCodes, totalLines);
    setProgressPercent(ui->note_progressBar, totalNotes, totalLines);
    setProgressPercent(ui->blank_progressBar, totalBlanks, totalLines);
}

void CountCode::paintEvent(QPaintEvent *event) {
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void CountCode::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (m_emptyStateLabel) {
        m_emptyStateLabel->setGeometry(ui->tableWidget->viewport()->rect());
    }
}

void CountCode::onBtnOpenFileClicked()
{
    if (m_counting) {
        return;
    }
    onBtnClearClicked();
    ui->txtPath->clear();
#if defined(Q_OS_OPENHARMONY) || defined(OPENHARMONY)
    const QString nameFilter = QStringLiteral("所有文件 (*)");
#else
    const QString filter = QString("代码文件(%1)").arg(ui->txtFilter->text().trimmed());
    const QString nameFilter = filter;
#endif
    const QStringList rawFiles = QFileDialog::getOpenFileNames(this, "选择文件", defaultDialogPath(), nameFilter);
    const QStringList files = normalizeSelectedPaths(rawFiles);
    if (!files.isEmpty()) {
#if defined(Q_OS_OPENHARMONY) || defined(OPENHARMONY)
        for (int i = 0; i < files.count(); ++i) {
            logPathDiagnostic(QStringLiteral("file"), rawFiles.value(i), files.at(i));
        }
#endif
        ui->txtFile->setText(files.join("|"));
        startFilesCount(files);
    }
}

void CountCode::onBtnOpenPathClicked()
{
    if (m_counting) {
        return;
    }
    onBtnClearClicked();
    ui->txtFile->clear();
    const QString rawPath = QFileDialog::getExistingDirectory(this, "选择目录", defaultDialogPath(),  QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    const QString path = normalizeSelectedPath(rawPath);

    if (!path.isEmpty()) {
        logPathDiagnostic(QStringLiteral("directory"), rawPath, path);
        ui->txtPath->setText(path);
        startDirectoryCount(path);
    }
}

void CountCode::onCountProgress(int matched, int counted, int total)
{
    if (!m_counting) {
        return;
    }
    if (total < 0) {
        showMessageRow(QStringLiteral("正在扫描目录... 已找到 %1 个匹配文件").arg(matched));
    } else {
        showMessageRow(QStringLiteral("正在统计 %1 / %2 个文件...").arg(counted).arg(total));
    }
}

void CountCode::onCountFinished(const QList<CodeFileStat> &stats, bool canceled)
{
    setCountingState(false);
    populateResults(stats);
    if (stats.isEmpty()) {
        if (canceled) {
            showMessageRow(QStringLiteral("统计已取消"));
        } else if (!ui->txtPath->text().isEmpty()) {
            showMessageRow(QStringLiteral("没有找到匹配的代码文件，请检查目录权限或过滤规则：%1").arg(ui->txtFilter->text().trimmed()));
        } else {
            showMessageRow(QStringLiteral("已选择的文件不符合当前过滤规则：%1").arg(ui->txtFilter->text().trimmed()));
        }
    }
}

void CountCode::onBtnClearClicked()
{
    if (m_counting) {
        //统计进行中时该按钮是“取消统计”
        m_worker->requestCancel();
        return;
    }

    ui->txtCount->setText("0");
    ui->txtSize->setText("0");
    ui->txtRow->setText("0");

    ui->txtCode->setText("0");
    ui->txtNote->setText("0");
    ui->txtBlank->setText("0");

    //ui->labPercentCode->setText("0%");
    //ui->labPercentNote->setText("0%");
    //ui->labPercentBlank->setText("0%");
    ui->code_progressBar->setFormat("0%");
    ui->note_progressBar->setFormat("0%");
    ui->blank_progressBar->setFormat("0%");
    ui->code_progressBar->setValue(0);
    ui->note_progressBar->setValue(0);
    ui->blank_progressBar->setValue(0);

    ui->tableWidget->clearSpans();
    ui->tableWidget->setRowCount(0);
    m_emptyStateLabel->setGeometry(ui->tableWidget->viewport()->rect());
    m_emptyStateLabel->show();
}

void CountCode::onEditingFinished() {
    //qDebug()<<"编辑完成";
    if (m_counting) {
        return;
    }
    onBtnClearClicked();
    if (!ui->txtFile->text().isEmpty()) {
        startFilesCount(ui->txtFile->text().split('|'));
    }
    else if (!ui->txtPath->text().isEmpty()) {
        startDirectoryCount(normalizeSelectedPath(ui->txtPath->text()));
    }
}
