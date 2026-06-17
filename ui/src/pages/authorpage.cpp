#include "authorpage.h"
#include "authordialog.h"
#include "manage.h"

#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>

AuthorPage::AuthorPage(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // 工具按钮栏
    QHBoxLayout *toolbar = new QHBoxLayout;
    m_btnAdd    = new QPushButton(QStringLiteral("新增"));
    m_btnEdit   = new QPushButton(QStringLiteral("编辑"));
    m_btnDelete = new QPushButton(QStringLiteral("删除"));
    m_searchEdit = new QLineEdit;
    m_searchEdit->setPlaceholderText(QStringLiteral("搜索..."));
    m_searchEdit->setClearButtonEnabled(true);
    m_btnSearch = new QPushButton(QStringLiteral("搜索"));

    toolbar->addWidget(m_btnAdd);
    toolbar->addWidget(m_btnEdit);
    toolbar->addWidget(m_btnDelete);
    toolbar->addStretch();
    toolbar->addWidget(m_searchEdit);
    toolbar->addWidget(m_btnSearch);

    // 表格
    m_table = new QTableWidget;
    m_table->setColumnCount(6);
    QStringList headers;
    headers << QStringLiteral("ID")
            << QStringLiteral("姓名")
            << QStringLiteral("性别")
            << QStringLiteral("单位")
            << QStringLiteral("邮箱")
            << QStringLiteral("研究领域");
    m_table->setHorizontalHeaderLabels(headers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setColumnHidden(0, true);

    mainLayout->addLayout(toolbar);
    mainLayout->addWidget(m_table);

    connect(m_btnAdd,    &QPushButton::clicked, this, &AuthorPage::onAdd);
    connect(m_btnEdit,   &QPushButton::clicked, this, &AuthorPage::onEdit);
    connect(m_btnDelete, &QPushButton::clicked, this, &AuthorPage::onDelete);
    connect(m_btnSearch, &QPushButton::clicked, this, &AuthorPage::onSearch);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &AuthorPage::onSearch);

    refreshTable();
}

void AuthorPage::refreshTable()
{
    const auto &all = LibraryManager::getInstance().getAllAuthors();
    m_table->setRowCount(static_cast<int>(all.size()));
    int row = 0;
    for (const auto &pair : all) {
        const Author &a = pair.second;
        QTableWidgetItem *idItem = new QTableWidgetItem;
        idItem->setData(Qt::UserRole, a.getId());
        m_table->setItem(row, 0, idItem);
        m_table->setItem(row, 1, new QTableWidgetItem(
            QString::fromStdString(a.getName())));
        m_table->setItem(row, 2, new QTableWidgetItem(
            QString::fromStdString(a.getGender())));
        m_table->setItem(row, 3, new QTableWidgetItem(
            QString::fromStdString(a.getAffiliation())));
        m_table->setItem(row, 4, new QTableWidgetItem(
            QString::fromStdString(a.getEmail())));

        QString areas;
        for (size_t i = 0; i < a.getResearchAreas().size(); ++i) {
            if (i > 0) areas += "; ";
            areas += QString::fromStdString(a.getResearchAreas()[i]);
        }
        m_table->setItem(row, 5, new QTableWidgetItem(areas));
        ++row;
    }
}

void AuthorPage::onAdd()
{
    AuthorDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("新增作者"));
    if (dlg.exec() == QDialog::Accepted) {
        LibraryManager::getInstance().addAuthor(dlg.getEntity());
        refreshTable();
    }
}

void AuthorPage::onEdit()
{
    int row = m_table->currentRow();
    if (row < 0) {
        QMessageBox::information(this, QStringLiteral("提示"),
                                 QStringLiteral("请先选择一条记录。"));
        return;
    }
    IdType id = m_table->item(row, 0)->data(Qt::UserRole).toInt();
    Author *a = LibraryManager::getInstance().findAuthor(id);
    if (!a) return;

    AuthorDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("编辑作者"));
    dlg.setEntity(*a);
    if (dlg.exec() == QDialog::Accepted) {
        LibraryManager::getInstance().updateAuthor(id, dlg.getEntity());
        refreshTable();
    }
}

void AuthorPage::onDelete()
{
    int row = m_table->currentRow();
    if (row < 0) {
        QMessageBox::information(this, QStringLiteral("提示"),
                                 QStringLiteral("请先选择一条记录。"));
        return;
    }
    IdType id = m_table->item(row, 0)->data(Qt::UserRole).toInt();
    auto result = QMessageBox::question(this, QStringLiteral("确认删除"),
        QStringLiteral("确定要删除该作者吗？（ID: %1）").arg(id));
    if (result == QMessageBox::Yes) {
        LibraryManager::getInstance().removeAuthor(id);
        refreshTable();
    }
}

void AuthorPage::onSearch()
{
    QString keyword = m_searchEdit->text().trimmed();
    for (int row = 0; row < m_table->rowCount(); ++row) {
        bool match = keyword.isEmpty();
        for (int col = 1; col < m_table->columnCount(); ++col) {
            QTableWidgetItem *item = m_table->item(row, col);
            if (item && item->text().contains(keyword, Qt::CaseInsensitive)) {
                match = true;
                break;
            }
        }
        m_table->setRowHidden(row, !match);
    }
}
