#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QAction>
#include <QCloseEvent>
#include <QComboBox>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontMetrics>
#include <QFormLayout>
#include <QFrame>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QKeySequence>
#include <QStatusBar>
#include <QSplitter>
#include <QTableWidget>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QToolBar>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QUrl>

#include <algorithm>
#include <functional>
#include <set>

#include "paperdialog.h"
#include "authordialog.h"
#include "AttachmentFileUtils.h"
#include "StoragePaths.h"

namespace {
constexpr int kRoleNodeType = Qt::UserRole;
constexpr int kRoleCatalogId = Qt::UserRole + 1;
constexpr int kRoleNodeKey = Qt::UserRole + 2;
constexpr int kRoleDeletable = Qt::UserRole + 3;
constexpr int kRolePaperId = Qt::UserRole + 4;
constexpr int kRoleAuthorId = Qt::UserRole + 5;

QString joinStrings(const QStringList &items, const QString &separator = QStringLiteral("; "))
{
    return items.join(separator);
}

QString normalizedAbsolutePath(const QString &path)
{
    return QDir::fromNativeSeparators(QDir::cleanPath(QFileInfo(path).absoluteFilePath()));
}

bool pathIsInsideDirectory(const QString &filePath, const QString &directoryPath)
{
    if (filePath.trimmed().isEmpty() || directoryPath.trimmed().isEmpty()) {
        return false;
    }

    QString file = normalizedAbsolutePath(filePath);
    QString dir = QDir::fromNativeSeparators(QDir::cleanPath(QDir(directoryPath).absolutePath()));
#ifdef Q_OS_WIN
    file = file.toLower();
    dir = dir.toLower();
#endif
    return file == dir || file.startsWith(dir + QLatin1Char('/'));
}

bool pathIsStrictlyInsideDirectory(const QString &path, const QString &directoryPath)
{
    if (path.trimmed().isEmpty() || directoryPath.trimmed().isEmpty()) {
        return false;
    }

    QString target = normalizedAbsolutePath(path);
    QString dir = QDir::fromNativeSeparators(QDir::cleanPath(QDir(directoryPath).absolutePath()));
#ifdef Q_OS_WIN
    target = target.toLower();
    dir = dir.toLower();
#endif
    return target != dir && target.startsWith(dir + QLatin1Char('/'));
}

bool pathIsInsideDataDirectory(const QString &path)
{
    return pathIsInsideDirectory(path, StoragePaths::dataRootPath());
}

bool isDeletableAttachmentDirectory(const QString &path)
{
    return !path.trimmed().isEmpty()
        && QFileInfo(path).isDir()
        && pathIsStrictlyInsideDirectory(path, StoragePaths::attachmentsDirectoryPath());
}

QString normalizedComparablePath(const QString &path)
{
    QString normalized = normalizedAbsolutePath(QDir::fromNativeSeparators(path));
#ifdef Q_OS_WIN
    normalized = normalized.toLower();
#endif
    return normalized;
}

}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setupUi();
    setupMenuBar();
    setupToolbar();
    m_defaultDataPath = defaultDataFilePath();
    if (loadDefaultData()) {
        showStatus(QStringLiteral("已自动加载上次保存的数据"));
    } else {
        rebuildCatalogTree();
        selectSystemNode(QStringLiteral("all"));
        showStatus(QStringLiteral("就绪"));
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupUi()
{
    setWindowTitle(QStringLiteral("科研文献管理系统"));
    resize(1320, 820);

    QWidget *central = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    auto *splitter = new QSplitter(Qt::Horizontal, central);
    splitter->setChildrenCollapsible(false);

    m_sidebar = new QTreeWidget(splitter);
    m_sidebar->setHeaderHidden(true);
    m_sidebar->setMinimumWidth(250);
    m_sidebar->setUniformRowHeights(true);
    m_sidebar->setIndentation(18);
    m_sidebar->setDropIndicatorShown(true);

    m_table = new QTableWidget(splitter);
    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("标题"),
        QStringLiteral("关键词"),
        QStringLiteral("发表时间"),
        QStringLiteral("作者"),
        QStringLiteral("上传时间"),
        QStringLiteral("全文附件")
    });
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setDragEnabled(true);
    m_table->setDragDropMode(QAbstractItemView::DragOnly);
    m_table->setAlternatingRowColors(true);
    m_table->horizontalHeader()->setStretchLastSection(false);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Interactive);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_table->verticalHeader()->setVisible(false);
    m_table->setWordWrap(false);

    setupDetailPanel();

    splitter->addWidget(m_sidebar);
    splitter->addWidget(m_table);
    splitter->addWidget(m_detailPanel);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setStretchFactor(2, 0);

    mainLayout->addWidget(splitter, 1);
    setCentralWidget(central);

    connect(m_sidebar, &QTreeWidget::currentItemChanged,
            this, &MainWindow::onSidebarItemChanged);
    connect(m_table, &QTableWidget::cellDoubleClicked,
            this, &MainWindow::onOpenPaperAttachment);
    connect(m_table, &QTableWidget::currentItemChanged,
            this, [this]() { onPaperSelectionChanged(); });
    m_sidebar->setAcceptDrops(true);
    m_sidebar->viewport()->setAcceptDrops(true);
    m_sidebar->viewport()->installEventFilter(this);
    m_sidebar->setContextMenuPolicy(Qt::CustomContextMenu);

    applyTreeStyle();

    connect(m_sidebar, &QWidget::customContextMenuRequested,
            this, &MainWindow::onCatalogContextMenu);
    rebuildCatalogTree();
    selectSystemNode(QStringLiteral("all"));
}

void MainWindow::setupMenuBar()
{
    QMenu *fileMenu = menuBar()->addMenu(QStringLiteral("文件(&F)"));

    m_saveAction = fileMenu->addAction(QStringLiteral("保存(&S)"));
    m_saveAction->setShortcut(QKeySequence::Save);
    connect(m_saveAction, &QAction::triggered, this, &MainWindow::onSave);

    m_loadAction = fileMenu->addAction(QStringLiteral("加载(&L)"));
    m_loadAction->setShortcut(QKeySequence::Open);
    connect(m_loadAction, &QAction::triggered, this, &MainWindow::onLoad);

    m_importAction = fileMenu->addAction(QStringLiteral("导入数据(&I)"));
    connect(m_importAction, &QAction::triggered, this, &MainWindow::onImport);

    fileMenu->addSeparator();

    QAction *exitAction = fileMenu->addAction(QStringLiteral("退出(&X)"));
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QMainWindow::close);
}

void MainWindow::setupToolbar()
{
    auto *toolbar = addToolBar(QStringLiteral("文献工具栏"));
    toolbar->setMovable(false);
    toolbar->setToolButtonStyle(Qt::ToolButtonTextOnly);

    m_addPaperAction = toolbar->addAction(QStringLiteral("上传/新增文献"));
    connect(m_addPaperAction, &QAction::triggered, this, &MainWindow::onAddPaper);

    m_addAuthorAction = toolbar->addAction(QStringLiteral("新增作者"));
    connect(m_addAuthorAction, &QAction::triggered, this, &MainWindow::onAddAuthor);

    m_editPaperAction = toolbar->addAction(QStringLiteral("编辑文献"));
    connect(m_editPaperAction, &QAction::triggered, this, &MainWindow::onEditPaper);

    m_deletePaperAction = toolbar->addAction(QStringLiteral("删除文献"));
    connect(m_deletePaperAction, &QAction::triggered, this, &MainWindow::onDeletePaper);

    toolbar->addSeparator();

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(QStringLiteral("搜索标题、关键词、作者"));
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setMaximumWidth(360);
    toolbar->addWidget(m_searchEdit);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);

    m_searchButton = new QPushButton(QStringLiteral("搜索"), this);
    toolbar->addWidget(m_searchButton);
    connect(m_searchButton, &QPushButton::clicked, this, &MainWindow::onSearchClicked);

    m_showAllButton = new QPushButton(QStringLiteral("显示全部"), this);
    toolbar->addWidget(m_showAllButton);
    connect(m_showAllButton, &QPushButton::clicked, this, &MainWindow::onShowAllClicked);

    m_selectAllButton = new QPushButton(QStringLiteral("全选"), this);
    toolbar->addWidget(m_selectAllButton);
    connect(m_selectAllButton, &QPushButton::clicked, this, &MainWindow::onSelectAllClicked);

    toolbar->addSeparator();
    auto *spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolbar->addWidget(spacer);

    m_viewDetailButton = new QPushButton(QStringLiteral("查看"), this);
    m_viewDetailButton->setEnabled(false);
    toolbar->addWidget(m_viewDetailButton);
    connect(m_viewDetailButton, &QPushButton::clicked, this, &MainWindow::onViewPaperDetail);
}

void MainWindow::setupDetailPanel()
{
    m_detailPanel = new QWidget;
    m_detailPanel->setMinimumWidth(300);
    m_detailPanel->setMaximumWidth(360);
    m_detailPanel->setStyleSheet(
        "QWidget { background: #ffffff; }"
        "QLabel { color: #111827; }"
        "QPushButton { padding: 6px 12px; }");

    auto *panelLayout = new QVBoxLayout(m_detailPanel);
    panelLayout->setContentsMargins(14, 14, 14, 14);
    panelLayout->setSpacing(10);

    m_detailHeadingLabel = new QLabel(QStringLiteral("文献详情"), m_detailPanel);
    QFont titleFont = m_detailHeadingLabel->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 2);
    m_detailHeadingLabel->setFont(titleFont);
    panelLayout->addWidget(m_detailHeadingLabel);

    auto *scrollArea = new QScrollArea(m_detailPanel);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    auto *content = new QWidget(scrollArea);
    m_detailForm = new QFormLayout(content);
    m_detailForm->setContentsMargins(0, 0, 0, 0);
    m_detailForm->setSpacing(10);
    m_detailForm->setLabelAlignment(Qt::AlignTop | Qt::AlignRight);

    auto makeValueLabel = [content]() {
        auto *label = new QLabel(QStringLiteral("未填写"), content);
        label->setWordWrap(true);
        label->setTextInteractionFlags(Qt::TextSelectableByMouse);
        label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
        return label;
    };

    m_detailTitleLabel = makeValueLabel();
    m_detailAuthorsLabel = makeValueLabel();
    m_detailSourceLabel = makeValueLabel();
    m_detailPublishDateLabel = makeValueLabel();
    m_detailUploadTimeLabel = makeValueLabel();
    m_detailKeywordsLabel = makeValueLabel();
    m_detailFilePathLabel = makeValueLabel();

    m_detailForm->addRow(QStringLiteral("完整标题:"), m_detailTitleLabel);
    m_detailForm->addRow(QStringLiteral("作者名字:"), m_detailAuthorsLabel);
    m_detailForm->addRow(QStringLiteral("出版物:"), m_detailSourceLabel);
    m_detailForm->addRow(QStringLiteral("发表时间:"), m_detailPublishDateLabel);
    m_detailForm->addRow(QStringLiteral("上传时间:"), m_detailUploadTimeLabel);
    m_detailForm->addRow(QStringLiteral("关键词:"), m_detailKeywordsLabel);
    m_detailForm->addRow(QString(), m_detailFilePathLabel);

    m_attachmentSectionLabel = new QLabel(QStringLiteral("附件:"), content);
    QFont attachmentFont = m_attachmentSectionLabel->font();
    attachmentFont.setBold(true);
    m_attachmentSectionLabel->setFont(attachmentFont);
    m_attachmentSectionLabel->setAlignment(Qt::AlignLeft);
    m_detailForm->addRow(m_attachmentSectionLabel);

    m_fullTextRowWidget = new QWidget(content);
    auto *fullTextLayout = new QHBoxLayout(m_fullTextRowWidget);
    fullTextLayout->setContentsMargins(0, 0, 0, 0);
    fullTextLayout->setSpacing(8);
    m_openFullTextButton = new QPushButton(QStringLiteral("打开全文"), m_fullTextRowWidget);
    m_openFullTextButton->setEnabled(false);
    fullTextLayout->addWidget(m_openFullTextButton);
    fullTextLayout->addStretch(1);
    connect(m_openFullTextButton, &QPushButton::clicked,
            this, &MainWindow::onOpenFullText);
    m_detailForm->addRow(QStringLiteral("全文文件:"), m_fullTextRowWidget);

    m_noteRowWidget = new QWidget(content);
    auto *noteLayout = new QVBoxLayout(m_noteRowWidget);
    noteLayout->setContentsMargins(0, 0, 0, 0);
    noteLayout->setSpacing(6);
    auto *noteButtonLayout = new QHBoxLayout;
    noteButtonLayout->setContentsMargins(0, 0, 0, 0);
    noteButtonLayout->setSpacing(6);
    m_noteComboBox = new QComboBox(m_noteRowWidget);
    m_noteComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_uploadNoteButton = new QPushButton(QStringLiteral("上传笔记"), m_noteRowWidget);
    m_openNoteButton = new QPushButton(QStringLiteral("打开笔记"), m_noteRowWidget);
    m_openNoteButton->setEnabled(false);
    noteButtonLayout->addWidget(m_uploadNoteButton);
    noteButtonLayout->addWidget(m_openNoteButton);
    noteButtonLayout->addStretch(1);
    noteLayout->addLayout(noteButtonLayout);
    noteLayout->addWidget(m_noteComboBox);
    connect(m_uploadNoteButton, &QPushButton::clicked,
            this, &MainWindow::onUploadPaperNote);
    connect(m_openNoteButton, &QPushButton::clicked,
            this, &MainWindow::onOpenSelectedPaperNote);
    connect(m_noteComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) {
                if (m_openNoteButton && m_noteComboBox) {
                    m_openNoteButton->setEnabled(m_noteComboBox->currentData().toLongLong() != INVALID_ID);
                }
            });
    m_detailForm->addRow(QStringLiteral("笔记:"), m_noteRowWidget);

    scrollArea->setWidget(content);
    panelLayout->addWidget(scrollArea, 1);

    m_detailPanel->hide();
}

void MainWindow::applyTreeStyle()
{
    m_sidebar->setStyleSheet(
        "QTreeWidget { background: #f5f6f8; border: none; font-size: 14px; }"
        "QTreeWidget::item { height: 28px; padding: 3px 6px; }"
        "QTreeWidget::item:selected { background: #d7e8ff; color: #1f3b66; }"
        "QTreeWidget::branch:has-children { image: none; }"
        "QTreeWidget::branch:closed:has-children { image: none; }"
        "QTreeWidget::branch:open:has-children { image: none; }");
    m_table->setStyleSheet(
        "QTableWidget { border: none; gridline-color: #e5e7eb; font-size: 13px; }"
        "QHeaderView::section { background: #f3f4f6; padding: 8px; border: none; border-right: 1px solid #e5e7eb; }"
        "QTableWidget::item:selected { background: #e8f2ff; color: #111827; }");
}

void MainWindow::rebuildCatalogTree()
{
    const QString previousKey = currentNodeKey();
    m_sidebar->blockSignals(true);
    m_sidebar->clear();

    auto *myLibrary = new QTreeWidgetItem(m_sidebar, {QStringLiteral("我的文库")});
    myLibrary->setData(0, kRoleNodeType, static_cast<int>(LibraryNodeType::PaperSystem));
    myLibrary->setData(0, kRoleNodeKey, QStringLiteral("library"));
    myLibrary->setData(0, kRoleDeletable, false);

    auto *allPapers = new QTreeWidgetItem(myLibrary, {QStringLiteral("全部文献")});
    allPapers->setData(0, kRoleNodeType, static_cast<int>(LibraryNodeType::PaperSystem));
    allPapers->setData(0, kRoleNodeKey, QStringLiteral("all"));
    allPapers->setData(0, kRoleDeletable, false);

    auto *recent = new QTreeWidgetItem(myLibrary, {QStringLiteral("最近添加")});
    recent->setData(0, kRoleNodeType, static_cast<int>(LibraryNodeType::PaperSystem));
    recent->setData(0, kRoleNodeKey, QStringLiteral("recent"));
    recent->setData(0, kRoleDeletable, false);

    auto *uncategorized = new QTreeWidgetItem(myLibrary, {QStringLiteral("未分类")});
    uncategorized->setData(0, kRoleNodeType, static_cast<int>(LibraryNodeType::PaperSystem));
    uncategorized->setData(0, kRoleNodeKey, QStringLiteral("uncategorized"));
    uncategorized->setData(0, kRoleDeletable, false);

    const auto &catalogs = LibraryManager::getInstance().getAllCatalogs();
    QMap<IdType, QTreeWidgetItem*> itemMap;
    itemMap.insert(INVALID_ID, myLibrary);

    std::function<void(QTreeWidgetItem*, IdType)> buildChildren = [&](QTreeWidgetItem *parentItem, IdType parentCatalogId) {
        for (const auto &pair : catalogs) {
            const Catalog &catalog = pair.second;
            if (catalog.getParentId() == parentCatalogId) {
                auto *item = new QTreeWidgetItem(parentItem, {QString::fromStdString(catalog.getName())});
                item->setData(0, kRoleNodeType, static_cast<int>(LibraryNodeType::PaperCatalog));
                item->setData(0, kRoleCatalogId, static_cast<int>(catalog.getId()));
                item->setData(0, kRoleNodeKey, QStringLiteral("catalog_%1").arg(catalog.getId()));
                item->setData(0, kRoleDeletable, true);
                itemMap.insert(catalog.getId(), item);
                buildChildren(item, catalog.getId());
            }
        }
    };

    buildChildren(myLibrary, INVALID_ID);

    auto *authorLibrary = new QTreeWidgetItem(m_sidebar, {QStringLiteral("作者库")});
    authorLibrary->setData(0, kRoleNodeType, static_cast<int>(LibraryNodeType::AuthorSystem));
    authorLibrary->setData(0, kRoleNodeKey, QStringLiteral("authors"));
    authorLibrary->setData(0, kRoleDeletable, false);

    const auto &authorCatalogs = LibraryManager::getInstance().getAllAuthorCatalogs();
    std::function<void(QTreeWidgetItem*, IdType)> buildAuthorChildren = [&](QTreeWidgetItem *parentItem, IdType parentCatalogId) {
        for (const auto &pair : authorCatalogs) {
            const AuthorCatalog &catalog = pair.second;
            if (catalog.getParentId() == parentCatalogId) {
                auto *item = new QTreeWidgetItem(parentItem, {QString::fromStdString(catalog.getName())});
                item->setData(0, kRoleNodeType, static_cast<int>(LibraryNodeType::AuthorCatalog));
                item->setData(0, kRoleCatalogId, static_cast<int>(catalog.getId()));
                item->setData(0, kRoleNodeKey, QStringLiteral("author_catalog_%1").arg(catalog.getId()));
                item->setData(0, kRoleDeletable, true);
                buildAuthorChildren(item, catalog.getId());
            }
        }
    };

    buildAuthorChildren(authorLibrary, INVALID_ID);

    myLibrary->setExpanded(true);
    authorLibrary->setExpanded(true);
    recent->setExpanded(true);
    m_sidebar->expandItem(myLibrary);
    m_sidebar->expandItem(authorLibrary);

    m_sidebar->blockSignals(false);
    QTreeWidgetItem *target = findNodeByKey(previousKey);
    if (!target) {
        target = allPapers;
    }
    m_sidebar->setCurrentItem(target);
}

QTreeWidgetItem *MainWindow::findNodeByKey(const QString &key) const
{
    if (key.isEmpty()) {
        return nullptr;
    }
    auto matchesKey = [&](QTreeWidgetItem *item) {
        return item && item->data(0, kRoleNodeKey).toString() == key;
    };
    std::function<QTreeWidgetItem*(QTreeWidgetItem*)> walk = [&](QTreeWidgetItem *parent) -> QTreeWidgetItem* {
        if (!parent) return nullptr;
        if (matchesKey(parent)) return parent;
        for (int i = 0; i < parent->childCount(); ++i) {
            if (auto *found = walk(parent->child(i))) return found;
        }
        return nullptr;
    };
    for (int i = 0; i < m_sidebar->topLevelItemCount(); ++i) {
        if (auto *found = walk(m_sidebar->topLevelItem(i))) return found;
    }
    return nullptr;
}

QString MainWindow::currentNodeKey() const
{
    if (auto *item = m_sidebar->currentItem()) {
        return item->data(0, kRoleNodeKey).toString();
    }
    return m_currentNodeKey;
}

bool MainWindow::isSystemNode(const QTreeWidgetItem *item) const
{
    if (!item) return false;
    const int type = item->data(0, kRoleNodeType).toInt();
    return type == static_cast<int>(LibraryNodeType::PaperSystem)
        || type == static_cast<int>(LibraryNodeType::AuthorSystem);
}

bool MainWindow::isUserCatalogNode(const QTreeWidgetItem *item) const
{
    if (!item) return false;
    return item->data(0, kRoleNodeType).toInt() == static_cast<int>(LibraryNodeType::PaperCatalog);
}

bool MainWindow::isUserAuthorCatalogNode(const QTreeWidgetItem *item) const
{
    if (!item) return false;
    return item->data(0, kRoleNodeType).toInt() == static_cast<int>(LibraryNodeType::AuthorCatalog);
}

IdType MainWindow::currentCatalogId() const
{
    if (auto *item = m_sidebar->currentItem(); item && isUserCatalogNode(item)) {
        return static_cast<IdType>(item->data(0, kRoleCatalogId).toInt());
    }
    return INVALID_ID;
}

IdType MainWindow::currentAuthorCatalogId() const
{
    if (auto *item = m_sidebar->currentItem(); item && isUserAuthorCatalogNode(item)) {
        return static_cast<IdType>(item->data(0, kRoleCatalogId).toInt());
    }
    return INVALID_ID;
}

bool MainWindow::isAuthorMode() const
{
    return m_contentMode == MainContentMode::Authors;
}

void MainWindow::updateContentModeForNode(const QTreeWidgetItem *item)
{
    if (!item) {
        m_contentMode = MainContentMode::Papers;
        return;
    }

    const int type = item->data(0, kRoleNodeType).toInt();
    if (type == static_cast<int>(LibraryNodeType::AuthorSystem)
        || type == static_cast<int>(LibraryNodeType::AuthorCatalog)) {
        m_contentMode = MainContentMode::Authors;
    } else {
        m_contentMode = MainContentMode::Papers;
    }
}

void MainWindow::updateToolbarForMode()
{
    const bool authors = isAuthorMode();
    if (m_editPaperAction) {
        m_editPaperAction->setText(authors ? QStringLiteral("编辑作者") : QStringLiteral("编辑文献"));
    }
    if (m_deletePaperAction) {
        m_deletePaperAction->setText(authors ? QStringLiteral("删除作者") : QStringLiteral("删除文献"));
    }
    if (m_searchEdit) {
        m_searchEdit->setPlaceholderText(authors
            ? QStringLiteral("搜索姓名、性别、单位、Email、研究领域")
            : QStringLiteral("搜索标题、关键词、作者"));
    }
}

void MainWindow::selectSystemNode(const QString &key)
{
    m_currentNodeKey = key;
    if (auto *item = findNodeByKey(key)) {
        m_sidebar->setCurrentItem(item);
    }
}

std::vector<Paper> MainWindow::collectCurrentPapers() const
{
    auto &mgr = LibraryManager::getInstance();
    if (auto *item = m_sidebar->currentItem()) {
        const QString key = item->data(0, kRoleNodeKey).toString();
        if (key == QStringLiteral("all") || key == QStringLiteral("library")) {
            std::vector<Paper> papers;
            const auto &all = mgr.getAllPapers();
            for (const auto &pair : all) {
                papers.push_back(pair.second);
            }
            return papers;
        }
        if (key == QStringLiteral("recent")) {
            std::vector<Paper> papers;
            const auto &all = mgr.getAllPapers();
            for (const auto &pair : all) {
                papers.push_back(pair.second);
            }
            std::sort(papers.begin(), papers.end(), [](const Paper &a, const Paper &b) {
                return a.getUploadTime() > b.getUploadTime();
            });
            if (papers.size() > 30) {
                papers.resize(30);
            }
            return papers;
        }
        if (key == QStringLiteral("uncategorized")) {
            std::set<IdType> inCatalog;
            for (const auto &catalogPair : mgr.getAllCatalogs()) {
                for (IdType paperId : catalogPair.second.getPaperIds()) {
                    inCatalog.insert(paperId);
                }
            }
            std::vector<Paper> papers;
            for (const auto &pair : mgr.getAllPapers()) {
                if (!inCatalog.count(pair.first)) {
                    papers.push_back(pair.second);
                }
            }
            return papers;
        }
        if (isUserCatalogNode(item)) {
            return mgr.getPapersInCatalog(currentCatalogId());
        }
    }

    std::vector<Paper> papers;
    for (const auto &pair : mgr.getAllPapers()) {
        papers.push_back(pair.second);
    }
    return papers;
}

std::vector<Author> MainWindow::collectCurrentAuthors() const
{
    auto &mgr = LibraryManager::getInstance();
    if (auto *item = m_sidebar->currentItem()) {
        if (isUserAuthorCatalogNode(item)) {
            return mgr.getAuthorsInCatalog(currentAuthorCatalogId());
        }
    }

    std::vector<Author> authors;
    for (const auto &pair : mgr.getAllAuthors()) {
        authors.push_back(pair.second);
    }
    return authors;
}

QString MainWindow::paperAuthorsText(const Paper &paper) const
{
    auto &mgr = LibraryManager::getInstance();
    QStringList names;
    for (IdType id : paper.getAuthorIds()) {
        names << QString::fromStdString(mgr.getAuthorName(id));
    }
    return joinStrings(names);
}

QString MainWindow::paperKeywordsText(const Paper &paper) const
{
    QStringList kws;
    for (const auto &kw : paper.getKeywords()) {
        kws << QString::fromStdString(kw);
    }
    return joinStrings(kws, QStringLiteral(", "));
}

QString MainWindow::authorResearchAreasText(const Author &author) const
{
    QStringList areas;
    for (const auto &area : author.getResearchAreas()) {
        areas << QString::fromStdString(area);
    }
    return joinStrings(areas);
}

QString MainWindow::authorPapersText(IdType authorId) const
{
    QStringList lines;
    for (const auto &pair : LibraryManager::getInstance().getAllPapers()) {
        const Paper &paper = pair.second;
        const auto &authorIds = paper.getAuthorIds();
        if (std::find(authorIds.begin(), authorIds.end(), authorId) == authorIds.end()) {
            continue;
        }

        const QString title = QString::fromStdString(paper.getTitle()).trimmed().isEmpty()
            ? QStringLiteral("未命名文献")
            : QString::fromStdString(paper.getTitle());
        const QString publishDate = QString::fromStdString(paper.getPublishDate()).trimmed().isEmpty()
            ? QStringLiteral("发表时间未填写")
            : QString::fromStdString(paper.getPublishDate());
        lines << QStringLiteral("- %1（%2）").arg(title, publishDate);
    }

    return lines.isEmpty() ? QStringLiteral("暂无关联文献") : lines.join(QLatin1Char('\n'));
}

QString MainWindow::paperCatalogText(const Paper &paper) const
{
    if (paper.getFilePath().empty()) {
        return QStringLiteral("无");
    }
    return QStringLiteral("打开");
}

QString MainWindow::paperUploadTimeText(const Paper &paper) const
{
    if (!paper.getUploadTime().empty()) {
        return QString::fromStdString(paper.getUploadTime());
    }
    return QStringLiteral("-");
}

void MainWindow::ensureUploadTime(Paper &paper) const
{
    if (paper.getUploadTime().empty()) {
        paper.setUploadTime(defaultUploadTime().toStdString());
    }
}

QString MainWindow::defaultUploadTime() const
{
    return QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
}

std::vector<Paper> MainWindow::filterPapers(const std::vector<Paper> &papers, const QString &keyword) const
{
    const QString trimmed = keyword.trimmed();
    if (trimmed.isEmpty()) {
        return papers;
    }

    std::vector<Paper> filtered;
    for (const Paper &paper : papers) {
        const QString title = QString::fromStdString(paper.getTitle());
        const QString keywords = paperKeywordsText(paper);
        const QString authors = paperAuthorsText(paper);
        if (title.contains(trimmed, Qt::CaseInsensitive) ||
            keywords.contains(trimmed, Qt::CaseInsensitive) ||
            authors.contains(trimmed, Qt::CaseInsensitive)) {
            filtered.push_back(paper);
        }
    }
    return filtered;
}

std::vector<Author> MainWindow::filterAuthors(const std::vector<Author> &authors, const QString &keyword) const
{
    const QString trimmed = keyword.trimmed();
    if (trimmed.isEmpty()) {
        return authors;
    }

    std::vector<Author> filtered;
    for (const Author &author : authors) {
        const QString name = QString::fromStdString(author.getName());
        const QString gender = QString::fromStdString(author.getGender());
        const QString affiliation = QString::fromStdString(author.getAffiliation());
        const QString email = QString::fromStdString(author.getEmail());
        const QString areas = authorResearchAreasText(author);
        if (name.contains(trimmed, Qt::CaseInsensitive) ||
            gender.contains(trimmed, Qt::CaseInsensitive) ||
            affiliation.contains(trimmed, Qt::CaseInsensitive) ||
            email.contains(trimmed, Qt::CaseInsensitive) ||
            areas.contains(trimmed, Qt::CaseInsensitive)) {
            filtered.push_back(author);
        }
    }
    return filtered;
}

void MainWindow::updatePaperTable(const std::vector<Paper> &papers)
{
    m_table->setRowCount(0);
    m_table->clear();
    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("标题"),
        QStringLiteral("关键词"),
        QStringLiteral("发表时间"),
        QStringLiteral("作者"),
        QStringLiteral("上传时间"),
        QStringLiteral("全文附件")
    });
    m_table->setDragEnabled(true);
    m_table->setDragDropMode(QAbstractItemView::DragOnly);
    m_table->horizontalHeader()->setStretchLastSection(false);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Interactive);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_table->setRowCount(static_cast<int>(papers.size()));
    for (int row = 0; row < static_cast<int>(papers.size()); ++row) {
        const Paper &paper = papers[static_cast<size_t>(row)];
        auto *titleItem = new QTableWidgetItem(QString::fromStdString(paper.getTitle()));
        titleItem->setData(kRolePaperId, static_cast<qint64>(paper.getId()));
        m_table->setItem(row, 0, titleItem);
        m_table->setItem(row, 1, new QTableWidgetItem(paperKeywordsText(paper)));
        m_table->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(paper.getPublishDate())));
        m_table->setItem(row, 3, new QTableWidgetItem(paperAuthorsText(paper)));
        m_table->setItem(row, 4, new QTableWidgetItem(paperUploadTimeText(paper)));

        auto *openButton = new QPushButton(paperCatalogText(paper), m_table);
        openButton->setEnabled(!paper.getFilePath().empty());
        connect(openButton, &QPushButton::clicked, this, [this, paper]() {
            const QString path = QString::fromStdString(paper.getFilePath());
            if (path.isEmpty() || !QFile::exists(path)) {
                QMessageBox::warning(this, QStringLiteral("无法打开"), QStringLiteral("PDF 文件不存在或路径无效。"));
                return;
            }
            if (!QDesktopServices::openUrl(QUrl::fromLocalFile(path))) {
                QMessageBox::warning(this, QStringLiteral("无法打开"), QStringLiteral("系统默认程序无法打开该文件。"));
            }
        });
        m_table->setCellWidget(row, 5, openButton);
    }
    m_table->resizeRowsToContents();
    adjustAuthorColumnWidth();
    onPaperSelectionChanged();
}

void MainWindow::updateAuthorTable(const std::vector<Author> &authors)
{
    m_table->setRowCount(0);
    m_table->clear();
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("姓名"),
        QStringLiteral("性别"),
        QStringLiteral("单位"),
        QStringLiteral("Email"),
        QStringLiteral("研究领域")
    });
    m_table->setDragEnabled(false);
    m_table->setDragDropMode(QAbstractItemView::NoDragDrop);
    m_table->horizontalHeader()->setStretchLastSection(false);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    m_table->setRowCount(static_cast<int>(authors.size()));

    for (int row = 0; row < static_cast<int>(authors.size()); ++row) {
        const Author &author = authors[static_cast<size_t>(row)];
        auto *nameItem = new QTableWidgetItem(QString::fromStdString(author.getName()));
        nameItem->setData(kRoleAuthorId, static_cast<qint64>(author.getId()));
        m_table->setItem(row, 0, nameItem);
        m_table->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(author.getGender())));
        m_table->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(author.getAffiliation())));
        m_table->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(author.getEmail())));
        m_table->setItem(row, 4, new QTableWidgetItem(authorResearchAreasText(author)));
    }

    m_table->resizeRowsToContents();
    onPaperSelectionChanged();
}

void MainWindow::refreshPaperTable()
{
    const QString keyword = m_searchEdit ? m_searchEdit->text() : QString();
    if (isAuthorMode()) {
        updateAuthorTable(filterAuthors(collectCurrentAuthors(), keyword));
    } else {
        updatePaperTable(filterPapers(collectCurrentPapers(), keyword));
    }
}

void MainWindow::setDetailRowLabel(QWidget *field, const QString &text)
{
    if (!m_detailForm || !field) {
        return;
    }
    if (auto *label = qobject_cast<QLabel *>(m_detailForm->labelForField(field))) {
        label->setText(text);
    }
}

void MainWindow::setDetailRowVisible(QWidget *field, bool visible)
{
    if (!m_detailForm || !field) {
        return;
    }
    if (auto *label = m_detailForm->labelForField(field)) {
        label->setVisible(visible);
    }
    field->setVisible(visible);
}

void MainWindow::setAttachmentControlsVisible(bool visible)
{
    setDetailRowVisible(m_attachmentSectionLabel, visible);
    setDetailRowVisible(m_fullTextRowWidget, visible);
    setDetailRowVisible(m_noteRowWidget, visible);
}

void MainWindow::refreshNoteControls(IdType paperId)
{
    if (!m_noteComboBox || !m_openNoteButton) {
        return;
    }

    struct NoteItem {
        QString displayName;
        IdType attachmentId = INVALID_ID;
    };

    std::vector<NoteItem> notes;
    for (const Attachment &attachment : LibraryManager::getInstance().getAttachmentsByPaper(paperId)) {
        const QString filePath = QString::fromStdString(attachment.getFilePath()).trimmed();
        if (filePath.isEmpty()) {
            continue;
        }

        QString displayName = QString::fromStdString(attachment.getName()).trimmed();
        if (displayName.isEmpty()) {
            displayName = QFileInfo(filePath).fileName();
        }
        notes.push_back({displayName, attachment.getId()});
    }

    std::sort(notes.begin(), notes.end(), [](const NoteItem &a, const NoteItem &b) {
        return QString::localeAwareCompare(a.displayName, b.displayName) < 0;
    });

    const QSignalBlocker blocker(m_noteComboBox);
    m_noteComboBox->clear();
    if (notes.empty()) {
        m_noteComboBox->addItem(QStringLiteral("暂无笔记"), static_cast<qint64>(INVALID_ID));
        m_openNoteButton->setEnabled(false);
        return;
    }

    for (const NoteItem &note : notes) {
        m_noteComboBox->addItem(note.displayName, static_cast<qint64>(note.attachmentId));
    }
    m_noteComboBox->setCurrentIndex(0);
    m_openNoteButton->setEnabled(true);
}

void MainWindow::updateDetailPanel(IdType paperId)
{
    Paper *paper = LibraryManager::getInstance().findPaper(paperId);
    if (!paper) {
        clearDetailPanel();
        return;
    }

    auto valueOrDefault = [](const QString &value) {
        const QString trimmed = value.trimmed();
        return trimmed.isEmpty() ? QStringLiteral("未填写") : trimmed;
    };

    const QString authors = paperAuthorsText(*paper);
    QString source = QStringLiteral("未填写");
    if (paper->getSourceId() != INVALID_ID) {
        source = valueOrDefault(QString::fromStdString(
            LibraryManager::getInstance().getSourceName(paper->getSourceId())));
    }

    const QString filePath = QDir::fromNativeSeparators(QString::fromStdString(paper->getFilePath()));

    m_detailPaperId = paperId;
    m_detailAuthorId = INVALID_ID;
    if (m_detailHeadingLabel) {
        m_detailHeadingLabel->setText(QStringLiteral("文献详情"));
    }
    setDetailRowLabel(m_detailTitleLabel, QStringLiteral("完整标题:"));
    setDetailRowLabel(m_detailAuthorsLabel, QStringLiteral("作者名字:"));
    setDetailRowLabel(m_detailSourceLabel, QStringLiteral("出版物:"));
    setDetailRowLabel(m_detailPublishDateLabel, QStringLiteral("发表时间:"));
    setDetailRowLabel(m_detailUploadTimeLabel, QStringLiteral("上传时间:"));
    setDetailRowLabel(m_detailKeywordsLabel, QStringLiteral("关键词:"));
    setDetailRowVisible(m_detailTitleLabel, true);
    setDetailRowVisible(m_detailAuthorsLabel, true);
    setDetailRowVisible(m_detailSourceLabel, true);
    setDetailRowVisible(m_detailPublishDateLabel, true);
    setDetailRowVisible(m_detailUploadTimeLabel, true);
    setDetailRowVisible(m_detailKeywordsLabel, true);
    setDetailRowVisible(m_detailFilePathLabel, false);
    setAttachmentControlsVisible(true);
    m_detailTitleLabel->setText(valueOrDefault(QString::fromStdString(paper->getTitle())));
    m_detailAuthorsLabel->setText(valueOrDefault(authors));
    m_detailSourceLabel->setText(source);
    m_detailPublishDateLabel->setText(valueOrDefault(QString::fromStdString(paper->getPublishDate())));
    m_detailUploadTimeLabel->setText(valueOrDefault(QString::fromStdString(paper->getUploadTime())));
    m_detailKeywordsLabel->setText(valueOrDefault(paperKeywordsText(*paper)));
    m_detailFilePathLabel->clear();
    m_detailFilePathLabel->setToolTip(QString());
    if (m_openFullTextButton) {
        m_openFullTextButton->setEnabled(!filePath.trimmed().isEmpty() && QFile::exists(filePath));
    }
    if (m_uploadNoteButton) {
        m_uploadNoteButton->setEnabled(true);
    }
    refreshNoteControls(paperId);

    m_detailPanel->show();
}

void MainWindow::updateAuthorDetailPanel(IdType authorId)
{
    Author *author = LibraryManager::getInstance().findAuthor(authorId);
    if (!author) {
        clearDetailPanel();
        return;
    }

    auto valueOrDefault = [](const QString &value) {
        const QString trimmed = value.trimmed();
        return trimmed.isEmpty() ? QStringLiteral("未填写") : trimmed;
    };

    m_detailPaperId = INVALID_ID;
    m_detailAuthorId = authorId;
    if (m_detailHeadingLabel) {
        m_detailHeadingLabel->setText(QStringLiteral("作者详情"));
    }
    setDetailRowLabel(m_detailTitleLabel, QStringLiteral("姓名:"));
    setDetailRowLabel(m_detailAuthorsLabel, QStringLiteral("性别:"));
    setDetailRowLabel(m_detailSourceLabel, QStringLiteral("单位:"));
    setDetailRowLabel(m_detailPublishDateLabel, QStringLiteral("Email:"));
    setDetailRowLabel(m_detailUploadTimeLabel, QStringLiteral("研究领域:"));
    setDetailRowLabel(m_detailKeywordsLabel, QStringLiteral("该作者的文献:"));
    setDetailRowVisible(m_detailTitleLabel, true);
    setDetailRowVisible(m_detailAuthorsLabel, true);
    setDetailRowVisible(m_detailSourceLabel, true);
    setDetailRowVisible(m_detailPublishDateLabel, true);
    setDetailRowVisible(m_detailUploadTimeLabel, true);
    setDetailRowVisible(m_detailKeywordsLabel, true);
    setDetailRowVisible(m_detailFilePathLabel, false);
    setAttachmentControlsVisible(false);

    m_detailTitleLabel->setText(valueOrDefault(QString::fromStdString(author->getName())));
    m_detailAuthorsLabel->setText(valueOrDefault(QString::fromStdString(author->getGender())));
    m_detailSourceLabel->setText(valueOrDefault(QString::fromStdString(author->getAffiliation())));
    m_detailPublishDateLabel->setText(valueOrDefault(QString::fromStdString(author->getEmail())));
    m_detailUploadTimeLabel->setText(valueOrDefault(authorResearchAreasText(*author)));
    m_detailKeywordsLabel->setText(authorPapersText(authorId));
    m_detailFilePathLabel->clear();
    m_detailFilePathLabel->setToolTip(QString());
    if (m_openFullTextButton) {
        m_openFullTextButton->setEnabled(false);
    }
    if (m_openNoteButton) {
        m_openNoteButton->setEnabled(false);
    }

    m_detailPanel->show();
}

void MainWindow::clearDetailPanel()
{
    m_detailPaperId = INVALID_ID;
    m_detailAuthorId = INVALID_ID;
    if (m_detailTitleLabel) {
        m_detailTitleLabel->setText(QStringLiteral("未填写"));
        m_detailAuthorsLabel->setText(QStringLiteral("未填写"));
        m_detailSourceLabel->setText(QStringLiteral("未填写"));
        m_detailPublishDateLabel->setText(QStringLiteral("未填写"));
        m_detailUploadTimeLabel->setText(QStringLiteral("未填写"));
        m_detailKeywordsLabel->setText(QStringLiteral("未填写"));
        m_detailFilePathLabel->setText(QStringLiteral("无附件"));
        m_detailFilePathLabel->setToolTip(QString());
    }
    if (m_openFullTextButton) {
        m_openFullTextButton->setEnabled(false);
    }
    if (m_openNoteButton) {
        m_openNoteButton->setEnabled(false);
    }
    if (m_uploadNoteButton) {
        m_uploadNoteButton->setEnabled(false);
    }
    if (m_noteComboBox) {
        m_noteComboBox->clear();
        m_noteComboBox->addItem(QStringLiteral("暂无笔记"), static_cast<qint64>(INVALID_ID));
    }
    setAttachmentControlsVisible(false);
    if (m_detailPanel) {
        m_detailPanel->hide();
    }
}

void MainWindow::onViewPaperDetail()
{
    if (isAuthorMode()) {
        const IdType authorId = selectedAuthorId();
        if (authorId == INVALID_ID) {
            clearDetailPanel();
            return;
        }
        if (m_detailPanel && m_detailPanel->isVisible() && m_detailAuthorId == authorId) {
            m_detailPanel->hide();
            return;
        }
        updateAuthorDetailPanel(authorId);
        return;
    }

    const IdType paperId = selectedPaperId();
    if (paperId == INVALID_ID) {
        clearDetailPanel();
        return;
    }
    if (m_detailPanel && m_detailPanel->isVisible() && m_detailPaperId == paperId) {
        m_detailPanel->hide();
        return;
    }
    updateDetailPanel(paperId);
}

void MainWindow::onPaperSelectionChanged()
{
    const bool hasPaper = isAuthorMode()
        ? selectedAuthorId() != INVALID_ID
        : selectedPaperId() != INVALID_ID;
    if (m_viewDetailButton) {
        m_viewDetailButton->setEnabled(hasPaper);
    }
}

void MainWindow::onOpenFullText()
{
    Paper *paper = LibraryManager::getInstance().findPaper(m_detailPaperId);
    if (!paper) {
        clearDetailPanel();
        QMessageBox::warning(this, QStringLiteral("无法打开"), QStringLiteral("全文文件不存在或路径无效"));
        return;
    }

    const QString path = QDir::fromNativeSeparators(QString::fromStdString(paper->getFilePath()));
    if (path.isEmpty() || !QFile::exists(path)) {
        QMessageBox::warning(this, QStringLiteral("无法打开"), QStringLiteral("全文文件不存在或路径无效"));
        return;
    }
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(path))) {
        QMessageBox::warning(this, QStringLiteral("无法打开"), QStringLiteral("系统默认程序无法打开该文件。"));
    }
}

void MainWindow::onUploadPaperNote()
{
    const IdType paperId = m_detailPaperId;
    if (paperId == INVALID_ID) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请先选择并查看一篇文献。"));
        return;
    }

    Paper *paper = LibraryManager::getInstance().findPaper(paperId);
    if (!paper) {
        clearDetailPanel();
        QMessageBox::warning(this, QStringLiteral("上传失败"), QStringLiteral("当前文献不存在。"));
        return;
    }

    const QStringList sourcePaths = QFileDialog::getOpenFileNames(
        this,
        QStringLiteral("上传笔记"),
        QString(),
        noteAttachmentFileFilter());
    if (sourcePaths.isEmpty()) {
        return;
    }

    const AttachmentUploadResult uploadResult = uploadNoteAttachmentsForPaper(*paper, sourcePaths);

    if (uploadResult.successCount > 0) {
        saveDefaultData();
        updateDetailPanel(paperId);
        showStatus(QStringLiteral("已上传 %1 个笔记文件").arg(uploadResult.successCount));
    }

    if (!uploadResult.failedFiles.isEmpty()) {
        QMessageBox::warning(
            this,
            QStringLiteral("部分笔记上传失败"),
            QStringLiteral("以下文件上传失败：\n%1").arg(uploadResult.failedFiles.join(QLatin1Char('\n'))));
    }
}

void MainWindow::onOpenSelectedPaperNote()
{
    if (!m_noteComboBox) {
        return;
    }

    const IdType attachmentId = static_cast<IdType>(m_noteComboBox->currentData().toLongLong());
    if (attachmentId == INVALID_ID) {
        return;
    }

    Attachment *attachment = LibraryManager::getInstance().findAttachment(attachmentId);
    if (!attachment) {
        QMessageBox::warning(this, QStringLiteral("无法打开"), QStringLiteral("笔记文件不存在或路径无效。"));
        refreshNoteControls(m_detailPaperId);
        return;
    }

    const QString path = QDir::fromNativeSeparators(QString::fromStdString(attachment->getFilePath()));
    if (path.isEmpty() || !QFile::exists(path)) {
        QMessageBox::warning(this, QStringLiteral("无法打开"), QStringLiteral("笔记文件不存在或路径无效。"));
        return;
    }
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(path))) {
        QMessageBox::warning(this, QStringLiteral("无法打开"), QStringLiteral("系统默认程序无法打开该文件。"));
    }
}

void MainWindow::onSidebarItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    Q_UNUSED(previous)
    if (!current) return;
    m_currentNodeKey = current->data(0, kRoleNodeKey).toString();
    const MainContentMode previousMode = m_contentMode;
    updateContentModeForNode(current);
    updateToolbarForMode();
    if (previousMode != m_contentMode) {
        clearDetailPanel();
    }
    refreshPaperTable();
}

void MainWindow::onSearchTextChanged(const QString &text)
{
    Q_UNUSED(text)
    refreshPaperTable();
}

void MainWindow::onSearchClicked()
{
    refreshPaperTable();
}

void MainWindow::onShowAllClicked()
{
    m_searchEdit->clear();
    if (!isAuthorMode()) {
        selectSystemNode(QStringLiteral("all"));
    }
    refreshPaperTable();
}

void MainWindow::onSelectAllClicked()
{
    if (!m_table || m_table->rowCount() == 0) {
        return;
    }
    m_table->selectAll();
    onPaperSelectionChanged();
}

void MainWindow::onAddPaper()
{
    PaperDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("上传/新增文献"));
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    Paper paper = dialog.getPaper();
    ensureUploadTime(paper);
    const IdType paperId = LibraryManager::getInstance().addPaper(paper);
    addPaperToCurrentCatalog(paperId);
    rebuildCatalogTree();
    restoreSelectionAfterReload(currentNodeKey());
    refreshPaperTable();
    showStatus(QStringLiteral("已新增文献"));
}

void MainWindow::onAddAuthor()
{
    AuthorDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("新增作者"));
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    Author author = dialog.getEntity();
    const IdType authorId = LibraryManager::getInstance().addAuthor(author);
    addAuthorToCurrentCatalog(authorId);
    const QString key = currentNodeKey();
    rebuildCatalogTree();
    restoreSelectionAfterReload(key);
    refreshPaperTable();
    showStatus(QStringLiteral("已新增作者"));
}

void MainWindow::onEditPaper()
{
    if (isAuthorMode()) {
        const IdType authorId = selectedAuthorId();
        if (authorId == INVALID_ID) {
            QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请先选择一位作者。"));
            return;
        }
        Author *author = LibraryManager::getInstance().findAuthor(authorId);
        if (!author) {
            return;
        }

        AuthorDialog dialog(this);
        dialog.setWindowTitle(QStringLiteral("编辑作者"));
        dialog.setEntity(*author);
        if (dialog.exec() != QDialog::Accepted) {
            return;
        }

        Author updated = dialog.getEntity();
        updated.setId(authorId);
        LibraryManager::getInstance().updateAuthor(authorId, updated);
        refreshPaperTable();
        if (m_detailAuthorId == authorId && m_detailPanel && m_detailPanel->isVisible()) {
            updateAuthorDetailPanel(authorId);
        }
        showStatus(QStringLiteral("已更新作者"));
        return;
    }

    const IdType paperId = selectedPaperId();
    if (paperId == INVALID_ID) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请先选择一条文献。"));
        return;
    }
    Paper *paper = LibraryManager::getInstance().findPaper(paperId);
    if (!paper) {
        return;
    }

    PaperDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("编辑文献"));
    dialog.setPaper(*paper);
    connect(&dialog, &PaperDialog::attachmentsChanged, this,
            [this](IdType changedPaperId, int uploadedCount) {
                saveDefaultData();
                if (m_detailPaperId == changedPaperId && m_detailPanel && m_detailPanel->isVisible()) {
                    updateDetailPanel(changedPaperId);
                }
                showStatus(QStringLiteral("已上传 %1 个笔记文件").arg(uploadedCount));
            });
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    Paper updated = dialog.getPaper();
    if (updated.getUploadTime().empty()) {
        updated.setUploadTime(paper->getUploadTime());
    }
    ensureUploadTime(updated);
    updated.setId(paperId);
    LibraryManager::getInstance().updatePaper(paperId, updated);
    refreshPaperTable();
    if (m_detailPaperId == paperId && m_detailPanel && m_detailPanel->isVisible()) {
        updateDetailPanel(paperId);
    }
    showStatus(QStringLiteral("已更新文献"));
}

void MainWindow::onDeletePaper()
{
    if (isAuthorMode()) {
        const std::vector<IdType> authorIds = selectedAuthorIds();
        if (authorIds.empty()) {
            QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请先选择要删除的作者。"));
            return;
        }
        auto reply = QMessageBox::question(
            this,
            QStringLiteral("删除作者"),
            QStringLiteral("确定删除选中的 %1 位作者吗？作者会从关联文献和作者目录中移除。")
                .arg(static_cast<int>(authorIds.size())));
        if (reply != QMessageBox::Yes) {
            return;
        }

        const bool detailDeleted = std::find(authorIds.begin(), authorIds.end(), m_detailAuthorId) != authorIds.end();
        for (IdType authorId : authorIds) {
            LibraryManager::getInstance().removeAuthor(authorId);
        }
        if (detailDeleted) {
            clearDetailPanel();
        }
        refreshPaperTable();
        showStatus(QStringLiteral("已删除 %1 位作者").arg(static_cast<int>(authorIds.size())));
        return;
    }

    const std::vector<IdType> paperIds = selectedPaperIds();
    if (paperIds.empty()) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请先选择要删除的文献。"));
        return;
    }
    const QString deletePaperPrompt = paperIds.size() == 1
        ? QStringLiteral("确定删除该文献吗？\n系统将删除该文献记录，并删除保存在 data 目录中的全文文件、附件文件和对应附件文件夹。\ndata 目录外的原始文件不会被删除。")
        : QStringLiteral("确定删除选中的 %1 条文献吗？\n系统将删除这些文献记录，并删除保存在 data 目录中的全文文件、附件文件和对应附件文件夹。\ndata 目录外的原始文件不会被删除。")
            .arg(static_cast<int>(paperIds.size()));
    auto reply = QMessageBox::question(
        this,
        QStringLiteral("删除文献"),
        deletePaperPrompt);
    if (reply != QMessageBox::Yes) {
        return;
    }

    auto &mgr = LibraryManager::getInstance();
    const std::set<IdType> selectedPaperSet(paperIds.begin(), paperIds.end());
    std::set<IdType> attachmentIdsToDelete;
    QMap<QString, QString> filesToDelete;
    QMap<QString, QString> attachmentDirsToDelete;

    auto rememberDataFile = [&filesToDelete](const QString &rawPath) {
        const QString path = QDir::fromNativeSeparators(rawPath.trimmed());
        if (path.isEmpty()) {
            return;
        }
        const QFileInfo info(path);
        if (info.exists() && info.isFile() && pathIsInsideDataDirectory(path)) {
            filesToDelete.insert(normalizedComparablePath(path), info.absoluteFilePath());
        }
    };

    auto rememberAttachmentDir = [&attachmentDirsToDelete](const QString &rawPath) {
        const QString path = QDir::fromNativeSeparators(rawPath.trimmed());
        if (isDeletableAttachmentDirectory(path)) {
            attachmentDirsToDelete.insert(normalizedComparablePath(path), normalizedAbsolutePath(path));
        }
    };

    for (IdType paperId : paperIds) {
        if (Paper *paper = mgr.findPaper(paperId)) {
            rememberDataFile(QString::fromStdString(paper->getFilePath()));
            rememberAttachmentDir(attachmentDirectoryForPaper(*paper));

            std::set<IdType> paperAttachmentIds(paper->getAttachmentIds().begin(), paper->getAttachmentIds().end());
            for (const Attachment &attachment : mgr.getAttachmentsByPaper(paperId)) {
                paperAttachmentIds.insert(attachment.getId());
            }

            for (IdType attachmentId : paperAttachmentIds) {
                Attachment *attachment = mgr.findAttachment(attachmentId);
                if (!attachment) {
                    continue;
                }
                attachmentIdsToDelete.insert(attachmentId);
                const QString attachmentPath = QString::fromStdString(attachment->getFilePath());
                rememberDataFile(attachmentPath);
                if (!attachmentPath.trimmed().isEmpty()) {
                    rememberAttachmentDir(QFileInfo(QDir::fromNativeSeparators(attachmentPath)).absoluteDir().absolutePath());
                }
            }
        }
    }

    const bool detailDeleted = std::find(paperIds.begin(), paperIds.end(), m_detailPaperId) != paperIds.end();

    auto fileReferencedByRemainingPaper = [&mgr, &selectedPaperSet](const QString &filePath) {
        for (const auto &pair : mgr.getAllPapers()) {
            if (selectedPaperSet.count(pair.first)) {
                continue;
            }
            const QString otherPath = QString::fromStdString(pair.second.getFilePath()).trimmed();
            if (!otherPath.isEmpty() && normalizedComparablePath(otherPath) == filePath) {
                return true;
            }
        }
        return false;
    };

    auto fileReferencedByRemainingAttachment = [&mgr, &attachmentIdsToDelete](const QString &filePath) {
        for (const auto &pair : mgr.getAllAttachments()) {
            if (attachmentIdsToDelete.count(pair.first)) {
                continue;
            }
            const QString otherPath = QString::fromStdString(pair.second.getFilePath()).trimmed();
            if (!otherPath.isEmpty() && normalizedComparablePath(otherPath) == filePath) {
                return true;
            }
        }
        return false;
    };

    int deletedFileCount = 0;
    int failedFileCount = 0;
    for (auto it = filesToDelete.cbegin(); it != filesToDelete.cend(); ++it) {
        const QString normalizedPath = it.key();
        const QString filePath = it.value();
        if (fileReferencedByRemainingPaper(normalizedPath) || fileReferencedByRemainingAttachment(normalizedPath)) {
            continue;
        }
        if (!QFile::exists(filePath)) {
            continue;
        }
        if (QFile::remove(filePath)) {
            ++deletedFileCount;
        } else {
            ++failedFileCount;
        }
    }

    for (IdType attachmentId : attachmentIdsToDelete) {
        mgr.removeAttachment(attachmentId);
    }

    for (IdType paperId : paperIds) {
        mgr.removePaper(paperId);
    }

    auto attachmentDirectoryStillUsed = [&mgr](const QString &dirPath) {
        for (const auto &pair : mgr.getAllAttachments()) {
            const QString attachmentPath = QString::fromStdString(pair.second.getFilePath()).trimmed();
            if (!attachmentPath.isEmpty() && pathIsInsideDirectory(attachmentPath, dirPath)) {
                return true;
            }
        }
        return false;
    };

    int deletedDirCount = 0;
    int failedDirCount = 0;
    for (auto it = attachmentDirsToDelete.cbegin(); it != attachmentDirsToDelete.cend(); ++it) {
        const QString dirPath = it.value();
        if (!isDeletableAttachmentDirectory(dirPath) || attachmentDirectoryStillUsed(dirPath)) {
            continue;
        }
        if (QDir(dirPath).removeRecursively()) {
            ++deletedDirCount;
        } else {
            ++failedDirCount;
        }
    }

    if (detailDeleted) {
        clearDetailPanel();
    }
    rebuildCatalogTree();
    restoreSelectionAfterReload(currentNodeKey());
    refreshPaperTable();
    saveDefaultData();

    QString status = QStringLiteral("已删除文献及其附件：%1 条文献，%2 个文件，%3 个附件文件夹")
        .arg(static_cast<int>(paperIds.size()))
        .arg(deletedFileCount)
        .arg(deletedDirCount);
    if (failedFileCount > 0 || failedDirCount > 0) {
        status += QStringLiteral("，%1 个文件、%2 个文件夹删除失败")
            .arg(failedFileCount)
            .arg(failedDirCount);
    }
    showStatus(status);
}

void MainWindow::onAddCatalog()
{
    bool ok = false;
    const QString name = QInputDialog::getText(
        this,
        QStringLiteral("添加目录"),
        QStringLiteral("目录名称:"),
        QLineEdit::Normal,
        QString(),
        &ok).trimmed();
    if (!ok || name.isEmpty()) {
        return;
    }

    Catalog catalog;
    catalog.setName(name.toStdString());
    if (auto *item = m_sidebar->currentItem(); item && isUserCatalogNode(item)) {
        catalog.setParentId(currentCatalogId());
    } else {
        catalog.setParentId(INVALID_ID);
    }

    LibraryManager::getInstance().addCatalog(catalog);
    rebuildCatalogTree();
    restoreSelectionAfterReload(currentNodeKey());
    refreshPaperTable();
    showStatus(QStringLiteral("已添加目录"));
}

void MainWindow::onDeleteCatalog()
{
    auto *item = m_sidebar->currentItem();
    if (!isUserCatalogNode(item)) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("只能删除用户自定义目录。"));
        return;
    }

    const IdType catalogId = currentCatalogId();
    auto reply = QMessageBox::question(
        this,
        QStringLiteral("删除目录"),
        QStringLiteral("确定删除当前目录吗？目录内文献不会被删除。"));
    if (reply != QMessageBox::Yes) {
        return;
    }

    LibraryManager::getInstance().removeCatalog(catalogId);
    rebuildCatalogTree();
    selectSystemNode(QStringLiteral("all"));
    refreshPaperTable();
    showStatus(QStringLiteral("已删除目录"));
}

void MainWindow::onAddAuthorCatalog()
{
    bool ok = false;
    const QString name = QInputDialog::getText(
        this,
        QStringLiteral("添加作者目录"),
        QStringLiteral("目录名称:"),
        QLineEdit::Normal,
        QString(),
        &ok).trimmed();
    if (!ok || name.isEmpty()) {
        return;
    }

    AuthorCatalog catalog;
    catalog.setName(name.toStdString());
    if (auto *item = m_sidebar->currentItem(); item && isUserAuthorCatalogNode(item)) {
        catalog.setParentId(currentAuthorCatalogId());
    } else {
        catalog.setParentId(INVALID_ID);
    }

    const QString key = currentNodeKey();
    LibraryManager::getInstance().addAuthorCatalog(catalog);
    rebuildCatalogTree();
    restoreSelectionAfterReload(key);
    refreshPaperTable();
    showStatus(QStringLiteral("已添加作者目录"));
}

void MainWindow::onDeleteAuthorCatalog()
{
    auto *item = m_sidebar->currentItem();
    if (!isUserAuthorCatalogNode(item)) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("只能删除作者库下的用户自定义目录。"));
        return;
    }

    const IdType catalogId = currentAuthorCatalogId();
    auto reply = QMessageBox::question(
        this,
        QStringLiteral("删除作者目录"),
        QStringLiteral("确定删除当前作者目录吗？目录内作者不会被删除。"));
    if (reply != QMessageBox::Yes) {
        return;
    }

    LibraryManager::getInstance().removeAuthorCatalog(catalogId);
    rebuildCatalogTree();
    selectSystemNode(QStringLiteral("authors"));
    refreshPaperTable();
    showStatus(QStringLiteral("已删除作者目录"));
}

void MainWindow::onCatalogContextMenu(const QPoint &pos)
{
    QTreeWidgetItem *item = m_sidebar->itemAt(pos);
    QMenu menu(m_sidebar);

    if (item && isUserCatalogNode(item)) {
        m_sidebar->setCurrentItem(item);
        QAction *actAddChild = menu.addAction(QStringLiteral("添加子目录"));
        QAction *actDelete = menu.addAction(QStringLiteral("删除目录"));

        connect(actAddChild, &QAction::triggered, this, &MainWindow::onAddCatalog);
        connect(actDelete, &QAction::triggered, this, &MainWindow::onDeleteCatalog);
    } else if (item && isUserAuthorCatalogNode(item)) {
        m_sidebar->setCurrentItem(item);
        QAction *actAddChild = menu.addAction(QStringLiteral("添加子目录"));
        QAction *actDelete = menu.addAction(QStringLiteral("删除目录"));

        connect(actAddChild, &QAction::triggered, this, &MainWindow::onAddAuthorCatalog);
        connect(actDelete, &QAction::triggered, this, &MainWindow::onDeleteAuthorCatalog);
    } else if (item) {
        const QString key = item->data(0, kRoleNodeKey).toString();
        if (key == QStringLiteral("library")) {
            QAction *actAdd = menu.addAction(QStringLiteral("添加目录"));
            connect(actAdd, &QAction::triggered, this, &MainWindow::onAddCatalog);
        } else if (key == QStringLiteral("authors")) {
            m_sidebar->setCurrentItem(item);
            QAction *actAdd = menu.addAction(QStringLiteral("添加作者目录"));
            connect(actAdd, &QAction::triggered, this, &MainWindow::onAddAuthorCatalog);
        } else {
            return;
        }
    } else {
        return;
    }

    menu.exec(m_sidebar->viewport()->mapToGlobal(pos));
}

void MainWindow::onOpenPaperAttachment(int row, int column)
{
    if (column != 5) {
        return;
    }
    if (row < 0 || row >= m_table->rowCount()) {
        return;
    }
    auto *titleItem = m_table->item(row, 0);
    if (!titleItem) return;
    const IdType paperId = titleItem->data(kRolePaperId).toLongLong();
    Paper *paper = LibraryManager::getInstance().findPaper(paperId);
    if (!paper) return;

    const QString path = QString::fromStdString(paper->getFilePath());
    if (path.isEmpty() || !QFile::exists(path)) {
        QMessageBox::warning(this, QStringLiteral("无法打开"), QStringLiteral("PDF 文件不存在或路径无效。"));
        return;
    }
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(path))) {
        QMessageBox::warning(this, QStringLiteral("无法打开"), QStringLiteral("系统默认程序无法打开该文件。"));
    }
}

void MainWindow::addPaperToCurrentCatalog(IdType paperId) const
{
    if (auto *item = m_sidebar->currentItem(); item && isUserCatalogNode(item)) {
        LibraryManager::getInstance().addPaperToCatalog(paperId, currentCatalogId());
    }
}

void MainWindow::addAuthorToCurrentCatalog(IdType authorId) const
{
    if (auto *item = m_sidebar->currentItem(); item && isUserAuthorCatalogNode(item)) {
        LibraryManager::getInstance().addAuthorToCatalog(authorId, currentAuthorCatalogId());
    }
}

IdType MainWindow::selectedPaperId() const
{
    if (isAuthorMode()) {
        return INVALID_ID;
    }
    if (!m_table) {
        return INVALID_ID;
    }
    const int row = m_table->currentRow();
    if (row < 0) {
        return INVALID_ID;
    }
    QTableWidgetItem *titleItem = m_table->item(row, 0);
    if (!titleItem) {
        return INVALID_ID;
    }
    return static_cast<IdType>(titleItem->data(kRolePaperId).toLongLong());
}

IdType MainWindow::selectedAuthorId() const
{
    if (!isAuthorMode() || !m_table) {
        return INVALID_ID;
    }
    const int row = m_table->currentRow();
    if (row < 0) {
        return INVALID_ID;
    }
    QTableWidgetItem *nameItem = m_table->item(row, 0);
    if (!nameItem) {
        return INVALID_ID;
    }
    return static_cast<IdType>(nameItem->data(kRoleAuthorId).toLongLong());
}

std::vector<IdType> MainWindow::selectedPaperIds() const
{
    std::vector<IdType> ids;
    if (isAuthorMode() || !m_table) {
        return ids;
    }

    std::set<IdType> seen;
    const auto rows = m_table->selectionModel()
        ? m_table->selectionModel()->selectedRows(0)
        : QModelIndexList();
    for (const QModelIndex &index : rows) {
        if (QTableWidgetItem *item = m_table->item(index.row(), 0)) {
            const IdType id = static_cast<IdType>(item->data(kRolePaperId).toLongLong());
            if (id != INVALID_ID && seen.insert(id).second) {
                ids.push_back(id);
            }
        }
    }

    if (ids.empty()) {
        const IdType id = selectedPaperId();
        if (id != INVALID_ID) {
            ids.push_back(id);
        }
    }
    return ids;
}

std::vector<IdType> MainWindow::selectedAuthorIds() const
{
    std::vector<IdType> ids;
    if (!isAuthorMode() || !m_table) {
        return ids;
    }

    std::set<IdType> seen;
    const auto rows = m_table->selectionModel()
        ? m_table->selectionModel()->selectedRows(0)
        : QModelIndexList();
    for (const QModelIndex &index : rows) {
        if (QTableWidgetItem *item = m_table->item(index.row(), 0)) {
            const IdType id = static_cast<IdType>(item->data(kRoleAuthorId).toLongLong());
            if (id != INVALID_ID && seen.insert(id).second) {
                ids.push_back(id);
            }
        }
    }

    if (ids.empty()) {
        const IdType id = selectedAuthorId();
        if (id != INVALID_ID) {
            ids.push_back(id);
        }
    }
    return ids;
}

bool MainWindow::moveSelectedPaperToCatalog(IdType catalogId)
{
    const IdType paperId = selectedPaperId();
    if (paperId == INVALID_ID || catalogId == INVALID_ID) {
        return false;
    }

    auto &mgr = LibraryManager::getInstance();
    if (!mgr.findPaper(paperId) || !mgr.findCatalog(catalogId)) {
        return false;
    }

    std::vector<IdType> catalogIds;
    for (const auto &pair : mgr.getAllCatalogs()) {
        catalogIds.push_back(pair.first);
    }
    for (IdType id : catalogIds) {
        mgr.removePaperFromCatalog(paperId, id);
    }
    mgr.addPaperToCatalog(paperId, catalogId);
    rebuildCatalogTree();
    restoreSelectionAfterReload(QStringLiteral("catalog_%1").arg(catalogId));
    refreshPaperTable();
    showStatus(QStringLiteral("已将文献移动到目录"));
    return true;
}

void MainWindow::adjustAuthorColumnWidth()
{
    if (!m_table) {
        return;
    }

    const QFontMetrics fm(m_table->font());
    const int maxWidth = fm.horizontalAdvance(QString(30, QLatin1Char('M'))) + 28;
    int targetWidth = fm.horizontalAdvance(QStringLiteral("作者")) + 36;
    for (int row = 0; row < m_table->rowCount(); ++row) {
        if (QTableWidgetItem *item = m_table->item(row, 3)) {
            item->setToolTip(item->text());
            targetWidth = std::max(targetWidth, fm.horizontalAdvance(item->text()) + 28);
        }
    }
    m_table->setColumnWidth(3, std::min(targetWidth, maxWidth));
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (m_sidebar && watched == m_sidebar->viewport()) {
        if (event->type() == QEvent::DragEnter) {
            auto *dragEvent = static_cast<QDragEnterEvent *>(event);
            if (selectedPaperId() != INVALID_ID) {
                dragEvent->acceptProposedAction();
                return true;
            }
        } else if (event->type() == QEvent::DragMove) {
            auto *dragEvent = static_cast<QDragMoveEvent *>(event);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            QTreeWidgetItem *item = m_sidebar->itemAt(dragEvent->position().toPoint());
#else
            QTreeWidgetItem *item = m_sidebar->itemAt(dragEvent->pos());
#endif
            if (isUserCatalogNode(item) && selectedPaperId() != INVALID_ID) {
                dragEvent->acceptProposedAction();
            } else {
                dragEvent->ignore();
            }
            return true;
        } else if (event->type() == QEvent::Drop) {
            auto *dropEvent = static_cast<QDropEvent *>(event);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            QTreeWidgetItem *item = m_sidebar->itemAt(dropEvent->position().toPoint());
#else
            QTreeWidgetItem *item = m_sidebar->itemAt(dropEvent->pos());
#endif
            if (isUserCatalogNode(item)) {
                const IdType catalogId = static_cast<IdType>(item->data(0, kRoleCatalogId).toInt());
                if (moveSelectedPaperToCatalog(catalogId)) {
                    dropEvent->acceptProposedAction();
                }
            }
            return true;
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::restoreSelectionAfterReload(const QString &key)
{
    if (auto *item = findNodeByKey(key)) {
        m_sidebar->setCurrentItem(item);
    } else {
        selectSystemNode(QStringLiteral("all"));
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
        QStringLiteral("加载数据（替换当前库）"), QString(),
        QStringLiteral("数据文件 (*.txt);;所有文件 (*)"));
    if (path.isEmpty()) return;

    auto reply = QMessageBox::question(
        this,
        QStringLiteral("加载数据"),
        QStringLiteral("加载会清空当前库，并用所选文件替换当前数据。\n\n如果只是想把外部数据加进来，请使用“文件 > 导入数据”。\n\n确定继续加载吗？"));
    if (reply != QMessageBox::Yes) {
        return;
    }

    if (LibraryManager::getInstance().loadFromFile(path.toStdString())) {
        clearDetailPanel();
        rebuildCatalogTree();
        selectSystemNode(QStringLiteral("all"));
        saveDefaultData();
        refreshPaperTable();
        showStatus(QStringLiteral("已从 ") + path + QStringLiteral(" 加载数据"));
    } else {
        QMessageBox::warning(this,
            QStringLiteral("加载失败"),
            QStringLiteral("文件不存在或格式不正确。"));
    }
}

void MainWindow::onImport()
{
    QString path = QFileDialog::getOpenFileName(this,
        QStringLiteral("导入数据（合并到当前库）"), QString(),
        QStringLiteral("数据文件 (*.txt);;所有文件 (*)"));
    if (path.isEmpty()) return;

    if (LibraryManager::getInstance().importFromFile(path.toStdString())) {
        clearDetailPanel();
        rebuildCatalogTree();
        restoreSelectionAfterReload(currentNodeKey());
        saveDefaultData();
        refreshPaperTable();
        showStatus(QStringLiteral("已导入并合并: ") + path);
    } else {
        QMessageBox::warning(this,
            QStringLiteral("导入失败"),
            QStringLiteral("文件不存在或格式不正确。"));
    }
}

QString MainWindow::defaultDataFilePath() const
{
    QDir dir(StoragePaths::libraryDirectoryPath());
    return dir.filePath(QStringLiteral("library_data.txt"));
}

bool MainWindow::saveDefaultData()
{
    if (m_defaultDataPath.isEmpty()) {
        m_defaultDataPath = defaultDataFilePath();
    }
    StoragePaths::pdfDirectoryPath();
    auto &mgr = LibraryManager::getInstance();
    const bool splitSaved = mgr.saveToDirectory(StoragePaths::dataRootPath().toStdString());
    const bool backupSaved = mgr.saveToFile(m_defaultDataPath.toStdString());
    return splitSaved && backupSaved;
}

bool MainWindow::loadDefaultData()
{
    if (m_defaultDataPath.isEmpty()) {
        m_defaultDataPath = defaultDataFilePath();
    }

    auto &mgr = LibraryManager::getInstance();
    const QString dataRootPath = StoragePaths::dataRootPath();
    bool loaded = mgr.loadFromDirectory(dataRootPath.toStdString());

    if (!loaded) {
        loaded = migrateLegacyDataIfNeeded();
    }

    if (!loaded && QFile::exists(m_defaultDataPath)) {
        loaded = mgr.loadFromFile(m_defaultDataPath.toStdString());
        if (loaded) {
            saveDefaultData();
        }
    }

    if (!loaded) {
        return false;
    }
    rebuildCatalogTree();
    selectSystemNode(QStringLiteral("all"));
    refreshPaperTable();
    return true;
}

bool MainWindow::migrateLegacyDataIfNeeded()
{
    const QString legacyDataPath = StoragePaths::legacyAppDataPath();
    if (legacyDataPath.isEmpty()) {
        return false;
    }

    QDir legacyDir(legacyDataPath);
    const QString legacyFilePath = legacyDir.filePath(QStringLiteral("library_data.txt"));
    if (!QFile::exists(legacyFilePath)) {
        return false;
    }

    auto &mgr = LibraryManager::getInstance();
    if (!mgr.loadFromFile(legacyFilePath.toStdString())) {
        return false;
    }

    migrateLegacyPaperFiles(legacyDataPath);
    saveDefaultData();
    return true;
}

void MainWindow::migrateLegacyPaperFiles(const QString &legacyDataPath)
{
    QDir legacyDir(legacyDataPath);
    QDir legacyPapersDir(legacyDir.filePath(QStringLiteral("papers")));
    if (!legacyPapersDir.exists()) {
        return;
    }

    QDir newPdfDir(StoragePaths::pdfDirectoryPath());
    const QStringList fileNames = legacyPapersDir.entryList(QDir::Files | QDir::NoDotAndDotDot);
    for (const QString &fileName : fileNames) {
        const QString oldPath = legacyPapersDir.filePath(fileName);
        const QString newPath = newPdfDir.filePath(fileName);
        if (!QFile::exists(newPath)) {
            QFile::copy(oldPath, newPath);
        }
    }

    auto &mgr = LibraryManager::getInstance();
    std::vector<IdType> paperIds;
    for (const auto &pair : mgr.getAllPapers()) {
        paperIds.push_back(pair.first);
    }

    for (IdType paperId : paperIds) {
        Paper *paper = mgr.findPaper(paperId);
        if (!paper) {
            continue;
        }

        const QString filePath = QDir::fromNativeSeparators(QString::fromStdString(paper->getFilePath()));
        if (!pathIsInsideDirectory(filePath, legacyPapersDir.absolutePath())) {
            continue;
        }

        const QString fileName = QFileInfo(filePath).fileName();
        const QString newPath = newPdfDir.filePath(fileName);
        if (QFile::exists(newPath)) {
            paper->setFilePath(QDir::toNativeSeparators(newPath).toStdString());
        }
    }
}
