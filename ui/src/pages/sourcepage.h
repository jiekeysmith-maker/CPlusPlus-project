#ifndef SOURCEPAGE_H
#define SOURCEPAGE_H

#include <QWidget>

class QTableWidget;
class QLineEdit;
class QPushButton;

class SourcePage : public QWidget
{
    Q_OBJECT
public:
    explicit SourcePage(QWidget *parent = nullptr);
    void refreshTable();

private slots:
    void onAddJournal();
    void onAddConference();
    void onEdit();
    void onDelete();
    void onSearch();

private:
    QTableWidget *m_table;
    QLineEdit    *m_searchEdit;
    QPushButton  *m_btnAddJournal;
    QPushButton  *m_btnAddConference;
    QPushButton  *m_btnEdit;
    QPushButton  *m_btnDelete;
    QPushButton  *m_btnSearch;
};

#endif // SOURCEPAGE_H
