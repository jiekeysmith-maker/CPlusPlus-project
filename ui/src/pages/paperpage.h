#ifndef PAPERPAGE_H
#define PAPERPAGE_H

#include <QWidget>
#include <vector>

class QTableWidget;
class QLineEdit;
class QComboBox;
class QPushButton;

class PaperPage : public QWidget
{
    Q_OBJECT
public:
    explicit PaperPage(QWidget *parent = nullptr);
    void refreshTable();

private slots:
    void onAdd();
    void onEdit();
    void onDelete();
    void onSearch();
    void onShowAll();
    void onOpenFile();

private:
    void populateTable(const std::vector<class Paper> &papers);

    QTableWidget *m_table;
    QLineEdit    *m_searchEdit;
    QComboBox    *m_searchCombo;
    QPushButton  *m_btnAdd;
    QPushButton  *m_btnEdit;
    QPushButton  *m_btnDelete;
    QPushButton  *m_btnOpenFile;
    QPushButton  *m_btnSearch;
    QPushButton  *m_btnShowAll;
};

#endif // PAPERPAGE_H
