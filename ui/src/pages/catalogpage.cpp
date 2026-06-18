#include "catalogpage.h"
#include "catalogdialog.h"
#include "manage.h"

#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>
#include <QMimeData>
#include <QDropEvent>
#include <QListWidget>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QApplication>
#include <QStyle>

// ============================================================
// CatalogTreeWidget — drag & drop support
// ============================================================

void CatalogTreeWidget::dropEvent(QDropEvent *event)
{
    QTreeWidgetItem *targetItem = itemAt(event->position().toPoint());
    QList<QTreeWidgetItem*> sel = selectedItems();

    if (!targetItem || sel.isEmpty() || targetItem == sel.first()) {
        event->ignore();
        return;
    }

    QTreeWidgetItem *sourceItem = sel.first();
    int sourceType = sourceItem->data(0, ROLE_TYPE).toInt();
    int targetType = targetItem->data(0, ROLE_TYPE).toInt();

    if (sourceType == TYPE_PAPER && targetType == TYPE_CATALOG) {
        IdType paperId = sourceItem->data(0, ROLE_ID).toInt();
        IdType sourceCatId = sourceItem->data(0, ROLE_CATALOG_ID).toInt();
        IdType targetCatId = targetItem->data(0, ROLE_ID).toInt();

        if (sourceCatId == targetCatId) {
            event->ignore();
            return;
        }

        auto &mgr = LibraryManager::getInstance();
        mgr.removePaperFromCatalog(paperId, sourceCatId);
        mgr.addPaperToCatalog(paperId, targetCatId);

        event->accept();
        emit dropped();
    } else {
        event->ignore();
    }
}

QMimeData *CatalogTreeWidget::mimeData(const QList<QTreeWidgetItem*> &items) const
{
    QMimeData *data = QTreeWidget::mimeData(items);
    if (!items.isEmpty()) {
        int type = items.first()->data(0, ROLE_TYPE).toInt();
        if (type == TYPE_CATALOG) {
            QByteArray ba;
            ba.setNum(items.first()->data(0, ROLE_ID).toInt());
            data->setData(MIME_CATALOG_ID, ba);
        }
    }
    return data;
}

QStringList CatalogTreeWidget::mimeTypes() const
{
    QStringList types = QTreeWidget::mimeTypes();
    types << MIME_CATALOG_ID;
    return types;
}

// ============================================================
// PaperPickerDialog — checkable paper list for bulk selection
// ============================================================

class PaperPickerDialog : public QDialog
{
public:
    std::vector<IdType> selectedIds;

    PaperPickerDialog(QWidget *parent, IdType catalogId, bool catalogMode)
        : QDialog(parent)
    {
        setWindowTitle(catalogMode ? QStringLiteral("选择要从目录移除的文献")
                                   : QStringLiteral("选择要添加到目录的文献"));
        setModal(true);
        setMinimumSize(500, 400);

        auto *layout = new QVBoxLayout(this);
        auto *list = new QListWidget;

        auto &mgr = LibraryManager::getInstance();
        if (catalogMode) {
            for (const Paper &p : mgr.getPapersInCatalog(catalogId)) {
                auto *item = new QListWidgetItem(
                    QString("[%1] %2").arg(p.getId())
                        .arg(QString::fromStdString(p.getTitle())));
                item->setData(Qt::UserRole, p.getId());
                item->setCheckState(Qt::Unchecked);
                list->addItem(item);
            }
        } else {
            for (const auto &pair : mgr.getAllPapers()) {
                const Paper &p = pair.second;
                auto *item = new QListWidgetItem(
                    QString("[%1] %2").arg(p.getId())
                        .arg(QString::fromStdString(p.getTitle())));
                item->setData(Qt::UserRole, p.getId());
                item->setCheckState(Qt::Unchecked);
                list->addItem(item);
            }
        }
        layout->addWidget(list);

        auto *btnLayout = new QHBoxLayout;
        auto *btnAll = new QPushButton(QStringLiteral("全选"));
        auto *btnNone = new QPushButton(QStringLiteral("取消全选"));
        btnLayout->addWidget(btnAll);
        btnLayout->addWidget(btnNone);
        btnLayout->addStretch();
        layout->addLayout(btnLayout);

        auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        buttonBox->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确定"));
        buttonBox->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
        connect(buttonBox, &QDialogButtonBox::accepted, this, [this, list]() {
            for (int i = 0; i < list->count(); ++i) {
                if (list->item(i)->checkState() == Qt::Checked)
                    selectedIds.push_back(list->item(i)->data(Qt::UserRole).toInt());
            }
            accept();
        });
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
        layout->addWidget(buttonBox);

        connect(btnAll, &QPushButton::clicked, this, [list]() {
            for (int i = 0; i < list->count(); ++i)
                list->item(i)->setCheckState(Qt::Checked);
        });
        connect(btnNone, &QPushButton::clicked, this, [list]() {
            for (int i = 0; i < list->count(); ++i)
                list->item(i)->setCheckState(Qt::Unchecked);
        });
    }
};

// ============================================================
// CatalogPage
// ============================================================

CatalogPage::CatalogPage(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *toolbar = new QHBoxLayout;
    m_btnAddRoot     = new QPushButton(QStringLiteral("新增根目录"));
    m_btnAddChild    = new QPushButton(QStringLiteral("添加子目录"));
    m_btnEdit        = new QPushButton(QStringLiteral("编辑"));
    m_btnDelete      = new QPushButton(QStringLiteral("删除"));
    m_btnAddPaper    = new QPushButton(QStringLiteral("添加文献到目录"));
    m_btnRemovePaper = new QPushButton(QStringLiteral("从目录移除文献"));

    toolbar->addWidget(m_btnAddRoot);
    toolbar->addWidget(m_btnAddChild);
    toolbar->addWidget(m_btnEdit);
    toolbar->addWidget(m_btnDelete);
    toolbar->addStretch();
    toolbar->addWidget(m_btnAddPaper);
    toolbar->addWidget(m_btnRemovePaper);

    m_tree = new CatalogTreeWidget;
    QStringList headers;
    headers << QStringLiteral("名称")
            << QStringLiteral("层级/类型")
            << QStringLiteral("文献数/作者")
            << QStringLiteral("说明/文件");
    m_tree->setHeaderLabels(headers);
    m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tree->header()->setStretchLastSection(true);
    m_tree->setDragEnabled(true);
    m_tree->setAcceptDrops(true);
    m_tree->setDragDropMode(QAbstractItemView::DragDrop);
    m_tree->setDefaultDropAction(Qt::MoveAction);

    mainLayout->addLayout(toolbar);
    mainLayout->addWidget(m_tree);

    connect(m_btnAddRoot,     &QPushButton::clicked, this, &CatalogPage::onAddRoot);
    connect(m_btnAddChild,    &QPushButton::clicked, this, &CatalogPage::onAddChild);
    connect(m_btnEdit,        &QPushButton::clicked, this, &CatalogPage::onEdit);
    connect(m_btnDelete,      &QPushButton::clicked, this, &CatalogPage::onDelete);
    connect(m_btnAddPaper,    &QPushButton::clicked, this, &CatalogPage::onAddPaper);
    connect(m_btnRemovePaper, &QPushButton::clicked, this, &CatalogPage::onRemovePaper);
    connect(m_tree, &QTreeWidget::itemDoubleClicked, this, &CatalogPage::onItemDoubleClicked);
    connect(m_tree, &CatalogTreeWidget::dropped, this, [this]() { refreshTree(); });

    refreshTree();
}

void CatalogPage::showEvent(QShowEvent *event)
{
    refreshTree();
    QWidget::showEvent(event);
}

IdType CatalogPage::selectedCatalogId() const
{
    QTreeWidgetItem *item = m_tree->currentItem();
    if (!item) return INVALID_ID;
    return item->data(0, ROLE_ID).toInt();
}

void CatalogPage::populateTreeItem(QTreeWidgetItem *item, const Catalog &cat)
{
    item->setText(0, QString::fromStdString(cat.getName()));
    item->setText(1, QString::number(cat.getLevel()));
    item->setText(2, QString::number(cat.getPaperIds().size()));
    item->setText(3, QString::fromStdString(cat.getDescription()));
    item->setData(0, ROLE_ID, cat.getId());
    item->setData(0, ROLE_TYPE, TYPE_CATALOG);
    item->setFlags(item->flags() | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
}

void CatalogPage::populatePaperItem(QTreeWidgetItem *item, const Paper &paper)
{
    item->setText(0, QString::fromStdString(paper.getTitle()));
    item->setText(1, QStringLiteral("文献"));

    auto &mgr = LibraryManager::getInstance();
    const auto &authorIds = paper.getAuthorIds();
    QString authors;
    for (size_t i = 0; i < authorIds.size(); ++i) {
        if (i > 0) authors += QStringLiteral("; ");
        authors += QString::fromStdString(mgr.getAuthorName(authorIds[i]));
    }
    item->setText(2, authors);

    QString filePath = QString::fromStdString(paper.getFilePath());
    if (!filePath.isEmpty()) {
        QFileInfo fi(filePath);
        item->setText(3, fi.fileName());
    } else {
        item->setText(3, QString());
    }

    item->setData(0, ROLE_ID, paper.getId());
    item->setData(0, ROLE_TYPE, TYPE_PAPER);
    item->setFlags(item->flags() | Qt::ItemIsDragEnabled);
    item->setForeground(0, QColor(0, 0, 180));
}

void CatalogPage::addPaperItems(QTreeWidgetItem *parentItem, IdType catalogId)
{
    const Catalog *cat = LibraryManager::getInstance().findCatalog(catalogId);
    if (!cat) return;
    for (IdType paperId : cat->getPaperIds()) {
        Paper *paper = LibraryManager::getInstance().findPaper(paperId);
        if (!paper) continue;
        auto *item = new QTreeWidgetItem(parentItem);
        populatePaperItem(item, *paper);
        item->setData(0, ROLE_CATALOG_ID, catalogId);
    }
}

void CatalogPage::addChildItems(QTreeWidgetItem *parentItem, IdType parentId)
{
    const Catalog *cat = LibraryManager::getInstance().findCatalog(parentId);
    if (!cat) return;
    for (IdType childId : cat->getChildIds()) {
        const Catalog *child = LibraryManager::getInstance().findCatalog(childId);
        if (!child) continue;
        auto *item = new QTreeWidgetItem(parentItem);
        populateTreeItem(item, *child);
        addChildItems(item, childId);
        addPaperItems(item, childId);
    }
}

void CatalogPage::refreshTree()
{
    m_tree->clear();
    auto roots = LibraryManager::getInstance().getRootCatalogs();
    for (const auto &root : roots) {
        auto *item = new QTreeWidgetItem(m_tree);
        populateTreeItem(item, root);
        addChildItems(item, root.getId());
        addPaperItems(item, root.getId());
    }
    m_tree->expandAll();
}

// ============================================================
// CRUD slots
// ============================================================

void CatalogPage::onAddRoot()
{
    CatalogDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("新增根目录"));
    dlg.setFixedParent(INVALID_ID);
    if (dlg.exec() == QDialog::Accepted) {
        LibraryManager::getInstance().addCatalog(dlg.getEntity());
        refreshTree();
    }
}

void CatalogPage::onAddChild()
{
    IdType parentId = selectedCatalogId();
    if (parentId == INVALID_ID) {
        QMessageBox::information(this, QStringLiteral("提示"),
                                 QStringLiteral("请先选择一个父目录。"));
        return;
    }
    CatalogDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("添加子目录"));
    dlg.setFixedParent(parentId);
    if (dlg.exec() == QDialog::Accepted) {
        LibraryManager::getInstance().addCatalog(dlg.getEntity());
        refreshTree();
    }
}

void CatalogPage::onEdit()
{
    IdType id = selectedCatalogId();
    if (id == INVALID_ID) {
        QMessageBox::information(this, QStringLiteral("提示"),
                                 QStringLiteral("请先选择一个目录。"));
        return;
    }
    Catalog *cat = LibraryManager::getInstance().findCatalog(id);
    if (!cat) return;

    CatalogDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("编辑目录"));
    dlg.setEntity(*cat);
    if (dlg.exec() == QDialog::Accepted) {
        LibraryManager::getInstance().updateCatalog(id, dlg.getEntity());
        refreshTree();
    }
}

void CatalogPage::onDelete()
{
    IdType id = selectedCatalogId();
    if (id == INVALID_ID) {
        QMessageBox::information(this, QStringLiteral("提示"),
                                 QStringLiteral("请先选择一个目录。"));
        return;
    }
    auto result = QMessageBox::question(this, QStringLiteral("确认删除"),
        QStringLiteral("确定要删除该目录吗？（ID: %1）\n子目录将上移一级。\n注意：目录中的文献关联将丢失！").arg(id));
    if (result == QMessageBox::Yes) {
        LibraryManager::getInstance().removeCatalog(id);
        refreshTree();
    }
}

void CatalogPage::onAddPaper()
{
    IdType catId = selectedCatalogId();
    if (catId == INVALID_ID) {
        QMessageBox::information(this, QStringLiteral("提示"),
                                 QStringLiteral("请先选择一个目录。"));
        return;
    }
    PaperPickerDialog dlg(this, catId, false);
    if (dlg.exec() == QDialog::Accepted) {
        auto &mgr = LibraryManager::getInstance();
        for (IdType paperId : dlg.selectedIds) {
            mgr.addPaperToCatalog(paperId, catId);
        }
        refreshTree();
    }
}

void CatalogPage::onRemovePaper()
{
    IdType catId = selectedCatalogId();
    if (catId == INVALID_ID) {
        QMessageBox::information(this, QStringLiteral("提示"),
                                 QStringLiteral("请先选择一个目录。"));
        return;
    }
    PaperPickerDialog dlg(this, catId, true);
    if (dlg.exec() == QDialog::Accepted) {
        auto &mgr = LibraryManager::getInstance();
        for (IdType paperId : dlg.selectedIds) {
            mgr.removePaperFromCatalog(paperId, catId);
        }
        refreshTree();
    }
}

void CatalogPage::onItemDoubleClicked(QTreeWidgetItem *item, int /*column*/)
{
    int type = item->data(0, ROLE_TYPE).toInt();
    if (type != TYPE_PAPER) return;

    IdType paperId = item->data(0, ROLE_ID).toInt();
    Paper *paper = LibraryManager::getInstance().findPaper(paperId);
    if (!paper) return;

    QString filePath = QString::fromStdString(paper->getFilePath());
    if (filePath.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("提示"),
                                 QStringLiteral("该文献还没有上传全文文件。"));
        return;
    }

    QFileInfo info(filePath);
    if (!info.exists()) {
        QMessageBox::warning(this, QStringLiteral("文件不存在"),
                             QStringLiteral("找不到全文文件：\n%1").arg(filePath));
        return;
    }

    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(info.absoluteFilePath()))) {
        QMessageBox::warning(this, QStringLiteral("打开失败"),
                             QStringLiteral("系统无法打开该文件。"));
    }
}
