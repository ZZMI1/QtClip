// File: qcsnippetservice.cpp
// Author: ZZMI1
// Created: 2026-03-23
// Description: Implements the thin snippet service used to drive the first QtClip workflow.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include "qcsnippetservice.h"

#include <QDateTime>

namespace
{
QDateTime CurrentTimestamp()
{
    return QDateTime::currentDateTimeUtc();
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
}

void QCSnippetService::setLastError(const QString& strError) const
{
    m_strLastError = strError;
}


