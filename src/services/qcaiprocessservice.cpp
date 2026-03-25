// File: qcaiprocessservice.cpp
// Author: ZZMI1
// Created: 2026-03-23
// Description: Implements the minimal AI process service used by the first QtClip AI workflow.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include "qcaiprocessservice.h"

#include <QDateTime>
#include <QUrl>

#include "qcmockaiprovider.h"
#include "qcopenaicompatibleprovider.h"
#include "qcaiservice.h"
#include "qcsessionservice.h"
#include "qcsnippetservice.h"

namespace
{
bool IsChineseLanguage(const QString& strLanguage)
{
    const QString strValue = strLanguage.trimmed().toLower();
    return strValue.isEmpty() || strValue.startsWith(QString::fromUtf8("zh"));
}

QString BuildSnippetSystemPrompt(const QString& strLanguage)
{
    if (IsChineseLanguage(strLanguage))
        return QString::fromUtf8("你是一个学习笔记助手。请用简洁的中文编写学习总结。重点关注学生下次需要复习的内容。");

    return QString::fromUtf8("You are an assistant for study notes. Write a concise study summary in plain English. Focus on what the student should review next.");
}

QString BuildSessionSystemPrompt(const QString& strLanguage)
{
    if (IsChineseLanguage(strLanguage))
        return QString::fromUtf8("你是一个学习环节助手。请用简洁的中文编写环节总结，包含主要收获和复习重点。");

    return QString::fromUtf8("You are an assistant for study sessions. Write a concise session summary in plain English with main takeaways and review focus.");
}

QString BuildConnectionTestSystemPrompt()
{
    return QString::fromUtf8("You are testing whether an AI provider is reachable. Reply with a short confirmation sentence.");
}

QString BuildConnectionTestUserPrompt()
{
    return QString::fromUtf8("Return a short plain text confirmation that the configured model is reachable.");
}

QString FirstNonEmptyLine(const QString& strText)
{
    const QStringList vecLines = strText.split('\\n');
    for (int i = 0; i < vecLines.size(); ++i)
    {
        const QString strLine = vecLines.at(i).trimmed();
        if (!strLine.isEmpty())
            return strLine;
    }

    return QString();
}

QString NormalizeSummaryText(const QString& strText)
{
    QString strValue = strText.trimmed();
    if (strValue.startsWith(QString::fromUtf8("Mock summary:"), Qt::CaseInsensitive))
        strValue = strValue.mid(QString::fromUtf8("Mock summary:").size()).trimmed();
    return strValue;
}

bool LooksLikePromptEcho(const QString& strText)
{
    const QString strValue = strText.trimmed().toLower();
    if (strValue.isEmpty())
        return true;

    return strValue.startsWith(QString::fromUtf8("summarize "))
        || strValue.startsWith(QString::fromUtf8("please summarize"))
        || strValue.contains(QString::fromUtf8("return only the summary text"))
        || strValue.contains(QString::fromUtf8("output requirement"))
        || strValue.contains(QString::fromUtf8("snippet type:"))
        || strValue.contains(QString::fromUtf8("session title:"))
        || strValue.contains(QString::fromUtf8("source:"));
}
}

QCAiProcessService::QCAiProcessService(QCSessionService *pSessionService,
                                       QCSnippetService *pSnippetService,
                                       QCAiService *pAiService,
                                       QCSettingsService *pSettingsService)
    : m_pSessionService(pSessionService)
    , m_pSnippetService(pSnippetService)
    , m_pAiService(pAiService)
    , m_pSettingsService(pSettingsService)
    , m_strLastError()
{
}

QCAiProcessService::~QCAiProcessService()
{
}

bool QCAiProcessService::prepareSnippetSummary(qint64 nSnippetId,
                                               QCAiTaskExecutionContext *pExecutionContext) const
{
    clearError();

    if (nullptr == pExecutionContext)
    {
        setLastError(QString::fromUtf8("AI execution context output pointer is null."));
        return false;
    }

    if (nSnippetId <= 0)
    {
        setLastError(QString::fromUtf8("Snippet id is invalid."));
        return false;
    }

    QCSnippet snippet;
    if (!m_pSnippetService->getSnippetById(nSnippetId, &snippet))
    {
        setLastError(m_pSnippetService->lastError());
        return false;
    }

    QCAiRuntimeSettings aiSettings;
    if (!loadAiSettings(&aiSettings))
        return false;

    return buildSnippetExecutionContext(snippet, aiSettings, pExecutionContext);
}

bool QCAiProcessService::prepareSessionSummary(qint64 nSessionId,
                                               QCAiTaskExecutionContext *pExecutionContext) const
{
    clearError();

    if (nullptr == pExecutionContext)
    {
        setLastError(QString::fromUtf8("AI execution context output pointer is null."));
        return false;
    }

    if (nSessionId <= 0)
    {
        setLastError(QString::fromUtf8("Session id is invalid."));
        return false;
    }

    QCStudySession session;
    if (!m_pSessionService->getSessionById(nSessionId, &session))
    {
        setLastError(m_pSessionService->lastError());
        return false;
    }

    const QVector<QCSnippet> vecSnippets = m_pSnippetService->listSnippetsBySession(nSessionId);
    if (!m_pSnippetService->lastError().isEmpty())
    {
        setLastError(m_pSnippetService->lastError());
        return false;
    }

    QCAiRuntimeSettings aiSettings;
    if (!loadAiSettings(&aiSettings))
        return false;

    return buildSessionExecutionContext(session, vecSnippets, aiSettings, pExecutionContext);
}

bool QCAiProcessService::executePreparedTask(const QCAiTaskExecutionContext& executionContext,
                                             QCAiTaskExecutionResult *pExecutionResult) const
{
    if (nullptr == pExecutionResult)
        return false;

    pExecutionResult->m_bSuccess = false;
    pExecutionResult->m_context = executionContext;
    pExecutionResult->m_aiResponse = QCAiProviderResponse();
    pExecutionResult->m_strErrorMessage.clear();

    IQCAiProvider *pAiProvider = nullptr;
    if (!createProvider(executionContext.m_aiSettings, &pAiProvider) || nullptr == pAiProvider)
    {
        pExecutionResult->m_aiResponse.m_strProviderName = buildProviderMode(executionContext.m_aiSettings);
        pExecutionResult->m_aiResponse.m_strModelName = executionContext.m_aiSettings.m_strModel;
        pExecutionResult->m_aiResponse.m_strEndpoint = resolveProviderEndpoint(executionContext.m_aiSettings);
        pExecutionResult->m_strErrorMessage = formatProviderDiagnosticMessage(pExecutionResult->m_aiResponse.m_strProviderName,
                                                                              pExecutionResult->m_aiResponse.m_strModelName,
                                                                              pExecutionResult->m_aiResponse.m_strEndpoint,
                                                                              0,
                                                                              QString(),
                                                                              lastError().isEmpty()
                                                                                  ? QString::fromUtf8("Failed to create AI provider.")
                                                                                  : lastError());
        return false;
    }

    const bool bProviderSuccess = pAiProvider->generateText(executionContext.m_aiRequest,
                                                            &pExecutionResult->m_aiResponse);
    if (!bProviderSuccess)
    {
        if (pExecutionResult->m_aiResponse.m_strProviderName.trimmed().isEmpty())
            pExecutionResult->m_aiResponse.m_strProviderName = pAiProvider->providerName();
        if (pExecutionResult->m_aiResponse.m_strModelName.trimmed().isEmpty())
            pExecutionResult->m_aiResponse.m_strModelName = executionContext.m_aiSettings.m_strModel;
        if (pExecutionResult->m_aiResponse.m_strEndpoint.trimmed().isEmpty())
            pExecutionResult->m_aiResponse.m_strEndpoint = resolveProviderEndpoint(executionContext.m_aiSettings);

        pExecutionResult->m_strErrorMessage = formatProviderDiagnosticMessage(pExecutionResult->m_aiResponse.m_strProviderName,
                                                                              pExecutionResult->m_aiResponse.m_strModelName,
                                                                              pExecutionResult->m_aiResponse.m_strEndpoint,
                                                                              pExecutionResult->m_aiResponse.m_nHttpStatusCode,
                                                                              pExecutionResult->m_aiResponse.m_strErrorSummary,
                                                                              pAiProvider->lastError());
        delete pAiProvider;
        return false;
    }

    pExecutionResult->m_bSuccess = true;
    delete pAiProvider;
    return true;
}

bool QCAiProcessService::applyTaskResult(const QCAiTaskExecutionResult& executionResult)
{
    clearError();

    if (!executionResult.m_bSuccess)
    {
        persistFailedAiRecord(executionResult);
        setLastError(executionResult.m_strErrorMessage.trimmed().isEmpty()
            ? QString::fromUtf8("AI request failed.")
            : executionResult.m_strErrorMessage);
        return false;
    }

    switch (executionResult.m_context.m_aiTaskType)
    {
    case QCAiTaskType::SnippetSummaryTask:
        return persistCompletedSnippetSummary(executionResult);
    case QCAiTaskType::SessionSummaryTask:
        return persistCompletedSessionSummary(executionResult);
    default:
        setLastError(QString::fromUtf8("Unsupported AI task type for applyTaskResult."));
        return false;
    }
}

bool QCAiProcessService::testConnection(const QCAiRuntimeSettings& aiSettings,
                                        QCAiConnectionTestResult *pTestResult) const
{
    clearError();

    if (nullptr == pTestResult)
    {
        setLastError(QString::fromUtf8("AI connection test result output pointer is null."));
        return false;
    }

    pTestResult->m_bSuccess = false;
    pTestResult->m_strProviderMode = buildProviderMode(aiSettings);
    pTestResult->m_strProviderName.clear();
    pTestResult->m_strModelName = aiSettings.m_strModel.trimmed();
    pTestResult->m_strEndpoint = resolveProviderEndpoint(aiSettings);
    pTestResult->m_nHttpStatusCode = 0;
    pTestResult->m_strErrorSummary.clear();
    pTestResult->m_strMessage.clear();
    pTestResult->m_strRawResponse.clear();

    IQCAiProvider *pAiProvider = nullptr;
    if (!createProvider(aiSettings, &pAiProvider) || nullptr == pAiProvider)
    {
        pTestResult->m_strMessage = formatProviderDiagnosticMessage(pTestResult->m_strProviderMode,
                                                                    pTestResult->m_strModelName,
                                                                    pTestResult->m_strEndpoint,
                                                                    0,
                                                                    QString(),
                                                                    lastError());
        return false;
    }

    QCAiProviderRequest aiRequest;
    aiRequest.m_strSystemPrompt = BuildConnectionTestSystemPrompt();
    aiRequest.m_strUserPrompt = BuildConnectionTestUserPrompt();
    aiRequest.m_strModelName = aiSettings.m_strModel;
    aiRequest.m_nTimeoutMs = 20000;

    QCAiProviderResponse aiResponse;
    const bool bProviderSuccess = pAiProvider->generateText(aiRequest, &aiResponse);
    pTestResult->m_strProviderName = aiResponse.m_strProviderName.trimmed().isEmpty()
        ? pAiProvider->providerName()
        : aiResponse.m_strProviderName;
    if (pTestResult->m_strModelName.isEmpty())
        pTestResult->m_strModelName = aiResponse.m_strModelName;
    if (pTestResult->m_strEndpoint.isEmpty())
        pTestResult->m_strEndpoint = aiResponse.m_strEndpoint;
    pTestResult->m_nHttpStatusCode = aiResponse.m_nHttpStatusCode;
    pTestResult->m_strErrorSummary = aiResponse.m_strErrorSummary;
    pTestResult->m_strRawResponse = aiResponse.m_strRawResponse;

    if (!bProviderSuccess)
    {
        pTestResult->m_strMessage = formatProviderDiagnosticMessage(pTestResult->m_strProviderMode,
                                                                    pTestResult->m_strModelName,
                                                                    pTestResult->m_strEndpoint,
                                                                    pTestResult->m_nHttpStatusCode,
                                                                    pTestResult->m_strErrorSummary,
                                                                    pAiProvider->lastError());
        delete pAiProvider;
        return false;
    }

    pTestResult->m_bSuccess = true;
    pTestResult->m_strProviderName = aiResponse.m_strProviderName;
    pTestResult->m_strModelName = aiResponse.m_strModelName;
    pTestResult->m_strEndpoint = aiResponse.m_strEndpoint;
    pTestResult->m_nHttpStatusCode = aiResponse.m_nHttpStatusCode;
    pTestResult->m_strMessage = QString::fromUtf8("Connection OK\nMode: %1\nModel: %2\nEndpoint: %3")
        .arg(pTestResult->m_strProviderMode,
             pTestResult->m_strModelName,
             pTestResult->m_strEndpoint.isEmpty() ? QString::fromUtf8("(not reported)") : pTestResult->m_strEndpoint);
    delete pAiProvider;
    return true;
}

bool QCAiProcessService::summarizeSnippet(qint64 nSnippetId)
{
    QCAiTaskExecutionContext executionContext;
    if (!prepareSnippetSummary(nSnippetId, &executionContext))
        return false;

    QCAiTaskExecutionResult executionResult;
    executePreparedTask(executionContext, &executionResult);
    return applyTaskResult(executionResult);
}

bool QCAiProcessService::summarizeSession(qint64 nSessionId)
{
    QCAiTaskExecutionContext executionContext;
    if (!prepareSessionSummary(nSessionId, &executionContext))
        return false;

    QCAiTaskExecutionResult executionResult;
    executePreparedTask(executionContext, &executionResult);
    return applyTaskResult(executionResult);
}

QString QCAiProcessService::lastError() const
{
    return m_strLastError;
}

void QCAiProcessService::clearError() const
{
    m_strLastError.clear();
}

bool QCAiProcessService::loadAiSettings(QCAiRuntimeSettings *pAiSettings) const
{
    if (nullptr == m_pSettingsService)
    {
        setLastError(QString::fromUtf8("Settings service is null."));
        return false;
    }

    if (!m_pSettingsService->loadAiSettings(pAiSettings))
    {
        setLastError(m_pSettingsService->lastError());
        return false;
    }

    return true;
}

bool QCAiProcessService::buildSnippetExecutionContext(const QCSnippet& snippet,
                                                      const QCAiRuntimeSettings& aiSettings,
                                                      QCAiTaskExecutionContext *pExecutionContext) const
{
    QString strAppLanguage = QString::fromUtf8("zh-CN");
    if (nullptr != m_pSettingsService)
        m_pSettingsService->getAppLanguage(&strAppLanguage);
    if (nullptr == pExecutionContext)
    {
        setLastError(QString::fromUtf8("AI execution context output pointer is null."));
        return false;
    }

    pExecutionContext->m_nSessionId = snippet.sessionId();
    pExecutionContext->m_nSnippetId = snippet.id();
    pExecutionContext->m_aiTaskType = QCAiTaskType::SnippetSummaryTask;
    pExecutionContext->m_aiSettings = aiSettings;
    pExecutionContext->m_aiRequest.m_strSystemPrompt = BuildSnippetSystemPrompt(strAppLanguage);
    pExecutionContext->m_aiRequest.m_strUserPrompt = buildSnippetPrompt(snippet);
    pExecutionContext->m_aiRequest.m_strLocalImageFilePath.clear();
    pExecutionContext->m_aiRequest.m_strModelName = aiSettings.m_strModel;
    pExecutionContext->m_aiRequest.m_nTimeoutMs = snippet.type() == QCSnippetType::ImageSnippetType ? 90000 : 60000;

    if (snippet.type() == QCSnippetType::ImageSnippetType)
    {
        QCAttachment primaryAttachment;
        if (!m_pSnippetService->getPrimaryAttachmentBySnippetId(snippet.id(), &primaryAttachment))
        {
            setLastError(m_pSnippetService->lastError().trimmed().isEmpty()
                ? QString::fromUtf8("Primary image attachment is unavailable for the selected snippet.")
                : m_pSnippetService->lastError());
            return false;
        }

        if (primaryAttachment.filePath().trimmed().isEmpty())
        {
            setLastError(QString::fromUtf8("Primary image attachment path is empty."));
            return false;
        }

        pExecutionContext->m_aiRequest.m_strLocalImageFilePath = primaryAttachment.filePath().trimmed();
    }

    return true;
}

bool QCAiProcessService::buildSessionExecutionContext(const QCStudySession& session,
                                                      const QVector<QCSnippet>& vecSnippets,
                                                      const QCAiRuntimeSettings& aiSettings,
                                                      QCAiTaskExecutionContext *pExecutionContext) const
{
    QString strAppLanguage = QString::fromUtf8("zh-CN");
    if (nullptr != m_pSettingsService)
        m_pSettingsService->getAppLanguage(&strAppLanguage);
    if (nullptr == pExecutionContext)
    {
        setLastError(QString::fromUtf8("AI execution context output pointer is null."));
        return false;
    }

    pExecutionContext->m_nSessionId = session.id();
    pExecutionContext->m_nSnippetId = 0;
    pExecutionContext->m_aiTaskType = QCAiTaskType::SessionSummaryTask;
    pExecutionContext->m_aiSettings = aiSettings;
    pExecutionContext->m_aiRequest.m_strSystemPrompt = BuildSessionSystemPrompt(strAppLanguage);
    const QString strPreviousSummary = findLatestCompletedSessionSummary(session.id());
    pExecutionContext->m_aiRequest.m_strUserPrompt = buildSessionPrompt(session, vecSnippets, strPreviousSummary);
    pExecutionContext->m_aiRequest.m_strLocalImageFilePath.clear();
    pExecutionContext->m_aiRequest.m_strModelName = aiSettings.m_strModel;
    pExecutionContext->m_aiRequest.m_nTimeoutMs = 60000;
    return true;
}

bool QCAiProcessService::createProvider(const QCAiRuntimeSettings& aiSettings, IQCAiProvider **ppAiProvider) const
{
    if (nullptr == ppAiProvider)
    {
        setLastError(QString::fromUtf8("AI provider output pointer is null."));
        return false;
    }

    *ppAiProvider = nullptr;
    if (aiSettings.m_bUseMockProvider)
    {
        *ppAiProvider = new QCMockAiProvider(aiSettings.m_strModel);
        return true;
    }

    if (aiSettings.m_strBaseUrl.trimmed().isEmpty())
    {
        setLastError(QString::fromUtf8("AI base URL is missing."));
        return false;
    }

    if (aiSettings.m_strApiKey.trimmed().isEmpty())
    {
        setLastError(QString::fromUtf8("AI API key is missing."));
        return false;
    }

    if (aiSettings.m_strModel.trimmed().isEmpty())
    {
        setLastError(QString::fromUtf8("AI model is missing."));
        return false;
    }

    *ppAiProvider = new QCOpenAiCompatibleProvider(aiSettings.m_strBaseUrl,
                                                   aiSettings.m_strApiKey,
                                                   aiSettings.m_strModel);
    return true;
}

QString QCAiProcessService::buildSnippetPrompt(const QCSnippet& snippet) const
{
    QString strAppLanguage = QString::fromUtf8("zh-CN");
    if (nullptr != m_pSettingsService)
        m_pSettingsService->getAppLanguage(&strAppLanguage);

    QString strPrompt;
    if (IsChineseLanguage(strAppLanguage))
    {
        if (snippet.type() == QCSnippetType::ImageSnippetType)
            strPrompt += QString::fromUtf8("请总结以下截图内容，以便后续复习。请将图片作为主要的信息来源。\n");
        else
            strPrompt += QString::fromUtf8("请总结这段学习内容，以便后续复习。\n");
    }
    else if (snippet.type() == QCSnippetType::ImageSnippetType)
        strPrompt += QString::fromUtf8("Summarize the attached screenshot for later review. Use the image as the primary source of truth.\n");
    else
        strPrompt += QString::fromUtf8("Summarize this learning snippet for later review.\n");

    strPrompt += QString::fromUtf8("Snippet Type: %1\n").arg(snippet.type() == QCSnippetType::ImageSnippetType
        ? QString::fromUtf8("image")
        : snippet.type() == QCSnippetType::CodeSnippetType
            ? QString::fromUtf8("code")
            : QString::fromUtf8("text"));
    strPrompt += QString::fromUtf8("Title: %1\n").arg(snippet.title());
    strPrompt += QString::fromUtf8("Note: %1\n").arg(snippet.note());
    strPrompt += QString::fromUtf8("Content: %1\n").arg(snippet.contentText());
    strPrompt += QString::fromUtf8("Source: %1\n").arg(snippet.source());
    strPrompt += IsChineseLanguage(strAppLanguage)
        ? QString::fromUtf8("仅返回总结正文。")
        : QString::fromUtf8("Return only the summary text.");
    return strPrompt;
}

QString QCAiProcessService::buildSessionPrompt(const QCStudySession& session,
                                               const QVector<QCSnippet>& vecSnippets,
                                               const QString& strPreviousSummary) const
{
    QString strAppLanguage = QString::fromUtf8("zh-CN");
    if (nullptr != m_pSettingsService)
        m_pSettingsService->getAppLanguage(&strAppLanguage);

    QString strPrompt;
    strPrompt += IsChineseLanguage(strAppLanguage)
        ? QString::fromUtf8("请基于当前 Session 的全部学习内容，生成新的完整总结。\n")
        : QString::fromUtf8("Generate a refreshed full summary for this session based on all current learning items.\n");
    strPrompt += IsChineseLanguage(strAppLanguage)
        ? QString::fromUtf8("如果提供了上一次总结，请把它当作上下文并在其基础上迭代，不要丢失已确认结论。\n")
        : QString::fromUtf8("If a previous summary is provided, treat it as context and iterate on top of it without losing validated conclusions.\n");
    strPrompt += QString::fromUtf8("Session Title: %1\n").arg(session.title());
    strPrompt += QString::fromUtf8("Course: %1\n").arg(session.courseName());
    strPrompt += QString::fromUtf8("Description: %1\n\n").arg(session.description());

    if (!strPreviousSummary.trimmed().isEmpty())
    {
        strPrompt += IsChineseLanguage(strAppLanguage)
            ? QString::fromUtf8("上一次总结：\n")
            : QString::fromUtf8("Previous summary:\n");
        strPrompt += strPreviousSummary.trimmed() + QString::fromUtf8("\n\n");
    }

    strPrompt += IsChineseLanguage(strAppLanguage)
        ? QString::fromUtf8("当前 Snippets（尤其是图片类）:\n")
        : QString::fromUtf8("Current snippets (especially image items):\n");
    for (int i = 0; i < vecSnippets.size(); ++i)
    {
        const QCSnippet& snippet = vecSnippets.at(i);
        strPrompt += QString::fromUtf8("- Type: %1 | Title: %2 | Note: %3 | Content: %4 | Summary: %5\n")
            .arg(snippet.type() == QCSnippetType::ImageSnippetType ? QString::fromUtf8("image") : QString::fromUtf8("text"),
                 snippet.title(),
                 snippet.note(),
                 snippet.contentText(),
                 snippet.summary());
    }

    strPrompt += IsChineseLanguage(strAppLanguage)
        ? QString::fromUtf8("输出要求：返回新的完整 Session 总结，仅输出总结正文。")
        : QString::fromUtf8("Output requirement: return the refreshed complete session summary text only.");
    return strPrompt;
}

QString QCAiProcessService::findLatestCompletedSessionSummary(qint64 nSessionId) const
{
    if (nSessionId <= 0 || nullptr == m_pAiService)
        return QString();

    const QVector<QCAiRecord> vecAiRecords = m_pAiService->listAiRecordsBySession(nSessionId);
    if (!m_pAiService->lastError().trimmed().isEmpty())
        return QString();

    QDateTime dateTimeLatest;
    QString strLatestSummary;
    for (int i = 0; i < vecAiRecords.size(); ++i)
    {
        const QCAiRecord& aiRecord = vecAiRecords.at(i);
        if (aiRecord.taskType() != QCAiTaskType::SessionSummaryTask)
            continue;
        if (aiRecord.status().trimmed().compare(QString::fromUtf8("completed"), Qt::CaseInsensitive) != 0)
            continue;
        if (aiRecord.responseText().trimmed().isEmpty())
            continue;
        if (!dateTimeLatest.isValid() || aiRecord.createdAt() > dateTimeLatest)
        {
            dateTimeLatest = aiRecord.createdAt();
            strLatestSummary = aiRecord.responseText().trimmed();
        }
    }

    return strLatestSummary;
}

QString QCAiProcessService::buildProviderMode(const QCAiRuntimeSettings& aiSettings) const
{
    return aiSettings.m_bUseMockProvider
        ? QString::fromUtf8("mock")
        : QString::fromUtf8("openai-compatible");
}

QString QCAiProcessService::resolveProviderEndpoint(const QCAiRuntimeSettings& aiSettings) const
{
    if (aiSettings.m_bUseMockProvider)
        return QString::fromUtf8("mock://local");

    QString strBaseUrl = aiSettings.m_strBaseUrl.trimmed();
    while (strBaseUrl.endsWith('/'))
        strBaseUrl.chop(1);
    if (strBaseUrl.isEmpty())
        return QString();

    return QString::fromUtf8("%1/chat/completions").arg(strBaseUrl);
}

QString QCAiProcessService::formatProviderDiagnosticMessage(const QString& strProviderMode,
                                                            const QString& strModelName,
                                                            const QString& strEndpoint,
                                                            int nHttpStatusCode,
                                                            const QString& strProviderErrorSummary,
                                                            const QString& strBaseMessage) const
{
    QStringList vecLines;
    vecLines.append(QString::fromUtf8("Provider request failed."));
    vecLines.append(QString::fromUtf8("Mode: %1").arg(strProviderMode.trimmed().isEmpty() ? QString::fromUtf8("unknown") : strProviderMode.trimmed()));
    vecLines.append(QString::fromUtf8("Model: %1").arg(strModelName.trimmed().isEmpty() ? QString::fromUtf8("(empty)") : strModelName.trimmed()));
    vecLines.append(QString::fromUtf8("Endpoint: %1").arg(strEndpoint.trimmed().isEmpty() ? QString::fromUtf8("(unresolved)") : strEndpoint.trimmed()));
    if (nHttpStatusCode > 0)
        vecLines.append(QString::fromUtf8("HTTP Status: %1").arg(nHttpStatusCode));

    const QString strErrorSummary = strProviderErrorSummary.trimmed();
    if (!strErrorSummary.isEmpty())
        vecLines.append(QString::fromUtf8("Provider Error: %1").arg(strErrorSummary));

    QString strMessage = strBaseMessage.trimmed();
    if (strMessage.isEmpty())
        strMessage = QString::fromUtf8("Unknown provider error.");
    vecLines.append(QString::fromUtf8("Message: %1").arg(strMessage));
    return vecLines.join(QString::fromUtf8("\n"));
}

bool QCAiProcessService::persistCompletedSnippetSummary(const QCAiTaskExecutionResult& executionResult)
{
    const QString strSummary = NormalizeSummaryText(executionResult.m_aiResponse.m_strText);
    if (strSummary.isEmpty() || LooksLikePromptEcho(strSummary))
    {
        setLastError(QString::fromUtf8("AI provider returned an invalid snippet summary."));
        return false;
    }

    QCSnippet snippet;
    if (!m_pSnippetService->getSnippetById(executionResult.m_context.m_nSnippetId, &snippet))
    {
        setLastError(m_pSnippetService->lastError());
        return false;
    }

    QCAiRecord aiRecord;
    aiRecord.setSessionId(snippet.sessionId());
    aiRecord.setSnippetId(snippet.id());
    aiRecord.setTaskType(QCAiTaskType::SnippetSummaryTask);
    aiRecord.setProviderName(executionResult.m_aiResponse.m_strProviderName);
    aiRecord.setModelName(executionResult.m_aiResponse.m_strModelName);
    aiRecord.setPromptText(executionResult.m_context.m_aiRequest.m_strUserPrompt);
    aiRecord.setResponseText(strSummary);
    aiRecord.setStatus(QString::fromUtf8("completed"));
    aiRecord.setCreatedAt(QDateTime::currentDateTimeUtc());
    if (!m_pAiService->createAiRecord(&aiRecord))
    {
        setLastError(m_pAiService->lastError());
        return false;
    }

    snippet.setSummary(strSummary);
    snippet.setUpdatedAt(QDateTime::currentDateTimeUtc());
    if (!m_pSnippetService->updateSnippet(&snippet))
    {
        setLastError(m_pSnippetService->lastError());
        return false;
    }

    return true;
}

bool QCAiProcessService::persistCompletedSessionSummary(const QCAiTaskExecutionResult& executionResult)
{
    const QString strSummary = NormalizeSummaryText(executionResult.m_aiResponse.m_strText);
    if (strSummary.isEmpty() || LooksLikePromptEcho(strSummary))
    {
        setLastError(QString::fromUtf8("AI provider returned an invalid session summary."));
        return false;
    }

    QCAiRecord aiRecord;
    aiRecord.setSessionId(executionResult.m_context.m_nSessionId);
    aiRecord.setTaskType(QCAiTaskType::SessionSummaryTask);
    aiRecord.setProviderName(executionResult.m_aiResponse.m_strProviderName);
    aiRecord.setModelName(executionResult.m_aiResponse.m_strModelName);
    aiRecord.setPromptText(executionResult.m_context.m_aiRequest.m_strUserPrompt);
    aiRecord.setResponseText(strSummary);
    aiRecord.setStatus(QString::fromUtf8("completed"));
    aiRecord.setCreatedAt(QDateTime::currentDateTimeUtc());
    if (!m_pAiService->createAiRecord(&aiRecord))
    {
        setLastError(m_pAiService->lastError());
        return false;
    }

    return true;
}

bool QCAiProcessService::persistFailedAiRecord(const QCAiTaskExecutionResult& executionResult)
{
    if (nullptr == m_pAiService)
        return false;

    QCAiRecord aiRecord;
    aiRecord.setSessionId(executionResult.m_context.m_nSessionId);
    aiRecord.setSnippetId(executionResult.m_context.m_nSnippetId);
    aiRecord.setTaskType(executionResult.m_context.m_aiTaskType);
    aiRecord.setProviderName(executionResult.m_aiResponse.m_strProviderName.trimmed().isEmpty()
        ? QString::fromUtf8("unknown")
        : executionResult.m_aiResponse.m_strProviderName);
    aiRecord.setModelName(executionResult.m_aiResponse.m_strModelName.isEmpty()
        ? executionResult.m_context.m_aiSettings.m_strModel
        : executionResult.m_aiResponse.m_strModelName);
    aiRecord.setPromptText(executionResult.m_context.m_aiRequest.m_strUserPrompt);
    aiRecord.setStatus(QString::fromUtf8("failed"));
    aiRecord.setErrorMessage(executionResult.m_strErrorMessage);
    aiRecord.setCreatedAt(QDateTime::currentDateTimeUtc());
    return m_pAiService->createAiRecord(&aiRecord);
}

void QCAiProcessService::setLastError(const QString& strError) const
{
    m_strLastError = strError;
}



