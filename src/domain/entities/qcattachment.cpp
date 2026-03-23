// File: qcattachment.cpp
// Author: ZZMI1
// Created: 2026-03-23
// Description: Implements the attachment entity used by the QtClip domain layer.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include "qcattachment.h"

QCAttachment::QCAttachment()
    : m_nId(0)
    , m_nSnippetId(0)
    , m_strFilePath()
    , m_strFileName()
    , m_strMimeType()
    , m_dateTimeCreatedAt()
{
}

QCAttachment::~QCAttachment()
{
}

qint64 QCAttachment::id() const
{
    return m_nId;
}

void QCAttachment::setId(qint64 nId)
{
    m_nId = nId;
}

qint64 QCAttachment::snippetId() const
{
    return m_nSnippetId;
}

void QCAttachment::setSnippetId(qint64 nSnippetId)
{
    m_nSnippetId = nSnippetId;
}

QString QCAttachment::filePath() const
{
    return m_strFilePath;
}

void QCAttachment::setFilePath(const QString& strFilePath)
{
    m_strFilePath = strFilePath;
}

QString QCAttachment::fileName() const
{
    return m_strFileName;
}

void QCAttachment::setFileName(const QString& strFileName)
{
    m_strFileName = strFileName;
}

QString QCAttachment::mimeType() const
{
    return m_strMimeType;
}

void QCAttachment::setMimeType(const QString& strMimeType)
{
    m_strMimeType = strMimeType;
}

QDateTime QCAttachment::createdAt() const
{
    return m_dateTimeCreatedAt;
}

void QCAttachment::setCreatedAt(const QDateTime& dateTimeCreatedAt)
{
    m_dateTimeCreatedAt = dateTimeCreatedAt;
}
