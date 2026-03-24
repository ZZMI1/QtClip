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
#include <QTextStream>

#include "qcexportdataservice.h"
#include "qcmdexportrenderer.h"

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
    if (nullptr == pExportPreview)
        return false;

    pExportPreview->m_strSessionTitle.clear();
    pExportPreview->m_strCourseName.clear();
    pExportPreview->m_nSnippetCount = 0;
    pExportPreview->m_nImageSnippetCount = 0;
    pExportPreview->m_nArchivedSnippetCount = 0;
    pExportPreview->m_nFavoriteSnippetCount = 0;
    pExportPreview->m_nSummarizedSnippetCount = 0;
    pExportPreview->m_bHasSessionSummary = false;

    if (nullptr == m_pExportDataService)
        return false;

    QCExportContext exportContext;
    if (!m_pExportDataService->buildExportContext(nSessionId, &exportContext))
        return false;

    pExportPreview->m_strSessionTitle = exportContext.m_session.title();
    pExportPreview->m_strCourseName = exportContext.m_session.courseName();
    pExportPreview->m_nSnippetCount = exportContext.m_vecSnippetContexts.size();
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

    QFileInfo fileInfo(strOutputFilePath);
    QDir outputDirectory = fileInfo.dir();
    if (!outputDirectory.exists() && !outputDirectory.mkpath(QString::fromUtf8(".")))
    {
        setLastError(QString::fromUtf8("Failed to create Markdown output directory."));
        return false;
    }

    QFile outputFile(strOutputFilePath);
    if (!outputFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
    {
        setLastError(QString::fromUtf8("Failed to open Markdown output file."));
        return false;
    }

    QTextStream outputStream(&outputFile);
    outputStream << strMarkdown;
    outputFile.close();

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
