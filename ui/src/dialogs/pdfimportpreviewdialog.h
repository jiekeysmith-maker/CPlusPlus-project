#ifndef PDFIMPORTPREVIEWDIALOG_H
#define PDFIMPORTPREVIEWDIALOG_H

#include <QDialog>

#include "CoreTypes.h"

class QComboBox;
class QLabel;
class QLineEdit;
class QTextEdit;
class QWidget;
struct PdfMetadata;

class PdfImportPreviewDialog : public QDialog
{
public:
    explicit PdfImportPreviewDialog(QWidget *parent = nullptr);

    void setMetadata(const PdfMetadata &metadata);
    PdfMetadata metadata() const;
    IdType selectedSourceId() const;

private:
    QWidget *lineEditorWithSource(QLineEdit *editor, QLabel *sourceLabel);
    QWidget *textEditorWithSource(QTextEdit *editor, QLabel *sourceLabel);
    void setSourceText(QLabel *label, const QString &text, const QString &fallback);
    void populateSourceSuggestion(const QString &suggestion);

    QLineEdit *m_doiEdit = nullptr;
    QLineEdit *m_titleEdit = nullptr;
    QLineEdit *m_authorEdit = nullptr;
    QLineEdit *m_keywordsEdit = nullptr;
    QTextEdit *m_abstractEdit = nullptr;
    QLineEdit *m_dateEdit = nullptr;
    QLineEdit *m_issueEdit = nullptr;
    QLineEdit *m_volumeEdit = nullptr;
    QLineEdit *m_issueNumberEdit = nullptr;
    QLineEdit *m_pageEdit = nullptr;
    QLineEdit *m_arxivEdit = nullptr;
    QTextEdit *m_remarkEdit = nullptr;

    QLabel *m_summaryLabel = nullptr;
    QLabel *m_doiSourceLabel = nullptr;
    QLabel *m_titleSourceLabel = nullptr;
    QLabel *m_authorSourceLabel = nullptr;
    QLabel *m_keywordsSourceLabel = nullptr;
    QLabel *m_abstractSourceLabel = nullptr;
    QLabel *m_dateSourceLabel = nullptr;
    QLabel *m_issueSourceLabel = nullptr;
    QLabel *m_volumeSourceLabel = nullptr;
    QLabel *m_issueNumberSourceLabel = nullptr;
    QLabel *m_pageSourceLabel = nullptr;
    QLabel *m_arxivSourceLabel = nullptr;
    QLabel *m_remarkSourceLabel = nullptr;
    QLabel *m_publicationHintLabel = nullptr;
    QComboBox *m_sourceCombo = nullptr;
};

#endif // PDFIMPORTPREVIEWDIALOG_H
