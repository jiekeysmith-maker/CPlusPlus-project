#include "sourcepage.h"
#include "sourcedialog.h"
#include "manage.h"

#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>

SourcePage::SourcePage(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *toolbar = new QHBoxLayout;
    m_btnAddJournal    = new QPushButton(QStringLiteral("新增期刊"));
    m_btnAddConference = new QPushButton(QStringLiteral("新增会议"));
    m_btnEdit   = new QPushButton(QStringLiteral("编辑"));
    m_btnDelete = new QPushButton(QStringLiteral("删除"));
    m_searchEdit = new QLineEdit;
    m_searchEdit->setPlaceholderText(QStringLiteral("搜索..."));
    m_searchEdit->setClearButtonEnabled(true);
    m_btnSearch = new QPushButton(QStringLiteral("搜索"));

    toolbar->addWidget(m_btnAddJournal);
    toolbar->addWidget(m_btnAddConference);
    toolbar->addWidget(m_btnEdit);
    toolbar->addWidget(m_btnDelete);
    toolbar->addStretch();
    toolbar->addWidget(m_searchEdit);
    toolbar->addWidget(m_btnSearch);

    m_table = new QTableWidget;
    m_table->setColumnCount(8);
    QStringList headers;
    headers << QStringLiteral("ID")
            << QStringLiteral("类型")
            << QStringLiteral("简称")
            << QStringLiteral("全名")
            << QStringLiteral("领域")
            << QStringLiteral("出版单位")
            << QStringLiteral("检索类型")
            << QStringLiteral("影响因子/会议地址");
    m_table->setHorizontalHeaderLabels(headers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setColumnHidden(0, true);

    mainLayout->addLayout(toolbar);
    mainLayout->addWidget(m_table);

    connect(m_btnAddJournal,    &QPushButton::clicked, this, &SourcePage::onAddJournal);
    connect(m_btnAddConference, &QPushButton::clicked, this, &SourcePage::onAddConference);
    connect(m_btnEdit,   &QPushButton::clicked, this, &SourcePage::onEdit);
    connect(m_btnDelete, &QPushButton::clicked, this, &SourcePage::onDelete);
    connect(m_btnSearch, &QPushButton::clicked, this, &SourcePage::onSearch);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &SourcePage::onSearch);

    refreshTable();
}

void SourcePage::refreshTable()
{
    const auto &all = LibraryManager::getInstance().getAllSources();
    m_table->setRowCount(static_cast<int>(all.size()));
    int row = 0;
    for (const auto &pair : all) {
        const Source *s = pair.second.get();
        QTableWidgetItem *idItem = new QTableWidgetItem;
        idItem->setData(Qt::UserRole, s->getId());
        m_table->setItem(row, 0, idItem);
        m_table->setItem(row, 1, new QTableWidgetItem(
            QString::fromStdString(s->getType())));
        m_table->setItem(row, 2, new QTableWidgetItem(
            QString::fromStdString(s->getShortName())));
        m_table->setItem(row, 3, new QTableWidgetItem(
            QString::fromStdString(s->getFullName())));
        m_table->setItem(row, 4, new QTableWidgetItem(
            QString::fromStdString(s->getField())));
        m_table->setItem(row, 5, new QTableWidgetItem(
            QString::fromStdString(s->getPublisher())));
        m_table->setItem(row, 6, new QTableWidgetItem(
            QString::fromStdString(s->getRetrievalType())));

        if (s->getType() == "Journal") {
            const Journal *j = dynamic_cast<const Journal*>(s);
            m_table->setItem(row, 7, new QTableWidgetItem(
                QString::number(j->getImpactFactor(), 'f', 3)));
        } else {
            const Conference *c = dynamic_cast<const Conference*>(s);
            m_table->setItem(row, 7, new QTableWidgetItem(
                QString::fromStdString(c->getMeetingAddress())));
        }
        ++row;
    }
}

void SourcePage::onAddJournal()
{
    SourceDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("新增期刊"));
    dlg.setForceType(QStringLiteral("Journal"));
    if (dlg.exec() == QDialog::Accepted) {
        LibraryManager::getInstance().addSource(dlg.getSource());
        refreshTable();
    }
}

void SourcePage::onAddConference()
{
    SourceDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("新增会议"));
    dlg.setForceType(QStringLiteral("Conference"));
    if (dlg.exec() == QDialog::Accepted) {
        LibraryManager::getInstance().addSource(dlg.getSource());
        refreshTable();
    }
}

void SourcePage::onEdit()
{
    int row = m_table->currentRow();
    if (row < 0) {
        QMessageBox::information(this, QStringLiteral("提示"),
                                 QStringLiteral("请先选择一条记录。"));
        return;
    }
    IdType id = m_table->item(row, 0)->data(Qt::UserRole).toInt();
    Source *s = LibraryManager::getInstance().findSource(id);
    if (!s) return;

    SourceDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("编辑出版物"));
    dlg.setSource(*s);
    if (dlg.exec() == QDialog::Accepted) {
        LibraryManager::getInstance().updateSource(id, dlg.getSource());
        refreshTable();
    }
}

void SourcePage::onDelete()
{
    int row = m_table->currentRow();
    if (row < 0) {
        QMessageBox::information(this, QStringLiteral("提示"),
                                 QStringLiteral("请先选择一条记录。"));
        return;
    }
    IdType id = m_table->item(row, 0)->data(Qt::UserRole).toInt();
    auto result = QMessageBox::question(this, QStringLiteral("确认删除"),
        QStringLiteral("确定要删除该出版物吗？（ID: %1）").arg(id));
    if (result == QMessageBox::Yes) {
        LibraryManager::getInstance().removeSource(id);
        refreshTable();
    }
}

void SourcePage::onSearch()
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
