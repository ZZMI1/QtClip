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

    const QString strSessionTitle = exportContext.m_session.title().trimmed().isEmpty()
        ? QString::fromUtf8("Untitled Session")
        : exportContext.m_session.title().trimmed();
    const QString strSessionSummary = findSessionSummary(exportContext);

    int nImageSnippetCount = 0;
    int nArchivedSnippetCount = 0;
    int nFavoriteSnippetCount = 0;
    int nSummarizedSnippetCount = 0;
    for (int i = 0; i < exportContext.m_vecSnippetContexts.size(); ++i)
    {
        const QCExportSnippetContext& snippetContext = exportContext.m_vecSnippetContexts.at(i);
        if (snippetContext.m_snippet.type() == QCSnippetType::ImageSnippetType)
            ++nImageSnippetCount;
        if (snippetContext.m_snippet.isArchived())
            ++nArchivedSnippetCount;
        if (snippetContext.m_snippet.isFavorite())
            ++nFavoriteSnippetCount;
        if (!findSnippetSummary(snippetContext).trimmed().isEmpty())
            ++nSummarizedSnippetCount;
    }

    stream << "# " << strSessionTitle << "\n\n";
    stream << "> Exported from QtClip on " << formatDateTime(QDateTime::currentDateTimeUtc()) << "\n\n";

    stream << "## Session Details\n\n";
    stream << "- Course: " << (exportContext.m_session.courseName().trimmed().isEmpty() ? QString::fromUtf8("N/A") : exportContext.m_session.courseName().trimmed()) << "\n";
    stream << "- Status: " << (exportContext.m_session.status() == QCSessionStatus::FinishedSessionStatus ? QString::fromUtf8("Finished") : QString::fromUtf8("Active")) << "\n";
    stream << "- Started At: " << formatDateTime(exportContext.m_session.startedAt()) << "\n";
    stream << "- Ended At: " << formatDateTime(exportContext.m_session.endedAt()) << "\n";
    if (!exportContext.m_session.description().trimmed().isEmpty())
        stream << "- Description: " << exportContext.m_session.description().trimmed() << "\n";
    stream << "\n";

    stream << "## Overview\n\n";
    stream << "- Total Snippets: " << exportContext.m_vecSnippetContexts.size() << "\n";
    stream << "- Image Snippets: " << nImageSnippetCount << "\n";
    stream << "- Archived Snippets: " << nArchivedSnippetCount << "\n";
    stream << "- Favorite Snippets: " << nFavoriteSnippetCount << "\n";
    stream << "- Snippets With Summary: " << nSummarizedSnippetCount << "\n\n";

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
            : snippetContext.m_snippet.title().trimmed();
        const QString strSnippetSummary = findSnippetSummary(snippetContext);
        QStringList vecStateTokens;
        vecStateTokens.append(snippetContext.m_snippet.type() == QCSnippetType::ImageSnippetType
            ? QString::fromUtf8("Image")
            : snippetContext.m_snippet.type() == QCSnippetType::CodeSnippetType
                ? QString::fromUtf8("Code")
                : QString::fromUtf8("Text"));
        if (snippetContext.m_snippet.isFavorite())
            vecStateTokens.append(QString::fromUtf8("Favorite"));
        if (snippetContext.m_snippet.isArchived())
            vecStateTokens.append(QString::fromUtf8("Archived"));
        if (snippetContext.m_snippet.noteLevel() == QCNoteLevel::ReviewNoteLevel)
            vecStateTokens.append(QString::fromUtf8("Review"));

        stream << "### " << (i + 1) << ". " << strSnippetTitle << "\n\n";
        stream << "- State: " << vecStateTokens.join(QString::fromUtf8(", ")) << "\n";
        stream << "- Captured At: " << formatDateTime(snippetContext.m_snippet.capturedAt()) << "\n";
        if (!snippetContext.m_snippet.source().trimmed().isEmpty())
            stream << "- Source: " << snippetContext.m_snippet.source().trimmed() << "\n";

        if (!snippetContext.m_snippet.note().trimmed().isEmpty())
        {
            stream << "\n#### Note\n\n";
            stream << snippetContext.m_snippet.note().trimmed() << "\n";
        }

        if (!snippetContext.m_snippet.contentText().trimmed().isEmpty())
        {
            stream << "\n#### Content\n\n";
            stream << snippetContext.m_snippet.contentText().trimmed() << "\n";
        }

        if (!strSnippetSummary.isEmpty())
        {
            stream << "\n#### Summary\n\n";
            stream << strSnippetSummary << "\n";
        }

        if (snippetContext.m_bHasPrimaryAttachment)
        {
            stream << "\n#### Attachment\n\n";
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
                if (aiRecord.responseText().trimmed().isEmpty())
                    continue;

                stream << "- " << taskTypeLabel(aiRecord.taskType())
                       << ": " << aiRecord.responseText().trimmed() << "\n";
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
