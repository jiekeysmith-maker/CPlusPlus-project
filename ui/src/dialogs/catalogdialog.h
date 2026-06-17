#ifndef CATALOGDIALOG_H
#define CATALOGDIALOG_H

#include <QDialog>
#include "manage.h"

class QLineEdit;
class QComboBox;

class CatalogDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CatalogDialog(QWidget *parent = nullptr);

    Catalog getEntity() const;
    void setEntity(const Catalog &cat);
    void setFixedParent(IdType parentId);

protected:
    void accept() override;

private:
    void setupUi();
    void populateParentCombo();

    QLineEdit *m_nameEdit;
    QLineEdit *m_descEdit;
    QComboBox *m_parentCombo;
    bool       m_fixedParent;
};

#endif // CATALOGDIALOG_H
