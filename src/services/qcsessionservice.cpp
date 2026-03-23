// File: qcsessionservice.cpp
// Author: ZZMI1
// Created: 2026-03-23
// Description: Implements the thin study session service used to drive the first QtClip workflow.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include "qcsessionservice.h"

#include <QDateTime>

namespace
{
QDateTime CurrentTimestamp()
{
    return QDateTime::currentDateTimeUtc();
}
}

QCSessionService::QCSessionService(IQCSessionRepository *pSessionRepository)
    : m_pSessionRepository(pSessionRepository)
    , m_strLastError()
{
}

QCSessionService::~QCSessionService()
{
}

bool QCSessionService::createSession(QCStudySession *pSession)
{
    clearError();

    if (nullptr == pSession)
    {
        setLastError(QString::fromUtf8("QCStudySession input pointer is null."));
        return false;
    }

    if (nullptr == m_pSessionRepository)
    {
        setLastError(QString::fromUtf8("Session repository is null."));
        return false;
    }

    if (pSession->title().trimmed().isEmpty())
    {
        setLastError(QString::fromUtf8("Study session title is empty."));
        return false;
    }

    if (!pSession->startedAt().isValid())
        pSession->setStartedAt(CurrentTimestamp());

    if (!pSession->createdAt().isValid())
        pSession->setCreatedAt(pSession->startedAt());

    if (!pSession->updatedAt().isValid())
        pSession->setUpdatedAt(pSession->createdAt());

    pSession->setStatus(QCSessionStatus::ActiveSessionStatus);

    qint64 nSessionId = 0;
    if (!m_pSessionRepository->createSession(*pSession, &nSessionId))
    {
        setLastError(QString::fromUtf8("Failed to create study session."));
        return false;
    }

    pSession->setId(nSessionId);
    return true;
}

bool QCSessionService::finishSession(qint64 nSessionId, const QDateTime& dateTimeEndedAt)
{
    clearError();

    if (nullptr == m_pSessionRepository)
    {
        setLastError(QString::fromUtf8("Session repository is null."));
        return false;
    }

    if (nSessionId <= 0)
    {
        setLastError(QString::fromUtf8("Study session id is invalid."));
        return false;
    }

    if (!dateTimeEndedAt.isValid())
    {
        setLastError(QString::fromUtf8("Study session endedAt is invalid."));
        return false;
    }

    if (!m_pSessionRepository->finishSession(nSessionId, dateTimeEndedAt))
    {
        setLastError(QString::fromUtf8("Failed to finish study session."));
        return false;
    }

    return true;
}

bool QCSessionService::getSessionById(qint64 nSessionId, QCStudySession *pSession) const
{
    clearError();

    if (nullptr == pSession)
    {
        setLastError(QString::fromUtf8("QCStudySession output pointer is null."));
        return false;
    }

    if (nullptr == m_pSessionRepository)
    {
        setLastError(QString::fromUtf8("Session repository is null."));
        return false;
    }

    if (nSessionId <= 0)
    {
        setLastError(QString::fromUtf8("Study session id is invalid."));
        return false;
    }

    if (!m_pSessionRepository->getSessionById(nSessionId, pSession))
    {
        setLastError(QString::fromUtf8("Study session was not found."));
        return false;
    }

    return true;
}

bool QCSessionService::getActiveSession(QCStudySession *pSession) const
{
    clearError();

    if (nullptr == pSession)
    {
        setLastError(QString::fromUtf8("QCStudySession output pointer is null."));
        return false;
    }

    if (nullptr == m_pSessionRepository)
    {
        setLastError(QString::fromUtf8("Session repository is null."));
        return false;
    }

    if (!m_pSessionRepository->getActiveSession(pSession))
    {
        setLastError(QString::fromUtf8("No active study session was found."));
        return false;
    }

    return true;
}

QVector<QCStudySession> QCSessionService::listSessions() const
{
    clearError();

    if (nullptr == m_pSessionRepository)
    {
        setLastError(QString::fromUtf8("Session repository is null."));
        return QVector<QCStudySession>();
    }

    return m_pSessionRepository->listSessions();
}

QString QCSessionService::lastError() const
{
    return m_strLastError;
}

void QCSessionService::clearError() const
{
    m_strLastError.clear();
}

void QCSessionService::setLastError(const QString& strError) const
{
    m_strLastError = strError;
}
