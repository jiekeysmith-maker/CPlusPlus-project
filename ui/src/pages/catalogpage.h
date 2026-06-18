#ifndef CATALOGPAGE_H
#define CATALOGPAGE_H

#include <QWidget>
#include <QTreeWidget>
#include "manage.h"

class QTreeWidgetItem;

constexpr int ROLE_ID         = Qt::UserRole;
constexpr int ROLE_TYPE       = Qt::UserRole + 1;
constexpr int ROLE_CATALOG_ID = Qt::UserRole + 2;
constexpr int TYPE_CATALOG    = 0;
constexpr int TYPE_PAPER      = 1;
constexpr const char* MIME_CATALOG_ID = "application/x-literature-catalog-id";

class CatalogTreeWidget : public QTreeWidget
{
    Q_OBJECT
public:
    explicit CatalogTreeWidget(QWidget *parent = nullptr) : QTreeWidget(parent) {}

signals:
    void dropped();

protected:
    void dropEvent(QDropEvent *event) override;
    QMimeData *mimeData(const QList<QTreeWidgetItem*> &items) const override;
    QStringList mimeTypes() const override;
};

class CatalogPage : public QWidget
{
    Q_OBJECT
public:
    explicit CatalogPage(QWidget *parent = nullptr);
    void refreshTree();

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void onAddRoot();
    void onAddChild();
    void onEdit();
    void onDelete();
    void onAddPaper();
    void onRemovePaper();
    void onItemDoubleClicked(QTreeWidgetItem *item, int column);
    void onContextMenu(const QPoint &pos);

private:
    void addChildItems(QTreeWidgetItem *parentItem, IdType parentId);
    void addPaperItems(QTreeWidgetItem *parentItem, IdType catalogId);
    void populateTreeItem(QTreeWidgetItem *item, const Catalog &cat);
    void populatePaperItem(QTreeWidgetItem *item, const Paper &paper);
    IdType selectedCatalogId() const;

    CatalogTreeWidget *m_tree;
};

#endif // CATALOGPAGE_H
