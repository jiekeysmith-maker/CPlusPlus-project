#ifndef PAPERDIALOG_H
#define PAPERDIALOG_H

#include <QDialog>
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
    QString title;
    QString author;
    QString keywords;
    QString subject;
    QString publicationDate;
    QString pageRange;
    QString abstract;
    QString rawText;
    bool isValid() const { return !title.isEmpty() || !author.isEmpty() || !keywords.isEmpty() || !subject.isEmpty(); }
};

class PaperDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PaperDialog(QWidget *parent = nullptr);

    Paper getPaper() const;
    void setPaper(const Paper &p);

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
    static QString normalizePdfDate(const QString &value);
    static QString firstNonEmpty(const QStringList &values);

    // 基本信息
    QLineEdit *m_titleEdit;
    QLineEdit *m_keywordsEdit;
    QTextEdit *m_abstractEdit;
    QLineEdit *m_dateEdit;
    QLineEdit *m_issueEdit;
    QLineEdit *m_issueNumEdit;
    QLineEdit *m_pageEdit;
    QLineEdit *m_filePathEdit;
    QPushButton *m_btnSelectFile;
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
