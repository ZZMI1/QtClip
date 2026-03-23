// File: qcmockaiprovider.cpp
// Author: ZZMI1
// Created: 2026-03-23
// Description: Implements the mock AI provider used by the first QtClip offline workflow.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include "qcmockaiprovider.h"

#include <QStringList>

namespace
{
QString FirstNonEmptyLine(const QString& strText)
{
    const QStringList vecLines = strText.split('\n');
    for (int i = 0; i < vecLines.size(); ++i)
    {
        const QString strLine = vecLines.at(i).trimmed();
        if (!strLine.isEmpty())
            return strLine;
    }

    return QString();
}
}

QCMockAiProvider::QCMockAiProvider(const QString& strModelName)
    : m_strModelName(strModelName.trimmed().isEmpty() ? QString::fromUtf8("mock-summary-v1") : strModelName)
    , m_strLastError()
{
}

QCMockAiProvider::~QCMockAiProvider()
{
}

bool QCMockAiProvider::generateText(const QCAiProviderRequest& aiRequest,
                                    QCAiProviderResponse *pAiResponse)
{
    clearError();

    if (nullptr == pAiResponse)
    {
        setLastError(QString::fromUtf8("AI response output pointer is null."));
        return false;
    }

    pAiResponse->m_strProviderName = providerName();
    pAiResponse->m_strModelName = m_strModelName;
    pAiResponse->m_strText = buildMockText(aiRequest);
    pAiResponse->m_strRawResponse = pAiResponse->m_strText;
    pAiResponse->m_strEndpoint = QString::fromUtf8("mock://local");
    pAiResponse->m_nHttpStatusCode = 0;
    pAiResponse->m_strErrorSummary.clear();
    return true;
}

QString QCMockAiProvider::providerName() const
{
    return QString::fromUtf8("mock");
}

QString QCMockAiProvider::lastError() const
{
    return m_strLastError;
}

void QCMockAiProvider::clearError() const
{
    m_strLastError.clear();
}

QString QCMockAiProvider::buildMockText(const QCAiProviderRequest& aiRequest) const
{
    QString strSeedText = FirstNonEmptyLine(aiRequest.m_strUserPrompt);
    if (strSeedText.isEmpty())
        strSeedText = aiRequest.m_strUserPrompt.simplified();

    if (strSeedText.isEmpty())
        strSeedText = QString::fromUtf8("No source content was provided.");

    if (strSeedText.size() > 180)
        strSeedText = strSeedText.left(180) + QString::fromUtf8("...");

    return QString::fromUtf8("Mock summary: %1").arg(strSeedText);
}

void QCMockAiProvider::setLastError(const QString& strError) const
{
    m_strLastError = strError;
}
