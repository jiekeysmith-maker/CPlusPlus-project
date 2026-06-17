#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QListWidget>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QSplitter>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QStandardPaths>

#include "manage.h"
#include "authorpage.h"
#include "sourcepage.h"
#include "paperpage.h"
#include "attachmentpage.h"
#include "catalogpage.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setupUi();
    setupMenuBar();
    createPages();
    m_defaultDataPath = defaultDataFilePath();
    if (loadDefaultData()) {
        showStatus(QStringLiteral("已自动加载上次保存的数据"));
    } else {
        showStatus(QStringLiteral("就绪"));
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupUi()
{
    setWindowTitle(QStringLiteral("科研文献管理系统 v2.0"));
    resize(1100, 700);

    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    QHBoxLayout *mainLayout = new QHBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 左侧导航栏
    m_sidebar = new QListWidget;
    m_sidebar->setFixedWidth(140);
    m_sidebar->setStyleSheet(
        "QListWidget { background: #2c3e50; color: white; border: none; font-size: 14px; }"
        "QListWidget::item { padding: 14px 8px; text-align: center; }"
        "QListWidget::item:selected { background: #3498db; }"
        "QListWidget::item:hover { background: #34495e; }");

    m_sidebar->addItem(QStringLiteral("作者管理"));
    m_sidebar->addItem(QStringLiteral("出版物管理"));
    m_sidebar->addItem(QStringLiteral("文献管理"));
    m_sidebar->addItem(QStringLiteral("附件管理"));
    m_sidebar->addItem(QStringLiteral("目录管理"));

    for (int i = 0; i < m_sidebar->count(); ++i) {
        m_sidebar->item(i)->setTextAlignment(Qt::AlignCenter);
    }

    // 右侧内容区
    m_stack = new QStackedWidget;

    mainLayout->addWidget(m_sidebar);
    mainLayout->addWidget(m_stack, 1);

    connect(m_sidebar, &QListWidget::currentRowChanged,
            this, &MainWindow::onSidebarChanged);

    m_sidebar->setCurrentRow(0);
}

void MainWindow::setupMenuBar()
{
    QMenu *fileMenu = menuBar()->addMenu(QStringLiteral("文件(&F)"));

    QAction *saveAction = fileMenu->addAction(QStringLiteral("保存(&S)"));
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, &MainWindow::onSave);

    QAction *loadAction = fileMenu->addAction(QStringLiteral("加载(&L)"));
    loadAction->setShortcut(QKeySequence::Open);
    connect(loadAction, &QAction::triggered, this, &MainWindow::onLoad);

    fileMenu->addSeparator();

    QAction *exitAction = fileMenu->addAction(QStringLiteral("退出(&X)"));
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QMainWindow::close);
}

void MainWindow::createPages()
{
    m_authorPage     = new AuthorPage;
    m_sourcePage     = new SourcePage;
    m_paperPage      = new PaperPage;
    m_attachmentPage = new AttachmentPage;
    m_catalogPage    = new CatalogPage;

    m_stack->addWidget(m_authorPage);
    m_stack->addWidget(m_sourcePage);
    m_stack->addWidget(m_paperPage);
    m_stack->addWidget(m_attachmentPage);
    m_stack->addWidget(m_catalogPage);
}

void MainWindow::onSidebarChanged(int row)
{
    m_stack->setCurrentIndex(row);
}

void MainWindow::onSave()
{
    QString path = QFileDialog::getSaveFileName(this,
        QStringLiteral("保存数据"), QString(),
        QStringLiteral("数据文件 (*.txt)"));
    if (path.isEmpty()) return;

    if (LibraryManager::getInstance().saveToFile(path.toStdString())) {
        showStatus(QStringLiteral("已保存到: ") + path);
    } else {
        QMessageBox::warning(this,
            QStringLiteral("保存失败"),
            QStringLiteral("无法写入文件，请检查路径权限。"));
    }
}

void MainWindow::onLoad()
{
    QString path = QFileDialog::getOpenFileName(this,
        QStringLiteral("加载数据"), QString(),
        QStringLiteral("数据文件 (*.txt);;所有文件 (*)"));
    if (path.isEmpty()) return;

    if (LibraryManager::getInstance().loadFromFile(path.toStdString())) {
        refreshAllPages();
        saveDefaultData();
        showStatus(QStringLiteral("已从 ") + path + QStringLiteral(" 加载数据"));
    } else {
        QMessageBox::warning(this,
            QStringLiteral("加载失败"),
            QStringLiteral("文件不存在或格式不正确。"));
    }
}

void MainWindow::showStatus(const QString &msg)
{
    statusBar()->showMessage(msg, 5000);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveDefaultData();
    QMainWindow::closeEvent(event);
}

void MainWindow::refreshAllPages()
{
    m_authorPage->refreshTable();
    m_sourcePage->refreshTable();
    m_paperPage->refreshTable();
    m_attachmentPage->refreshTable();
    m_catalogPage->refreshTree();
}

QString MainWindow::defaultDataFilePath() const
{
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dataDir.isEmpty()) {
        dataDir = QCoreApplication::applicationDirPath() + QStringLiteral("/data");
    }
    QDir dir(dataDir);
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }
    return dir.filePath(QStringLiteral("library_data.txt"));
}

bool MainWindow::saveDefaultData()
{
    if (m_defaultDataPath.isEmpty()) {
        m_defaultDataPath = defaultDataFilePath();
    }
    return LibraryManager::getInstance().saveToFile(m_defaultDataPath.toStdString());
}

bool MainWindow::loadDefaultData()
{
    if (m_defaultDataPath.isEmpty()) {
        m_defaultDataPath = defaultDataFilePath();
    }
    if (!QFile::exists(m_defaultDataPath)) {
        return false;
    }
    if (!LibraryManager::getInstance().loadFromFile(m_defaultDataPath.toStdString())) {
        return false;
    }
    refreshAllPages();
    return true;
}
