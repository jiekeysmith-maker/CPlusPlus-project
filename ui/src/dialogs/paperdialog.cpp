#include "paperdialog.h"

#include <QLineEdit>
#include <QTextEdit>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QTabWidget>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QDateTime>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QRegularExpression>

// ========== 作者选择器子对话框 ==========
class AuthorPickerDialog : public QDialog
{
public:
    std::vector<IdType> selectedIds;
    AuthorPickerDialog(QWidget *parent, const std::vector<IdType> &preSelected)
        : QDialog(parent)
    {
        setWindowTitle(QStringLiteral("选择作者"));
        setModal(true);
        setMinimumSize(400, 350);

        auto *layout = new QVBoxLayout(this);
        auto *list = new QListWidget;
        const auto &authors = LibraryManager::getInstance().getAllAuthors();
        for (const auto &pair : authors) {
            auto *item = new QListWidgetItem(
                QString("[%1] %2").arg(pair.first)
                    .arg(QString::fromStdString(pair.second.getName())));
            item->setData(Qt::UserRole, pair.first);
            item->setCheckState(
                std::find(preSelected.begin(), preSelected.end(), pair.first) != preSelected.end()
                ? Qt::Checked : Qt::Unchecked);
            list->addItem(item);
        }
        layout->addWidget(list);

        auto *btnLayout = new QHBoxLayout;
        auto *btnAll = new QPushButton(QStringLiteral("全选"));
        auto *btnNone = new QPushButton(QStringLiteral("取消全选"));
        btnLayout->addWidget(btnAll);
        btnLayout->addWidget(btnNone);
        btnLayout->addStretch();
        layout->addLayout(btnLayout);

        auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        buttonBox->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确定"));
        buttonBox->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
        connect(buttonBox, &QDialogButtonBox::accepted, this, [this, list]() {
            for (int i = 0; i < list->count(); ++i) {
                if (list->item(i)->checkState() == Qt::Checked)
                    selectedIds.push_back(list->item(i)->data(Qt::UserRole).toInt());
            }
            accept();
        });
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
        layout->addWidget(buttonBox);

        connect(btnAll, &QPushButton::clicked, this, [list]() {
            for (int i = 0; i < list->count(); ++i)
                list->item(i)->setCheckState(Qt::Checked);
        });
        connect(btnNone, &QPushButton::clicked, this, [list]() {
            for (int i = 0; i < list->count(); ++i)
                list->item(i)->setCheckState(Qt::Unchecked);
        });
    }
};

// ========== 出版物选择器子对话框 ==========
class SourcePickerDialog : public QDialog
{
public:
    IdType selectedId = INVALID_ID;
    SourcePickerDialog(QWidget *parent, IdType preSelected)
        : QDialog(parent)
    {
        setWindowTitle(QStringLiteral("选择出版物"));
        setModal(true);
        setMinimumSize(400, 300);

        auto *layout = new QVBoxLayout(this);
        auto *list = new QListWidget;
        const auto &sources = LibraryManager::getInstance().getAllSources();
        for (const auto &pair : sources) {
            auto *s = pair.second.get();
            auto *item = new QListWidgetItem(
                QString("[%1] %2 (%3)")
                    .arg(s->getId())
                    .arg(QString::fromStdString(s->getShortName()))
                    .arg(QString::fromStdString(s->getType())));
            item->setData(Qt::UserRole, s->getId());
            list->addItem(item);
            if (s->getId() == preSelected)
                list->setCurrentItem(item);
        }
        layout->addWidget(list);

        auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        buttonBox->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确定"));
        buttonBox->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
        connect(buttonBox, &QDialogButtonBox::accepted, this, [this, list]() {
            if (list->currentItem())
                selectedId = list->currentItem()->data(Qt::UserRole).toInt();
            accept();
        });
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
        layout->addWidget(buttonBox);
    }
};

// ========== 附件选择器子对话框 ==========
class AttachmentPickerDialog : public QDialog
{
public:
    std::vector<IdType> selectedIds;
    AttachmentPickerDialog(QWidget *parent, const std::vector<IdType> &preSelected)
        : QDialog(parent)
    {
        setWindowTitle(QStringLiteral("选择附件"));
        setModal(true);
        setMinimumSize(400, 300);

        auto *layout = new QVBoxLayout(this);
        auto *list = new QListWidget;
        const auto &attachments = LibraryManager::getInstance().getAllAttachments();
        for (const auto &pair : attachments) {
            auto *item = new QListWidgetItem(
                QString("[%1] %2").arg(pair.first)
                    .arg(QString::fromStdString(pair.second.getName())));
            item->setData(Qt::UserRole, pair.first);
            item->setCheckState(
                std::find(preSelected.begin(), preSelected.end(), pair.first) != preSelected.end()
                ? Qt::Checked : Qt::Unchecked);
            list->addItem(item);
        }
        layout->addWidget(list);

        auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        buttonBox->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确定"));
        buttonBox->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
        connect(buttonBox, &QDialogButtonBox::accepted, this, [this, list]() {
            for (int i = 0; i < list->count(); ++i) {
                if (list->item(i)->checkState() == Qt::Checked)
                    selectedIds.push_back(list->item(i)->data(Qt::UserRole).toInt());
            }
            accept();
        });
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
        layout->addWidget(buttonBox);
    }
};

// ========== PaperDialog ==========

PaperDialog::PaperDialog(QWidget *parent)
    : QDialog(parent), m_selectedSourceId(INVALID_ID)
{
    setupUi();
    setModal(true);
    setMinimumSize(600, 550);
}

void PaperDialog::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);

    auto *tabs = new QTabWidget;

    // ---- Tab 1: 基本信息 ----
    auto *tabBasic = new QWidget;
    auto *formBasic = new QFormLayout(tabBasic);
    m_codeEdit     = new QLineEdit;
    m_codeEdit->setPlaceholderText(QStringLiteral("请输入 DOI 编号"));
    m_titleEdit     = new QLineEdit;
    m_keywordsEdit  = new QLineEdit;
    m_keywordsEdit->setPlaceholderText(QStringLiteral("多个关键词用分号(;)分隔"));
    m_abstractEdit  = new QTextEdit;
    m_abstractEdit->setMaximumHeight(80);
    m_dateEdit      = new QLineEdit;
    m_dateEdit->setPlaceholderText(QStringLiteral("YYYY-MM-DD"));
    m_issueEdit     = new QLineEdit;
    m_issueNumEdit  = new QLineEdit;
    m_pageEdit      = new QLineEdit;
    m_filePathEdit  = new QLineEdit;
    m_filePathEdit->setReadOnly(true);
    m_btnSelectFile = new QPushButton(QStringLiteral("选择全文文件..."));
    m_remarkEdit    = new QLineEdit;

    formBasic->addRow(QStringLiteral("DOI编号:"), m_codeEdit);
    formBasic->addRow(QStringLiteral("标题:"), m_titleEdit);
    formBasic->addRow(QStringLiteral("关键词:"), m_keywordsEdit);
    formBasic->addRow(QStringLiteral("摘要:"), m_abstractEdit);
    formBasic->addRow(QStringLiteral("发表时间:"), m_dateEdit);
    formBasic->addRow(QStringLiteral("刊期:"), m_issueEdit);
    formBasic->addRow(QStringLiteral("刊号:"), m_issueNumEdit);
    formBasic->addRow(QStringLiteral("页码:"), m_pageEdit);
    auto *fileLayout = new QHBoxLayout;
    fileLayout->addWidget(m_filePathEdit, 1);
    fileLayout->addWidget(m_btnSelectFile);
    formBasic->addRow(QStringLiteral("全文文件:"), fileLayout);
    formBasic->addRow(QStringLiteral("备注:"), m_remarkEdit);
    tabs->addTab(tabBasic, QStringLiteral("基本信息"));

    // ---- Tab 2: 作者与出版物 ----
    auto *tabRel = new QWidget;
    auto *formRel = new QFormLayout(tabRel);

    m_authorList = new QListWidget;
    m_btnSelectAuthors = new QPushButton(QStringLiteral("选择作者..."));
    auto *authorLayout = new QVBoxLayout;
    authorLayout->addWidget(m_authorList);
    authorLayout->addWidget(m_btnSelectAuthors);
    formRel->addRow(QStringLiteral("作者:"), authorLayout);

    m_sourceLabel = new QLabel(QStringLiteral("（未选择）"));
    m_btnSelectSource = new QPushButton(QStringLiteral("选择出版物..."));
    auto *sourceLayout = new QHBoxLayout;
    sourceLayout->addWidget(m_sourceLabel, 1);
    sourceLayout->addWidget(m_btnSelectSource);
    formRel->addRow(QStringLiteral("出版物:"), sourceLayout);
    tabs->addTab(tabRel, QStringLiteral("作者与出版物"));

    // ---- Tab 3: 附件 ----
    auto *tabAtt = new QWidget;
    auto *formAtt = new QFormLayout(tabAtt);
    m_attachmentList = new QListWidget;
    m_btnSelectAttachments = new QPushButton(QStringLiteral("选择附件..."));
    auto *attLayout = new QVBoxLayout;
    attLayout->addWidget(m_attachmentList);
    attLayout->addWidget(m_btnSelectAttachments);
    formAtt->addRow(QStringLiteral("附件:"), attLayout);
    tabs->addTab(tabAtt, QStringLiteral("附件"));

    mainLayout->addWidget(tabs);

    auto *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttonBox->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确定"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    connect(m_btnSelectAuthors,    &QPushButton::clicked, this, &PaperDialog::onSelectAuthors);
    connect(m_btnSelectSource,     &QPushButton::clicked, this, &PaperDialog::onSelectSource);
    connect(m_btnSelectAttachments, &QPushButton::clicked, this, &PaperDialog::onSelectAttachments);
    connect(m_btnSelectFile,       &QPushButton::clicked, this, &PaperDialog::onSelectFile);
}

void PaperDialog::onSelectAuthors()
{
    AuthorPickerDialog dlg(this, m_selectedAuthorIds);
    if (dlg.exec() == QDialog::Accepted) {
        m_selectedAuthorIds = dlg.selectedIds;
        m_authorList->clear();
        auto &mgr = LibraryManager::getInstance();
        for (IdType id : m_selectedAuthorIds) {
            m_authorList->addItem(QString("[%1] %2")
                .arg(id)
                .arg(QString::fromStdString(mgr.getAuthorName(id))));
        }
    }
}

void PaperDialog::onSelectSource()
{
    SourcePickerDialog dlg(this, m_selectedSourceId);
    if (dlg.exec() == QDialog::Accepted) {
        m_selectedSourceId = dlg.selectedId;
        if (m_selectedSourceId != INVALID_ID) {
            m_sourceLabel->setText(QString("[%1] %2")
                .arg(m_selectedSourceId)
                .arg(QString::fromStdString(
                    LibraryManager::getInstance().getSourceName(m_selectedSourceId))));
        } else {
            m_sourceLabel->setText(QStringLiteral("（未选择）"));
        }
    }
}

void PaperDialog::onSelectAttachments()
{
    AttachmentPickerDialog dlg(this, m_selectedAttachmentIds);
    if (dlg.exec() == QDialog::Accepted) {
        m_selectedAttachmentIds = dlg.selectedIds;
        m_attachmentList->clear();
        auto &mgr = LibraryManager::getInstance();
        for (IdType id : m_selectedAttachmentIds) {
            Attachment *att = mgr.findAttachment(id);
            m_attachmentList->addItem(QString("[%1] %2")
                .arg(id)
                .arg(att ? QString::fromStdString(att->getName()) : QStringLiteral("未知")));
        }
    }
}

void PaperDialog::onSelectFile()
{
    QString sourcePath = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("选择论文PDF或全文文件"),
        QString(),
        QStringLiteral("论文 PDF (*.pdf);;全文文件 (*.pdf *.doc *.docx *.txt);;所有文件 (*)"));
    if (sourcePath.isEmpty()) {
        return;
    }

    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dataDir.isEmpty()) {
        dataDir = QCoreApplication::applicationDirPath() + QStringLiteral("/data");
    }
    QDir dir(dataDir);
    dir.mkpath(QStringLiteral("papers"));
    dir.cd(QStringLiteral("papers"));

    QFileInfo info(sourcePath);
    QString targetName = info.fileName();
    QString targetPath = dir.filePath(targetName);
    if (QFile::exists(targetPath)) {
        targetName = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMddhhmmsszzz_"))
            + info.fileName();
        targetPath = dir.filePath(targetName);
    }

    if (QFileInfo(sourcePath).absoluteFilePath() != QFileInfo(targetPath).absoluteFilePath()) {
        if (!QFile::copy(sourcePath, targetPath)) {
            QMessageBox::warning(this,
                QStringLiteral("上传失败"),
                QStringLiteral("无法复制该全文文件，请检查文件是否被占用或路径是否可写。"));
            return;
        }
    }

    m_filePathEdit->setText(QDir::toNativeSeparators(targetPath));

    if (info.suffix().compare(QStringLiteral("pdf"), Qt::CaseInsensitive) == 0) {
        const PdfMetadata metadata = extractPdfMetadata(sourcePath);
        if (metadata.isValid()) {
            applyPdfMetadata(metadata);
        } else {
            QMessageBox::information(
                this,
                QStringLiteral("提示"),
                QStringLiteral("已选择 PDF 文件，但未能自动提取到明显的文献信息。你仍然可以手动补充标题、作者和摘要。"));
        }
    }
}

void PaperDialog::accept()
{
    if (m_titleEdit->text().trimmed().isEmpty()) {
        m_titleEdit->setFocus();
        return;
    }
    QDialog::accept();
}

Paper PaperDialog::getPaper() const
{
    Paper p;
    p.setCode(m_codeEdit->text().toStdString());
    p.setTitle(m_titleEdit->text().toStdString());
    p.setPublishDate(m_dateEdit->text().toStdString());
    p.setIssue(m_issueEdit->text().toStdString());
    p.setIssueNumber(m_issueNumEdit->text().toStdString());
    p.setPageRange(m_pageEdit->text().toStdString());
    p.setRemark(m_remarkEdit->text().toStdString());
    p.setAbstract(m_abstractEdit->toPlainText().toStdString());
    p.setFilePath(QDir::fromNativeSeparators(m_filePathEdit->text()).toStdString());

    std::vector<std::string> kws;
    QString kwText = m_keywordsEdit->text().trimmed();
    if (!kwText.isEmpty()) {
        QStringList parts = kwText.split(';');
        for (const QString &part : parts) {
            QString trimmed = part.trimmed();
            if (!trimmed.isEmpty())
                kws.push_back(trimmed.toStdString());
        }
    }
    p.setKeywords(kws);

    p.setAuthorIds(m_selectedAuthorIds);
    p.setSourceId(m_selectedSourceId);
    p.setAttachmentIds(m_selectedAttachmentIds);
    return p;
}

void PaperDialog::setPaper(const Paper &p)
{
    m_codeEdit->setText(QString::fromStdString(p.getCode()));
    m_titleEdit->setText(QString::fromStdString(p.getTitle()));
    m_dateEdit->setText(QString::fromStdString(p.getPublishDate()));
    m_issueEdit->setText(QString::fromStdString(p.getIssue()));
    m_issueNumEdit->setText(QString::fromStdString(p.getIssueNumber()));
    m_pageEdit->setText(QString::fromStdString(p.getPageRange()));
    m_filePathEdit->setText(QDir::toNativeSeparators(QString::fromStdString(p.getFilePath())));
    m_remarkEdit->setText(QString::fromStdString(p.getRemark()));
    m_abstractEdit->setText(QString::fromStdString(p.getAbstract()));

    QString kws;
    for (size_t i = 0; i < p.getKeywords().size(); ++i) {
        if (i > 0) kws += ";";
        kws += QString::fromStdString(p.getKeywords()[i]);
    }
    m_keywordsEdit->setText(kws);

    m_selectedAuthorIds = p.getAuthorIds();
    m_authorList->clear();
    auto &mgr = LibraryManager::getInstance();
    for (IdType id : m_selectedAuthorIds) {
        m_authorList->addItem(QString("[%1] %2")
            .arg(id)
            .arg(QString::fromStdString(mgr.getAuthorName(id))));
    }

    m_selectedSourceId = p.getSourceId();
    if (m_selectedSourceId != INVALID_ID) {
        m_sourceLabel->setText(QString("[%1] %2")
            .arg(m_selectedSourceId)
            .arg(QString::fromStdString(mgr.getSourceName(m_selectedSourceId))));
    } else {
        m_sourceLabel->setText(QStringLiteral("（未选择）"));
    }

    m_selectedAttachmentIds = p.getAttachmentIds();
    m_attachmentList->clear();
    for (IdType id : m_selectedAttachmentIds) {
        Attachment *att = mgr.findAttachment(id);
        m_attachmentList->addItem(QString("[%1] %2")
            .arg(id)
            .arg(att ? QString::fromStdString(att->getName()) : QStringLiteral("未知")));
    }
}

QString PaperDialog::decodePdfLiteralString(const QString &value)
{
    QString out;
    out.reserve(value.size());
    for (int i = 0; i < value.size(); ++i) {
        const QChar ch = value.at(i);
        if (ch != '\\') {
            out.append(ch);
            continue;
        }
        if (i + 1 >= value.size()) {
            break;
        }
        const QChar next = value.at(++i);
        switch (next.unicode()) {
        case 'n': out.append('\n'); break;
        case 'r': out.append('\r'); break;
        case 't': out.append('\t'); break;
        case 'b': out.append('\b'); break;
        case 'f': out.append('\f'); break;
        case '(':
        case ')':
        case '\\': out.append(next); break;
        default: out.append(next); break;
        }
    }
    return out;
}

QString PaperDialog::normalizePdfDate(const QString &value)
{
    QString text = value.trimmed();
    if (text.startsWith(QStringLiteral("D:"))) {
        text.remove(0, 2);
    }
    if (text.size() >= 8) {
        return QStringLiteral("%1-%2-%3")
            .arg(text.left(4))
            .arg(text.mid(4, 2))
            .arg(text.mid(6, 2));
    }
    return text;
}

QString PaperDialog::firstNonEmpty(const QStringList &values)
{
    for (const QString &value : values) {
        if (!value.trimmed().isEmpty()) {
            return value.trimmed();
        }
    }
    return QString();
}

PdfMetadata PaperDialog::extractPdfMetadata(const QString &filePath) const
{
    PdfMetadata meta;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return meta;
    }

    const QByteArray data = file.readAll();
    const QString text = QString::fromLatin1(data);

    auto takeMatch = [&](const QRegularExpression &re) -> QString {
        QRegularExpressionMatch match = re.match(text);
        return match.hasMatch() ? match.captured(1).trimmed() : QString();
    };

    meta.title = firstNonEmpty({
        takeMatch(QRegularExpression(QStringLiteral(R"(/Title\s*\((.*?)\))"), QRegularExpression::DotMatchesEverythingOption)),
        takeMatch(QRegularExpression(QStringLiteral(R"(/Title\s*<([^>]+)>)"), QRegularExpression::DotMatchesEverythingOption)),
    });

    meta.author = firstNonEmpty({
        takeMatch(QRegularExpression(QStringLiteral(R"(/Author\s*\((.*?)\))"), QRegularExpression::DotMatchesEverythingOption)),
        takeMatch(QRegularExpression(QStringLiteral(R"(/Author\s*<([^>]+)>)"), QRegularExpression::DotMatchesEverythingOption)),
    });

    meta.subject = firstNonEmpty({
        takeMatch(QRegularExpression(QStringLiteral(R"(/Subject\s*\((.*?)\))"), QRegularExpression::DotMatchesEverythingOption)),
        takeMatch(QRegularExpression(QStringLiteral(R"(/Subject\s*<([^>]+)>)"), QRegularExpression::DotMatchesEverythingOption)),
    });

    meta.keywords = firstNonEmpty({
        takeMatch(QRegularExpression(QStringLiteral(R"(/Keywords\s*\((.*?)\))"), QRegularExpression::DotMatchesEverythingOption)),
        takeMatch(QRegularExpression(QStringLiteral(R"(/Keywords\s*<([^>]+)>)"), QRegularExpression::DotMatchesEverythingOption)),
    });

    meta.publicationDate = normalizePdfDate(firstNonEmpty({
        takeMatch(QRegularExpression(QStringLiteral(R"(/CreationDate\s*\((.*?)\))"), QRegularExpression::DotMatchesEverythingOption)),
        takeMatch(QRegularExpression(QStringLiteral(R"(/ModDate\s*\((.*?)\))"), QRegularExpression::DotMatchesEverythingOption)),
    }));

    const QRegularExpression pagesRe(QStringLiteral(R"(/Count\s+(\d+))"));
    const QRegularExpressionMatch pagesMatch = pagesRe.match(text);
    if (pagesMatch.hasMatch()) {
        meta.pageRange = QStringLiteral("1-%1").arg(pagesMatch.captured(1));
    }

    const QString cleaned = text.simplified();
    const int titleIndex = cleaned.indexOf(meta.title, 0, Qt::CaseInsensitive);
    if (!meta.title.isEmpty() && titleIndex >= 0) {
        const int end = qMin(cleaned.size(), titleIndex + 1200);
        meta.rawText = cleaned.mid(titleIndex, end - titleIndex);
        meta.abstract = cleaned.mid(titleIndex, qMin(600, end - titleIndex));
    } else {
        meta.rawText = cleaned.left(5000);
        meta.abstract = cleaned.left(400);
    }

    return meta;
}

void PaperDialog::applyPdfMetadata(const PdfMetadata &metadata)
{
    if (!metadata.title.isEmpty() && m_titleEdit->text().trimmed().isEmpty()) {
        m_titleEdit->setText(metadata.title);
    }

    if (!metadata.author.isEmpty() && m_selectedAuthorIds.empty()) {
        m_authorList->clear();
        const QStringList authors = metadata.author.split(QRegularExpression(QStringLiteral(R"([;,/、]+)")), Qt::SkipEmptyParts);
        auto &mgr = LibraryManager::getInstance();
        for (const QString &authorName : authors) {
            QString trimmed = authorName.trimmed();
            if (trimmed.isEmpty()) {
                continue;
            }
            m_authorList->addItem(trimmed);
            const auto &allAuthors = mgr.getAllAuthors();
            for (const auto &pair : allAuthors) {
                if (QString::fromStdString(pair.second.getName()).compare(trimmed, Qt::CaseInsensitive) == 0) {
                    m_selectedAuthorIds.push_back(pair.first);
                    break;
                }
            }
        }
    }

    if (!metadata.publicationDate.isEmpty() && m_dateEdit->text().trimmed().isEmpty()) {
        m_dateEdit->setText(metadata.publicationDate);
    }

    if (!metadata.keywords.isEmpty() && m_keywordsEdit->text().trimmed().isEmpty()) {
        QString keywords = metadata.keywords;
        keywords.replace(',', ';');
        m_keywordsEdit->setText(keywords);
    }

    if (!metadata.abstract.isEmpty() && m_abstractEdit->toPlainText().trimmed().isEmpty()) {
        m_abstractEdit->setPlainText(metadata.abstract);
    }

    if (!metadata.pageRange.isEmpty() && m_pageEdit->text().trimmed().isEmpty()) {
        m_pageEdit->setText(metadata.pageRange);
    }

    if (!metadata.subject.isEmpty() && m_remarkEdit->text().trimmed().isEmpty()) {
        m_remarkEdit->setText(metadata.subject);
    }
}
