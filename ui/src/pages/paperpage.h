#ifndef PAPERPAGE_H
#define PAPERPAGE_H

#include <QWidget>
#include <QTableWidget>
#include <vector>

class QLineEdit;
class QComboBox;
class QPushButton;

class PaperDropTableWidget : public QTableWidget
{
    Q_OBJECT
public:
    explicit PaperDropTableWidget(QWidget *parent = nullptr);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
};

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

    PaperDropTableWidget *m_table;
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
