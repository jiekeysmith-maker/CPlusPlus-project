#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QAction>
#include <QCloseEvent>
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
#include "sourcedialog.h"
#include "StoragePaths.h"

#include <QStackedWidget>
#include <QToolBar>

namespace {
constexpr int kRoleNodeType = Qt::UserRole;
constexpr int kRoleCatalogId = Qt::UserRole + 1;
constexpr int kRoleNodeKey = Qt::UserRole + 2;
constexpr int kRoleDeletable = Qt::UserRole + 3;
constexpr int kRolePaperId = Qt::UserRole + 4;

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

    m_table = new QTableWidget();
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
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
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

    setupSourceTable();
    setupSourceToolbar();

    m_stackedWidget = new QStackedWidget(splitter);
    m_stackedWidget->addWidget(m_table);       // page 0: 论文表
    m_stackedWidget->addWidget(m_sourceTable); // page 1: 出版物表

    setupDetailPanel();

    splitter->addWidget(m_sidebar);
    splitter->addWidget(m_stackedWidget);
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

    m_detailPanelTitle = new QLabel(QStringLiteral("文献详情"), m_detailPanel);
    QFont titleFont = m_detailPanelTitle->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 2);
    m_detailPanelTitle->setFont(titleFont);
    panelLayout->addWidget(m_detailPanelTitle);

    auto *scrollArea = new QScrollArea(m_detailPanel);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    auto *content = new QWidget(scrollArea);
    auto *form = new QFormLayout(content);
    form->setContentsMargins(0, 0, 0, 0);
    form->setSpacing(10);
    form->setLabelAlignment(Qt::AlignTop | Qt::AlignRight);

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

    form->addRow(QStringLiteral("完整标题:"), m_detailTitleLabel);
    form->addRow(QStringLiteral("作者名字:"), m_detailAuthorsLabel);
    form->addRow(QStringLiteral("出版物:"), m_detailSourceLabel);
    form->addRow(QStringLiteral("发表时间:"), m_detailPublishDateLabel);
    form->addRow(QStringLiteral("上传时间:"), m_detailUploadTimeLabel);
    form->addRow(QStringLiteral("关键词:"), m_detailKeywordsLabel);
    form->addRow(QStringLiteral("附件链接:"), m_detailFilePathLabel);

    m_openDetailAttachmentButton = new QPushButton(QStringLiteral("打开附件"), content);
    m_openDetailAttachmentButton->setEnabled(false);
    connect(m_openDetailAttachmentButton, &QPushButton::clicked,
            this, &MainWindow::onOpenDetailAttachment);
    form->addRow(QString(), m_openDetailAttachmentButton);

    scrollArea->setWidget(content);
    panelLayout->addWidget(scrollArea, 1);

    m_detailPanel->hide();
}

void MainWindow::setupSourceTable()
{
    m_sourceTable = new QTableWidget();
    m_sourceTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_sourceTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_sourceTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_sourceTable->setAlternatingRowColors(true);
    m_sourceTable->verticalHeader()->setVisible(false);
    m_sourceTable->setWordWrap(false);
    m_sourceTable->setStyleSheet(
        "QTableWidget { border: none; gridline-color: #e5e7eb; font-size: 13px; }"
        "QHeaderView::section { background: #f3f4f6; padding: 8px; border: none; border-right: 1px solid #e5e7eb; }"
        "QTableWidget::item:selected { background: #e8f2ff; color: #111827; }");

    connect(m_sourceTable, &QTableWidget::itemSelectionChanged,
            this, &MainWindow::onPaperSelectionChanged);
}

void MainWindow::setupSourceToolbar()
{
    m_sourceToolBar = new QToolBar(QStringLiteral("出版物工具栏"), this);
    m_sourceToolBar->setVisible(false);

    QAction *actAddJournal = m_sourceToolBar->addAction(QStringLiteral("新增期刊"));
    connect(actAddJournal, &QAction::triggered, this, [this]() {
        onAddPublication(QStringLiteral("Journal"));
    });

    QAction *actAddConf = m_sourceToolBar->addAction(QStringLiteral("新增会议"));
    connect(actAddConf, &QAction::triggered, this, [this]() {
        onAddPublication(QStringLiteral("Conference"));
    });

    QAction *actEdit = m_sourceToolBar->addAction(QStringLiteral("编辑"));
    connect(actEdit, &QAction::triggered, this, &MainWindow::onEditSourceFromToolbar);

    QAction *actDelete = m_sourceToolBar->addAction(QStringLiteral("删除"));
    connect(actDelete, &QAction::triggered, this, &MainWindow::onDeleteSourceFromToolbar);

    addToolBar(Qt::TopToolBarArea, m_sourceToolBar);
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

    // === 我的文库 ===
    auto *myLibrary = new QTreeWidgetItem(m_sidebar, {QStringLiteral("我的文库")});
    myLibrary->setData(0, kRoleNodeType, static_cast<int>(LibraryNodeType::System));
    myLibrary->setData(0, kRoleNodeKey, QStringLiteral("library"));
    myLibrary->setData(0, kRoleDeletable, false);

    auto *allPapers = new QTreeWidgetItem(myLibrary, {QStringLiteral("全部文献")});
    allPapers->setData(0, kRoleNodeType, static_cast<int>(LibraryNodeType::System));
    allPapers->setData(0, kRoleNodeKey, QStringLiteral("all"));
    allPapers->setData(0, kRoleDeletable, false);

    auto *recent = new QTreeWidgetItem(myLibrary, {QStringLiteral("最近添加")});
    recent->setData(0, kRoleNodeType, static_cast<int>(LibraryNodeType::System));
    recent->setData(0, kRoleNodeKey, QStringLiteral("recent"));
    recent->setData(0, kRoleDeletable, false);

    auto *uncategorized = new QTreeWidgetItem(myLibrary, {QStringLiteral("未分类")});
    uncategorized->setData(0, kRoleNodeType, static_cast<int>(LibraryNodeType::System));
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
                item->setData(0, kRoleNodeType, static_cast<int>(LibraryNodeType::Catalog));
                item->setData(0, kRoleCatalogId, static_cast<int>(catalog.getId()));
                item->setData(0, kRoleNodeKey, QStringLiteral("catalog_%1").arg(catalog.getId()));
                item->setData(0, kRoleDeletable, true);
                itemMap.insert(catalog.getId(), item);
                buildChildren(item, catalog.getId());
            }
        }
    };

    buildChildren(myLibrary, INVALID_ID);

    // === 出版物库 ===
    auto *pubRoot = new QTreeWidgetItem(m_sidebar, {QStringLiteral("出版物库")});
    pubRoot->setData(0, kRoleNodeType, static_cast<int>(LibraryNodeType::PublicationRoot));
    pubRoot->setData(0, kRoleNodeKey, QStringLiteral("publications"));
    pubRoot->setData(0, kRoleDeletable, false);

    auto *journals = new QTreeWidgetItem(pubRoot, {QStringLiteral("期刊")});
    journals->setData(0, kRoleNodeType, static_cast<int>(LibraryNodeType::PublicationGroup));
    journals->setData(0, kRoleNodeKey, QStringLiteral("journals"));
    journals->setData(0, kRoleDeletable, false);

    auto *conferences = new QTreeWidgetItem(pubRoot, {QStringLiteral("会议")});
    conferences->setData(0, kRoleNodeType, static_cast<int>(LibraryNodeType::PublicationGroup));
    conferences->setData(0, kRoleNodeKey, QStringLiteral("conferences"));
    conferences->setData(0, kRoleDeletable, false);

    myLibrary->setExpanded(true);
    pubRoot->setExpanded(true);
    recent->setExpanded(true);
    m_sidebar->expandItem(myLibrary);

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
    return item->data(0, kRoleNodeType).toInt() == static_cast<int>(LibraryNodeType::System);
}

bool MainWindow::isUserCatalogNode(const QTreeWidgetItem *item) const
{
    if (!item) return false;
    return item->data(0, kRoleNodeType).toInt() == static_cast<int>(LibraryNodeType::Catalog);
}

IdType MainWindow::currentCatalogId() const
{
    if (auto *item = m_sidebar->currentItem(); item && isUserCatalogNode(item)) {
        return static_cast<IdType>(item->data(0, kRoleCatalogId).toInt());
    }
    return INVALID_ID;
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
        // 出版物库节点
        const int nodeType = item->data(0, kRoleNodeType).toInt();
        if (nodeType == static_cast<int>(LibraryNodeType::PublicationGroup)) {
            const QString pubType = (key == QStringLiteral("conferences"))
                ? QStringLiteral("Conference") : QStringLiteral("Journal");
            std::vector<Paper> papers;
            for (const auto &paperPair : mgr.getAllPapers()) {
                const Paper &paper = paperPair.second;
                Source *src = mgr.findSource(paper.getSourceId());
                if (src && src->getType() == pubType.toStdString()) {
                    papers.push_back(paper);
                }
            }
            return papers;
        }
    }

    std::vector<Paper> papers;
    for (const auto &pair : mgr.getAllPapers()) {
        papers.push_back(pair.second);
    }
    return papers;
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

void MainWindow::updatePaperTable(const std::vector<Paper> &papers)
{
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

void MainWindow::refreshPaperTable()
{
    const QString keyword = m_searchEdit ? m_searchEdit->text() : QString();
    updatePaperTable(filterPapers(collectCurrentPapers(), keyword));
}

void MainWindow::refreshSourceTable(const QString &typeFilter)
{
    auto &mgr = LibraryManager::getInstance();
    const auto &sources = mgr.getAllSources();

    // 确定表头
    const bool showAll = (typeFilter == QStringLiteral("all"));
    if (showAll) {
        // 全部出版物：显示类型列
        m_sourceTable->setColumnCount(9);
        m_sourceTable->setHorizontalHeaderLabels({
            QStringLiteral("编号"),
            QStringLiteral("类型"),
            QStringLiteral("简称"),
            QStringLiteral("全名"),
            QStringLiteral("领域"),
            QStringLiteral("出版单位"),
            QStringLiteral("检索类型"),
            QStringLiteral("最新影响因子/开会地址"),
            QStringLiteral("备注")
        });
    } else if (typeFilter == QStringLiteral("Journal")) {
        m_sourceTable->setColumnCount(8);
        m_sourceTable->setHorizontalHeaderLabels({
            QStringLiteral("编号"),
            QStringLiteral("简称"),
            QStringLiteral("全名"),
            QStringLiteral("领域"),
            QStringLiteral("出版单位"),
            QStringLiteral("检索类型"),
            QStringLiteral("最新影响因子"),
            QStringLiteral("备注")
        });
    } else {
        m_sourceTable->setColumnCount(8);
        m_sourceTable->setHorizontalHeaderLabels({
            QStringLiteral("编号"),
            QStringLiteral("简称"),
            QStringLiteral("全名"),
            QStringLiteral("领域"),
            QStringLiteral("开会地址"),
            QStringLiteral("出版单位"),
            QStringLiteral("检索类型"),
            QStringLiteral("备注")
        });
    }

    // 筛选并按简称排序
    std::vector<std::shared_ptr<Source>> filtered;
    for (const auto &pair : sources) {
        if (showAll || pair.second->getType() == typeFilter.toStdString()) {
            filtered.push_back(pair.second);
        }
    }
    std::sort(filtered.begin(), filtered.end(), [](const auto &a, const auto &b) {
        return a->getShortName() < b->getShortName();
    });

    m_sourceTable->setRowCount(static_cast<int>(filtered.size()));
    for (int row = 0; row < static_cast<int>(filtered.size()); ++row) {
        const auto &src = filtered[row];
        int col = 0;

        auto *idItem = new QTableWidgetItem(QString::number(row + 1));
        idItem->setData(kRolePaperId, static_cast<qint64>(src->getId()));
        m_sourceTable->setItem(row, col++, idItem);

        if (showAll) {
            m_sourceTable->setItem(row, col++,
                new QTableWidgetItem(QString::fromStdString(src->getType())));
        }

        m_sourceTable->setItem(row, col++,
            new QTableWidgetItem(QString::fromStdString(src->getShortName())));
        m_sourceTable->setItem(row, col++,
            new QTableWidgetItem(QString::fromStdString(src->getFullName())));
        m_sourceTable->setItem(row, col++,
            new QTableWidgetItem(QString::fromStdString(src->getField())));

        if (typeFilter == QStringLiteral("Conference")) {
            auto *conf = dynamic_cast<const Conference*>(src.get());
            m_sourceTable->setItem(row, col++,
                new QTableWidgetItem(conf ? QString::fromStdString(conf->getMeetingAddress()) : QString()));
        }

        m_sourceTable->setItem(row, col++,
            new QTableWidgetItem(QString::fromStdString(src->getPublisher())));

        m_sourceTable->setItem(row, col++,
            new QTableWidgetItem(QString::fromStdString(src->getRetrievalType())));

        if (showAll || typeFilter == QStringLiteral("Journal")) {
            auto *journal = dynamic_cast<const Journal*>(src.get());
            const double impact = journal ? journal->getImpactFactor() : 0.0;
            m_sourceTable->setItem(row, col++,
                new QTableWidgetItem(impact > 0 ? QString::number(impact, 'f', 3) : QString()));
        }
        if (showAll && typeFilter != QStringLiteral("Journal")) {
            // 对于"all"模式，会议行的影响因子列留空，由会议地址补充
            // 已在上面处理
        }

        m_sourceTable->setItem(row, col++,
            new QTableWidgetItem(QString::fromStdString(src->getRemark())));
    }

    m_sourceTable->horizontalHeader()->setStretchLastSection(true);
    m_sourceTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_sourceTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_sourceTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
}

IdType MainWindow::selectedSourceId() const
{
    if (!m_sourceTable) return INVALID_ID;
    const int row = m_sourceTable->currentRow();
    if (row < 0) return INVALID_ID;
    QTableWidgetItem *idItem = m_sourceTable->item(row, 0);
    if (!idItem) return INVALID_ID;
    return static_cast<IdType>(idItem->data(kRolePaperId).toLongLong());
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

    const QString filePath = QString::fromStdString(paper->getFilePath());
    const QString nativeFilePath = QDir::toNativeSeparators(filePath);

    m_detailPaperId = paperId;
    m_detailTitleLabel->setText(valueOrDefault(QString::fromStdString(paper->getTitle())));
    m_detailAuthorsLabel->setText(valueOrDefault(authors));
    m_detailSourceLabel->setText(source);
    m_detailPublishDateLabel->setText(valueOrDefault(QString::fromStdString(paper->getPublishDate())));
    m_detailUploadTimeLabel->setText(valueOrDefault(QString::fromStdString(paper->getUploadTime())));
    m_detailKeywordsLabel->setText(valueOrDefault(paperKeywordsText(*paper)));
    m_detailFilePathLabel->setText(filePath.trimmed().isEmpty() ? QStringLiteral("无附件") : nativeFilePath);
    m_detailFilePathLabel->setToolTip(nativeFilePath);
    m_openDetailAttachmentButton->setVisible(true);
    m_openDetailAttachmentButton->setEnabled(!filePath.trimmed().isEmpty());

    m_detailPanel->show();
}

void MainWindow::showSourceDetail(IdType sourceId)
{
    auto &mgr = LibraryManager::getInstance();
    Source *src = mgr.findSource(sourceId);
    if (!src) {
        clearDetailPanel();
        return;
    }

    m_detailPaperId = sourceId;

    if (m_detailPanelTitle) {
        m_detailPanelTitle->setText(QStringLiteral("出版物详情"));
    }

    auto setLabel = [](QLabel *label, const QString &text) {
        if (label) {
            label->setText(text.isEmpty() ? QStringLiteral("未填写") : text);
        }
    };

    setLabel(m_detailTitleLabel, QString::fromStdString(src->getShortName()));
    setLabel(m_detailAuthorsLabel,
        src->getType() == "Journal" ? QStringLiteral("期刊") : QStringLiteral("会议"));
    setLabel(m_detailSourceLabel, QString::fromStdString(src->getFullName()));
    setLabel(m_detailPublishDateLabel, QString::fromStdString(src->getField()));
    setLabel(m_detailUploadTimeLabel, QString::fromStdString(src->getPublisher()));
    setLabel(m_detailKeywordsLabel, QString::fromStdString(src->getRetrievalType()));

    QString extra;
    if (src->getType() == "Journal") {
        auto *j = dynamic_cast<const Journal*>(src);
        if (j && j->getImpactFactor() > 0.0) {
            extra = QStringLiteral("最新影响因子: ") + QString::number(j->getImpactFactor(), 'f', 3);
        }
    } else {
        auto *c = dynamic_cast<const Conference*>(src);
        extra = QStringLiteral("开会地址: ") + QString::fromStdString(c ? c->getMeetingAddress() : "");
    }
    setLabel(m_detailFilePathLabel, extra);

    // 更新表单行标签
    auto updateRowLabel = [this](int row, const QString &text) {
        if (auto *form = qobject_cast<QFormLayout*>(m_detailPanel->findChild<QScrollArea*>()->widget()->layout())) {
            if (auto *label = qobject_cast<QLabel*>(form->itemAt(row, QFormLayout::LabelRole)->widget())) {
                label->setText(text);
            }
        }
    };

    updateRowLabel(0, QStringLiteral("简称:"));
    updateRowLabel(1, QStringLiteral("类型:"));
    updateRowLabel(2, QStringLiteral("全名:"));
    updateRowLabel(3, QStringLiteral("领域:"));
    updateRowLabel(4, QStringLiteral("出版单位:"));
    updateRowLabel(5, QStringLiteral("检索类型:"));
    updateRowLabel(6, QStringLiteral("补充信息:"));

    if (m_openDetailAttachmentButton) {
        m_openDetailAttachmentButton->setVisible(false);
    }
    if (m_detailPanel) {
        m_detailPanel->show();
    }
}

void MainWindow::clearDetailPanel()
{
    m_detailPaperId = INVALID_ID;
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
    if (m_openDetailAttachmentButton) {
        m_openDetailAttachmentButton->setEnabled(false);
    }
    if (m_detailPanel) {
        m_detailPanel->hide();
    }
}

void MainWindow::restoreDetailPanelLabels()
{
    auto setRowLabel = [this](int row, const QString &text) {
        if (auto *form = qobject_cast<QFormLayout*>(
                m_detailPanel->findChild<QScrollArea*>()->widget()->layout())) {
            if (auto *label = qobject_cast<QLabel*>(
                    form->itemAt(row, QFormLayout::LabelRole)->widget())) {
                label->setText(text);
            }
        }
    };
    setRowLabel(0, QStringLiteral("完整标题:"));
    setRowLabel(1, QStringLiteral("作者名字:"));
    setRowLabel(2, QStringLiteral("出版物:"));
    setRowLabel(3, QStringLiteral("发表时间:"));
    setRowLabel(4, QStringLiteral("上传时间:"));
    setRowLabel(5, QStringLiteral("关键词:"));
    setRowLabel(6, QStringLiteral("附件链接:"));
    if (m_openDetailAttachmentButton) {
        m_openDetailAttachmentButton->setVisible(true);
    }
}

void MainWindow::onViewPaperDetail()
{
    // 出版物视图：显示出版物详情
    if (m_stackedWidget && m_stackedWidget->currentIndex() == 1) {
        const IdType sourceId = selectedSourceId();
        if (sourceId == INVALID_ID) {
            clearDetailPanel();
            return;
        }
        if (m_detailPanel && m_detailPanel->isVisible() && m_detailPaperId == sourceId) {
            m_detailPanel->hide();
            return;
        }
        showSourceDetail(sourceId);
        return;
    }

    // 论文视图：显示论文详情
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
    const bool hasPaper = selectedPaperId() != INVALID_ID;
    const bool hasSource = (selectedSourceId() != INVALID_ID);
    if (m_viewDetailButton) {
        m_viewDetailButton->setEnabled(hasPaper || hasSource);
    }

    // 详情面板已打开时自动刷新
    if (m_detailPanel && m_detailPanel->isVisible()) {
        if (hasSource && m_stackedWidget && m_stackedWidget->currentIndex() == 1) {
            if (m_detailPaperId != selectedSourceId()) {
                showSourceDetail(selectedSourceId());
            }
        } else if (hasPaper && m_detailPaperId != selectedPaperId()) {
            updateDetailPanel(selectedPaperId());
        }
    }
}

void MainWindow::onOpenDetailAttachment()
{
    Paper *paper = LibraryManager::getInstance().findPaper(m_detailPaperId);
    if (!paper) {
        clearDetailPanel();
        QMessageBox::warning(this, QStringLiteral("无法打开"), QStringLiteral("PDF 文件不存在或路径无效"));
        return;
    }

    const QString path = QDir::fromNativeSeparators(QString::fromStdString(paper->getFilePath()));
    if (path.isEmpty() || !QFile::exists(path)) {
        QMessageBox::warning(this, QStringLiteral("无法打开"), QStringLiteral("PDF 文件不存在或路径无效"));
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

    const int nodeType = current->data(0, kRoleNodeType).toInt();
    if (nodeType == static_cast<int>(LibraryNodeType::PublicationRoot)
        || nodeType == static_cast<int>(LibraryNodeType::PublicationGroup)) {
        // 切换到出版物表
        m_stackedWidget->setCurrentIndex(1);
        m_sourceToolBar->setVisible(true);
        m_detailPanel->hide();
        if (m_currentNodeKey == QStringLiteral("journals")) {
            refreshSourceTable(QStringLiteral("Journal"));
        } else if (m_currentNodeKey == QStringLiteral("conferences")) {
            refreshSourceTable(QStringLiteral("Conference"));
        } else {
            refreshSourceTable(QStringLiteral("all"));
        }
    } else {
        // 切换回论文表
        m_stackedWidget->setCurrentIndex(0);
        m_sourceToolBar->setVisible(false);
        if (m_detailPanelTitle) {
            m_detailPanelTitle->setText(QStringLiteral("文献详情"));
        }
        restoreDetailPanelLabels();
        refreshPaperTable();
    }
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
    selectSystemNode(QStringLiteral("all"));
    refreshPaperTable();
}

void MainWindow::onAddPaper()
{
    PaperDialog dialog(this);
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

void MainWindow::onEditPaper()
{
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
    dialog.setPaper(*paper);
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
    const IdType paperId = selectedPaperId();
    if (paperId == INVALID_ID) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请先选择一条文献。"));
        return;
    }
    auto reply = QMessageBox::question(
        this,
        QStringLiteral("删除文献"),
        QStringLiteral("确定删除该文献吗？系统只删除记录，不会删除本地 PDF 文件。"));
    if (reply != QMessageBox::Yes) {
        return;
    }

    LibraryManager::getInstance().removePaper(paperId);
    if (m_detailPaperId == paperId) {
        clearDetailPanel();
    }
    rebuildCatalogTree();
    restoreSelectionAfterReload(currentNodeKey());
    refreshPaperTable();
    showStatus(QStringLiteral("已删除文献"));
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

void MainWindow::onAddPublication(const QString &forceType)
{
    SourceDialog dlg(this);
    if (!forceType.isEmpty()) {
        dlg.setForceType(forceType);
    }
    if (dlg.exec() != QDialog::Accepted) {
        return;
    }

    auto src = dlg.getSource();
    auto &mgr = LibraryManager::getInstance();
    mgr.addSource(src);

    const QString filter = (src->getType() == "Journal")
        ? QStringLiteral("Journal") : QStringLiteral("Conference");
    refreshSourceTable(filter);
    showStatus(QStringLiteral("已添加出版物"));
}

void MainWindow::onEditSourceFromToolbar()
{
    const IdType sourceId = selectedSourceId();
    if (sourceId == INVALID_ID) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请先在表中选择一个出版物。"));
        return;
    }
    auto &mgr = LibraryManager::getInstance();
    Source *src = mgr.findSource(sourceId);
    if (!src) return;

    SourceDialog dlg(this);
    dlg.setSource(*src);
    if (dlg.exec() != QDialog::Accepted) return;

    auto updatedSrc = dlg.getSource();
    updatedSrc->setId(sourceId);
    mgr.updateSource(sourceId, updatedSrc);
    refreshSourceTable(QString::fromStdString(updatedSrc->getType()));
    showStatus(QStringLiteral("已更新出版物"));
}

void MainWindow::onDeleteSourceFromToolbar()
{
    const IdType sourceId = selectedSourceId();
    if (sourceId == INVALID_ID) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请先在表中选择一个出版物。"));
        return;
    }
    auto &mgr = LibraryManager::getInstance();
    Source *src = mgr.findSource(sourceId);
    if (!src) return;

    auto reply = QMessageBox::question(this,
        QStringLiteral("删除出版物"),
        QStringLiteral("确定删除出版物「%1」吗？相关文献将变为未关联状态。")
            .arg(QString::fromStdString(src->getShortName())));
    if (reply != QMessageBox::Yes) return;

    mgr.removeSource(sourceId);
    refreshSourceTable(QStringLiteral("all"));
    showStatus(QStringLiteral("已删除出版物"));
}

void MainWindow::onCatalogContextMenu(const QPoint &pos)
{
    QTreeWidgetItem *item = m_sidebar->itemAt(pos);
    QMenu menu(m_sidebar);

    if (!item) {
        return;
    }

    m_sidebar->setCurrentItem(item);
    const int nodeType = item->data(0, kRoleNodeType).toInt();
    const QString key = item->data(0, kRoleNodeKey).toString();

    // —— 用户目录节点 ——
    if (isUserCatalogNode(item)) {
        QAction *actAddChild = menu.addAction(QStringLiteral("添加子目录"));
        QAction *actDelete = menu.addAction(QStringLiteral("删除目录"));
        connect(actAddChild, &QAction::triggered, this, &MainWindow::onAddCatalog);
        connect(actDelete, &QAction::triggered, this, &MainWindow::onDeleteCatalog);
    }
    // —— "我的文库"根节点 ——
    else if (key == QStringLiteral("library")) {
        QAction *actAdd = menu.addAction(QStringLiteral("添加目录"));
        connect(actAdd, &QAction::triggered, this, &MainWindow::onAddCatalog);
    }
    // —— "出版物库"根节点 ——
    else if (key == QStringLiteral("publications")) {
        QAction *actAddJournal = menu.addAction(QStringLiteral("新增期刊"));
        QAction *actAddConf = menu.addAction(QStringLiteral("新增会议"));
        connect(actAddJournal, &QAction::triggered, this, [this]() {
            onAddPublication(QStringLiteral("Journal"));
        });
        connect(actAddConf, &QAction::triggered, this, [this]() {
            onAddPublication(QStringLiteral("Conference"));
        });
    }
    // —— "期刊" 分组 ——
    else if (key == QStringLiteral("journals")) {
        QAction *actAdd = menu.addAction(QStringLiteral("新增期刊"));
        connect(actAdd, &QAction::triggered, this, [this]() {
            onAddPublication(QStringLiteral("Journal"));
        });
    }
    // —— "会议" 分组 ——
    else if (key == QStringLiteral("conferences")) {
        QAction *actAdd = menu.addAction(QStringLiteral("新增会议"));
        connect(actAdd, &QAction::triggered, this, [this]() {
            onAddPublication(QStringLiteral("Conference"));
        });
    }
    else {
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

IdType MainWindow::selectedPaperId() const
{
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
        QStringLiteral("加载数据"), QString(),
        QStringLiteral("数据文件 (*.txt);;所有文件 (*)"));
    if (path.isEmpty()) return;

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
