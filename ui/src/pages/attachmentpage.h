#ifndef ATTACHMENTPAGE_H
#define ATTACHMENTPAGE_H

#include <QWidget>

class QTableWidget;
class QLineEdit;
class QPushButton;

class AttachmentPage : public QWidget
{
    Q_OBJECT
public:
    explicit AttachmentPage(QWidget *parent = nullptr);
    void refreshTable();

private slots:
    void onAdd();
    void onEdit();
    void onDelete();
    void onSearch();
    void onOpenFile();

private:
    QTableWidget *m_table;
    QLineEdit    *m_searchEdit;
    QPushButton  *m_btnAdd;
    QPushButton  *m_btnEdit;
    QPushButton  *m_btnDelete;
    QPushButton  *m_btnSearch;
    QPushButton  *m_btnOpenFile;
};

#endif // ATTACHMENTPAGE_H
