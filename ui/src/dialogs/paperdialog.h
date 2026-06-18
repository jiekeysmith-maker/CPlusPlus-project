#ifndef PAPERDIALOG_H
#define PAPERDIALOG_H

#include <QDialog>
#include <QByteArray>
#include <QMap>
#include <QString>
#include <QStringList>
#include "manage.h"

class QLineEdit;
class QTextEdit;
class QListWidget;
class QLabel;
class QPushButton;

struct PdfMetadata
{
    QString doi;
    QString title;
    QString author;
    QString keywords;
    QString subject;
    QString publicationDate;
    QString issue;
    QString issueNumber;
    QString pageRange;
    QString abstract;
    QString rawText;
    bool isValid() const
    {
        return !doi.isEmpty() || !title.isEmpty() || !author.isEmpty()
            || !keywords.isEmpty() || !subject.isEmpty()
            || !publicationDate.isEmpty() || !issue.isEmpty()
            || !issueNumber.isEmpty() || !pageRange.isEmpty()
            || !abstract.isEmpty();
    }
};

class PaperDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PaperDialog(QWidget *parent = nullptr);

    Paper getPaper() const;
    void setPaper(const Paper &p);

protected:
    void accept() override;

private slots:
    void onSelectAuthors();
    void onSelectSource();
    void onSelectAttachments();
    void onSelectFile();

private:
    void setupUi();
    PdfMetadata extractPdfMetadata(const QString &filePath) const;
    void applyPdfMetadata(const PdfMetadata &metadata);
    static QString decodePdfLiteralString(const QString &value);
    static QByteArray decodePdfLiteralBytes(const QString &value);
    static QString decodePdfHexString(const QString &value);
    static QString decodePdfBytes(const QByteArray &bytes);
    static QString decodePdfValue(const QString &value);
    static QString normalizePdfDate(const QString &value);
    static QString firstNonEmpty(const QStringList &values);
    static QString extractPdfTextWithExternalTool(const QString &filePath);
    static QString extractPdfTextFromPdfData(const QByteArray &data);
    static QString extractPdfTextFromContentStream(const QByteArray &stream);
    static QByteArray inflatePdfStream(const QByteArray &stream);
    static QString normalizeExtractedPdfText(const QString &text);
    static QString extractDoiFromPdfText(const QString &text);
    static QString extractArxivIdFromPdfText(const QString &text);
    static QString extractVenueFromPdfText(const QString &text);
    static QString extractIssueNumberFromPdfText(const QString &text);
    static QString extractPublicationDateFromPdfText(const QString &text);
    static QString extractAbstractFromPdfText(const QString &text);
    static QString extractKeywordsFromPdfText(const QString &text);
    static QStringList firstMeaningfulPdfLines(const QString &text);
    static bool isWeakPdfTitle(const QString &value);
    static QString extractTitleFromPdfText(const QStringList &chunks);
    static QString extractAuthorsFromPdfText(const QStringList &chunks, const QString &title);
    static QStringList splitAuthorNames(const QString &value);
    IdType findOrCreateAuthor(const QString &name) const;
    void refreshSelectedAuthorList();

    // 基本信息
    QLineEdit *m_codeEdit;
    QLineEdit *m_titleEdit;
    QLineEdit *m_keywordsEdit;
    QTextEdit *m_abstractEdit;
    QLineEdit *m_dateEdit;
    QLineEdit *m_issueEdit;
    QLineEdit *m_issueNumEdit;
    QLineEdit *m_pageEdit;
    QLineEdit *m_filePathEdit;
    QPushButton *m_btnSelectFile;
    QLineEdit *m_uploadTimeEdit;
    QLineEdit *m_remarkEdit;

    // 作者与出版物
    QListWidget *m_authorList;
    QLabel      *m_sourceLabel;
    QPushButton *m_btnSelectAuthors;
    QPushButton *m_btnSelectSource;

    // 附件
    QListWidget *m_attachmentList;
    QPushButton *m_btnSelectAttachments;

    // 内部数据
    std::vector<IdType> m_selectedAuthorIds;
    IdType              m_selectedSourceId;
    std::vector<IdType> m_selectedAttachmentIds;
};

#endif // PAPERDIALOG_H
