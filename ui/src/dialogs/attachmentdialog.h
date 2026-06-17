#ifndef ATTACHMENTDIALOG_H
#define ATTACHMENTDIALOG_H

#include <QDialog>
#include "manage.h"

class QLineEdit;
class QTextEdit;
class QComboBox;

class AttachmentDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AttachmentDialog(QWidget *parent = nullptr);

    Attachment getEntity() const;
    void setEntity(const Attachment &att);

protected:
    void accept() override;

private:
    void setupUi();

    QLineEdit *m_nameEdit;
    QComboBox *m_paperCombo;
    QTextEdit *m_contentEdit;
};

#endif // ATTACHMENTDIALOG_H
