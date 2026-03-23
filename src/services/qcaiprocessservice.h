#ifndef QTCLIP_QCAIPROCESSSERVICE_H_
#define QTCLIP_QCAIPROCESSSERVICE_H_

// File: qcaiprocessservice.h
// Author: ZZMI1
// Created: 2026-03-23
// Description: Declares the minimal AI process service used by the first QtClip AI workflow.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include <QString>
#include <QVector>

#include "../domain/entities/qcairecord.h"
#include "../domain/entities/qcsnippet.h"
#include "../domain/entities/qcstudysession.h"
#include "qcaiprovider.h"
#include "qcsettingsservice.h"

class IQCAiProvider;
class QCAiService;
class QCSessionService;
class QCSnippetService;

struct QCAiTaskExecutionContext
{
    qint64 m_nSessionId;
    qint64 m_nSnippetId;
    QCAiTaskType m_aiTaskType;
    QCAiRuntimeSettings m_aiSettings;
    QCAiProviderRequest m_aiRequest;
};

struct QCAiTaskExecutionResult
{
    bool m_bSuccess;
    QCAiTaskExecutionContext m_context;
    QCAiProviderResponse m_aiResponse;
    QString m_strErrorMessage;
};

struct QCAiConnectionTestResult
{
    bool m_bSuccess;
    QString m_strProviderMode;
    QString m_strProviderName;
    QString m_strModelName;
    QString m_strEndpoint;
    int m_nHttpStatusCode;
    QString m_strErrorSummary;
    QString m_strMessage;
    QString m_strRawResponse;
};

class QCAiProcessService
{
public:
    QCAiProcessService(QCSessionService *pSessionService,
                       QCSnippetService *pSnippetService,
                       QCAiService *pAiService,
                       QCSettingsService *pSettingsService);
    ~QCAiProcessService();

    bool prepareSnippetSummary(qint64 nSnippetId, QCAiTaskExecutionContext *pExecutionContext) const;
    bool prepareSessionSummary(qint64 nSessionId, QCAiTaskExecutionContext *pExecutionContext) const;
    bool executePreparedTask(const QCAiTaskExecutionContext& executionContext,
                             QCAiTaskExecutionResult *pExecutionResult) const;
    bool applyTaskResult(const QCAiTaskExecutionResult& executionResult);
    bool testConnection(const QCAiRuntimeSettings& aiSettings,
                        QCAiConnectionTestResult *pTestResult) const;

    bool summarizeSnippet(qint64 nSnippetId);
    bool summarizeSession(qint64 nSessionId);

    QString lastError() const;
    void clearError() const;

private:
    QCAiProcessService(const QCAiProcessService& other);
    QCAiProcessService& operator=(const QCAiProcessService& other);

    bool loadAiSettings(QCAiRuntimeSettings *pAiSettings) const;
    bool buildSnippetExecutionContext(const QCSnippet& snippet,
                                      const QCAiRuntimeSettings& aiSettings,
                                      QCAiTaskExecutionContext *pExecutionContext) const;
    bool buildSessionExecutionContext(const QCStudySession& session,
                                      const QVector<QCSnippet>& vecSnippets,
                                      const QCAiRuntimeSettings& aiSettings,
                                      QCAiTaskExecutionContext *pExecutionContext) const;
    bool createProvider(const QCAiRuntimeSettings& aiSettings, IQCAiProvider **ppAiProvider) const;
    QString buildSnippetPrompt(const QCSnippet& snippet) const;
    QString buildSessionPrompt(const QCStudySession& session, const QVector<QCSnippet>& vecSnippets) const;
    QString buildProviderMode(const QCAiRuntimeSettings& aiSettings) const;
    QString resolveProviderEndpoint(const QCAiRuntimeSettings& aiSettings) const;
    QString formatProviderDiagnosticMessage(const QString& strProviderMode,
                                           const QString& strModelName,
                                           const QString& strEndpoint,
                                           int nHttpStatusCode,
                                           const QString& strProviderErrorSummary,
                                           const QString& strBaseMessage) const;
    bool persistCompletedSnippetSummary(const QCAiTaskExecutionResult& executionResult);
    bool persistCompletedSessionSummary(const QCAiTaskExecutionResult& executionResult);
    bool persistFailedAiRecord(const QCAiTaskExecutionResult& executionResult);
    void setLastError(const QString& strError) const;

private:
    QCSessionService *m_pSessionService;
    QCSnippetService *m_pSnippetService;
    QCAiService *m_pAiService;
    QCSettingsService *m_pSettingsService;
    mutable QString m_strLastError;
};

#endif // QTCLIP_QCAIPROCESSSERVICE_H_
