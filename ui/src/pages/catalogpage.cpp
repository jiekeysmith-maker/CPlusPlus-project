#include "catalogpage.h"
#include "catalogdialog.h"
#include "manage.h"

#include <QTreeWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>

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

    m_tree = new QTreeWidget;
    QStringList headers;
    headers << QStringLiteral("名称")
            << QStringLiteral("层级")
            << QStringLiteral("文献数")
            << QStringLiteral("说明");
    m_tree->setHeaderLabels(headers);
    m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tree->header()->setStretchLastSection(true);

    mainLayout->addLayout(toolbar);
    mainLayout->addWidget(m_tree);

    connect(m_btnAddRoot,     &QPushButton::clicked, this, &CatalogPage::onAddRoot);
    connect(m_btnAddChild,    &QPushButton::clicked, this, &CatalogPage::onAddChild);
    connect(m_btnEdit,        &QPushButton::clicked, this, &CatalogPage::onEdit);
    connect(m_btnDelete,      &QPushButton::clicked, this, &CatalogPage::onDelete);
    connect(m_btnAddPaper,    &QPushButton::clicked, this, &CatalogPage::onAddPaper);
    connect(m_btnRemovePaper, &QPushButton::clicked, this, &CatalogPage::onRemovePaper);

    refreshTree();
}

IdType CatalogPage::selectedCatalogId() const
{
    QTreeWidgetItem *item = m_tree->currentItem();
    if (!item) return INVALID_ID;
    return item->data(0, Qt::UserRole).toInt();
}

void CatalogPage::populateTreeItem(QTreeWidgetItem *item, const Catalog &cat)
{
    item->setText(0, QString::fromStdString(cat.getName()));
    item->setText(1, QString::number(cat.getLevel()));
    item->setText(2, QString::number(cat.getPaperIds().size()));
    item->setText(3, QString::fromStdString(cat.getDescription()));
    item->setData(0, Qt::UserRole, cat.getId());
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
    }
    m_tree->expandAll();
}

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
        QStringLiteral("确定要删除该目录吗？（ID: %1）\n子目录将上移一级。").arg(id));
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
    bool ok;
    int paperId = QInputDialog::getInt(this,
        QStringLiteral("添加文献到目录"),
        QStringLiteral("请输入文献ID:"),
        0, 0, 999999, 1, &ok);
    if (!ok) return;
    if (LibraryManager::getInstance().addPaperToCatalog(paperId, catId)) {
        refreshTree();
    } else {
        QMessageBox::warning(this, QStringLiteral("操作失败"),
                             QStringLiteral("文献或目录不存在。"));
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
    bool ok;
    int paperId = QInputDialog::getInt(this,
        QStringLiteral("从目录移除文献"),
        QStringLiteral("请输入文献ID:"),
        0, 0, 999999, 1, &ok);
    if (!ok) return;
    if (LibraryManager::getInstance().removePaperFromCatalog(paperId, catId)) {
        refreshTree();
    } else {
        QMessageBox::warning(this, QStringLiteral("操作失败"),
                             QStringLiteral("目录不存在。"));
    }
}
