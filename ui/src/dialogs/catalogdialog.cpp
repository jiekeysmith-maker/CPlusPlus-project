#include "catalogdialog.h"

#include <QLineEdit>
#include <QComboBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>

CatalogDialog::CatalogDialog(QWidget *parent)
    : QDialog(parent), m_fixedParent(false)
{
    setupUi();
    setModal(true);
    setMinimumWidth(400);
}

void CatalogDialog::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QFormLayout *form = new QFormLayout;
    m_nameEdit   = new QLineEdit;
    m_descEdit   = new QLineEdit;
    m_parentCombo = new QComboBox;

    populateParentCombo();

    form->addRow(QStringLiteral("名称:"), m_nameEdit);
    form->addRow(QStringLiteral("说明:"), m_descEdit);
    form->addRow(QStringLiteral("父目录:"), m_parentCombo);

    mainLayout->addLayout(form);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttonBox->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确定"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

void CatalogDialog::populateParentCombo()
{
    m_parentCombo->clear();
    m_parentCombo->addItem(QStringLiteral("无（根目录）"), INVALID_ID);
    const auto &all = LibraryManager::getInstance().getAllCatalogs();
    for (const auto &pair : all) {
        const Catalog &c = pair.second;
        QString indent(c.getLevel() * 2, ' ');
        QString text = QString("%1[%2] %3")
            .arg(indent)
            .arg(c.getId())
            .arg(QString::fromStdString(c.getName()));
        m_parentCombo->addItem(text, c.getId());
    }
}

void CatalogDialog::setFixedParent(IdType parentId)
{
    m_fixedParent = true;
    int idx = m_parentCombo->findData(parentId);
    if (idx >= 0) m_parentCombo->setCurrentIndex(idx);
    m_parentCombo->setEnabled(false);
}

Catalog CatalogDialog::getEntity() const
{
    Catalog cat;
    cat.setName(m_nameEdit->text().toStdString());
    cat.setDescription(m_descEdit->text().toStdString());
    cat.setParentId(m_parentCombo->currentData().toInt());
    return cat;
}

void CatalogDialog::setEntity(const Catalog &cat)
{
    m_nameEdit->setText(QString::fromStdString(cat.getName()));
    m_descEdit->setText(QString::fromStdString(cat.getDescription()));
    int idx = m_parentCombo->findData(cat.getParentId());
    if (idx >= 0) m_parentCombo->setCurrentIndex(idx);
}
