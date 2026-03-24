// File: qcsnippetservice.cpp
// Author: ZZMI1
// Created: 2026-03-23
// Description: Implements the thin snippet service used to drive the first QtClip workflow.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include "qcsnippetservice.h"

#include <QDateTime>
#include <QSet>

namespace
{
QDateTime CurrentTimestamp()
{
    return QDateTime::currentDateTimeUtc();
}

QString BuildTimestampTitleSuffix(const QDateTime& dateTimeValue)
{
    return dateTimeValue.isValid()
        ? dateTimeValue.toLocalTime().toString(QString::fromUtf8("yyyy-MM-dd hh:mm"))
        : QString::fromUtf8("Unknown Time");
}

QString FirstMeaningfulLine(const QString& strText)
{
    const QStringList vecLines = strText.split('\n');
    for (int i = 0; i < vecLines.size(); ++i)
    {
        QString strLine = vecLines.at(i);
        strLine.replace(QString::fromUtf8("\r"), QString());
        strLine = strLine.trimmed();
        if (!strLine.isEmpty())
            return strLine;
    }

    return QString();
}
}

QCSnippetService::QCSnippetService(IQCSnippetRepository *pSnippetRepository)
    : m_pSnippetRepository(pSnippetRepository)
    , m_strLastError()
{
}

QCSnippetService::~QCSnippetService()
{
}

bool QCSnippetService::createTextSnippet(QCSnippet *pSnippet)
{
    clearError();

    if (nullptr == pSnippet)
    {
        setLastError(QString::fromUtf8("QCSnippet input pointer is null."));
        return false;
    }

    if (nullptr == m_pSnippetRepository)
    {
        setLastError(QString::fromUtf8("Snippet repository is null."));
        return false;
    }

    pSnippet->setType(QCSnippetType::TextSnippetType);
    normalizeSnippetForCreate(pSnippet);
    if (!validateSnippetInput(*pSnippet))
        return false;

    qint64 nSnippetId = 0;
    if (!m_pSnippetRepository->createSnippet(*pSnippet, &nSnippetId))
    {
        setLastError(QString::fromUtf8("Failed to create text snippet."));
        return false;
    }

    pSnippet->setId(nSnippetId);
    return true;
}

bool QCSnippetService::createCodeSnippet(QCSnippet *pSnippet)
{
    clearError();

    if (nullptr == pSnippet)
    {
        setLastError(QString::fromUtf8("QCSnippet input pointer is null."));
        return false;
    }

    if (nullptr == m_pSnippetRepository)
    {
        setLastError(QString::fromUtf8("Snippet repository is null."));
        return false;
    }

    pSnippet->setType(QCSnippetType::CodeSnippetType);
    normalizeSnippetForCreate(pSnippet);
    if (!validateSnippetInput(*pSnippet))
        return false;

    qint64 nSnippetId = 0;
    if (!m_pSnippetRepository->createSnippet(*pSnippet, &nSnippetId))
    {
        setLastError(QString::fromUtf8("Failed to create code snippet."));
        return false;
    }

    pSnippet->setId(nSnippetId);
    return true;
}

bool QCSnippetService::createImageSnippetWithPrimaryAttachment(QCSnippet *pSnippet,
                                                               QCAttachment *pPrimaryAttachment)
{
    clearError();

    if (nullptr == pSnippet || nullptr == pPrimaryAttachment)
    {
        setLastError(QString::fromUtf8("Image snippet input is invalid."));
        return false;
    }

    if (nullptr == m_pSnippetRepository)
    {
        setLastError(QString::fromUtf8("Snippet repository is null."));
        return false;
    }

    if (pSnippet->type() != QCSnippetType::ImageSnippetType)
    {
        setLastError(QString::fromUtf8("Image snippet service requires snippet.type to be ImageSnippetType."));
        return false;
    }

    if (pSnippet->sessionId() <= 0)
    {
        setLastError(QString::fromUtf8("Image snippet sessionId is invalid."));
        return false;
    }

    if (pPrimaryAttachment->filePath().trimmed().isEmpty())
    {
        setLastError(QString::fromUtf8("Primary attachment file path is empty."));
        return false;
    }

    normalizeSnippetForCreate(pSnippet);
    if (!validateSnippetInput(*pSnippet))
        return false;

    if (!pPrimaryAttachment->createdAt().isValid())
        pPrimaryAttachment->setCreatedAt(pSnippet->createdAt());

    qint64 nSnippetId = 0;
    if (!m_pSnippetRepository->createSnippetWithPrimaryAttachment(*pSnippet, *pPrimaryAttachment, &nSnippetId))
    {
        setLastError(QString::fromUtf8("Failed to create image snippet with primary attachment."));
        return false;
    }

    pSnippet->setId(nSnippetId);
    pPrimaryAttachment->setSnippetId(nSnippetId);
    return true;
}

bool QCSnippetService::updateSnippet(QCSnippet *pSnippet)
{
    clearError();

    if (nullptr == pSnippet)
    {
        setLastError(QString::fromUtf8("QCSnippet input pointer is null."));
        return false;
    }

    if (nullptr == m_pSnippetRepository)
    {
        setLastError(QString::fromUtf8("Snippet repository is null."));
        return false;
    }

    if (pSnippet->id() <= 0)
    {
        setLastError(QString::fromUtf8("Snippet id is invalid."));
        return false;
    }

    if (!pSnippet->updatedAt().isValid())
        pSnippet->setUpdatedAt(CurrentTimestamp());

    if (!validateSnippetInput(*pSnippet))
        return false;

    if (!m_pSnippetRepository->updateSnippet(*pSnippet))
    {
        setLastError(QString::fromUtf8("Failed to update snippet."));
        return false;
    }

    return true;
}


bool QCSnippetService::duplicateSnippet(qint64 nSnippetId, qint64 nTargetSessionId, qint64 *pnNewSnippetId)
{
    clearError();

    if (nullptr == pnNewSnippetId)
    {
        setLastError(QString::fromUtf8("Duplicate snippet output pointer is null."));
        return false;
    }

    *pnNewSnippetId = 0;

    if (nTargetSessionId <= 0)
    {
        setLastError(QString::fromUtf8("Target session id is invalid."));
        return false;
    }

    QCSnippet sourceSnippet;
    if (!getSnippetById(nSnippetId, &sourceSnippet))
        return false;

    QCSnippet duplicateSnippet = sourceSnippet;
    duplicateSnippet.setId(0);
    duplicateSnippet.setSessionId(nTargetSessionId);
    duplicateSnippet.setIsArchived(false);
    duplicateSnippet.setCreatedAt(QDateTime());
    duplicateSnippet.setUpdatedAt(QDateTime());
    duplicateSnippet.setCapturedAt(CurrentTimestamp());

    const QString strBaseTitle = sourceSnippet.title().trimmed().isEmpty()
        ? buildDefaultTitle(sourceSnippet)
        : sourceSnippet.title().trimmed();
    duplicateSnippet.setTitle(QString::fromUtf8("%1 (Copy)").arg(strBaseTitle));

    if (sourceSnippet.type() == QCSnippetType::ImageSnippetType)
    {
        QCAttachment primaryAttachment;
        if (!getPrimaryAttachmentBySnippetId(nSnippetId, &primaryAttachment))
            return false;

        primaryAttachment.setId(0);
        primaryAttachment.setSnippetId(0);
        primaryAttachment.setCreatedAt(CurrentTimestamp());
        if (!createImageSnippetWithPrimaryAttachment(&duplicateSnippet, &primaryAttachment))
            return false;
    }
    else if (sourceSnippet.type() == QCSnippetType::CodeSnippetType)
    {
        if (!createCodeSnippet(&duplicateSnippet))
            return false;
    }
    else
    {
        if (!createTextSnippet(&duplicateSnippet))
            return false;
    }

    *pnNewSnippetId = duplicateSnippet.id();
    return true;
}

bool QCSnippetService::moveSnippetsToSession(const QVector<qint64>& vecSnippetIds, qint64 nTargetSessionId)
{
    clearError();

    if (nTargetSessionId <= 0)
    {
        setLastError(QString::fromUtf8("Target session id is invalid."));
        return false;
    }

    if (vecSnippetIds.isEmpty())
    {
        setLastError(QString::fromUtf8("Snippet selection is empty."));
        return false;
    }

    QSet<qint64> setUniqueSnippetIds;
    int nMovedCount = 0;
    for (int i = 0; i < vecSnippetIds.size(); ++i)
    {
        const qint64 nSnippetId = vecSnippetIds.at(i);
        if (nSnippetId <= 0)
        {
            setLastError(QString::fromUtf8("Snippet id is invalid."));
            return false;
        }

        if (setUniqueSnippetIds.contains(nSnippetId))
            continue;

        setUniqueSnippetIds.insert(nSnippetId);

        QCSnippet snippet;
        if (!getSnippetById(nSnippetId, &snippet))
            return false;

        if (snippet.sessionId() == nTargetSessionId)
            continue;

        snippet.setSessionId(nTargetSessionId);
        if (!updateSnippet(&snippet))
            return false;

        ++nMovedCount;
    }

    if (nMovedCount <= 0)
    {
        setLastError(QString::fromUtf8("Selected snippets already belong to the target session."));
        return false;
    }

    return true;
}

bool QCSnippetService::deleteSnippet(qint64 nSnippetId)
{
    clearError();

    if (nullptr == m_pSnippetRepository)
    {
        setLastError(QString::fromUtf8("Snippet repository is null."));
        return false;
    }

    if (nSnippetId <= 0)
    {
        setLastError(QString::fromUtf8("Snippet id is invalid."));
        return false;
    }

    if (!m_pSnippetRepository->deleteSnippet(nSnippetId))
    {
        setLastError(QString::fromUtf8("Failed to delete snippet."));
        return false;
    }

    return true;
}

bool QCSnippetService::getSnippetById(qint64 nSnippetId, QCSnippet *pSnippet) const
{
    clearError();

    if (nullptr == pSnippet)
    {
        setLastError(QString::fromUtf8("QCSnippet output pointer is null."));
        return false;
    }

    if (nullptr == m_pSnippetRepository)
    {
        setLastError(QString::fromUtf8("Snippet repository is null."));
        return false;
    }

    if (nSnippetId <= 0)
    {
        setLastError(QString::fromUtf8("Snippet id is invalid."));
        return false;
    }

    if (!m_pSnippetRepository->getSnippetById(nSnippetId, pSnippet))
    {
        setLastError(QString::fromUtf8("Snippet was not found."));
        return false;
    }

    return true;
}

bool QCSnippetService::getPrimaryAttachmentBySnippetId(qint64 nSnippetId,
                                                       QCAttachment *pPrimaryAttachment) const
{
    clearError();

    if (nullptr == pPrimaryAttachment)
    {
        setLastError(QString::fromUtf8("QCAttachment output pointer is null."));
        return false;
    }

    if (nullptr == m_pSnippetRepository)
    {
        setLastError(QString::fromUtf8("Snippet repository is null."));
        return false;
    }

    if (nSnippetId <= 0)
    {
        setLastError(QString::fromUtf8("Snippet id is invalid."));
        return false;
    }

    if (!m_pSnippetRepository->getPrimaryAttachmentBySnippetId(nSnippetId, pPrimaryAttachment))
    {
        setLastError(QString::fromUtf8("Primary attachment was not found."));
        return false;
    }

    return true;
}

QVector<QCSnippet> QCSnippetService::listSnippetsBySession(qint64 nSessionId) const
{
    clearError();

    if (nullptr == m_pSnippetRepository)
    {
        setLastError(QString::fromUtf8("Snippet repository is null."));
        return QVector<QCSnippet>();
    }

    if (nSessionId <= 0)
    {
        setLastError(QString::fromUtf8("Study session id is invalid."));
        return QVector<QCSnippet>();
    }

    return m_pSnippetRepository->listSnippetsBySession(nSessionId);
}

QVector<QCSnippet> QCSnippetService::querySnippets(qint64 nSessionId,
                                                  const QString& strKeyword,
                                                  bool bFavoriteOnly,
                                                  bool bReviewOnly,
                                                  qint64 nTagId) const
{
    clearError();

    if (nullptr == m_pSnippetRepository)
    {
        setLastError(QString::fromUtf8("Snippet repository is null."));
        return QVector<QCSnippet>();
    }

    if (nSessionId <= 0)
    {
        setLastError(QString::fromUtf8("Study session id is invalid."));
        return QVector<QCSnippet>();
    }

    if (nTagId < 0)
    {
        setLastError(QString::fromUtf8("Tag id is invalid."));
        return QVector<QCSnippet>();
    }

    return m_pSnippetRepository->querySnippets(nSessionId,
                                               strKeyword.trimmed(),
                                               bFavoriteOnly,
                                               bReviewOnly,
                                               nTagId);
}
bool QCSnippetService::setFavorite(qint64 nSnippetId, bool bIsFavorite)
{
    clearError();

    if (nullptr == m_pSnippetRepository)
    {
        setLastError(QString::fromUtf8("Snippet repository is null."));
        return false;
    }

    if (nSnippetId <= 0)
    {
        setLastError(QString::fromUtf8("Snippet id is invalid."));
        return false;
    }

    if (!m_pSnippetRepository->setFavorite(nSnippetId, bIsFavorite))
    {
        setLastError(QString::fromUtf8("Failed to update snippet favorite state."));
        return false;
    }

    return true;
}

bool QCSnippetService::setReviewState(qint64 nSnippetId, bool bIsReview)
{
    clearError();

    if (nSnippetId <= 0)
    {
        setLastError(QString::fromUtf8("Snippet id is invalid."));
        return false;
    }

    QCSnippet snippet;
    if (!getSnippetById(nSnippetId, &snippet))
        return false;

    snippet.setNoteLevel(bIsReview ? QCNoteLevel::ReviewNoteLevel : QCNoteLevel::ImportantNoteLevel);
    return updateSnippet(&snippet);
}
bool QCSnippetService::setArchived(qint64 nSnippetId, bool bIsArchived)
{
    clearError();

    if (nullptr == m_pSnippetRepository)
    {
        setLastError(QString::fromUtf8("Snippet repository is null."));
        return false;
    }

    if (nSnippetId <= 0)
    {
        setLastError(QString::fromUtf8("Snippet id is invalid."));
        return false;
    }

    if (!m_pSnippetRepository->setArchived(nSnippetId, bIsArchived))
    {
        setLastError(QString::fromUtf8("Failed to update snippet archived state."));
        return false;
    }

    return true;
}

QString QCSnippetService::lastError() const
{
    return m_strLastError;
}

void QCSnippetService::clearError() const
{
    m_strLastError.clear();
}

bool QCSnippetService::validateSnippetInput(const QCSnippet& snippet) const
{
    if (snippet.id() < 0)
    {
        setLastError(QString::fromUtf8("Snippet id is invalid."));
        return false;
    }

    if (snippet.sessionId() <= 0)
    {
        setLastError(QString::fromUtf8("Snippet sessionId is invalid."));
        return false;
    }

    if (!snippet.capturedAt().isValid() || !snippet.createdAt().isValid() || !snippet.updatedAt().isValid())
    {
        setLastError(QString::fromUtf8("Snippet time fields are invalid."));
        return false;
    }

    return true;
}

QString QCSnippetService::buildDefaultTitle(const QCSnippet& snippet) const
{
    const QString strTimestampSuffix = BuildTimestampTitleSuffix(snippet.capturedAt().isValid()
        ? snippet.capturedAt()
        : snippet.createdAt());
    const QString strSource = snippet.source().trimmed().toLower();

    if (snippet.type() == QCSnippetType::ImageSnippetType)
    {
        if (strSource == QString::fromUtf8("screen"))
            return QString::fromUtf8("Screen Capture %1").arg(strTimestampSuffix);
        if (strSource == QString::fromUtf8("region"))
            return QString::fromUtf8("Region Capture %1").arg(strTimestampSuffix);
        if (strSource == QString::fromUtf8("file"))
            return QString::fromUtf8("Imported Image %1").arg(strTimestampSuffix);
        return QString::fromUtf8("Image Snippet %1").arg(strTimestampSuffix);
    }

    if (snippet.type() == QCSnippetType::TextSnippetType)
    {
        const QString strContentPreview = FirstMeaningfulLine(snippet.contentText()).left(48).trimmed();
        if (!strContentPreview.isEmpty())
            return strContentPreview;

        return QString::fromUtf8("Text Snippet %1").arg(strTimestampSuffix);
    }

    if (snippet.type() == QCSnippetType::CodeSnippetType)
        return QString::fromUtf8("Code Snippet %1").arg(strTimestampSuffix);

    return QString::fromUtf8("Snippet %1").arg(strTimestampSuffix);
}

void QCSnippetService::normalizeSnippetForCreate(QCSnippet *pSnippet) const
{
    if (nullptr == pSnippet)
        return;

    const QDateTime dateTimeNow = CurrentTimestamp();
    if (!pSnippet->capturedAt().isValid())
        pSnippet->setCapturedAt(dateTimeNow);

    if (!pSnippet->createdAt().isValid())
        pSnippet->setCreatedAt(pSnippet->capturedAt());

    if (!pSnippet->updatedAt().isValid())
        pSnippet->setUpdatedAt(pSnippet->createdAt());

    if (pSnippet->title().trimmed().isEmpty())
        pSnippet->setTitle(buildDefaultTitle(*pSnippet));
}

void QCSnippetService::setLastError(const QString& strError) const
{
    m_strLastError = strError;
}


