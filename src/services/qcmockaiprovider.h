#ifndef QTCLIP_QCMOCKAIPROVIDER_H_
#define QTCLIP_QCMOCKAIPROVIDER_H_

// File: qcmockaiprovider.h
// Author: ZZMI1
// Created: 2026-03-23
// Description: Declares the mock AI provider used by the first QtClip offline workflow.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include <QString>

#include "qcaiprovider.h"

class QCMockAiProvider : public IQCAiProvider
{
public:
    explicit QCMockAiProvider(const QString& strModelName);
    virtual ~QCMockAiProvider() override;

    virtual bool generateText(const QCAiProviderRequest& aiRequest,
                              QCAiProviderResponse *pAiResponse) override;
    virtual QString providerName() const override;
    virtual QString lastError() const override;
    virtual void clearError() const override;

private:
    QCMockAiProvider(const QCMockAiProvider& other);
    QCMockAiProvider& operator=(const QCMockAiProvider& other);

    QString buildMockText(const QCAiProviderRequest& aiRequest) const;
    void setLastError(const QString& strError) const;

private:
    QString m_strModelName;
    mutable QString m_strLastError;
};

#endif // QTCLIP_QCMOCKAIPROVIDER_H_
