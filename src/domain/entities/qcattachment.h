#ifndef QTCLIP_QCATTACHMENT_H_
#define QTCLIP_QCATTACHMENT_H_

// File: qcattachment.h
// Author: ZZMI1
// Created: 2026-03-23
// Description: Declares the attachment entity used by the QtClip domain layer.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include <QDateTime>
#include <QString>

class QCAttachment
{
public:
    QCAttachment();
    ~QCAttachment();

    qint64 id() const;
    void setId(qint64 nId);

    qint64 snippetId() const;
    void setSnippetId(qint64 nSnippetId);

    QString filePath() const;
    void setFilePath(const QString& strFilePath);

    QString fileName() const;
    void setFileName(const QString& strFileName);

    QString mimeType() const;
    void setMimeType(const QString& strMimeType);

    QDateTime createdAt() const;
    void setCreatedAt(const QDateTime& dateTimeCreatedAt);

private:
    qint64 m_nId;
    qint64 m_nSnippetId;
    QString m_strFilePath;
    QString m_strFileName;
    QString m_strMimeType;
    QDateTime m_dateTimeCreatedAt;
};

#endif // QTCLIP_QCATTACHMENT_H_
