#include "attachmentdialog.h"

#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QMessageBox>

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

    m_filePathEdit = new QLineEdit;
    m_filePathEdit->setReadOnly(true);
    m_filePathEdit->setPlaceholderText(QStringLiteral("未选择文件"));
    m_btnSelectFile = new QPushButton(QStringLiteral("选择附件文件..."));
    QHBoxLayout *fileLayout = new QHBoxLayout;
    fileLayout->addWidget(m_filePathEdit, 1);
    fileLayout->addWidget(m_btnSelectFile);

    form->addRow(QStringLiteral("名称:"), m_nameEdit);
    form->addRow(QStringLiteral("所属文献:"), m_paperCombo);
    form->addRow(QStringLiteral("附件文件:"), fileLayout);
    form->addRow(QStringLiteral("备注/说明:"), m_contentEdit);

    mainLayout->addLayout(form);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttonBox->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确定"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_btnSelectFile, &QPushButton::clicked, this, &AttachmentDialog::onSelectFile);
    mainLayout->addWidget(buttonBox);
}

void AttachmentDialog::onSelectFile()
{
    QString sourcePath = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("选择附件文件"),
        QString(),
        QStringLiteral("所有文件 (*)"));
    if (sourcePath.isEmpty()) {
        return;
    }

    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dataDir.isEmpty()) {
        dataDir = QCoreApplication::applicationDirPath() + QStringLiteral("/data");
    }
    QDir dir(dataDir);
    dir.mkpath(QStringLiteral("attachments"));
    dir.cd(QStringLiteral("attachments"));

    QFileInfo info(sourcePath);
    QString targetName = info.fileName();
    QString targetPath = dir.filePath(targetName);
    if (QFile::exists(targetPath)) {
        targetName = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMddhhmmsszzz_"))
            + info.fileName();
        targetPath = dir.filePath(targetName);
    }

    if (QFileInfo(sourcePath).absoluteFilePath() != QFileInfo(targetPath).absoluteFilePath()) {
        if (!QFile::copy(sourcePath, targetPath)) {
            QMessageBox::warning(this,
                QStringLiteral("上传失败"),
                QStringLiteral("无法复制附件文件，请检查文件是否被占用或路径是否可写。"));
            return;
        }
    }

    m_filePathEdit->setText(QDir::toNativeSeparators(targetPath));
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
    att.setFilePath(QDir::fromNativeSeparators(m_filePathEdit->text()).toStdString());
    return att;
}

void AttachmentDialog::setEntity(const Attachment &att)
{
    m_nameEdit->setText(QString::fromStdString(att.getName()));
    int idx = m_paperCombo->findData(att.getPaperId());
    if (idx >= 0) m_paperCombo->setCurrentIndex(idx);
    m_contentEdit->setText(QString::fromStdString(att.getContent()));
    m_filePathEdit->setText(QDir::toNativeSeparators(QString::fromStdString(att.getFilePath())));
}
