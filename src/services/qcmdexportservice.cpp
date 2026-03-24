// File: qcmdexportservice.cpp
// Author: ZZMI1
// Created: 2026-03-23
// Description: Implements the Markdown export service used by the first QtClip export workflow.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include "qcmdexportservice.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <QTextStream>

#include "qcexportdataservice.h"
#include "qcmdexportrenderer.h"

namespace
{
bool PopulatePreviewFromContext(const QCExportContext& exportContext, QCMdExportPreview *pExportPreview)
{
    if (nullptr == pExportPreview)
        return false;

    pExportPreview->m_strSessionTitle = exportContext.m_session.title();
    pExportPreview->m_strCourseName = exportContext.m_session.courseName();
    pExportPreview->m_nSnippetCount = exportContext.m_vecSnippetContexts.size();
    pExportPreview->m_nImageSnippetCount = 0;
    pExportPreview->m_nArchivedSnippetCount = 0;
    pExportPreview->m_nFavoriteSnippetCount = 0;
    pExportPreview->m_nSummarizedSnippetCount = 0;
    pExportPreview->m_bHasSessionSummary = false;

    for (int i = 0; i < exportContext.m_vecSnippetContexts.size(); ++i)
    {
        const QCExportSnippetContext& snippetContext = exportContext.m_vecSnippetContexts.at(i);
        if (snippetContext.m_snippet.type() == QCSnippetType::ImageSnippetType)
            ++pExportPreview->m_nImageSnippetCount;
        if (snippetContext.m_snippet.isArchived())
            ++pExportPreview->m_nArchivedSnippetCount;
        if (snippetContext.m_snippet.isFavorite())
            ++pExportPreview->m_nFavoriteSnippetCount;
        if (!snippetContext.m_snippet.summary().trimmed().isEmpty())
            ++pExportPreview->m_nSummarizedSnippetCount;
    }

    for (int i = 0; i < exportContext.m_vecSessionAiRecords.size(); ++i)
    {
        if (exportContext.m_vecSessionAiRecords.at(i).taskType() == QCAiTaskType::SessionSummaryTask
            && !exportContext.m_vecSessionAiRecords.at(i).responseText().trimmed().isEmpty())
        {
            pExportPreview->m_bHasSessionSummary = true;
            break;
        }
    }

    return true;
}

bool FilterExportContextBySnippetIds(const QCExportContext& exportContext,
                                     const QVector<qint64>& vecSnippetIds,
                                     QCExportContext *pFilteredContext,
                                     QString *pstrError)
{
    if (nullptr == pFilteredContext)
        return false;

    QSet<qint64> setSnippetIds;
    for (int i = 0; i < vecSnippetIds.size(); ++i)
    {
        if (vecSnippetIds.at(i) > 0)
            setSnippetIds.insert(vecSnippetIds.at(i));
    }

    if (setSnippetIds.isEmpty())
    {
        if (nullptr != pstrError)
            *pstrError = QString::fromUtf8("No snippets are available for export.");
        return false;
    }

    QCExportContext filteredContext = exportContext;
    filteredContext.m_vecSnippetContexts.clear();
    for (int i = 0; i < exportContext.m_vecSnippetContexts.size(); ++i)
    {
        const QCExportSnippetContext& snippetContext = exportContext.m_vecSnippetContexts.at(i);
        if (setSnippetIds.contains(snippetContext.m_snippet.id()))
            filteredContext.m_vecSnippetContexts.append(snippetContext);
    }

    if (filteredContext.m_vecSnippetContexts.isEmpty())
    {
        if (nullptr != pstrError)
            *pstrError = QString::fromUtf8("The selected snippets were not found in the export context.");
        return false;
    }

    *pFilteredContext = filteredContext;
    return true;
}

bool WriteMarkdownToFile(const QString& strMarkdown, const QString& strOutputFilePath, QString *pstrError)
{
    QFileInfo fileInfo(strOutputFilePath);
    QDir outputDirectory = fileInfo.dir();
    if (!outputDirectory.exists() && !outputDirectory.mkpath(QString::fromUtf8(".")))
    {
        if (nullptr != pstrError)
            *pstrError = QString::fromUtf8("Failed to create Markdown output directory: %1").arg(outputDirectory.absolutePath());
        return false;
    }

    QFile outputFile(strOutputFilePath);
    if (!outputFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
    {
        if (nullptr != pstrError)
            *pstrError = QString::fromUtf8("Failed to open Markdown output file: %1").arg(QDir::toNativeSeparators(strOutputFilePath));
        return false;
    }

    QTextStream outputStream(&outputFile);
    outputStream << strMarkdown;
    outputFile.close();
    return true;
}
}

QCMdExportService::QCMdExportService(QCExportDataService *pExportDataService,
                                     QCMdExportRenderer *pMdExportRenderer)
    : m_pExportDataService(pExportDataService)
    , m_pMdExportRenderer(pMdExportRenderer)
    , m_strLastError()
{
}

QCMdExportService::~QCMdExportService()
{
}

bool QCMdExportService::buildExportPreview(qint64 nSessionId, QCMdExportPreview *pExportPreview) const
{
    clearError();

    if (nullptr == pExportPreview)
        return false;

    if (nullptr == m_pExportDataService)
        return false;

    QCExportContext exportContext;
    if (!m_pExportDataService->buildExportContext(nSessionId, &exportContext))
    {
        setLastError(m_pExportDataService->lastError());
        return false;
    }

    return PopulatePreviewFromContext(exportContext, pExportPreview);
}

bool QCMdExportService::buildExportPreviewForSnippets(qint64 nSessionId, const QVector<qint64>& vecSnippetIds, QCMdExportPreview *pExportPreview) const
{
    clearError();

    if (nullptr == pExportPreview)
        return false;

    if (nullptr == m_pExportDataService)
        return false;

    QCExportContext exportContext;
    if (!m_pExportDataService->buildExportContext(nSessionId, &exportContext))
    {
        setLastError(m_pExportDataService->lastError());
        return false;
    }

    QCExportContext filteredContext;
    QString strError;
    if (!FilterExportContextBySnippetIds(exportContext, vecSnippetIds, &filteredContext, &strError))
    {
        setLastError(strError);
        return false;
    }

    return PopulatePreviewFromContext(filteredContext, pExportPreview);
}

bool QCMdExportService::exportSessionToFile(qint64 nSessionId, const QString& strOutputFilePath)
{
    clearError();

    if (nullptr == m_pExportDataService || nullptr == m_pMdExportRenderer)
    {
        setLastError(QString::fromUtf8("Markdown export service dependencies are not ready."));
        return false;
    }

    if (nSessionId <= 0)
    {
        setLastError(QString::fromUtf8("Study session id is invalid."));
        return false;
    }

    if (strOutputFilePath.trimmed().isEmpty())
    {
        setLastError(QString::fromUtf8("Markdown output file path is empty."));
        return false;
    }

    QCExportContext exportContext;
    if (!m_pExportDataService->buildExportContext(nSessionId, &exportContext))
    {
        setLastError(m_pExportDataService->lastError());
        return false;
    }

    QString strMarkdown;
    if (!m_pMdExportRenderer->renderMarkdown(exportContext, &strMarkdown))
    {
        setLastError(m_pMdExportRenderer->lastError());
        return false;
    }

    QString strError;
    if (!WriteMarkdownToFile(strMarkdown, strOutputFilePath, &strError))
    {
        setLastError(strError);
        return false;
    }

    return true;
}

bool QCMdExportService::exportSnippetsToFile(qint64 nSessionId, const QVector<qint64>& vecSnippetIds, const QString& strOutputFilePath)
{
    clearError();

    if (nullptr == m_pExportDataService || nullptr == m_pMdExportRenderer)
    {
        setLastError(QString::fromUtf8("Markdown export service dependencies are not ready."));
        return false;
    }

    if (nSessionId <= 0)
    {
        setLastError(QString::fromUtf8("Study session id is invalid."));
        return false;
    }

    if (strOutputFilePath.trimmed().isEmpty())
    {
        setLastError(QString::fromUtf8("Markdown output file path is empty."));
        return false;
    }

    QCExportContext exportContext;
    if (!m_pExportDataService->buildExportContext(nSessionId, &exportContext))
    {
        setLastError(m_pExportDataService->lastError());
        return false;
    }

    QCExportContext filteredContext;
    QString strError;
    if (!FilterExportContextBySnippetIds(exportContext, vecSnippetIds, &filteredContext, &strError))
    {
        setLastError(strError);
        return false;
    }

    QString strMarkdown;
    if (!m_pMdExportRenderer->renderMarkdown(filteredContext, &strMarkdown))
    {
        setLastError(m_pMdExportRenderer->lastError());
        return false;
    }

    if (!WriteMarkdownToFile(strMarkdown, strOutputFilePath, &strError))
    {
        setLastError(strError);
        return false;
    }

    return true;
}

QString QCMdExportService::lastError() const
{
    return m_strLastError;
}

void QCMdExportService::clearError() const
{
    m_strLastError.clear();
}

void QCMdExportService::setLastError(const QString& strError) const
{
    m_strLastError = strError;
}
