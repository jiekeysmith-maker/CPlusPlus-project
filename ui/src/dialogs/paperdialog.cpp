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
#include <QStandardPaths>
#include <QCoreApplication>
#include <QProcess>
#include <QRegularExpression>
#include <QSet>
#include <QVector>

#include <algorithm>
#include <cstring>

#include "StoragePaths.h"

#if __has_include(<zlib.h>)
#include <zlib.h>
#define PAPERDIALOG_HAS_ZLIB 1
#else
#define PAPERDIALOG_HAS_ZLIB 0
#endif

namespace {
bool isCjkCharacter(const QChar &ch)
{
    const uint u = ch.unicode();
    return (u >= 0x4E00 && u <= 0x9FFF)
        || (u >= 0x3400 && u <= 0x4DBF)
        || (u >= 0xF900 && u <= 0xFAFF);
}

bool isControlCharacter(const QChar &ch)
{
    const ushort u = ch.unicode();
    return u < 0x20 || (u >= 0x7f && u <= 0x9f);
}

bool looksReadablePdfText(const QString &value)
{
    const QString text = value.simplified();
    if (text.size() < 2 || text.contains(QChar(0xfffd))) {
        return false;
    }

    int useful = 0;
    int bad = 0;
    int cjk = 0;
    int ascii = 0;
    int highLatin = 0;
    for (const QChar &ch : text) {
        const ushort u = ch.unicode();
        if (ch.isLetterOrNumber() || ch.isSpace() || QStringLiteral(".,;:!?()[]{}<>/-_+=*&%#@'\"").contains(ch)) {
            ++useful;
            if (u < 128 && ch.isLetterOrNumber()) {
                ++ascii;
            } else if ((u >= 0x00C0 && u <= 0x024F) && !isCjkCharacter(ch)) {
                ++highLatin;
            }
            if (isCjkCharacter(ch)) {
                ++cjk;
            }
        } else if (isControlCharacter(ch)) {
            ++bad;
        } else {
            ++useful;
        }
    }

    if (bad > 0) {
        return false;
    }
    if (highLatin > text.size() / 3 && cjk == 0 && ascii < 4) {
        return false;
    }
    return useful >= std::max(2, static_cast<int>(text.size() * 55 / 100));
}

bool looksLikeAffiliationOrHeader(const QString &value)
{
    const QString lower = value.simplified().toLower();
    static const QStringList noiseWords = {
        QStringLiteral("@"),
        QStringLiteral("http"),
        QStringLiteral("www."),
        QStringLiteral("doi"),
        QStringLiteral("arxiv"),
        QStringLiteral("university"),
        QStringLiteral("institute"),
        QStringLiteral("college"),
        QStringLiteral("department"),
        QStringLiteral("school of"),
        QStringLiteral("laboratory"),
        QStringLiteral("academy"),
        QStringLiteral("faculty"),
        QStringLiteral("school"),
        QStringLiteral("centre"),
        QStringLiteral("center"),
        QStringLiteral("inc."),
        QStringLiteral("corp"),
        QStringLiteral("ltd"),
        QStringLiteral("llc"),
        QStringLiteral("foundation"),
        QStringLiteral("association"),
        QStringLiteral("society"),
        QStringLiteral("proceedings"),
        QStringLiteral("journal"),
        QStringLiteral("conference"),
        QStringLiteral("copyright"),
        QStringLiteral("accepted"),
        QStringLiteral("submitted")
    };
    for (const QString &word : noiseWords) {
        if (lower.contains(word)) {
            return true;
        }
    }
    return false;
}

bool looksLikeSectionHeading(const QString &value)
{
    const QString lower = value.simplified().toLower();
    if (lower == QStringLiteral("abstract")
        || lower == QStringLiteral("keywords")
        || lower == QStringLiteral("index terms")
        || lower == QStringLiteral("references")) {
        return true;
    }

    const QRegularExpression sectionRe(
        QStringLiteral(R"(^(\d+(\.\d+)*|[ivxlcdm]+)\.?\s+(abstract|introduction|background|related work|references|conclusion)\b)"),
        QRegularExpression::CaseInsensitiveOption);
    return sectionRe.match(lower).hasMatch();
}

bool looksLikeStandalonePageMarker(const QString &value)
{
    const QString text = value.simplified();
    if (text.size() <= 4 && text.contains(QRegularExpression(QStringLiteral(R"(^-?\d+-?$)")))) {
        return true;
    }
    return text.contains(QRegularExpression(QStringLiteral(R"(^page\s+\d+(\s+of\s+\d+)?$)"),
        QRegularExpression::CaseInsensitiveOption));
}
}

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
    m_uploadTimeEdit = new QLineEdit;
    m_uploadTimeEdit->setPlaceholderText(QStringLiteral("自动生成，格式 yyyy-MM-dd HH:mm:ss"));
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
    formBasic->addRow(QStringLiteral("上传时间:"), m_uploadTimeEdit);
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

    QDir dir(StoragePaths::pdfDirectoryPath());
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }

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
    if (m_uploadTimeEdit->text().trimmed().isEmpty()) {
        m_uploadTimeEdit->setText(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")));
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
    p.setUploadTime(m_uploadTimeEdit->text().trimmed().toStdString());

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
    m_uploadTimeEdit->setText(QString::fromStdString(p.getUploadTime()));
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
    return decodePdfBytes(decodePdfLiteralBytes(value)).simplified();
}

QByteArray PaperDialog::decodePdfLiteralBytes(const QString &value)
{
    QByteArray out;
    out.reserve(value.size());
    for (int i = 0; i < value.size(); ++i) {
        const QChar ch = value.at(i);
        if (ch != '\\') {
            out.append(static_cast<char>(ch.unicode() & 0xff));
            continue;
        }
        if (i + 1 >= value.size()) {
            break;
        }
        const QChar next = value.at(++i);
        if (next == '\r' || next == '\n') {
            if (next == '\r' && i + 1 < value.size() && value.at(i + 1) == '\n') {
                ++i;
            }
            continue;
        }
        if (next >= QLatin1Char('0') && next <= QLatin1Char('7')) {
            int code = next.unicode() - '0';
            int digits = 1;
            while (digits < 3 && i + 1 < value.size()) {
                const QChar oct = value.at(i + 1);
                if (oct < QLatin1Char('0') || oct > QLatin1Char('7')) {
                    break;
                }
                code = code * 8 + (oct.unicode() - '0');
                ++i;
                ++digits;
            }
            out.append(static_cast<char>(code & 0xff));
            continue;
        }
        switch (next.unicode()) {
        case 'n': out.append('\n'); break;
        case 'r': out.append('\r'); break;
        case 't': out.append('\t'); break;
        case 'b': out.append('\b'); break;
        case 'f': out.append('\f'); break;
        case '(':
        case ')':
        case '\\': out.append(static_cast<char>(next.unicode() & 0xff)); break;
        default: out.append(static_cast<char>(next.unicode() & 0xff)); break;
        }
    }
    return out;
}

QString PaperDialog::decodePdfHexString(const QString &value)
{
    QString hex = value.trimmed();
    if (hex.startsWith('<')) hex.remove(0, 1);
    if (hex.endsWith('>')) hex.chop(1);
    hex.remove(QRegularExpression(QStringLiteral(R"(\s+)")));
    if (hex.size() % 2 == 1) {
        hex.append(QLatin1Char('0'));
    }

    QByteArray bytes;
    bytes.reserve(hex.size() / 2);
    for (int i = 0; i + 1 < hex.size(); i += 2) {
        bool ok = false;
        const char ch = static_cast<char>(hex.mid(i, 2).toUInt(&ok, 16));
        if (ok) {
            bytes.append(ch);
        }
    }

    return decodePdfBytes(bytes).simplified();
}

QString PaperDialog::decodePdfBytes(const QByteArray &bytes)
{
    if (bytes.isEmpty()) {
        return QString();
    }

    auto decodeUtf16 = [](const QByteArray &data, int start, bool bigEndian) {
        QString out;
        for (int i = start; i + 1 < data.size(); i += 2) {
            const ushort hi = static_cast<unsigned char>(data[i]);
            const ushort lo = static_cast<unsigned char>(data[i + 1]);
            const ushort code = bigEndian ? static_cast<ushort>((hi << 8) | lo)
                                          : static_cast<ushort>((lo << 8) | hi);
            if (code != 0) {
                out.append(QChar(code));
            }
        }
        return out;
    };

    QString decoded;
    if (bytes.size() >= 2
        && static_cast<unsigned char>(bytes[0]) == 0xFE
        && static_cast<unsigned char>(bytes[1]) == 0xFF) {
        decoded = decodeUtf16(bytes, 2, true);
    } else if (bytes.size() >= 2
        && static_cast<unsigned char>(bytes[0]) == 0xFF
        && static_cast<unsigned char>(bytes[1]) == 0xFE) {
        decoded = decodeUtf16(bytes, 2, false);
    } else if (bytes.size() >= 6 && bytes.size() % 2 == 0) {
        int zeroEven = 0;
        int zeroOdd = 0;
        for (int i = 0; i < bytes.size(); ++i) {
            if (bytes.at(i) == '\0') {
                (i % 2 == 0 ? zeroEven : zeroOdd)++;
            }
        }
        const int pairs = bytes.size() / 2;
        if (zeroEven > pairs / 2) {
            decoded = decodeUtf16(bytes, 0, true);
        } else if (zeroOdd > pairs / 2) {
            decoded = decodeUtf16(bytes, 0, false);
        }
    }

    if (decoded.isEmpty()) {
        decoded = QString::fromUtf8(bytes.constData(), bytes.size());
        if (decoded.contains(QChar(0xfffd))) {
            decoded = QString::fromLatin1(bytes.constData(), bytes.size());
        }
    }

    QString clean;
    clean.reserve(decoded.size());
    for (const QChar &ch : decoded) {
        if (ch == QChar(0) || ch == QChar(0xfffd)) {
            continue;
        }
        if (isControlCharacter(ch) && ch != '\n' && ch != '\r' && ch != '\t') {
            continue;
        }
        clean.append(ch);
    }
    return clean.simplified();
}

QString PaperDialog::decodePdfValue(const QString &value)
{
    QString text = value.trimmed();
    if (text.startsWith('(') && text.endsWith(')')) {
        text = text.mid(1, text.size() - 2);
        return decodePdfLiteralString(text).simplified();
    }
    if (text.startsWith('<') && text.endsWith('>')) {
        return decodePdfHexString(text).simplified();
    }
    if (text.contains(QRegularExpression(QStringLiteral(R"(^[0-9A-Fa-f\s]{4,}$)")))) {
        return decodePdfHexString(text).simplified();
    }
    return text.simplified();
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

static QString findPdfTool(const QString &name)
{
    // 1. System PATH
    const QString sysPath = QStandardPaths::findExecutable(name);
    if (!sysPath.isEmpty()) return sysPath;

    // 2. Next to the executable
    const QString exeDir = QCoreApplication::applicationDirPath();
    const QString exePath = QDir(exeDir).filePath(name + QStringLiteral(".exe"));
    if (QFile::exists(exePath)) return QDir::toNativeSeparators(exePath);

    // 3. tools/ subdirectory next to exe
    const QString toolsPath = QDir(exeDir).filePath(QStringLiteral("tools/") + name + QStringLiteral(".exe"));
    if (QFile::exists(toolsPath)) return QDir::toNativeSeparators(toolsPath);

    return QString();
}

QString PaperDialog::extractPdfTextWithExternalTool(const QString &filePath)
{
    struct ToolRun {
        QString program;
        QStringList arguments;
    };

    const QString pdftotext = findPdfTool(QStringLiteral("pdftotext"));
    const QString mutool = findPdfTool(QStringLiteral("mutool"));
    QVector<ToolRun> tools;
    if (!pdftotext.isEmpty()) {
        tools.push_back({
            pdftotext,
            {
                QStringLiteral("-layout"),
                QStringLiteral("-enc"),
                QStringLiteral("UTF-8"),
                QStringLiteral("-f"),
                QStringLiteral("1"),
                QStringLiteral("-l"),
                QStringLiteral("4"),
                filePath,
                QStringLiteral("-")
            }
        });
    }
    if (!mutool.isEmpty()) {
        tools.push_back({
            mutool,
            {
                QStringLiteral("draw"),
                QStringLiteral("-F"),
                QStringLiteral("txt"),
                QStringLiteral("-o"),
                QStringLiteral("-"),
                filePath,
                QStringLiteral("1-4")
            }
        });
    }

    for (const ToolRun &tool : tools) {
        QProcess process;
        process.start(tool.program, tool.arguments);
        if (!process.waitForFinished(2500)) {
            process.kill();
            process.waitForFinished(1000);
            continue;
        }
        if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
            continue;
        }
        const QString normalized = normalizeExtractedPdfText(QString::fromUtf8(process.readAllStandardOutput()));
        if (normalized.size() > 40) {
            return normalized;
        }
    }

    return QString();
}

QByteArray PaperDialog::inflatePdfStream(const QByteArray &stream)
{
#if PAPERDIALOG_HAS_ZLIB
    auto inflateWithWindowBits = [](const QByteArray &input, int windowBits) {
        QByteArray output;
        z_stream zstream;
        std::memset(&zstream, 0, sizeof(zstream));
        if (inflateInit2(&zstream, windowBits) != Z_OK) {
            return output;
        }

        zstream.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(input.constData()));
        zstream.avail_in = static_cast<uInt>(input.size());

        char buffer[32768];
        int result = Z_OK;
        while (result == Z_OK) {
            zstream.next_out = reinterpret_cast<Bytef *>(buffer);
            zstream.avail_out = sizeof(buffer);
            result = inflate(&zstream, Z_NO_FLUSH);
            if (result == Z_OK || result == Z_STREAM_END) {
                output.append(buffer, sizeof(buffer) - zstream.avail_out);
            }
        }

        inflateEnd(&zstream);
        if (result != Z_STREAM_END) {
            output.clear();
        }
        return output;
    };

    QByteArray output = inflateWithWindowBits(stream, MAX_WBITS);
    if (output.isEmpty()) {
        output = inflateWithWindowBits(stream, -MAX_WBITS);
    }
    return output;
#else
    Q_UNUSED(stream);
    return QByteArray();
#endif
}

QString PaperDialog::extractPdfTextFromContentStream(const QByteArray &stream)
{
    const QString content = QString::fromLatin1(stream.constData(), stream.size());
    QString result;
    QStringList operands;

    auto isDelimiter = [](const QChar &ch) {
        return ch.isSpace() || QStringLiteral("[]<>()/{}%").contains(ch);
    };

    auto appendText = [&](QString text, bool newLineBefore = false) {
        text = text.simplified();
        const bool singleUsefulChar = text.size() == 1
            && (text.at(0).isLetterOrNumber() || isCjkCharacter(text.at(0)));
        if (!singleUsefulChar && !looksReadablePdfText(text)) {
            return;
        }
        if (newLineBefore && !result.isEmpty() && !result.endsWith('\n')) {
            result.append('\n');
        } else if (singleUsefulChar && !result.isEmpty() && !result.endsWith('\n')) {
            result.append(text);
            return;
        } else if (!result.isEmpty() && !result.endsWith('\n')) {
            result.append(' ');
        }
        result.append(text);
    };

    auto parseLiteral = [&](int &i) {
        QString literal;
        int depth = 1;
        ++i;
        while (i < content.size() && depth > 0) {
            const QChar ch = content.at(i);
            if (ch == '\\') {
                literal.append(ch);
                if (i + 1 < content.size()) {
                    literal.append(content.at(++i));
                }
                ++i;
                continue;
            }
            if (ch == '(') {
                ++depth;
                literal.append(ch);
                ++i;
                continue;
            }
            if (ch == ')') {
                --depth;
                if (depth == 0) {
                    ++i;
                    break;
                }
                literal.append(ch);
                ++i;
                continue;
            }
            literal.append(ch);
            ++i;
        }
        return decodePdfLiteralString(literal);
    };

    auto parseHex = [&](int &i) {
        ++i;
        QString hex;
        while (i < content.size() && content.at(i) != '>') {
            hex.append(content.at(i));
            ++i;
        }
        if (i < content.size() && content.at(i) == '>') {
            ++i;
        }
        return decodePdfHexString(hex);
    };

    auto parseArray = [&](int &i) {
        QString joined;
        ++i;
        while (i < content.size() && content.at(i) != ']') {
            const QChar ch = content.at(i);
            if (ch.isSpace()) {
                ++i;
                continue;
            }
            if (ch == '(') {
                joined.append(parseLiteral(i));
                continue;
            }
            if (ch == '<' && i + 1 < content.size() && content.at(i + 1) != '<') {
                joined.append(parseHex(i));
                continue;
            }
            QString token;
            while (i < content.size()
                && !content.at(i).isSpace()
                && content.at(i) != ']'
                && content.at(i) != '('
                && content.at(i) != '<') {
                token.append(content.at(i));
                ++i;
            }
            bool ok = false;
            const double spacing = token.toDouble(&ok);
            if (ok && spacing < -120 && !joined.endsWith(' ')) {
                joined.append(' ');
            } else if (token.isEmpty()) {
                ++i;
            }
        }
        if (i < content.size() && content.at(i) == ']') {
            ++i;
        }
        return joined.simplified();
    };

    for (int i = 0; i < content.size();) {
        const QChar ch = content.at(i);
        if (ch.isSpace()) {
            ++i;
            continue;
        }
        if (ch == '%') {
            while (i < content.size() && content.at(i) != '\n' && content.at(i) != '\r') {
                ++i;
            }
            continue;
        }
        if (ch == '(') {
            operands << parseLiteral(i);
            continue;
        }
        if (ch == '<' && i + 1 < content.size() && content.at(i + 1) != '<') {
            operands << parseHex(i);
            continue;
        }
        if (ch == '<' && i + 1 < content.size() && content.at(i + 1) == '<') {
            i += 2;
            operands.clear();
            continue;
        }
        if (ch == '>' && i + 1 < content.size() && content.at(i + 1) == '>') {
            i += 2;
            operands.clear();
            continue;
        }
        if (ch == '[') {
            operands << parseArray(i);
            continue;
        }
        if (ch == '\'' || ch == '"') {
            if (!operands.isEmpty()) {
                appendText(operands.last(), true);
            }
            operands.clear();
            ++i;
            continue;
        }
        if (ch == '/') {
            while (i < content.size() && !content.at(i).isSpace()) {
                ++i;
            }
            continue;
        }
        if (isDelimiter(ch)) {
            ++i;
            continue;
        }

        QString token;
        while (i < content.size() && !isDelimiter(content.at(i))) {
            token.append(content.at(i));
            ++i;
        }

        if (token == QStringLiteral("BI")) {
            const int imageEnd = content.indexOf(QStringLiteral("\nEI"), i);
            if (imageEnd >= 0) {
                i = imageEnd + 3;
            }
            operands.clear();
        } else if (token == QStringLiteral("Tj") || token == QStringLiteral("TJ")) {
            if (!operands.isEmpty()) {
                appendText(operands.last());
            }
            operands.clear();
        } else if (token == QStringLiteral("T*")
            || token == QStringLiteral("Td")
            || token == QStringLiteral("TD")
            || token == QStringLiteral("Tm")
            || token == QStringLiteral("ET")) {
            if (!result.isEmpty() && !result.endsWith('\n')) {
                result.append('\n');
            }
            operands.clear();
        } else if (!token.isEmpty() && token.at(0).isLetter()) {
            operands.clear();
        }
    }

    return normalizeExtractedPdfText(result);
}

QString PaperDialog::extractPdfTextFromPdfData(const QByteArray &data)
{
    QStringList pieces;
    int extractedChars = 0;
    int scannedStreams = 0;
    int searchPos = 0;
    while (searchPos < data.size()) {
        const int streamPos = data.indexOf("stream", searchPos);
        if (streamPos < 0) {
            break;
        }
        int contentStart = streamPos + 6;
        if (contentStart < data.size() && data.at(contentStart) == '\r') {
            ++contentStart;
        }
        if (contentStart < data.size() && data.at(contentStart) == '\n') {
            ++contentStart;
        }
        const int endPos = data.indexOf("endstream", contentStart);
        if (endPos < 0) {
            break;
        }

        const int dictStart = data.lastIndexOf("<<", streamPos);
        const QByteArray dict = dictStart >= 0 ? data.mid(dictStart, streamPos - dictStart) : QByteArray();
        QByteArray streamData = data.mid(contentStart, endPos - contentStart);
        while (!streamData.isEmpty() && (streamData.endsWith('\n') || streamData.endsWith('\r'))) {
            streamData.chop(1);
        }
        ++scannedStreams;

        const bool imageStream = dict.contains("/Subtype") && dict.contains("/Image");
        const bool fontStream = dict.contains("/FontFile") || dict.contains("/CIDSet")
            || dict.contains("/ToUnicode") || dict.contains("/ICCProfile");
        if (imageStream || fontStream || streamData.size() > 1024 * 1024) {
            searchPos = endPos + 9;
            continue;
        }

        QByteArray decoded = streamData;
        if (dict.contains("/FlateDecode") || dict.contains("/Fl")) {
            const QByteArray inflated = inflatePdfStream(streamData);
            if (!inflated.isEmpty()) {
                decoded = inflated;
            }
        }
        if (decoded.size() > 2 * 1024 * 1024) {
            searchPos = endPos + 9;
            continue;
        }

        if (decoded.contains("BT") && (decoded.contains("Tj") || decoded.contains("TJ"))) {
            const QString text = extractPdfTextFromContentStream(decoded);
            if (!text.isEmpty()) {
                pieces << text;
                extractedChars += text.size();
            }
        }

        if (extractedChars > 50000
            || (extractedChars > 2500 && pieces.join(QStringLiteral("\n")).contains(QRegularExpression(QStringLiteral(R"(\b(abstract|摘要)\b)"), QRegularExpression::CaseInsensitiveOption)))
            || scannedStreams >= 300) {
            break;
        }
        searchPos = endPos + 9;
    }

    if (pieces.isEmpty()) {
        const QString raw = QString::fromLatin1(data.constData(), data.size());
        QRegularExpression literalTextRe(QStringLiteral(R"(\(((?:\\.|[^\\()]){3,240})\)\s*(?:Tj|'|"|TJ))"),
            QRegularExpression::DotMatchesEverythingOption);
        auto literalIt = literalTextRe.globalMatch(raw);
        while (literalIt.hasNext() && pieces.size() < 120) {
            const QRegularExpressionMatch match = literalIt.next();
            const QString decoded = decodePdfLiteralString(match.captured(1));
            if (looksReadablePdfText(decoded)) {
                pieces << decoded;
            }
        }

        QRegularExpression hexTextRe(QStringLiteral(R"(<([0-9A-Fa-f\s]{6,480})>\s*(?:Tj|'|"|TJ))"),
            QRegularExpression::DotMatchesEverythingOption);
        auto hexIt = hexTextRe.globalMatch(raw);
        while (hexIt.hasNext() && pieces.size() < 180) {
            const QRegularExpressionMatch match = hexIt.next();
            const QString decoded = decodePdfHexString(match.captured(1));
            if (looksReadablePdfText(decoded)) {
                pieces << decoded;
            }
        }
    }

    return normalizeExtractedPdfText(pieces.join(QStringLiteral("\n")));
}

QString PaperDialog::normalizeExtractedPdfText(const QString &text)
{
    QString normalized = text;
    normalized.replace('\r', '\n');
    normalized.replace('\f', '\n');
    normalized.replace(QChar(0x00a0), QLatin1Char(' '));

    QStringList lines;
    for (QString line : normalized.split('\n')) {
        line = line.simplified();
        if (line.isEmpty() || looksLikeStandalonePageMarker(line)) {
            continue;
        }
        if (!looksReadablePdfText(line)) {
            continue;
        }
        lines << line;
    }
    return lines.join(QStringLiteral("\n")).trimmed();
}

QString PaperDialog::extractDoiFromPdfText(const QString &text)
{
    QRegularExpression doiRe(QStringLiteral(R"(\b10\.\d{4,9}/[-._;()/:A-Z0-9]+\b)"),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = doiRe.match(text);
    return match.hasMatch() ? match.captured(0).trimmed() : QString();
}

QString PaperDialog::extractArxivIdFromPdfText(const QString &text)
{
    QRegularExpression arxivRe(QStringLiteral(R"(\barXiv\s*:\s*([0-9]{4}\.[0-9]{4,5}(?:v\d+)?|[a-z.-]+/[0-9]{7}(?:v\d+)?))"),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = arxivRe.match(text);
    return match.hasMatch() ? QStringLiteral("arXiv:%1").arg(match.captured(1).trimmed()) : QString();
}

QString PaperDialog::extractVenueFromPdfText(const QString &text)
{
    const QString normalized = normalizeExtractedPdfText(text);
    const QStringList lines = normalized.split('\n', Qt::SkipEmptyParts);
    for (QString line : lines) {
        line = line.simplified();
        if (line.size() < 8 || line.size() > 180) {
            continue;
        }
        const QString lower = line.toLower();
        if (lower.contains(QStringLiteral("conference on"))
            || lower.contains(QStringLiteral("proceedings of"))
            || lower.contains(QStringLiteral("journal of"))
            || lower.contains(QStringLiteral("transactions on"))
            || lower.contains(QStringLiteral("neurips"))
            || lower.contains(QStringLiteral("icml"))
            || lower.contains(QStringLiteral("iclr"))
            || lower.contains(QStringLiteral("cvpr"))
            || lower.contains(QStringLiteral("eccv"))
            || lower.contains(QStringLiteral("aaai"))
            || lower.contains(QStringLiteral("ijcai"))) {
            line.remove(QRegularExpression(QStringLiteral(R"(\.$)")));
            return line;
        }
    }

    const QString arxiv = extractArxivIdFromPdfText(normalized);
    return arxiv.isEmpty() ? QString() : QStringLiteral("arXiv");
}

QString PaperDialog::extractIssueNumberFromPdfText(const QString &text)
{
    const QString normalized = normalizeExtractedPdfText(text);
    const QString arxiv = extractArxivIdFromPdfText(normalized);
    if (!arxiv.isEmpty()) {
        return arxiv;
    }

    const QStringList lines = normalized.split('\n', Qt::SkipEmptyParts);
    QRegularExpression parenthesizedAcronymRe(QStringLiteral(R"(\(([A-Z][A-Za-z0-9-]{1,12}(?:\s*[- ]\s*\d{2,4})?)\))"));
    for (const QString &line : lines) {
        const QString lower = line.toLower();
        const bool venueLine = lower.contains(QStringLiteral("conference"))
            || lower.contains(QStringLiteral("proceedings"))
            || lower.contains(QStringLiteral("journal"))
            || lower.contains(QStringLiteral("transactions"))
            || lower.contains(QStringLiteral("workshop"))
            || lower.contains(QStringLiteral("symposium"));
        if (!venueLine) {
            continue;
        }
        QRegularExpressionMatch parenthesized = parenthesizedAcronymRe.match(line);
        if (parenthesized.hasMatch()) {
            return parenthesized.captured(1).simplified();
        }
    }

    QRegularExpression acronymYearRe(QStringLiteral(R"(\b([A-Z]{2,12})\s*[- ]\s*(\d{2,4})\b)"));
    for (const QString &line : lines) {
        QRegularExpressionMatch acronymYear = acronymYearRe.match(line);
        if (acronymYear.hasMatch()) {
            return QStringLiteral("%1 %2").arg(acronymYear.captured(1), acronymYear.captured(2));
        }
    }

    return QString();
}

QString PaperDialog::extractPublicationDateFromPdfText(const QString &text)
{
    QRegularExpression dateRe(
        QStringLiteral(R"(\b(\d{1,2})\s+(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Sept|Oct|Nov|Dec)[a-z]*\s+(\d{4})\b)"),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = dateRe.match(text);
    if (!match.hasMatch()) {
        return QString();
    }

    static const QMap<QString, QString> months = {
        {QStringLiteral("jan"), QStringLiteral("01")},
        {QStringLiteral("feb"), QStringLiteral("02")},
        {QStringLiteral("mar"), QStringLiteral("03")},
        {QStringLiteral("apr"), QStringLiteral("04")},
        {QStringLiteral("may"), QStringLiteral("05")},
        {QStringLiteral("jun"), QStringLiteral("06")},
        {QStringLiteral("jul"), QStringLiteral("07")},
        {QStringLiteral("aug"), QStringLiteral("08")},
        {QStringLiteral("sep"), QStringLiteral("09")},
        {QStringLiteral("sept"), QStringLiteral("09")},
        {QStringLiteral("oct"), QStringLiteral("10")},
        {QStringLiteral("nov"), QStringLiteral("11")},
        {QStringLiteral("dec"), QStringLiteral("12")}
    };
    const QString day = QStringLiteral("%1").arg(match.captured(1).toInt(), 2, 10, QLatin1Char('0'));
    const QString monthKey = match.captured(2).left(4).toLower();
    const QString month = months.value(monthKey, months.value(monthKey.left(3)));
    if (month.isEmpty()) {
        return QString();
    }
    return QStringLiteral("%1-%2-%3").arg(match.captured(3), month, day);
}

QString PaperDialog::extractAbstractFromPdfText(const QString &text)
{
    const QString normalized = normalizeExtractedPdfText(text);
    if (normalized.isEmpty()) {
        return QString();
    }

    const QString stopPattern = QStringLiteral(
        R"((?=\n\s*(keywords?|index\s+terms|关键词|1\.?\s+introduction|i\.?\s+introduction|introduction|2\.?\s+|references)\b))");
    QRegularExpression abstractRe(
        QStringLiteral(R"((?:^|\n)\s*(abstract|摘要)\s*[:：\-–—]?\s*(.+?))") + stopPattern,
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatch match = abstractRe.match(normalized);
    if (!match.hasMatch()) {
        QRegularExpression looseRe(
            QStringLiteral(R"((?:^|\n)\s*(abstract|摘要)\s*[:：\-–—]?\s*(.{80,1800}))"),
            QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
        match = looseRe.match(normalized);
    }
    if (!match.hasMatch()) {
        return QString();
    }

    QString abstract = match.captured(2).simplified();
    abstract.remove(QRegularExpression(QStringLiteral(R"(\b(keywords?|index\s+terms|关键词)\b.*$)"),
        QRegularExpression::CaseInsensitiveOption));
    if (abstract.size() > 2200) {
        abstract = abstract.left(2200).trimmed();
    }
    return looksReadablePdfText(abstract) ? abstract : QString();
}

QString PaperDialog::extractKeywordsFromPdfText(const QString &text)
{
    const QString normalized = normalizeExtractedPdfText(text);
    QRegularExpression keywordsRe(
        QStringLiteral(R"((?:^|\n)\s*(keywords?|index\s+terms|关键词)\s*[:：\-–—]?\s*(.+?)(?=\n\s*(abstract|摘要|1\.?\s+introduction|i\.?\s+introduction|introduction|2\.?\s+|references)\b|$))"),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    const QRegularExpressionMatch match = keywordsRe.match(normalized);
    if (!match.hasMatch()) {
        return QString();
    }

    QString keywords = match.captured(2).simplified();
    if (keywords.size() > 300) {
        keywords = keywords.left(300).trimmed();
    }
    keywords.replace(QRegularExpression(QStringLiteral(R"(\s*[,;，、]\s*)")), QStringLiteral(";"));
    return keywords;
}

QStringList PaperDialog::firstMeaningfulPdfLines(const QString &text)
{
    const QString normalized = normalizeExtractedPdfText(text);
    QStringList result;
    const QStringList lines = normalized.split('\n', Qt::SkipEmptyParts);
    for (QString line : lines) {
        line = line.simplified();
        if (line.isEmpty() || looksLikeStandalonePageMarker(line)) {
            continue;
        }
        result << line;
        if (result.size() >= 90 || line.contains(QRegularExpression(QStringLiteral(R"(^\s*(abstract|摘要|1\.?\s+introduction|i\.?\s+introduction|introduction)\b)"),
            QRegularExpression::CaseInsensitiveOption))) {
            break;
        }
    }
    return result;
}

bool PaperDialog::isWeakPdfTitle(const QString &value)
{
    const QString title = value.simplified();
    const QString lower = title.toLower();
    if (title.size() < 5 || title.size() > 220) {
        return true;
    }
    if (!looksReadablePdfText(title) || looksLikeSectionHeading(title)) {
        return true;
    }
    static const QStringList weakWords = {
        QStringLiteral("introduction"),
        QStringLiteral("abstract"),
        QStringLiteral("references"),
        QStringLiteral("bibliography"),
        QStringLiteral("contents"),
        QStringLiteral("untitled"),
        QStringLiteral("microsoft word"),
        QStringLiteral("doi:"),
        QStringLiteral("arxiv"),
        QStringLiteral("keywords")
    };
    for (const QString &word : weakWords) {
        if (lower == word || lower.startsWith(word + QStringLiteral(" "))
            || lower.contains(QStringLiteral(" ") + word + QStringLiteral(" "))) {
            return true;
        }
    }
    int letters = 0;
    for (const QChar &ch : title) {
        if (ch.isLetter()) ++letters;
    }
    return letters < 4;
}

QString PaperDialog::extractTitleFromPdfText(const QStringList &chunks)
{
    for (int i = 0; i < chunks.size(); ++i) {
        QString chunk = chunks.at(i);
        chunk = chunk.simplified();
        if (chunk.isEmpty() || isWeakPdfTitle(chunk)) {
            continue;
        }
        const QString lower = chunk.toLower();
        if (looksLikeAffiliationOrHeader(chunk)
            || lower.contains(QStringLiteral("issn"))
            || lower.contains(QStringLiteral("isbn"))) {
            continue;
        }

        QString title = chunk;
        while (i + 1 < chunks.size()) {
            QString next = chunks.at(i + 1).simplified();
            const QString nextLower = next.toLower();
            if (next.isEmpty()
                || isWeakPdfTitle(next)
                || looksLikeAffiliationOrHeader(next)
                || nextLower.startsWith(QStringLiteral("abstract"))
                || nextLower.startsWith(QStringLiteral("keywords"))) {
                break;
            }
            const QStringList possibleAuthors = splitAuthorNames(next);
            if (possibleAuthors.size() >= 2
                || next.contains(QRegularExpression(QStringLiteral(R"([0-9∗*†‡§]\s*$)")))
                || (possibleAuthors.size() == 1 && i + 2 < chunks.size()
                    && looksLikeAffiliationOrHeader(chunks.at(i + 2)))) {
                break;
            }
            const QString combined = (title + QLatin1Char(' ') + next).simplified();
            if (combined.size() > 220) {
                break;
            }
            title = combined;
            ++i;
        }
        return title;
    }
    return QString();
}

QString PaperDialog::extractAuthorsFromPdfText(const QStringList &chunks, const QString &title)
{
    bool passedTitle = title.isEmpty();
    QStringList authorLines;
    bool previousWasAffiliation = false;
    for (int i = 0; i < chunks.size(); ++i) {
        QString chunk = chunks.at(i);
        chunk = chunk.simplified();
        if (chunk.isEmpty()) {
            continue;
        }
        if (!passedTitle) {
            if (chunk.compare(title, Qt::CaseInsensitive) == 0
                || title.contains(chunk, Qt::CaseInsensitive)
                || chunk.contains(title, Qt::CaseInsensitive)) {
                passedTitle = true;
            }
            continue;
        }

        const QString lower = chunk.toLower();
        if (lower.contains(QStringLiteral("abstract"))
            || lower.contains(QStringLiteral("keywords"))
            || lower.contains(QStringLiteral("introduction"))) {
            break;
        }
        if (looksLikeAffiliationOrHeader(chunk)) {
            previousWasAffiliation = true;
            continue;
        }
        const QString next = (i + 1 < chunks.size()) ? chunks.at(i + 1).simplified() : QString();
        if (previousWasAffiliation && next.contains(QRegularExpression(QStringLiteral(R"(\S+@\S+)")))) {
            previousWasAffiliation = false;
            continue;
        }

        const QStringList names = splitAuthorNames(chunk);
        if (!names.isEmpty()) {
            authorLines << chunk;
        }
        previousWasAffiliation = false;
    }

    return splitAuthorNames(authorLines.join(QStringLiteral("; "))).join(QStringLiteral("; "));
}

QStringList PaperDialog::splitAuthorNames(const QString &value)
{
    QString text = value.simplified();
    text.remove(QRegularExpression(QStringLiteral(R"(\bhttps?://\S+)"), QRegularExpression::CaseInsensitiveOption));
    text.remove(QRegularExpression(QStringLiteral(R"(\S+@\S+)")));
    text.replace(QRegularExpression(QStringLiteral(R"(\b(and|AND|And)\b)")), QStringLiteral(";"));
    text.replace(QRegularExpression(QStringLiteral(R"((?<=\p{L})\s*\d+(?:\s*[,;]\s*\d+)*(?:\s*[\x{2217}\x{2020}\x{2021}\x{00a7}*]+)?\s+(?=[A-Z\x{4e00}-\x{9fff}]))")), QStringLiteral(";"));
    text.replace(QRegularExpression(QStringLiteral(R"((?<=\p{L})[\x{2217}\x{2020}\x{2021}\x{00a7}*]+(?=\s|$))")), QString());
    text.remove(QRegularExpression(QStringLiteral(R"([\x{2217}\x{2020}\x{2021}\x{00a7}*]+)")));
    text.replace(QRegularExpression(QStringLiteral(R"([;\x{3001}\x{ff0c}]+)")), QStringLiteral(";"));
    text.replace(QRegularExpression(QStringLiteral(R"(,(?=\s*[A-Z\x{4e00}-\x{9fff}]))")), QStringLiteral(";"));
    text.replace(QRegularExpression(QStringLiteral(R"(\s+\d+(\s*[,;])?)")), QStringLiteral(";"));

    QStringList result;
    QSet<QString> seen;
    const QStringList parts = text.split(';', Qt::SkipEmptyParts);
    for (QString name : parts) {
        name = name.simplified();
        name.remove(QRegularExpression(QStringLiteral(R"(^\d+\s*)")));
        name.remove(QRegularExpression(QStringLiteral(R"(\s*\d+$)")));
        name = name.trimmed();
        if (name.size() < 2 || name.size() > 80) {
            continue;
        }
        int letters = 0;
        for (const QChar &ch : name) {
            if (ch.isLetter()) ++letters;
        }
        if (letters < 2) {
            continue;
        }
        const QString lower = name.toLower();
        if (lower.contains(QStringLiteral("abstract"))
            || lower.contains(QStringLiteral("keywords"))
            || lower.contains(QStringLiteral("correspondence"))
            || lower.contains(QStringLiteral("contribution"))
            || looksLikeAffiliationOrHeader(name)
            || lower.contains(QStringLiteral("research"))
            || lower.contains(QStringLiteral("laboratories"))
            || lower.contains(QStringLiteral("lab"))
            || lower.contains(QStringLiteral("company"))
            || lower.contains(QStringLiteral("group"))) {
            continue;
        }

        const bool hasCjk = name.contains(QRegularExpression(QStringLiteral(R"([\x{4e00}-\x{9fff}])")));
        if (!hasCjk) {
            const QStringList words = name.split(' ', Qt::SkipEmptyParts);
            if (words.size() < 2 || words.size() > 6) {
                continue;
            }

            int wordsWithLowercase = 0;
            int acronymWords = 0;
            for (const QString &word : words) {
                bool hasLetter = false;
                bool hasLower = false;
                bool allUpper = true;
                for (const QChar &ch : word) {
                    if (!ch.isLetter()) {
                        continue;
                    }
                    hasLetter = true;
                    if (ch.isLower()) {
                        hasLower = true;
                        allUpper = false;
                    } else if (!ch.isUpper()) {
                        allUpper = false;
                    }
                }
                if (!hasLetter) {
                    continue;
                }
                if (hasLower) {
                    ++wordsWithLowercase;
                }
                if (allUpper && word.size() > 1) {
                    ++acronymWords;
                }
            }
            if (wordsWithLowercase == 0 || acronymWords == words.size()) {
                continue;
            }
            if (name.contains(QRegularExpression(QStringLiteral(R"(\b(of|for|and|the|in|at)\b)"), QRegularExpression::CaseInsensitiveOption))) {
                continue;
            }
        }
        if (looksLikeStandalonePageMarker(name)) {
            continue;
        }
        if (!seen.contains(lower)) {
            seen.insert(lower);
            result << name;
        }
    }
    return result;
}

IdType PaperDialog::findOrCreateAuthor(const QString &name) const
{
    const QString trimmed = name.simplified();
    if (trimmed.isEmpty()) {
        return INVALID_ID;
    }

    auto &mgr = LibraryManager::getInstance();
    for (const auto &pair : mgr.getAllAuthors()) {
        if (QString::fromStdString(pair.second.getName()).compare(trimmed, Qt::CaseInsensitive) == 0) {
            return pair.first;
        }
    }

    Author author;
    author.setName(trimmed.toStdString());
    return mgr.addAuthor(author);
}

void PaperDialog::refreshSelectedAuthorList()
{
    m_authorList->clear();
    auto &mgr = LibraryManager::getInstance();
    for (IdType id : m_selectedAuthorIds) {
        m_authorList->addItem(QString("[%1] %2")
            .arg(id)
            .arg(QString::fromStdString(mgr.getAuthorName(id))));
    }
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
        return match.hasMatch() ? decodePdfValue(match.captured(1)).trimmed() : QString();
    };

    const QString externalText = extractPdfTextWithExternalTool(filePath);
    const QString internalText = extractPdfTextFromPdfData(data);
    QString plainText = externalText;
    if (!internalText.isEmpty() && !plainText.contains(internalText.left(qMin(80, internalText.size())))) {
        if (!plainText.isEmpty()) {
            plainText.append('\n');
        }
        plainText.append(internalText);
    }
    plainText = normalizeExtractedPdfText(plainText);
    const QStringList textChunks = firstMeaningfulPdfLines(plainText);

    meta.title = firstNonEmpty({
        takeMatch(QRegularExpression(QStringLiteral(R"(/Title\s*(\((?:\\.|[^\\()])*\)|<[^>]+>))"), QRegularExpression::DotMatchesEverythingOption)),
    });
    if (isWeakPdfTitle(meta.title)) {
        const QString titleFromText = extractTitleFromPdfText(textChunks);
        if (!titleFromText.isEmpty()) {
            meta.title = titleFromText;
        }
    }

    meta.author = firstNonEmpty({
        takeMatch(QRegularExpression(QStringLiteral(R"(/Author\s*(\((?:\\.|[^\\()])*\)|<[^>]+>))"), QRegularExpression::DotMatchesEverythingOption)),
    });
    if (splitAuthorNames(meta.author).isEmpty()) {
        const QString authorsFromText = extractAuthorsFromPdfText(textChunks, meta.title);
        if (!authorsFromText.isEmpty()) {
            meta.author = authorsFromText;
        }
    }

    meta.subject = firstNonEmpty({
        takeMatch(QRegularExpression(QStringLiteral(R"(/Subject\s*(\((?:\\.|[^\\()])*\)|<[^>]+>))"), QRegularExpression::DotMatchesEverythingOption)),
    });

    meta.keywords = firstNonEmpty({
        takeMatch(QRegularExpression(QStringLiteral(R"(/Keywords\s*(\((?:\\.|[^\\()])*\)|<[^>]+>))"), QRegularExpression::DotMatchesEverythingOption)),
        extractKeywordsFromPdfText(plainText),
    });

    meta.publicationDate = normalizePdfDate(firstNonEmpty({
        extractPublicationDateFromPdfText(plainText),
        takeMatch(QRegularExpression(QStringLiteral(R"(/CreationDate\s*(\((?:\\.|[^\\()])*\)|<[^>]+>))"), QRegularExpression::DotMatchesEverythingOption)),
        takeMatch(QRegularExpression(QStringLiteral(R"(/ModDate\s*(\((?:\\.|[^\\()])*\)|<[^>]+>))"), QRegularExpression::DotMatchesEverythingOption)),
    }));

    int pageCount = 0;
    QRegularExpression pageTypeRe(QStringLiteral(R"(/Type\s*/Page\b(?!s))"));
    auto pageTypeIt = pageTypeRe.globalMatch(text);
    while (pageTypeIt.hasNext()) {
        pageTypeIt.next();
        ++pageCount;
    }
    if (pageCount <= 0) {
        const QRegularExpression pagesRe(QStringLiteral(R"(/Count\s+(\d+))"));
        const QRegularExpressionMatch pagesMatch = pagesRe.match(text);
        if (pagesMatch.hasMatch()) {
            pageCount = pagesMatch.captured(1).toInt();
        }
    }
    if (pageCount > 0) {
        meta.pageRange = QStringLiteral("1-%1").arg(pageCount);
    }

    meta.doi = extractDoiFromPdfText(plainText);
    if (meta.doi.isEmpty()) {
        meta.doi = extractDoiFromPdfText(text);
    }
    meta.issue = firstNonEmpty({
        extractVenueFromPdfText(meta.subject),
        extractVenueFromPdfText(plainText)
    });
    meta.issueNumber = firstNonEmpty({
        extractDoiFromPdfText(meta.subject),
        extractIssueNumberFromPdfText(plainText),
        extractIssueNumberFromPdfText(meta.issue)
    });

    meta.abstract = extractAbstractFromPdfText(plainText);
    if (meta.abstract.isEmpty() && meta.subject.size() > 120 && looksReadablePdfText(meta.subject)) {
        meta.abstract = meta.subject;
    }
    meta.rawText = plainText.left(5000);

    return meta;
}

void PaperDialog::applyPdfMetadata(const PdfMetadata &metadata)
{
    if (m_codeEdit->text().trimmed().isEmpty()) {
        const QString code = firstNonEmpty({metadata.doi, metadata.issueNumber});
        if (!code.isEmpty()) {
            m_codeEdit->setText(code);
        }
    }

    if (!metadata.title.isEmpty() && m_titleEdit->text().trimmed().isEmpty()) {
        m_titleEdit->setText(metadata.title);
    }

    if (!metadata.author.isEmpty() && m_selectedAuthorIds.empty()) {
        m_selectedAuthorIds.clear();
        const QStringList authors = splitAuthorNames(metadata.author);
        QSet<IdType> seenIds;
        for (const QString &authorName : authors) {
            QString trimmed = authorName.simplified();
            if (trimmed.isEmpty()) {
                continue;
            }
            const IdType id = findOrCreateAuthor(trimmed);
            if (id != INVALID_ID && !seenIds.contains(id)) {
                seenIds.insert(id);
                m_selectedAuthorIds.push_back(id);
            }
        }
        refreshSelectedAuthorList();
    }

    if (!metadata.publicationDate.isEmpty() && m_dateEdit->text().trimmed().isEmpty()) {
        m_dateEdit->setText(metadata.publicationDate);
    }

    if (!metadata.issue.isEmpty() && m_issueEdit->text().trimmed().isEmpty()) {
        m_issueEdit->setText(metadata.issue);
    }

    if (!metadata.issueNumber.isEmpty() && m_issueNumEdit->text().trimmed().isEmpty()) {
        m_issueNumEdit->setText(metadata.issueNumber);
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
