#ifndef AUTHORDIALOG_H
#define AUTHORDIALOG_H

#include <QDialog>
#include "manage.h"

class QLineEdit;
class QComboBox;

class AuthorDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AuthorDialog(QWidget *parent = nullptr);

    Author getEntity() const;
    void setEntity(const Author &a);

private:
    void setupUi();

    QLineEdit *m_nameEdit;
    QComboBox *m_genderCombo;
    QLineEdit *m_affiliationEdit;
    QLineEdit *m_emailEdit;
    QLineEdit *m_areasEdit;
};

#endif // AUTHORDIALOG_H
