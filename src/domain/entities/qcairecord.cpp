// File: qcairecord.cpp
// Author: ZZMI1
// Created: 2026-03-23
// Description: Implements the AI record entity used by the QtClip domain layer.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include "qcairecord.h"

QCAiRecord::QCAiRecord()
    : m_nId(0)
    , m_nSessionId(0)
    , m_nSnippetId(0)
    , m_aiTaskType(QCAiTaskType::SnippetSummaryTask)
    , m_strProviderName()
    , m_strModelName()
    , m_strPromptText()
    , m_strResponseText()
    , m_strStatus()
    , m_strErrorMessage()
    , m_dateTimeCreatedAt()
{
}

QCAiRecord::~QCAiRecord()
{
}

qint64 QCAiRecord::id() const
{
    return m_nId;
}

void QCAiRecord::setId(qint64 nId)
{
    m_nId = nId;
}

qint64 QCAiRecord::sessionId() const
{
    return m_nSessionId;
}

void QCAiRecord::setSessionId(qint64 nSessionId)
{
    m_nSessionId = nSessionId;
}

qint64 QCAiRecord::snippetId() const
{
    return m_nSnippetId;
}

void QCAiRecord::setSnippetId(qint64 nSnippetId)
{
    m_nSnippetId = nSnippetId;
}

QCAiTaskType QCAiRecord::taskType() const
{
    return m_aiTaskType;
}

void QCAiRecord::setTaskType(QCAiTaskType aiTaskType)
{
    m_aiTaskType = aiTaskType;
}

QString QCAiRecord::providerName() const
{
    return m_strProviderName;
}

void QCAiRecord::setProviderName(const QString& strProviderName)
{
    m_strProviderName = strProviderName;
}

QString QCAiRecord::modelName() const
{
    return m_strModelName;
}

void QCAiRecord::setModelName(const QString& strModelName)
{
    m_strModelName = strModelName;
}

QString QCAiRecord::promptText() const
{
    return m_strPromptText;
}

void QCAiRecord::setPromptText(const QString& strPromptText)
{
    m_strPromptText = strPromptText;
}

QString QCAiRecord::responseText() const
{
    return m_strResponseText;
}

void QCAiRecord::setResponseText(const QString& strResponseText)
{
    m_strResponseText = strResponseText;
}

QString QCAiRecord::status() const
{
    return m_strStatus;
}

void QCAiRecord::setStatus(const QString& strStatus)
{
    m_strStatus = strStatus;
}

QString QCAiRecord::errorMessage() const
{
    return m_strErrorMessage;
}

void QCAiRecord::setErrorMessage(const QString& strErrorMessage)
{
    m_strErrorMessage = strErrorMessage;
}

QDateTime QCAiRecord::createdAt() const
{
    return m_dateTimeCreatedAt;
}

void QCAiRecord::setCreatedAt(const QDateTime& dateTimeCreatedAt)
{
    m_dateTimeCreatedAt = dateTimeCreatedAt;
}
