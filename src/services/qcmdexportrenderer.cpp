// File: qcmdexportrenderer.cpp
// Author: ZZMI1
// Created: 2026-03-23
// Description: Implements the Markdown renderer used by the first QtClip export workflow.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include "qcmdexportrenderer.h"

#include <QDateTime>
#include <QTextStream>
#include <QStringList>

QCMdExportRenderer::QCMdExportRenderer()
    : m_strLastError()
{
}

QCMdExportRenderer::~QCMdExportRenderer()
{
}

bool QCMdExportRenderer::renderMarkdown(const QCExportContext& exportContext, QString *pstrMarkdown) const
{
    clearError();

    if (nullptr == pstrMarkdown)
    {
        setLastError(QString::fromUtf8("Markdown output pointer is null."));
        return false;
    }

    if (exportContext.m_session.id() <= 0)
    {
        setLastError(QString::fromUtf8("Export context session is invalid."));
        return false;
    }

    QString strMarkdown;
    QTextStream stream(&strMarkdown);

    const QString strSessionTitle = exportContext.m_session.title().trimmed().isEmpty()
        ? QString::fromUtf8("Untitled Session")
        : exportContext.m_session.title().trimmed();

    QString strAiSummary = findSessionSummary(exportContext).trimmed();
    if (strAiSummary.isEmpty())
    {
        QStringList vecSummaries;
        for (int i = 0; i < exportContext.m_vecSnippetContexts.size(); ++i)
        {
            const QString strSnippetSummary = findSnippetSummary(exportContext.m_vecSnippetContexts.at(i)).trimmed();
            if (!strSnippetSummary.isEmpty())
                vecSummaries.append(strSnippetSummary);
        }
        strAiSummary = vecSummaries.join(QString::fromUtf8("\n\n"));
    }

    stream << "# " << strSessionTitle << "\n\n";
    const QString strAiSummaryLabel = QString::fromWCharArray(L"AI\u603b\u7ed3");
    const QString strEmptySummaryLabel = QString::fromWCharArray(L"\u6682\u65e0 AI\u603b\u7ed3");
    stream << "## " << strAiSummaryLabel << "\n\n";
    stream << (strAiSummary.isEmpty() ? strEmptySummaryLabel : strAiSummary) << "\n";

    *pstrMarkdown = strMarkdown;
    return true;
}

QString QCMdExportRenderer::lastError() const
{
    return m_strLastError;
}

void QCMdExportRenderer::clearError() const
{
    m_strLastError.clear();
}

QString QCMdExportRenderer::formatDateTime(const QDateTime& dateTimeValue) const
{
    if (!dateTimeValue.isValid())
        return QString::fromUtf8("N/A");

    return dateTimeValue.toString(Qt::ISODate);
}

QString QCMdExportRenderer::findSnippetSummary(const QCExportSnippetContext& snippetContext) const
{
    if (!snippetContext.m_snippet.summary().trimmed().isEmpty())
        return snippetContext.m_snippet.summary();

    for (int i = 0; i < snippetContext.m_vecAiRecords.size(); ++i)
    {
        const QCAiRecord& aiRecord = snippetContext.m_vecAiRecords.at(i);
        if (aiRecord.taskType() == QCAiTaskType::SnippetSummaryTask && !aiRecord.responseText().trimmed().isEmpty())
            return aiRecord.responseText();
    }

    return QString();
}

QString QCMdExportRenderer::findSessionSummary(const QCExportContext& exportContext) const
{
    for (int i = 0; i < exportContext.m_vecSessionAiRecords.size(); ++i)
    {
        const QCAiRecord& aiRecord = exportContext.m_vecSessionAiRecords.at(i);
        if (aiRecord.taskType() == QCAiTaskType::SessionSummaryTask && !aiRecord.responseText().trimmed().isEmpty())
            return aiRecord.responseText();
    }

    return QString();
}

QString QCMdExportRenderer::taskTypeLabel(QCAiTaskType aiTaskType) const
{
    switch (aiTaskType)
    {
    case QCAiTaskType::SnippetTitleTask:
        return QString::fromUtf8("Snippet Title");
    case QCAiTaskType::SnippetSummaryTask:
        return QString::fromUtf8("Snippet Summary");
    case QCAiTaskType::SnippetTagsTask:
        return QString::fromUtf8("Snippet Tags");
    case QCAiTaskType::SessionSummaryTask:
        return QString::fromUtf8("Session Summary");
    }

    return QString::fromUtf8("AI Task");
}

void QCMdExportRenderer::setLastError(const QString& strError) const
{
    m_strLastError = strError;
}
