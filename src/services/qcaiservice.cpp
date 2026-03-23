// File: qcaiservice.cpp
// Author: ZZMI1
// Created: 2026-03-23
// Description: Implements the thin AI record service used to drive the first QtClip workflow.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include "qcaiservice.h"

#include <QDateTime>

namespace
{
QDateTime CurrentTimestamp()
{
    return QDateTime::currentDateTimeUtc();
}
}

QCAiService::QCAiService(IQCAiRecordRepository *pAiRecordRepository)
    : m_pAiRecordRepository(pAiRecordRepository)
    , m_strLastError()
{
}

QCAiService::~QCAiService()
{
}

bool QCAiService::createAiRecord(QCAiRecord *pAiRecord)
{
    clearError();

    if (nullptr == pAiRecord)
    {
        setLastError(QString::fromUtf8("QCAiRecord input pointer is null."));
        return false;
    }

    if (nullptr == m_pAiRecordRepository)
    {
        setLastError(QString::fromUtf8("AI record repository is null."));
        return false;
    }

    if (!pAiRecord->createdAt().isValid())
        pAiRecord->setCreatedAt(CurrentTimestamp());

    if (!validateAiRecord(*pAiRecord))
        return false;

    qint64 nAiRecordId = 0;
    if (!m_pAiRecordRepository->createAiRecord(*pAiRecord, &nAiRecordId))
    {
        setLastError(QString::fromUtf8("Failed to create AI record."));
        return false;
    }

    pAiRecord->setId(nAiRecordId);
    return true;
}

bool QCAiService::updateAiRecord(const QCAiRecord& aiRecord)
{
    clearError();

    if (nullptr == m_pAiRecordRepository)
    {
        setLastError(QString::fromUtf8("AI record repository is null."));
        return false;
    }

    if (aiRecord.id() <= 0)
    {
        setLastError(QString::fromUtf8("AI record id is invalid."));
        return false;
    }

    if (!validateAiRecord(aiRecord))
        return false;

    if (!m_pAiRecordRepository->updateAiRecord(aiRecord))
    {
        setLastError(QString::fromUtf8("Failed to update AI record."));
        return false;
    }

    return true;
}

bool QCAiService::getAiRecordById(qint64 nAiRecordId, QCAiRecord *pAiRecord) const
{
    clearError();

    if (nullptr == pAiRecord)
    {
        setLastError(QString::fromUtf8("QCAiRecord output pointer is null."));
        return false;
    }

    if (nullptr == m_pAiRecordRepository)
    {
        setLastError(QString::fromUtf8("AI record repository is null."));
        return false;
    }

    if (nAiRecordId <= 0)
    {
        setLastError(QString::fromUtf8("AI record id is invalid."));
        return false;
    }

    if (!m_pAiRecordRepository->getAiRecordById(nAiRecordId, pAiRecord))
    {
        setLastError(QString::fromUtf8("AI record was not found."));
        return false;
    }

    return true;
}

QVector<QCAiRecord> QCAiService::listAiRecordsBySession(qint64 nSessionId) const
{
    clearError();

    if (nullptr == m_pAiRecordRepository)
    {
        setLastError(QString::fromUtf8("AI record repository is null."));
        return QVector<QCAiRecord>();
    }

    if (nSessionId <= 0)
    {
        setLastError(QString::fromUtf8("Study session id is invalid."));
        return QVector<QCAiRecord>();
    }

    return m_pAiRecordRepository->listAiRecordsBySession(nSessionId);
}

QVector<QCAiRecord> QCAiService::listAiRecordsBySnippet(qint64 nSnippetId) const
{
    clearError();

    if (nullptr == m_pAiRecordRepository)
    {
        setLastError(QString::fromUtf8("AI record repository is null."));
        return QVector<QCAiRecord>();
    }

    if (nSnippetId <= 0)
    {
        setLastError(QString::fromUtf8("Snippet id is invalid."));
        return QVector<QCAiRecord>();
    }

    return m_pAiRecordRepository->listAiRecordsBySnippet(nSnippetId);
}

QString QCAiService::lastError() const
{
    return m_strLastError;
}

void QCAiService::clearError() const
{
    m_strLastError.clear();
}

bool QCAiService::validateAiRecord(const QCAiRecord& aiRecord) const
{
    if (0 == aiRecord.sessionId() && 0 == aiRecord.snippetId())
    {
        setLastError(QString::fromUtf8("AI record requires sessionId or snippetId."));
        return false;
    }

    if (aiRecord.providerName().trimmed().isEmpty())
    {
        setLastError(QString::fromUtf8("AI record provider name is empty."));
        return false;
    }

    if (aiRecord.status().trimmed().isEmpty())
    {
        setLastError(QString::fromUtf8("AI record status is empty."));
        return false;
    }

    if (!aiRecord.createdAt().isValid())
    {
        setLastError(QString::fromUtf8("AI record createdAt is invalid."));
        return false;
    }

    if (aiRecord.taskType() == QCAiTaskType::SessionSummaryTask && aiRecord.sessionId() <= 0)
    {
        setLastError(QString::fromUtf8("SessionSummaryTask requires a valid sessionId."));
        return false;
    }

    if (aiRecord.taskType() != QCAiTaskType::SessionSummaryTask && aiRecord.snippetId() <= 0)
    {
        setLastError(QString::fromUtf8("Snippet AI tasks require a valid snippetId."));
        return false;
    }

    return true;
}

void QCAiService::setLastError(const QString& strError) const
{
    m_strLastError = strError;
}
