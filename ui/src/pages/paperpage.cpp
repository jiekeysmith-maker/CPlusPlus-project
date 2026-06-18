#include "paperpage.h"
#include "paperdialog.h"
#include "manage.h"

#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QDesktopServices>
#include <QFileInfo>
#include <QUrl>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>

constexpr const char* MIME_CATALOG_DRAG_ID = "application/x-literature-catalog-id";

// ============================================================
// PaperDropTableWidget — accepts catalog drops to add paper
// ============================================================

PaperDropTableWidget::PaperDropTableWidget(QWidget *parent)
    : QTableWidget(parent)
{
    setAcceptDrops(true);
    viewport()->setAcceptDrops(true);
    setDragDropOverwriteMode(false);
    setDropIndicatorShown(true);
}

void PaperDropTableWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat(MIME_CATALOG_DRAG_ID)) {
        event->acceptProposedAction();
    } else {
        QTableWidget::dragEnterEvent(event);
    }
}

void PaperDropTableWidget::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasFormat(MIME_CATALOG_DRAG_ID)) {
        event->acceptProposedAction();
    } else {
        QTableWidget::dragMoveEvent(event);
    }
}

void PaperDropTableWidget::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasFormat(MIME_CATALOG_DRAG_ID)) {
        QByteArray ba = event->mimeData()->data(MIME_CATALOG_DRAG_ID);
        IdType catalogId = ba.toInt();

        QTableWidgetItem *item = itemAt(event->position().toPoint());
        if (item) {
            int row = item->row();
            QTableWidgetItem *idItem = this->item(row, 0);
            if (idItem) {
                IdType paperId = idItem->data(Qt::UserRole).toInt();
                LibraryManager::getInstance().addPaperToCatalog(paperId, catalogId);
                event->acceptProposedAction();
                return;
            }
        }
        event->ignore();
    } else {
        QTableWidget::dropEvent(event);
    }
}

// ============================================================
// PaperPage
// ============================================================

PaperPage::PaperPage(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *toolbar = new QHBoxLayout;
    m_btnAdd    = new QPushButton(QStringLiteral("新增"));
    m_btnEdit   = new QPushButton(QStringLiteral("编辑"));
    m_btnDelete = new QPushButton(QStringLiteral("删除"));
    m_btnOpenFile = new QPushButton(QStringLiteral("打开全文"));

    m_searchCombo = new QComboBox;
    m_searchCombo->addItem(QStringLiteral("按标题"));
    m_searchCombo->addItem(QStringLiteral("按作者"));
    m_searchCombo->addItem(QStringLiteral("按关键词"));

    m_searchEdit = new QLineEdit;
    m_searchEdit->setPlaceholderText(QStringLiteral("搜索..."));
    m_searchEdit->setClearButtonEnabled(true);
    m_btnSearch  = new QPushButton(QStringLiteral("搜索"));
    m_btnShowAll = new QPushButton(QStringLiteral("显示全部"));

    toolbar->addWidget(m_btnAdd);
    toolbar->addWidget(m_btnEdit);
    toolbar->addWidget(m_btnDelete);
    toolbar->addWidget(m_btnOpenFile);
    toolbar->addStretch();
    toolbar->addWidget(m_searchCombo);
    toolbar->addWidget(m_searchEdit);
    toolbar->addWidget(m_btnSearch);
    toolbar->addWidget(m_btnShowAll);

    m_table = new PaperDropTableWidget;
    m_table->setColumnCount(10);
    QStringList headers;
    headers << QStringLiteral("ID")
            << QStringLiteral("DOI编号")
            << QStringLiteral("标题")
            << QStringLiteral("关键词")
            << QStringLiteral("发表时间")
            << QStringLiteral("出版物")
            << QStringLiteral("作者数")
            << QStringLiteral("刊期")
            << QStringLiteral("页码")
            << QStringLiteral("全文");
    m_table->setHorizontalHeaderLabels(headers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setColumnHidden(0, true);

    mainLayout->addLayout(toolbar);
    mainLayout->addWidget(m_table);

    connect(m_btnAdd,    &QPushButton::clicked, this, &PaperPage::onAdd);
    connect(m_btnEdit,   &QPushButton::clicked, this, &PaperPage::onEdit);
    connect(m_btnDelete, &QPushButton::clicked, this, &PaperPage::onDelete);
    connect(m_btnOpenFile, &QPushButton::clicked, this, &PaperPage::onOpenFile);
    connect(m_btnSearch, &QPushButton::clicked, this, &PaperPage::onSearch);
    connect(m_btnShowAll, &QPushButton::clicked, this, &PaperPage::onShowAll);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &PaperPage::onSearch);

    refreshTable();
}

void PaperPage::populateTable(const std::vector<Paper> &papers)
{
    m_table->setRowCount(static_cast<int>(papers.size()));
    auto &mgr = LibraryManager::getInstance();
    for (size_t row = 0; row < papers.size(); ++row) {
        const Paper &p = papers[row];
        QTableWidgetItem *idItem = new QTableWidgetItem;
        idItem->setData(Qt::UserRole, p.getId());
        m_table->setItem(static_cast<int>(row), 0, idItem);
        m_table->setItem(static_cast<int>(row), 1,
            new QTableWidgetItem(QString::fromStdString(p.getCode())));
        m_table->setItem(static_cast<int>(row), 2,
            new QTableWidgetItem(QString::fromStdString(p.getTitle())));

        QString kws;
        for (size_t i = 0; i < p.getKeywords().size(); ++i) {
            if (i > 0) kws += "; ";
            kws += QString::fromStdString(p.getKeywords()[i]);
        }
        m_table->setItem(static_cast<int>(row), 3, new QTableWidgetItem(kws));
        m_table->setItem(static_cast<int>(row), 4,
            new QTableWidgetItem(QString::fromStdString(p.getPublishDate())));
        m_table->setItem(static_cast<int>(row), 5,
            new QTableWidgetItem(QString::fromStdString(mgr.getSourceName(p.getSourceId()))));
        m_table->setItem(static_cast<int>(row), 6,
            new QTableWidgetItem(QString::number(p.getAuthorIds().size())));
        m_table->setItem(static_cast<int>(row), 7,
            new QTableWidgetItem(QString::fromStdString(p.getIssue())));
        m_table->setItem(static_cast<int>(row), 8,
            new QTableWidgetItem(QString::fromStdString(p.getPageRange())));
        m_table->setItem(static_cast<int>(row), 9,
            new QTableWidgetItem(p.getFilePath().empty()
                ? QStringLiteral("未上传")
                : QStringLiteral("已上传")));
    }
}

void PaperPage::refreshTable()
{
    const auto &all = LibraryManager::getInstance().getAllPapers();
    std::vector<Paper> papers;
    for (const auto &pair : all)
        papers.push_back(pair.second);
    populateTable(papers);
}

void PaperPage::onAdd()
{
    PaperDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("新增文献"));
    if (dlg.exec() == QDialog::Accepted) {
        LibraryManager::getInstance().addPaper(dlg.getPaper());
        refreshTable();
    }
}

void PaperPage::onEdit()
{
    int row = m_table->currentRow();
    if (row < 0) {
        QMessageBox::information(this, QStringLiteral("提示"),
                                 QStringLiteral("请先选择一条记录。"));
        return;
    }
    IdType id = m_table->item(row, 0)->data(Qt::UserRole).toInt();
    Paper *p = LibraryManager::getInstance().findPaper(id);
    if (!p) return;

    PaperDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("编辑文献"));
    dlg.setPaper(*p);
    if (dlg.exec() == QDialog::Accepted) {
        LibraryManager::getInstance().updatePaper(id, dlg.getPaper());
        refreshTable();
    }
}

void PaperPage::onDelete()
{
    int row = m_table->currentRow();
    if (row < 0) {
        QMessageBox::information(this, QStringLiteral("提示"),
                                 QStringLiteral("请先选择一条记录。"));
        return;
    }
    IdType id = m_table->item(row, 0)->data(Qt::UserRole).toInt();
    auto result = QMessageBox::question(this, QStringLiteral("确认删除"),
        QStringLiteral("确定要删除该文献吗？（ID: %1）\n关联的附件也将被删除。").arg(id));
    if (result == QMessageBox::Yes) {
        LibraryManager::getInstance().removePaper(id);
        refreshTable();
    }
}

void PaperPage::onSearch()
{
    QString keyword = m_searchEdit->text().trimmed();
    if (keyword.isEmpty()) return;

    auto &mgr = LibraryManager::getInstance();
    std::vector<Paper> results;
    int mode = m_searchCombo->currentIndex();
    switch (mode) {
    case 0: results = mgr.searchPapersByTitle(keyword.toStdString()); break;
    case 1: results = mgr.getPapersByAuthorName(keyword.toStdString()); break;
    case 2: results = mgr.searchPapersByKeyword(keyword.toStdString()); break;
    }
    populateTable(results);
}

void PaperPage::onShowAll()
{
    m_searchEdit->clear();
    refreshTable();
}

void PaperPage::onOpenFile()
{
    int row = m_table->currentRow();
    if (row < 0) {
        QMessageBox::information(this, QStringLiteral("提示"),
                                 QStringLiteral("请先选择一条文献记录。"));
        return;
    }

    IdType id = m_table->item(row, 0)->data(Qt::UserRole).toInt();
    Paper *p = LibraryManager::getInstance().findPaper(id);
    if (!p) return;

    QString filePath = QString::fromStdString(p->getFilePath());
    if (filePath.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("提示"),
                                 QStringLiteral("该文献还没有上传全文文件。"));
        return;
    }

    QFileInfo info(filePath);
    if (!info.exists()) {
        QMessageBox::warning(this, QStringLiteral("文件不存在"),
                             QStringLiteral("找不到全文文件：\n%1").arg(filePath));
        return;
    }

    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(info.absoluteFilePath()))) {
        QMessageBox::warning(this, QStringLiteral("打开失败"),
                             QStringLiteral("系统无法打开该文件，请确认电脑上安装了对应阅读软件。"));
    }
}
