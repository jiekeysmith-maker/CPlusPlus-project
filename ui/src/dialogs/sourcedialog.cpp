#include "sourcedialog.h"

#include <QLineEdit>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QStackedWidget>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QLabel>

SourceDialog::SourceDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi();
    setModal(true);
    setMinimumWidth(450);
}

void SourceDialog::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QFormLayout *form = new QFormLayout;

    m_typeCombo = new QComboBox;
    m_typeCombo->addItem(QStringLiteral("期刊"), QStringLiteral("Journal"));
    m_typeCombo->addItem(QStringLiteral("会议"), QStringLiteral("Conference"));

    m_shortNameEdit     = new QLineEdit;
    m_fullNameEdit      = new QLineEdit;
    m_fieldEdit         = new QLineEdit;
    m_publisherEdit     = new QLineEdit;
    m_publisherAddrEdit = new QLineEdit;
    m_retrievalEdit     = new QLineEdit;
    m_retrievalEdit->setPlaceholderText(QStringLiteral("SCI/EI/其他"));
    m_remarkEdit        = new QLineEdit;

    // 类型特定字段
    m_impactFactorSpin = new QDoubleSpinBox;
    m_impactFactorSpin->setRange(0.0, 9999.999);
    m_impactFactorSpin->setDecimals(3);
    m_impactFactorSpin->setValue(0.0);

    m_meetingAddrEdit = new QLineEdit;

    QWidget *journalPage = new QWidget;
    QFormLayout *journalForm = new QFormLayout(journalPage);
    journalForm->setContentsMargins(0, 0, 0, 0);
    journalForm->addRow(QStringLiteral("影响因子:"), m_impactFactorSpin);

    QWidget *confPage = new QWidget;
    QFormLayout *confForm = new QFormLayout(confPage);
    confForm->setContentsMargins(0, 0, 0, 0);
    confForm->addRow(QStringLiteral("会议地址:"), m_meetingAddrEdit);

    m_typeStack = new QStackedWidget;
    m_typeStack->addWidget(journalPage);
    m_typeStack->addWidget(confPage);

    form->addRow(QStringLiteral("类型:"), m_typeCombo);
    form->addRow(QStringLiteral("简称:"), m_shortNameEdit);
    form->addRow(QStringLiteral("全名:"), m_fullNameEdit);
    form->addRow(QStringLiteral("领域:"), m_fieldEdit);
    form->addRow(QStringLiteral("出版单位:"), m_publisherEdit);
    form->addRow(QStringLiteral("出版单位地址:"), m_publisherAddrEdit);
    form->addRow(QStringLiteral("检索类型:"), m_retrievalEdit);
    form->addRow(QStringLiteral("备注:"), m_remarkEdit);
    form->addRow(m_typeStack);

    mainLayout->addLayout(form);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttonBox->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确定"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SourceDialog::onTypeChanged);
}

void SourceDialog::onTypeChanged(int index)
{
    m_typeStack->setCurrentIndex(index);
}

void SourceDialog::accept()
{
    if (m_shortNameEdit->text().trimmed().isEmpty()) {
        m_shortNameEdit->setFocus();
        return;
    }
    QDialog::accept();
}

void SourceDialog::setForceType(const QString& type)
{
    int idx = m_typeCombo->findData(type);
    if (idx >= 0) {
        m_typeCombo->setCurrentIndex(idx);
        m_typeCombo->setEnabled(false);
    }
}

std::shared_ptr<Source> SourceDialog::getSource() const
{
    if (m_typeCombo->currentData().toString() == "Journal") {
        auto j = std::make_shared<Journal>();
        j->setShortName(m_shortNameEdit->text().toStdString());
        j->setFullName(m_fullNameEdit->text().toStdString());
        j->setField(m_fieldEdit->text().toStdString());
        j->setPublisher(m_publisherEdit->text().toStdString());
        j->setPublisherAddress(m_publisherAddrEdit->text().toStdString());
        j->setRetrievalType(m_retrievalEdit->text().toStdString());
        j->setRemark(m_remarkEdit->text().toStdString());
        j->setImpactFactor(m_impactFactorSpin->value());
        return j;
    } else {
        auto c = std::make_shared<Conference>();
        c->setShortName(m_shortNameEdit->text().toStdString());
        c->setFullName(m_fullNameEdit->text().toStdString());
        c->setField(m_fieldEdit->text().toStdString());
        c->setPublisher(m_publisherEdit->text().toStdString());
        c->setPublisherAddress(m_publisherAddrEdit->text().toStdString());
        c->setRetrievalType(m_retrievalEdit->text().toStdString());
        c->setRemark(m_remarkEdit->text().toStdString());
        c->setMeetingAddress(m_meetingAddrEdit->text().toStdString());
        return c;
    }
}

void SourceDialog::setSource(const Source &src)
{
    // 设置类型
    int idx = m_typeCombo->findData(QString::fromStdString(src.getType()));
    if (idx >= 0) m_typeCombo->setCurrentIndex(idx);

    m_shortNameEdit->setText(QString::fromStdString(src.getShortName()));
    m_fullNameEdit->setText(QString::fromStdString(src.getFullName()));
    m_fieldEdit->setText(QString::fromStdString(src.getField()));
    m_publisherEdit->setText(QString::fromStdString(src.getPublisher()));
    m_publisherAddrEdit->setText(QString::fromStdString(src.getPublisherAddress()));
    m_retrievalEdit->setText(QString::fromStdString(src.getRetrievalType()));
    m_remarkEdit->setText(QString::fromStdString(src.getRemark()));

    if (src.getType() == "Journal") {
        const Journal &j = dynamic_cast<const Journal&>(src);
        m_impactFactorSpin->setValue(j.getImpactFactor());
    } else {
        const Conference &c = dynamic_cast<const Conference&>(src);
        m_meetingAddrEdit->setText(QString::fromStdString(c.getMeetingAddress()));
    }
}
