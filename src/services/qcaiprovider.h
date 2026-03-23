#ifndef QTCLIP_QCAIPROVIDER_H_
#define QTCLIP_QCAIPROVIDER_H_

// File: qcaiprovider.h
// Author: ZZMI1
// Created: 2026-03-23
// Description: Declares the minimal AI provider abstraction used by the first QtClip AI workflow.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include <QString>

struct QCAiProviderRequest
{
    QString m_strSystemPrompt;
    QString m_strUserPrompt;
    QString m_strLocalImageFilePath;
    QString m_strModelName;
    int m_nTimeoutMs;
};

struct QCAiProviderResponse
{
    QString m_strProviderName;
    QString m_strModelName;
    QString m_strText;
    QString m_strRawResponse;
    QString m_strEndpoint;
    int m_nHttpStatusCode;
    QString m_strErrorSummary;
};

class IQCAiProvider
{
public:
    IQCAiProvider();
    virtual ~IQCAiProvider();

    virtual bool generateText(const QCAiProviderRequest& aiRequest,
                              QCAiProviderResponse *pAiResponse) = 0;
    virtual QString providerName() const = 0;
    virtual QString lastError() const = 0;
    virtual void clearError() const = 0;

private:
    IQCAiProvider(const IQCAiProvider& other);
    IQCAiProvider& operator=(const IQCAiProvider& other);
};

#endif // QTCLIP_QCAIPROVIDER_H_
