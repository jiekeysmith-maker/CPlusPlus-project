#ifndef CATALOGPAGE_H
#define CATALOGPAGE_H

#include <QWidget>
#include "manage.h"

class QTreeWidget;
class QTreeWidgetItem;
class QPushButton;

class CatalogPage : public QWidget
{
    Q_OBJECT
public:
    explicit CatalogPage(QWidget *parent = nullptr);
    void refreshTree();

private slots:
    void onAddRoot();
    void onAddChild();
    void onEdit();
    void onDelete();
    void onAddPaper();
    void onRemovePaper();

private:
    void addChildItems(QTreeWidgetItem *parentItem, IdType parentId);
    void populateTreeItem(QTreeWidgetItem *item, const Catalog &cat);
    IdType selectedCatalogId() const;

    QTreeWidget  *m_tree;
    QPushButton  *m_btnAddRoot;
    QPushButton  *m_btnAddChild;
    QPushButton  *m_btnEdit;
    QPushButton  *m_btnDelete;
    QPushButton  *m_btnAddPaper;
    QPushButton  *m_btnRemovePaper;
};

#endif // CATALOGPAGE_H
