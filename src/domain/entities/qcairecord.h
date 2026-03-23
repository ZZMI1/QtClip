#ifndef QTCLIP_QCAIRECORD_H_
#define QTCLIP_QCAIRECORD_H_

// File: qcairecord.h
// Author: ZZMI1
// Created: 2026-03-23
// Description: Declares the AI record entity used by the QtClip domain layer.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include <QDateTime>
#include <QString>

// AI task categories aligned with the first QtClip release.
enum class QCAiTaskType
{
    SnippetTitleTask,
    SnippetSummaryTask,
    SnippetTagsTask,
    SessionSummaryTask
};

class QCAiRecord
{
public:
    QCAiRecord();
    ~QCAiRecord();

    qint64 id() const;
    void setId(qint64 nId);

    qint64 sessionId() const;
    void setSessionId(qint64 nSessionId);

    qint64 snippetId() const;
    void setSnippetId(qint64 nSnippetId);

    QCAiTaskType taskType() const;
    void setTaskType(QCAiTaskType aiTaskType);

    QString providerName() const;
    void setProviderName(const QString& strProviderName);

    QString modelName() const;
    void setModelName(const QString& strModelName);

    QString promptText() const;
    void setPromptText(const QString& strPromptText);

    QString responseText() const;
    void setResponseText(const QString& strResponseText);

    QString status() const;
    void setStatus(const QString& strStatus);

    QString errorMessage() const;
    void setErrorMessage(const QString& strErrorMessage);

    QDateTime createdAt() const;
    void setCreatedAt(const QDateTime& dateTimeCreatedAt);

private:
    qint64 m_nId;
    qint64 m_nSessionId;
    qint64 m_nSnippetId;
    QCAiTaskType m_aiTaskType;
    QString m_strProviderName;
    QString m_strModelName;
    QString m_strPromptText;
    QString m_strResponseText;
    QString m_strStatus;
    QString m_strErrorMessage;
    QDateTime m_dateTimeCreatedAt;
};

#endif // QTCLIP_QCAIRECORD_H_
