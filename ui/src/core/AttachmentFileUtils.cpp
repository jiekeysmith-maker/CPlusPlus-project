#include "AttachmentFileUtils.h"

#include "Attachment.h"
#include "LibraryManager.h"
#include "StoragePaths.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

QString noteAttachmentFileFilter()
{
    return QStringLiteral("笔记文件 (*.doc *.docx *.pdf *.txt *.md);;Word 文件 (*.doc *.docx);;PDF 文件 (*.pdf);;文本文件 (*.txt *.md);;所有文件 (*)");
}

QString safeFolderNameFromPaperTitle(const Paper &paper)
{
    QString base = QString::fromStdString(paper.getTitle()).simplified().trimmed();
    const bool titleWasEmpty = base.isEmpty();

    const QString illegalChars = QStringLiteral("\\/:*?\"<>|");
    for (const QChar ch : illegalChars) {
        base.replace(ch, QLatin1Char('_'));
    }
    while (base.contains(QStringLiteral("__"))) {
        base.replace(QStringLiteral("__"), QStringLiteral("_"));
    }
    base = base.trimmed();
    if (base.isEmpty()) {
        base = QStringLiteral("未命名文献");
    }
    if (base.size() > 120) {
        base = base.left(120).trimmed();
    }
    if (titleWasEmpty) {
        return QStringLiteral("%1_%2").arg(base).arg(paper.getId());
    }
    return base;
}

QString attachmentDirectoryForPaper(const Paper &paper)
{
    QDir attachmentRoot(StoragePaths::attachmentsDirectoryPath());
    return attachmentRoot.filePath(safeFolderNameFromPaperTitle(paper));
}

QString uniqueAttachmentFilePath(const QString &dirPath, const QString &fileName)
{
    const QFileInfo info(fileName);
    QString baseName = info.completeBaseName();
    if (baseName.trimmed().isEmpty()) {
        baseName = QStringLiteral("note");
    }
    const QString suffix = info.suffix();
    QDir dir(dirPath);
    const QString originalName = info.fileName().trimmed().isEmpty()
        ? QStringLiteral("note")
        : info.fileName();
    QString candidate = dir.filePath(originalName);
    int index = 1;
    while (QFile::exists(candidate)) {
        const QString candidateName = suffix.isEmpty()
            ? QStringLiteral("%1_%2").arg(baseName).arg(index)
            : QStringLiteral("%1_%2.%3").arg(baseName).arg(index).arg(suffix);
        candidate = dir.filePath(candidateName);
        ++index;
    }
    return candidate;
}

AttachmentUploadResult uploadNoteAttachmentsForPaper(const Paper &paper, const QStringList &sourcePaths)
{
    AttachmentUploadResult result;
    if (paper.getId() == INVALID_ID) {
        for (const QString &sourcePath : sourcePaths) {
            const QFileInfo info(sourcePath);
            result.failedFiles << (info.fileName().isEmpty() ? sourcePath : info.fileName());
        }
        return result;
    }

    const QString targetDirPath = attachmentDirectoryForPaper(paper);
    QDir targetDir(targetDirPath);
    if (!targetDir.exists() && !QDir().mkpath(targetDirPath)) {
        for (const QString &sourcePath : sourcePaths) {
            const QFileInfo info(sourcePath);
            result.failedFiles << (info.fileName().isEmpty() ? sourcePath : info.fileName());
        }
        return result;
    }

    for (const QString &sourcePathRaw : sourcePaths) {
        const QString sourcePath = QDir::fromNativeSeparators(sourcePathRaw);
        QFileInfo sourceInfo(sourcePath);
        const QString displayName = sourceInfo.fileName().isEmpty()
            ? sourcePathRaw
            : sourceInfo.fileName();
        if (!sourceInfo.exists() || !sourceInfo.isFile()) {
            result.failedFiles << displayName;
            continue;
        }

        const QString targetPath = uniqueAttachmentFilePath(targetDirPath, sourceInfo.fileName());
        if (!QFile::copy(sourcePath, targetPath)) {
            result.failedFiles << displayName;
            continue;
        }

        Attachment attachment;
        attachment.setName(QFileInfo(targetPath).fileName().toStdString());
        attachment.setPaperId(paper.getId());
        attachment.setContent("笔记");
        attachment.setFilePath(QDir::fromNativeSeparators(targetPath).toStdString());
        const IdType attachmentId = LibraryManager::getInstance().addAttachment(attachment);
        result.attachmentIds.push_back(attachmentId);
        ++result.successCount;
    }

    return result;
}
