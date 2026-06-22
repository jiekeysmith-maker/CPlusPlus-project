#ifndef ATTACHMENT_FILE_UTILS_H
#define ATTACHMENT_FILE_UTILS_H

#include "CoreTypes.h"
#include "Paper.h"

#include <QString>
#include <QStringList>
#include <vector>

struct AttachmentUploadResult
{
    int successCount = 0;
    QStringList failedFiles;
    std::vector<IdType> attachmentIds;
};

QString noteAttachmentFileFilter();
QString safeFolderNameFromPaperTitle(const Paper &paper);
QString attachmentDirectoryForPaper(const Paper &paper);
QString uniqueAttachmentFilePath(const QString &dirPath, const QString &fileName);
AttachmentUploadResult uploadNoteAttachmentsForPaper(const Paper &paper, const QStringList &sourcePaths);

#endif // ATTACHMENT_FILE_UTILS_H
