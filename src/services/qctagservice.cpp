// File: qctagservice.cpp
// Author: ZZMI1
// Created: 2026-03-23
// Description: Implements the thin tag service used by the QtClip workflow.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include "qctagservice.h"

#include <QDateTime>
#include <QSet>

QCTagService::QCTagService(IQCTagRepository *pTagRepository)
    : m_pTagRepository(pTagRepository)
    , m_strLastError()
{
}

QCTagService::~QCTagService()
{
}

bool QCTagService::createTag(QCTag *pTag)
{
    clearError();

    if (nullptr == pTag)
    {
        setLastError(QString::fromUtf8("Tag output pointer is null."));
        return false;
    }

    if (nullptr == m_pTagRepository)
    {
        setLastError(QString::fromUtf8("Tag repository is null."));
        return false;
    }

    pTag->setName(pTag->name().trimmed());
    if (pTag->name().isEmpty())
    {
        setLastError(QString::fromUtf8("Tag name cannot be empty."));
        return false;
    }

    if (!pTag->createdAt().isValid())
        pTag->setCreatedAt(QDateTime::currentDateTimeUtc());

    qint64 nTagId = 0;
    if (!m_pTagRepository->createTag(*pTag, &nTagId))
    {
        setLastError(QString::fromUtf8("Failed to create tag."));
        return false;
    }

    pTag->setId(nTagId);
    return true;
}

bool QCTagService::updateTag(QCTag *pTag)
{
    clearError();

    if (nullptr == pTag)
    {
        setLastError(QString::fromUtf8("Tag output pointer is null."));
        return false;
    }

    if (nullptr == m_pTagRepository)
    {
        setLastError(QString::fromUtf8("Tag repository is null."));
        return false;
    }

    if (pTag->id() <= 0)
    {
        setLastError(QString::fromUtf8("Tag id is invalid."));
        return false;
    }

    pTag->setName(pTag->name().trimmed());
    if (pTag->name().isEmpty())
    {
        setLastError(QString::fromUtf8("Tag name cannot be empty."));
        return false;
    }

    if (!m_pTagRepository->updateTag(*pTag))
    {
        setLastError(QString::fromUtf8("Failed to update tag."));
        return false;
    }

    return true;
}

bool QCTagService::deleteTag(qint64 nTagId)
{
    clearError();

    if (nullptr == m_pTagRepository)
    {
        setLastError(QString::fromUtf8("Tag repository is null."));
        return false;
    }

    if (nTagId <= 0)
    {
        setLastError(QString::fromUtf8("Tag id is invalid."));
        return false;
    }

    if (!m_pTagRepository->deleteTag(nTagId))
    {
        setLastError(QString::fromUtf8("Failed to delete tag."));
        return false;
    }

    return true;
}

bool QCTagService::getTagById(qint64 nTagId, QCTag *pTag) const
{
    clearError();

    if (nullptr == m_pTagRepository)
    {
        setLastError(QString::fromUtf8("Tag repository is null."));
        return false;
    }

    if (nTagId <= 0)
    {
        setLastError(QString::fromUtf8("Tag id is invalid."));
        return false;
    }

    if (!m_pTagRepository->getTagById(nTagId, pTag))
    {
        setLastError(QString::fromUtf8("Failed to load tag by id."));
        return false;
    }

    return true;
}

bool QCTagService::getTagByName(const QString& strName, QCTag *pTag) const
{
    clearError();

    if (nullptr == m_pTagRepository)
    {
        setLastError(QString::fromUtf8("Tag repository is null."));
        return false;
    }

    const QString strTrimmedName = strName.trimmed();
    if (strTrimmedName.isEmpty())
    {
        setLastError(QString::fromUtf8("Tag name cannot be empty."));
        return false;
    }

    if (!m_pTagRepository->getTagByName(strTrimmedName, pTag))
    {
        setLastError(QString::fromUtf8("Failed to load tag by name."));
        return false;
    }

    return true;
}

QVector<QCTag> QCTagService::listTags() const
{
    clearError();

    if (nullptr == m_pTagRepository)
    {
        setLastError(QString::fromUtf8("Tag repository is null."));
        return QVector<QCTag>();
    }

    return m_pTagRepository->listTags();
}

QVector<QCTag> QCTagService::listTagsBySnippet(qint64 nSnippetId) const
{
    clearError();

    if (nullptr == m_pTagRepository)
    {
        setLastError(QString::fromUtf8("Tag repository is null."));
        return QVector<QCTag>();
    }

    if (nSnippetId <= 0)
    {
        setLastError(QString::fromUtf8("Snippet id is invalid."));
        return QVector<QCTag>();
    }

    return m_pTagRepository->listTagsBySnippet(nSnippetId);
}

bool QCTagService::replaceSnippetTags(qint64 nSnippetId, const QVector<qint64>& vecTagIds)
{
    clearError();

    if (nullptr == m_pTagRepository)
    {
        setLastError(QString::fromUtf8("Tag repository is null."));
        return false;
    }

    if (nSnippetId <= 0)
    {
        setLastError(QString::fromUtf8("Snippet id is invalid."));
        return false;
    }

    QSet<qint64> setUniqueIds;
    QVector<qint64> vecNormalizedIds;
    for (int i = 0; i < vecTagIds.size(); ++i)
    {
        const qint64 nTagId = vecTagIds.at(i);
        if (nTagId <= 0)
        {
            setLastError(QString::fromUtf8("Tag id is invalid."));
            return false;
        }

        if (!setUniqueIds.contains(nTagId))
        {
            setUniqueIds.insert(nTagId);
            vecNormalizedIds.append(nTagId);
        }
    }

    if (!m_pTagRepository->replaceSnippetTags(nSnippetId, vecNormalizedIds))
    {
        setLastError(QString::fromUtf8("Failed to replace snippet tags."));
        return false;
    }

    return true;
}

QString QCTagService::lastError() const
{
    return m_strLastError;
}

void QCTagService::clearError() const
{
    m_strLastError.clear();
}

void QCTagService::setLastError(const QString& strError) const
{
    m_strLastError = strError;
}
