#include "authordialog.h"

#include <QLineEdit>
#include <QComboBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>

AuthorDialog::AuthorDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi();
    setModal(true);
    setMinimumWidth(420);
}

void AuthorDialog::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QFormLayout *form = new QFormLayout;
    m_nameEdit        = new QLineEdit;
    m_genderCombo     = new QComboBox;
    m_affiliationEdit = new QLineEdit;
    m_emailEdit       = new QLineEdit;
    m_areasEdit       = new QLineEdit;
    m_areasEdit->setPlaceholderText(QStringLiteral("多个领域用分号(;)分隔"));

    m_genderCombo->addItem(QStringLiteral("男"));
    m_genderCombo->addItem(QStringLiteral("女"));
    m_genderCombo->addItem(QStringLiteral("其他"));

    form->addRow(QStringLiteral("姓名:"), m_nameEdit);
    form->addRow(QStringLiteral("性别:"), m_genderCombo);
    form->addRow(QStringLiteral("单位:"), m_affiliationEdit);
    form->addRow(QStringLiteral("邮箱:"), m_emailEdit);
    form->addRow(QStringLiteral("研究领域:"), m_areasEdit);

    mainLayout->addLayout(form);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttonBox->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确定"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

Author AuthorDialog::getEntity() const
{
    Author a;
    a.setName(m_nameEdit->text().toStdString());
    a.setGender(m_genderCombo->currentText().toStdString());
    a.setAffiliation(m_affiliationEdit->text().toStdString());
    a.setEmail(m_emailEdit->text().toStdString());

    std::vector<std::string> areas;
    QString areaText = m_areasEdit->text().trimmed();
    if (!areaText.isEmpty()) {
        QStringList parts = areaText.split(';');
        for (const QString &p : parts) {
            QString trimmed = p.trimmed();
            if (!trimmed.isEmpty())
                areas.push_back(trimmed.toStdString());
        }
    }
    a.setResearchAreas(areas);
    return a;
}

void AuthorDialog::setEntity(const Author &a)
{
    m_nameEdit->setText(QString::fromStdString(a.getName()));
    QString gender = QString::fromStdString(a.getGender());
    int idx = m_genderCombo->findText(gender);
    if (idx >= 0) m_genderCombo->setCurrentIndex(idx);
    m_affiliationEdit->setText(QString::fromStdString(a.getAffiliation()));
    m_emailEdit->setText(QString::fromStdString(a.getEmail()));

    QString areas;
    for (size_t i = 0; i < a.getResearchAreas().size(); ++i) {
        if (i > 0) areas += ";";
        areas += QString::fromStdString(a.getResearchAreas()[i]);
    }
    m_areasEdit->setText(areas);
}
