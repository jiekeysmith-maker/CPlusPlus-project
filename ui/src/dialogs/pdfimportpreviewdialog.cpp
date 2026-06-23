#include "pdfimportpreviewdialog.h"

#include "paperdialog.h"
#include "manage.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

namespace {
QString normalizeSuggestionKey(QString value)
{
    value = value.simplified().toLower();
    value.remove(QRegularExpression(QStringLiteral(R"(\b(proceedings|conference|journal|transactions|on|of|the|at|in)\b)"),
        QRegularExpression::CaseInsensitiveOption));
    value.replace(QRegularExpression(QStringLiteral(R"([^a-z0-9]+)")), QStringLiteral(" "));
    return value.simplified();
}

bool sourceMatchesSuggestion(const Source *source, const QString &suggestion)
{
    if (!source || suggestion.isEmpty()) {
        return false;
    }

    const QString suggestionKey = normalizeSuggestionKey(suggestion);
    if (suggestionKey.isEmpty() || suggestionKey == QStringLiteral("arxiv")) {
        return false;
    }

    const QString shortName = QString::fromStdString(source->getShortName());
    const QString fullName = QString::fromStdString(source->getFullName());
    const QString shortKey = normalizeSuggestionKey(shortName);
    const QString fullKey = normalizeSuggestionKey(fullName);

    return (!shortKey.isEmpty() && (suggestionKey.contains(shortKey) || shortKey.contains(suggestionKey)))
        || (!fullKey.isEmpty() && (suggestionKey.contains(fullKey) || fullKey.contains(suggestionKey)));
}
}

PdfImportPreviewDialog::PdfImportPreviewDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("PDF 导入预览"));
    setModal(true);
    setMinimumSize(680, 640);

    auto *mainLayout = new QVBoxLayout(this);

    auto *tipLabel = new QLabel(QStringLiteral("以下内容由系统自动识别生成，可能不完全准确。启用在线元数据查询且本地信息不足时，系统才会尝试联网补全；关闭时仅使用本地 PDF 解析。请确认后再导入。"));
    tipLabel->setWordWrap(true);
    mainLayout->addWidget(tipLabel);

    m_summaryLabel = new QLabel;
    m_summaryLabel->setWordWrap(true);
    m_summaryLabel->setStyleSheet(QStringLiteral("font-weight: 600; color: #1f2937;"));
    mainLayout->addWidget(m_summaryLabel);

    auto *scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    auto *content = new QWidget;
    auto *form = new QFormLayout(content);
    form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    auto makeSourceLabel = []() {
        auto *label = new QLabel;
        label->setWordWrap(true);
        label->setStyleSheet(QStringLiteral("color: #666; font-size: 12px;"));
        return label;
    };

    m_doiEdit = new QLineEdit;
    m_titleEdit = new QLineEdit;
    m_authorEdit = new QLineEdit;
    m_authorEdit->setPlaceholderText(QStringLiteral("多个作者用分号(;)分隔"));
    m_keywordsEdit = new QLineEdit;
    m_keywordsEdit->setPlaceholderText(QStringLiteral("多个关键词用分号(;)分隔"));
    m_abstractEdit = new QTextEdit;
    m_abstractEdit->setMinimumHeight(110);
    m_dateEdit = new QLineEdit;
    m_issueEdit = new QLineEdit;
    m_volumeEdit = new QLineEdit;
    m_issueNumberEdit = new QLineEdit;
    m_pageEdit = new QLineEdit;
    m_arxivEdit = new QLineEdit;
    m_remarkEdit = new QTextEdit;
    m_remarkEdit->setMinimumHeight(70);

    m_doiSourceLabel = makeSourceLabel();
    m_titleSourceLabel = makeSourceLabel();
    m_authorSourceLabel = makeSourceLabel();
    m_keywordsSourceLabel = makeSourceLabel();
    m_abstractSourceLabel = makeSourceLabel();
    m_dateSourceLabel = makeSourceLabel();
    m_issueSourceLabel = makeSourceLabel();
    m_volumeSourceLabel = makeSourceLabel();
    m_issueNumberSourceLabel = makeSourceLabel();
    m_pageSourceLabel = makeSourceLabel();
    m_arxivSourceLabel = makeSourceLabel();
    m_remarkSourceLabel = makeSourceLabel();

    form->addRow(QStringLiteral("DOI 编号:"), lineEditorWithSource(m_doiEdit, m_doiSourceLabel));
    form->addRow(QStringLiteral("标题:"), lineEditorWithSource(m_titleEdit, m_titleSourceLabel));
    form->addRow(QStringLiteral("作者:"), lineEditorWithSource(m_authorEdit, m_authorSourceLabel));
    form->addRow(QStringLiteral("关键词:"), lineEditorWithSource(m_keywordsEdit, m_keywordsSourceLabel));
    form->addRow(QStringLiteral("摘要:"), textEditorWithSource(m_abstractEdit, m_abstractSourceLabel));
    form->addRow(QStringLiteral("发表时间:"), lineEditorWithSource(m_dateEdit, m_dateSourceLabel));
    form->addRow(QStringLiteral("出版物 / 刊期:"), lineEditorWithSource(m_issueEdit, m_issueSourceLabel));
    form->addRow(QStringLiteral("卷号:"), lineEditorWithSource(m_volumeEdit, m_volumeSourceLabel));
    form->addRow(QStringLiteral("期号 / 刊号:"), lineEditorWithSource(m_issueNumberEdit, m_issueNumberSourceLabel));
    form->addRow(QStringLiteral("页码:"), lineEditorWithSource(m_pageEdit, m_pageSourceLabel));
    form->addRow(QStringLiteral("arXiv 编号:"), lineEditorWithSource(m_arxivEdit, m_arxivSourceLabel));
    form->addRow(QStringLiteral("备注:"), textEditorWithSource(m_remarkEdit, m_remarkSourceLabel));

    m_publicationHintLabel = new QLabel;
    m_publicationHintLabel->setWordWrap(true);
    m_sourceCombo = new QComboBox;
    auto *publicationLayout = new QVBoxLayout;
    publicationLayout->addWidget(m_publicationHintLabel);
    publicationLayout->addWidget(m_sourceCombo);
    form->addRow(QStringLiteral("出版物建议:"), publicationLayout);

    scrollArea->setWidget(content);
    mainLayout->addWidget(scrollArea, 1);

    auto *buttonBox = new QDialogButtonBox;
    auto *confirmButton = buttonBox->addButton(QStringLiteral("确认填入"), QDialogButtonBox::AcceptRole);
    auto *cancelButton = buttonBox->addButton(QStringLiteral("取消自动填入"), QDialogButtonBox::RejectRole);
    connect(confirmButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

QWidget *PdfImportPreviewDialog::lineEditorWithSource(QLineEdit *editor, QLabel *sourceLabel)
{
    auto *container = new QWidget;
    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(3);
    layout->addWidget(editor);
    layout->addWidget(sourceLabel);
    return container;
}

QWidget *PdfImportPreviewDialog::textEditorWithSource(QTextEdit *editor, QLabel *sourceLabel)
{
    auto *container = new QWidget;
    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(3);
    layout->addWidget(editor);
    layout->addWidget(sourceLabel);
    return container;
}

void PdfImportPreviewDialog::setSourceText(QLabel *label, const QString &text, const QString &fallback)
{
    label->setText(text.trimmed().isEmpty() ? fallback : text.trimmed());
}

void PdfImportPreviewDialog::setMetadata(const PdfMetadata &metadata)
{
    m_summaryLabel->setText(metadata.confidenceSummary.trimmed().isEmpty()
        ? QStringLiteral("元数据来源：本地 PDF 解析，低/中可信度，请检查")
        : QStringLiteral("元数据来源：%1").arg(metadata.confidenceSummary.trimmed()));

    m_doiEdit->setText(metadata.doi);
    m_titleEdit->setText(metadata.title);
    m_authorEdit->setText(metadata.author);
    m_keywordsEdit->setText(metadata.keywords);
    m_abstractEdit->setPlainText(metadata.abstract);
    m_dateEdit->setText(metadata.publicationDate);
    m_issueEdit->setText(metadata.issue);
    m_volumeEdit->setText(metadata.volume);
    m_issueNumberEdit->setText(metadata.issueNumber);
    m_pageEdit->setText(metadata.pageRange);
    m_arxivEdit->setText(metadata.arxivId);

    QString remark = metadata.subject;
    if (!metadata.arxivId.isEmpty() && !remark.contains(metadata.arxivId, Qt::CaseInsensitive)) {
        if (!remark.isEmpty()) {
            remark += QStringLiteral("\n");
        }
        remark += metadata.arxivId;
    }
    m_remarkEdit->setPlainText(remark);

    setSourceText(m_doiSourceLabel, metadata.doiSource, QStringLiteral("未识别到 DOI"));
    setSourceText(m_titleSourceLabel, metadata.titleSource, QStringLiteral("未识别到可信标题"));
    setSourceText(m_authorSourceLabel, metadata.authorSource, QStringLiteral("未识别到可信作者"));
    setSourceText(m_keywordsSourceLabel, metadata.keywordsSource, QStringLiteral("未识别到关键词"));
    setSourceText(m_abstractSourceLabel, metadata.abstractSource, QStringLiteral("未识别到摘要"));
    setSourceText(m_dateSourceLabel, metadata.publicationDateSource, QStringLiteral("未识别到发表时间"));
    setSourceText(m_issueSourceLabel, metadata.issueSource, QStringLiteral("未识别到刊期 / 出版物文本"));
    setSourceText(m_volumeSourceLabel, metadata.issueNumberSource, QStringLiteral("未识别到卷号"));
    setSourceText(m_issueNumberSourceLabel, metadata.issueNumberSource, QStringLiteral("未识别到刊号"));
    setSourceText(m_pageSourceLabel, metadata.pageRange.isEmpty() ? QString() : metadata.confidenceSummary, QStringLiteral("未识别到页码"));
    setSourceText(m_arxivSourceLabel, metadata.arxivSource, QStringLiteral("未识别到 arXiv 编号"));
    setSourceText(m_remarkSourceLabel, metadata.subjectSource, QStringLiteral("来自 PDF 主题或自动补充信息，低可信度，请检查"));
    populateSourceSuggestion(metadata.publicationSuggestion);
}

PdfMetadata PdfImportPreviewDialog::metadata() const
{
    PdfMetadata metadata;
    metadata.doi = m_doiEdit->text().trimmed();
    metadata.title = m_titleEdit->text().trimmed();
    metadata.author = m_authorEdit->text().trimmed();
    metadata.keywords = m_keywordsEdit->text().trimmed();
    metadata.abstract = m_abstractEdit->toPlainText().trimmed();
    metadata.publicationDate = m_dateEdit->text().trimmed();
    metadata.issue = m_issueEdit->text().trimmed();
    metadata.volume = m_volumeEdit->text().trimmed();
    metadata.issueNumber = m_issueNumberEdit->text().trimmed();
    metadata.pageRange = m_pageEdit->text().trimmed();
    metadata.arxivId = m_arxivEdit->text().trimmed();
    metadata.subject = m_remarkEdit->toPlainText().trimmed();
    return metadata;
}

IdType PdfImportPreviewDialog::selectedSourceId() const
{
    return m_sourceCombo->currentData().toInt();
}

void PdfImportPreviewDialog::populateSourceSuggestion(const QString &suggestion)
{
    m_sourceCombo->clear();
    m_sourceCombo->addItem(QStringLiteral("不关联出版物"), INVALID_ID);

    const QString cleanSuggestion = suggestion.simplified();
    if (cleanSuggestion.isEmpty()) {
        m_publicationHintLabel->setText(QStringLiteral("未识别到明确的会议或期刊。"));
        m_sourceCombo->setEnabled(false);
        return;
    }

    m_sourceCombo->setEnabled(true);
    int matchCount = 0;
    const auto &sources = LibraryManager::getInstance().getAllSources();
    for (const auto &pair : sources) {
        Source *source = pair.second.get();
        if (!sourceMatchesSuggestion(source, cleanSuggestion)) {
            continue;
        }
        ++matchCount;
        m_sourceCombo->addItem(
            QStringLiteral("[%1] %2 (%3)")
                .arg(source->getId())
                .arg(QString::fromStdString(source->getShortName()))
                .arg(QString::fromStdString(source->getType())),
            source->getId());
    }

    if (matchCount > 0) {
        m_publicationHintLabel->setText(QStringLiteral("识别到出版物：%1。可选择关联出版物库中的已有条目。")
            .arg(cleanSuggestion));
    } else {
        m_publicationHintLabel->setText(QStringLiteral("识别到出版物：%1。出版物库中未找到该会议/期刊，可稍后在出版物库中新增。")
            .arg(cleanSuggestion));
    }
}
