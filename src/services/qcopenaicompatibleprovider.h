#ifndef QTCLIP_QCOPENAICOMPATIBLEPROVIDER_H_
#define QTCLIP_QCOPENAICOMPATIBLEPROVIDER_H_

// File: qcopenaicompatibleprovider.h
// Author: ZZMI1
// Created: 2026-03-23
// Description: Declares the minimal OpenAI-compatible HTTP provider used by the first QtClip AI workflow.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include <QString>

#include "qcaiprovider.h"

class QCOpenAiCompatibleProvider : public IQCAiProvider
{
public:
    QCOpenAiCompatibleProvider(const QString& strBaseUrl,
                               const QString& strApiKey,
                               const QString& strDefaultModel);
    virtual ~QCOpenAiCompatibleProvider() override;

    virtual bool generateText(const QCAiProviderRequest& aiRequest,
                              QCAiProviderResponse *pAiResponse) override;
    virtual QString providerName() const override;
    virtual QString lastError() const override;
    virtual void clearError() const override;

private:
    QCOpenAiCompatibleProvider(const QCOpenAiCompatibleProvider& other);
    QCOpenAiCompatibleProvider& operator=(const QCOpenAiCompatibleProvider& other);

    QString resolveChatCompletionsUrl() const;
    void setLastError(const QString& strError) const;

private:
    QString m_strBaseUrl;
    QString m_strApiKey;
    QString m_strDefaultModel;
    mutable QString m_strLastError;
};

#endif // QTCLIP_QCOPENAICOMPATIBLEPROVIDER_H_
