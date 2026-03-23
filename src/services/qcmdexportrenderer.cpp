// File: qcmdexportrenderer.cpp
// Author: ZZMI1
// Created: 2026-03-23
// Description: Implements the Markdown renderer used by the first QtClip export workflow.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include "qcmdexportrenderer.h"

#include <QDateTime>
#include <QTextStream>

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

    stream << "# " << exportContext.m_session.title() << "\n\n";
    stream << "- Course: " << exportContext.m_session.courseName() << "\n";
    stream << "- Started At: " << formatDateTime(exportContext.m_session.startedAt()) << "\n";
    stream << "- Ended At: " << formatDateTime(exportContext.m_session.endedAt()) << "\n\n";

    const QString strSessionSummary = findSessionSummary(exportContext);
    if (!strSessionSummary.isEmpty())
    {
        stream << "## Session Summary\n\n";
        stream << strSessionSummary << "\n\n";
    }

    stream << "## Snippet Timeline\n\n";
    for (int i = 0; i < exportContext.m_vecSnippetContexts.size(); ++i)
    {
        const QCExportSnippetContext& snippetContext = exportContext.m_vecSnippetContexts.at(i);
        const QString strSnippetTitle = snippetContext.m_snippet.title().trimmed().isEmpty()
            ? QString::fromUtf8("Untitled Snippet")
            : snippetContext.m_snippet.title();
        const QString strSnippetSummary = findSnippetSummary(snippetContext);

        stream << "### " << strSnippetTitle << "\n\n";
        stream << "- Captured At: " << formatDateTime(snippetContext.m_snippet.capturedAt()) << "\n";

        if (!snippetContext.m_snippet.note().trimmed().isEmpty())
            stream << "- Note: " << snippetContext.m_snippet.note() << "\n";

        if (!snippetContext.m_snippet.contentText().trimmed().isEmpty())
            stream << "- Content: " << snippetContext.m_snippet.contentText() << "\n";

        if (!strSnippetSummary.isEmpty())
            stream << "- Summary: " << strSnippetSummary << "\n";

        if (snippetContext.m_bHasPrimaryAttachment)
        {
            stream << "- Image Path: " << snippetContext.m_primaryAttachment.filePath() << "\n\n";
            stream << "![]("
                   << snippetContext.m_primaryAttachment.filePath()
                   << ")\n";
        }

        if (!snippetContext.m_vecAiRecords.isEmpty())
        {
            stream << "\n#### AI Records\n\n";
            for (int j = 0; j < snippetContext.m_vecAiRecords.size(); ++j)
            {
                const QCAiRecord& aiRecord = snippetContext.m_vecAiRecords.at(j);
                stream << "- " << taskTypeLabel(aiRecord.taskType())
                       << ": " << aiRecord.responseText() << "\n";
            }
        }

        stream << "\n";
    }

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
