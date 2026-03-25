// File: qcmockaiprovider.cpp
// Author: ZZMI1
// Created: 2026-03-23
// Description: Implements the mock AI provider used by the first QtClip offline workflow.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include "qcmockaiprovider.h"

#include <QStringList>

namespace
{
QString FirstNonEmptyLine(const QString& strText)
{
    const QStringList vecLines = strText.split('\n');
    for (int i = 0; i < vecLines.size(); ++i)
    {
        const QString strLine = vecLines.at(i).trimmed();
        if (!strLine.isEmpty())
            return strLine;
    }

    return QString();
}

QString CompactText(const QString& strText, int nMaxLength)
{
    QString strValue = strText;
    strValue.replace(QString::fromUtf8("\r"), QString());
    strValue.replace(QString::fromUtf8("\n"), QString::fromUtf8(" "));
    strValue = strValue.simplified();
    if (strValue.size() > nMaxLength)
        strValue = strValue.left(nMaxLength) + QString::fromUtf8("...");
    return strValue;
}

QString ExtractFieldValue(const QStringList& vecLines, const QString& strFieldName)
{
    const QString strPrefix = strFieldName + QString::fromUtf8(":");
    for (int i = 0; i < vecLines.size(); ++i)
    {
        const QString strLine = vecLines.at(i).trimmed();
        if (!strLine.startsWith(strPrefix, Qt::CaseInsensitive))
            continue;

        return strLine.mid(strPrefix.size()).trimmed();
    }

    return QString();
}

QString ExtractSnippetTitleFromSessionLine(const QString& strLine)
{
    const QStringList vecSegments = strLine.split('|');
    for (int i = 0; i < vecSegments.size(); ++i)
    {
        const QString strSegment = vecSegments.at(i).trimmed();
        if (strSegment.startsWith(QString::fromUtf8("Title:"), Qt::CaseInsensitive))
            return strSegment.mid(QString::fromUtf8("Title:").size()).trimmed();
    }

    return QString();
}

bool HasChineseText(const QString& strText)
{
    for (int i = 0; i < strText.size(); ++i)
    {
        const QChar ch = strText.at(i);
        if (ch.unicode() >= 0x4E00 && ch.unicode() <= 0x9FFF)
            return true;
    }

    return false;
}
}

QCMockAiProvider::QCMockAiProvider(const QString& strModelName)
    : m_strModelName(strModelName.trimmed().isEmpty() ? QString::fromUtf8("mock-summary-v1") : strModelName)
    , m_strLastError()
{
}

QCMockAiProvider::~QCMockAiProvider()
{
}

bool QCMockAiProvider::generateText(const QCAiProviderRequest& aiRequest,
                                    QCAiProviderResponse *pAiResponse)
{
    clearError();

    if (nullptr == pAiResponse)
    {
        setLastError(QString::fromUtf8("AI response output pointer is null."));
        return false;
    }

    pAiResponse->m_strProviderName = providerName();
    pAiResponse->m_strModelName = m_strModelName;
    pAiResponse->m_strText = buildMockText(aiRequest);
    pAiResponse->m_strRawResponse = pAiResponse->m_strText;
    pAiResponse->m_strEndpoint = QString::fromUtf8("mock://local");
    pAiResponse->m_nHttpStatusCode = 0;
    pAiResponse->m_strErrorSummary.clear();
    return true;
}

QString QCMockAiProvider::providerName() const
{
    return QString::fromUtf8("mock");
}

QString QCMockAiProvider::lastError() const
{
    return m_strLastError;
}

void QCMockAiProvider::clearError() const
{
    m_strLastError.clear();
}

QString QCMockAiProvider::buildMockText(const QCAiProviderRequest& aiRequest) const
{
    const QString strPrompt = aiRequest.m_strUserPrompt;
    const QStringList vecLines = strPrompt.split('\n');
    const bool bChinese = HasChineseText(strPrompt);

    if (strPrompt.contains(QString::fromUtf8("connection"), Qt::CaseInsensitive)
        || strPrompt.contains(QString::fromUtf8("confirmation"), Qt::CaseInsensitive)
        || strPrompt.contains(QString::fromUtf8("可达")))
    {
        return bChinese
            ? QString::fromUtf8("连接正常，Mock Provider 可用。")
            : QString::fromUtf8("Connection confirmed. Mock provider is reachable.");
    }

    const QString strSessionTitle = ExtractFieldValue(vecLines, QString::fromUtf8("Session Title"));
    if (!strSessionTitle.isEmpty())
    {
        int nSnippetCount = 0;
        QStringList vecSnippetTitles;
        for (int i = 0; i < vecLines.size(); ++i)
        {
            const QString strLine = vecLines.at(i).trimmed();
            if (!strLine.startsWith(QString::fromUtf8("- Type:"), Qt::CaseInsensitive))
                continue;

            ++nSnippetCount;
            const QString strSnippetTitle = ExtractSnippetTitleFromSessionLine(strLine);
            if (!strSnippetTitle.isEmpty() && vecSnippetTitles.size() < 2)
                vecSnippetTitles.append(CompactText(strSnippetTitle, 28));
        }

        const QString strCourse = CompactText(ExtractFieldValue(vecLines, QString::fromUtf8("Course")), 24);
        const QString strFocus = vecSnippetTitles.isEmpty()
            ? (bChinese ? QString::fromUtf8("本次学习要点需要进一步整理。") : QString::fromUtf8("Key points need further consolidation."))
            : (bChinese
                ? QString::fromUtf8("重点围绕 %1。")
                      .arg(vecSnippetTitles.join(QString::fromUtf8("、")))
                : QString::fromUtf8("Focus on %1.")
                      .arg(vecSnippetTitles.join(QString::fromUtf8(", "))));

        if (bChinese)
        {
            return QString::fromUtf8("本次 Session「%1」共整理 %2 条学习片段（课程：%3）。%4 建议下一步按概念梳理并补充关键细节。")
                .arg(CompactText(strSessionTitle, 32))
                .arg(nSnippetCount)
                .arg(strCourse.isEmpty() ? QString::fromUtf8("未填写") : strCourse)
                .arg(strFocus);
        }

        return QString::fromUtf8("Session '%1' includes %2 learning snippets (course: %3). %4 Next step: review concepts and fill in missing details.")
            .arg(CompactText(strSessionTitle, 32))
            .arg(nSnippetCount)
            .arg(strCourse.isEmpty() ? QString::fromUtf8("N/A") : strCourse)
            .arg(strFocus);
    }

    const QString strTitle = CompactText(ExtractFieldValue(vecLines, QString::fromUtf8("Title")), 36);
    const QString strNote = CompactText(ExtractFieldValue(vecLines, QString::fromUtf8("Note")), 48);
    const QString strContent = CompactText(ExtractFieldValue(vecLines, QString::fromUtf8("Content")), 64);
    const QString strMainPoint = !strContent.isEmpty() ? strContent : (!strTitle.isEmpty() ? strTitle : QString());

    if (!strMainPoint.isEmpty())
    {
        if (bChinese)
        {
            return QString::fromUtf8("本条学习片段的核心是：%1。建议复习重点：%2")
                .arg(strMainPoint)
                .arg(strNote.isEmpty() ? QString::fromUtf8("结合原图和上下文补全关键概念。") : strNote);
        }

        return QString::fromUtf8("Main point of this snippet: %1. Suggested review focus: %2")
            .arg(strMainPoint)
            .arg(strNote.isEmpty() ? QString::fromUtf8("Re-check the source image/text and complete key concepts.") : strNote);
    }

    const QString strFallback = CompactText(FirstNonEmptyLine(strPrompt), 90);
    if (strFallback.isEmpty())
        return bChinese ? QString::fromUtf8("暂无可总结内容。") : QString::fromUtf8("No source content to summarize.");

    return bChinese
        ? QString::fromUtf8("已完成总结：%1").arg(strFallback)
        : QString::fromUtf8("Summary generated: %1").arg(strFallback);
}

void QCMockAiProvider::setLastError(const QString& strError) const
{
    m_strLastError = strError;
}
