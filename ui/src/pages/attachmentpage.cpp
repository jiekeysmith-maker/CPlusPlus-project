#include "attachmentpage.h"
#include "attachmentdialog.h"
#include "manage.h"

#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>

AttachmentPage::AttachmentPage(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

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

    m_table = new QTableWidget;
    m_table->setColumnCount(5);
    QStringList headers;
    headers << QStringLiteral("ID")
            << QStringLiteral("名称")
            << QStringLiteral("所属文献ID")
            << QStringLiteral("所属文献标题")
            << QStringLiteral("内容预览");
    m_table->setHorizontalHeaderLabels(headers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setColumnHidden(0, true);

    mainLayout->addLayout(toolbar);
    mainLayout->addWidget(m_table);

    connect(m_btnAdd,    &QPushButton::clicked, this, &AttachmentPage::onAdd);
    connect(m_btnEdit,   &QPushButton::clicked, this, &AttachmentPage::onEdit);
    connect(m_btnDelete, &QPushButton::clicked, this, &AttachmentPage::onDelete);
    connect(m_btnSearch, &QPushButton::clicked, this, &AttachmentPage::onSearch);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &AttachmentPage::onSearch);

    refreshTable();
}

void AttachmentPage::refreshTable()
{
    const auto &all = LibraryManager::getInstance().getAllAttachments();
    m_table->setRowCount(static_cast<int>(all.size()));
    int row = 0;
    auto &mgr = LibraryManager::getInstance();
    for (const auto &pair : all) {
        const Attachment &att = pair.second;
        QTableWidgetItem *idItem = new QTableWidgetItem;
        idItem->setData(Qt::UserRole, att.getId());
        m_table->setItem(row, 0, idItem);
        m_table->setItem(row, 1, new QTableWidgetItem(
            QString::fromStdString(att.getName())));
        m_table->setItem(row, 2, new QTableWidgetItem(
            QString::number(att.getPaperId())));

        Paper *p = mgr.findPaper(att.getPaperId());
        m_table->setItem(row, 3, new QTableWidgetItem(
            p ? QString::fromStdString(p->getTitle()) : QStringLiteral("未知")));

        QString preview = QString::fromStdString(att.getContent());
        if (preview.length() > 50)
            preview = preview.left(50) + QStringLiteral("...");
        m_table->setItem(row, 4, new QTableWidgetItem(preview));
        ++row;
    }
}

void AttachmentPage::onAdd()
{
    AttachmentDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("新增附件"));
    if (dlg.exec() == QDialog::Accepted) {
        LibraryManager::getInstance().addAttachment(dlg.getEntity());
        refreshTable();
    }
}

void AttachmentPage::onEdit()
{
    int row = m_table->currentRow();
    if (row < 0) {
        QMessageBox::information(this, QStringLiteral("提示"),
                                 QStringLiteral("请先选择一条记录。"));
        return;
    }
    IdType id = m_table->item(row, 0)->data(Qt::UserRole).toInt();
    Attachment *att = LibraryManager::getInstance().findAttachment(id);
    if (!att) return;

    AttachmentDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("编辑附件"));
    dlg.setEntity(*att);
    if (dlg.exec() == QDialog::Accepted) {
        LibraryManager::getInstance().updateAttachment(id, dlg.getEntity());
        refreshTable();
    }
}

void AttachmentPage::onDelete()
{
    int row = m_table->currentRow();
    if (row < 0) {
        QMessageBox::information(this, QStringLiteral("提示"),
                                 QStringLiteral("请先选择一条记录。"));
        return;
    }
    IdType id = m_table->item(row, 0)->data(Qt::UserRole).toInt();
    auto result = QMessageBox::question(this, QStringLiteral("确认删除"),
        QStringLiteral("确定要删除该附件吗？（ID: %1）").arg(id));
    if (result == QMessageBox::Yes) {
        LibraryManager::getInstance().removeAttachment(id);
        refreshTable();
    }
}

void AttachmentPage::onSearch()
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
