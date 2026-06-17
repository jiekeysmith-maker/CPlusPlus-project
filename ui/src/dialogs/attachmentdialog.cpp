#include "attachmentdialog.h"

#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>

AttachmentDialog::AttachmentDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi();
    setModal(true);
    setMinimumWidth(450);
}

void AttachmentDialog::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QFormLayout *form = new QFormLayout;
    m_nameEdit   = new QLineEdit;
    m_paperCombo = new QComboBox;
    m_contentEdit = new QTextEdit;
    m_contentEdit->setMinimumHeight(120);

    // 填充文献下拉框
    const auto &papers = LibraryManager::getInstance().getAllPapers();
    m_paperCombo->addItem(QStringLiteral("-- 请选择文献 --"), INVALID_ID);
    for (const auto &pair : papers) {
        QString text = QString("[%1] %2")
            .arg(pair.first)
            .arg(QString::fromStdString(pair.second.getTitle()));
        m_paperCombo->addItem(text, pair.first);
    }

    form->addRow(QStringLiteral("名称:"), m_nameEdit);
    form->addRow(QStringLiteral("所属文献:"), m_paperCombo);
    form->addRow(QStringLiteral("内容:"), m_contentEdit);

    mainLayout->addLayout(form);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttonBox->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确定"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

void AttachmentDialog::accept()
{
    if (m_nameEdit->text().trimmed().isEmpty()) {
        m_nameEdit->setFocus();
        return;
    }
    if (m_paperCombo->currentData().toInt() == INVALID_ID) {
        m_paperCombo->setFocus();
        return;
    }
    QDialog::accept();
}

Attachment AttachmentDialog::getEntity() const
{
    Attachment att;
    att.setName(m_nameEdit->text().toStdString());
    att.setPaperId(m_paperCombo->currentData().toInt());
    att.setContent(m_contentEdit->toPlainText().toStdString());
    return att;
}

void AttachmentDialog::setEntity(const Attachment &att)
{
    m_nameEdit->setText(QString::fromStdString(att.getName()));
    int idx = m_paperCombo->findData(att.getPaperId());
    if (idx >= 0) m_paperCombo->setCurrentIndex(idx);
    m_contentEdit->setText(QString::fromStdString(att.getContent()));
}
