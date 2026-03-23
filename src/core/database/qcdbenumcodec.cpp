// File: qcdbenumcodec.cpp
// Author: ZZMI1
// Created: 2026-03-23
// Description: Implements enum to database text conversion helpers used by QtClip repositories.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include "qcdbenumcodec.h"

namespace
{
// Database TEXT values for QCSessionStatus.
static const char *g_pszActiveSessionStatus = "ActiveSessionStatus";
static const char *g_pszFinishedSessionStatus = "FinishedSessionStatus";

// Database TEXT values for QCSnippetType.
static const char *g_pszImageSnippetType = "ImageSnippetType";
static const char *g_pszTextSnippetType = "TextSnippetType";
static const char *g_pszCodeSnippetType = "CodeSnippetType";

// Database TEXT values for QCCaptureType.
static const char *g_pszScreenCaptureType = "ScreenCaptureType";
static const char *g_pszManualCaptureType = "ManualCaptureType";
static const char *g_pszImportCaptureType = "ImportCaptureType";

// Database TEXT values for QCNoteKind.
static const char *g_pszConceptNoteKind = "ConceptNoteKind";
static const char *g_pszCodeNoteKind = "CodeNoteKind";
static const char *g_pszHomeworkNoteKind = "HomeworkNoteKind";
static const char *g_pszDefinitionNoteKind = "DefinitionNoteKind";
static const char *g_pszQuestionNoteKind = "QuestionNoteKind";
static const char *g_pszReviewNoteKind = "ReviewNoteKind";

// Database TEXT values for QCNoteLevel.
static const char *g_pszNormalNoteLevel = "NormalNoteLevel";
static const char *g_pszImportantNoteLevel = "ImportantNoteLevel";
static const char *g_pszReviewNoteLevel = "ReviewNoteLevel";

// Database TEXT values for QCAiTaskType.
static const char *g_pszSnippetTitleTask = "SnippetTitleTask";
static const char *g_pszSnippetSummaryTask = "SnippetSummaryTask";
static const char *g_pszSnippetTagsTask = "SnippetTagsTask";
static const char *g_pszSessionSummaryTask = "SessionSummaryTask";
}

QString QCDbEnumCodec::toDbValue(QCSessionStatus sessionStatus)
{
    QString strValue;
    QString strErrorMessage;
    if (!toDbValue(sessionStatus, &strValue, &strErrorMessage))
        return QString();

    return strValue;
}

QString QCDbEnumCodec::toDbValue(QCSnippetType snippetType)
{
    QString strValue;
    QString strErrorMessage;
    if (!toDbValue(snippetType, &strValue, &strErrorMessage))
        return QString();

    return strValue;
}

QString QCDbEnumCodec::toDbValue(QCCaptureType captureType)
{
    QString strValue;
    QString strErrorMessage;
    if (!toDbValue(captureType, &strValue, &strErrorMessage))
        return QString();

    return strValue;
}

QString QCDbEnumCodec::toDbValue(QCNoteKind noteKind)
{
    QString strValue;
    QString strErrorMessage;
    if (!toDbValue(noteKind, &strValue, &strErrorMessage))
        return QString();

    return strValue;
}

QString QCDbEnumCodec::toDbValue(QCNoteLevel noteLevel)
{
    QString strValue;
    QString strErrorMessage;
    if (!toDbValue(noteLevel, &strValue, &strErrorMessage))
        return QString();

    return strValue;
}

QString QCDbEnumCodec::toDbValue(QCAiTaskType aiTaskType)
{
    QString strValue;
    QString strErrorMessage;
    if (!toDbValue(aiTaskType, &strValue, &strErrorMessage))
        return QString();

    return strValue;
}

bool QCDbEnumCodec::toDbValue(QCSessionStatus sessionStatus,
                              QString *pstrValue,
                              QString *pstrErrorMessage)
{
    if (nullptr == pstrValue)
    {
        if (nullptr != pstrErrorMessage)
            *pstrErrorMessage = buildNullOutputPointerMessage(QString::fromUtf8("QCSessionStatus"));

        return false;
    }

    switch (sessionStatus)
    {
    case QCSessionStatus::ActiveSessionStatus:
        *pstrValue = QString::fromUtf8(g_pszActiveSessionStatus);
        break;
    case QCSessionStatus::FinishedSessionStatus:
        *pstrValue = QString::fromUtf8(g_pszFinishedSessionStatus);
        break;
    default:
        if (nullptr != pstrErrorMessage)
            *pstrErrorMessage = buildInvalidEnumOutputMessage(QString::fromUtf8("QCSessionStatus"));

        return false;
    }

    if (nullptr != pstrErrorMessage)
        pstrErrorMessage->clear();

    return true;
}

bool QCDbEnumCodec::toDbValue(QCSnippetType snippetType,
                              QString *pstrValue,
                              QString *pstrErrorMessage)
{
    if (nullptr == pstrValue)
    {
        if (nullptr != pstrErrorMessage)
            *pstrErrorMessage = buildNullOutputPointerMessage(QString::fromUtf8("QCSnippetType"));

        return false;
    }

    switch (snippetType)
    {
    case QCSnippetType::ImageSnippetType:
        *pstrValue = QString::fromUtf8(g_pszImageSnippetType);
        break;
    case QCSnippetType::TextSnippetType:
        *pstrValue = QString::fromUtf8(g_pszTextSnippetType);
        break;
    case QCSnippetType::CodeSnippetType:
        *pstrValue = QString::fromUtf8(g_pszCodeSnippetType);
        break;
    default:
        if (nullptr != pstrErrorMessage)
            *pstrErrorMessage = buildInvalidEnumOutputMessage(QString::fromUtf8("QCSnippetType"));

        return false;
    }

    if (nullptr != pstrErrorMessage)
        pstrErrorMessage->clear();

    return true;
}

bool QCDbEnumCodec::toDbValue(QCCaptureType captureType,
                              QString *pstrValue,
                              QString *pstrErrorMessage)
{
    if (nullptr == pstrValue)
    {
        if (nullptr != pstrErrorMessage)
            *pstrErrorMessage = buildNullOutputPointerMessage(QString::fromUtf8("QCCaptureType"));

        return false;
    }

    switch (captureType)
    {
    case QCCaptureType::ScreenCaptureType:
        *pstrValue = QString::fromUtf8(g_pszScreenCaptureType);
        break;
    case QCCaptureType::ManualCaptureType:
        *pstrValue = QString::fromUtf8(g_pszManualCaptureType);
        break;
    case QCCaptureType::ImportCaptureType:
        *pstrValue = QString::fromUtf8(g_pszImportCaptureType);
        break;
    default:
        if (nullptr != pstrErrorMessage)
            *pstrErrorMessage = buildInvalidEnumOutputMessage(QString::fromUtf8("QCCaptureType"));

        return false;
    }

    if (nullptr != pstrErrorMessage)
        pstrErrorMessage->clear();

    return true;
}

bool QCDbEnumCodec::toDbValue(QCNoteKind noteKind,
                              QString *pstrValue,
                              QString *pstrErrorMessage)
{
    if (nullptr == pstrValue)
    {
        if (nullptr != pstrErrorMessage)
            *pstrErrorMessage = buildNullOutputPointerMessage(QString::fromUtf8("QCNoteKind"));

        return false;
    }

    switch (noteKind)
    {
    case QCNoteKind::ConceptNoteKind:
        *pstrValue = QString::fromUtf8(g_pszConceptNoteKind);
        break;
    case QCNoteKind::CodeNoteKind:
        *pstrValue = QString::fromUtf8(g_pszCodeNoteKind);
        break;
    case QCNoteKind::HomeworkNoteKind:
        *pstrValue = QString::fromUtf8(g_pszHomeworkNoteKind);
        break;
    case QCNoteKind::DefinitionNoteKind:
        *pstrValue = QString::fromUtf8(g_pszDefinitionNoteKind);
        break;
    case QCNoteKind::QuestionNoteKind:
        *pstrValue = QString::fromUtf8(g_pszQuestionNoteKind);
        break;
    case QCNoteKind::ReviewNoteKind:
        *pstrValue = QString::fromUtf8(g_pszReviewNoteKind);
        break;
    default:
        if (nullptr != pstrErrorMessage)
            *pstrErrorMessage = buildInvalidEnumOutputMessage(QString::fromUtf8("QCNoteKind"));

        return false;
    }

    if (nullptr != pstrErrorMessage)
        pstrErrorMessage->clear();

    return true;
}

bool QCDbEnumCodec::toDbValue(QCNoteLevel noteLevel,
                              QString *pstrValue,
                              QString *pstrErrorMessage)
{
    if (nullptr == pstrValue)
    {
        if (nullptr != pstrErrorMessage)
            *pstrErrorMessage = buildNullOutputPointerMessage(QString::fromUtf8("QCNoteLevel"));

        return false;
    }

    switch (noteLevel)
    {
    case QCNoteLevel::NormalNoteLevel:
        *pstrValue = QString::fromUtf8(g_pszNormalNoteLevel);
        break;
    case QCNoteLevel::ImportantNoteLevel:
        *pstrValue = QString::fromUtf8(g_pszImportantNoteLevel);
        break;
    case QCNoteLevel::ReviewNoteLevel:
        *pstrValue = QString::fromUtf8(g_pszReviewNoteLevel);
        break;
    default:
        if (nullptr != pstrErrorMessage)
            *pstrErrorMessage = buildInvalidEnumOutputMessage(QString::fromUtf8("QCNoteLevel"));

        return false;
    }

    if (nullptr != pstrErrorMessage)
        pstrErrorMessage->clear();

    return true;
}

bool QCDbEnumCodec::toDbValue(QCAiTaskType aiTaskType,
                              QString *pstrValue,
                              QString *pstrErrorMessage)
{
    if (nullptr == pstrValue)
    {
        if (nullptr != pstrErrorMessage)
            *pstrErrorMessage = buildNullOutputPointerMessage(QString::fromUtf8("QCAiTaskType"));

        return false;
    }

    switch (aiTaskType)
    {
    case QCAiTaskType::SnippetTitleTask:
        *pstrValue = QString::fromUtf8(g_pszSnippetTitleTask);
        break;
    case QCAiTaskType::SnippetSummaryTask:
        *pstrValue = QString::fromUtf8(g_pszSnippetSummaryTask);
        break;
    case QCAiTaskType::SnippetTagsTask:
        *pstrValue = QString::fromUtf8(g_pszSnippetTagsTask);
        break;
    case QCAiTaskType::SessionSummaryTask:
        *pstrValue = QString::fromUtf8(g_pszSessionSummaryTask);
        break;
    default:
        if (nullptr != pstrErrorMessage)
            *pstrErrorMessage = buildInvalidEnumOutputMessage(QString::fromUtf8("QCAiTaskType"));

        return false;
    }

    if (nullptr != pstrErrorMessage)
        pstrErrorMessage->clear();

    return true;
}

bool QCDbEnumCodec::fromDbValue(const QString& strValue,
                                QCSessionStatus *pSessionStatus,
                                QString *pstrErrorMessage)
{
    if (nullptr == pSessionStatus)
    {
        if (nullptr != pstrErrorMessage)
            *pstrErrorMessage = buildNullOutputPointerMessage(QString::fromUtf8("QCSessionStatus"));

        return false;
    }

    if (strValue == QString::fromUtf8(g_pszActiveSessionStatus))
    {
        *pSessionStatus = QCSessionStatus::ActiveSessionStatus;
        if (nullptr != pstrErrorMessage)
            pstrErrorMessage->clear();

        return true;
    }

    if (strValue == QString::fromUtf8(g_pszFinishedSessionStatus))
    {
        *pSessionStatus = QCSessionStatus::FinishedSessionStatus;
        if (nullptr != pstrErrorMessage)
            pstrErrorMessage->clear();

        return true;
    }

    if (nullptr != pstrErrorMessage)
        *pstrErrorMessage = buildInvalidEnumValueMessage(QString::fromUtf8("QCSessionStatus"), strValue);

    return false;
}

bool QCDbEnumCodec::fromDbValue(const QString& strValue,
                                QCSnippetType *pSnippetType,
                                QString *pstrErrorMessage)
{
    if (nullptr == pSnippetType)
    {
        if (nullptr != pstrErrorMessage)
            *pstrErrorMessage = buildNullOutputPointerMessage(QString::fromUtf8("QCSnippetType"));

        return false;
    }

    if (strValue == QString::fromUtf8(g_pszImageSnippetType))
    {
        *pSnippetType = QCSnippetType::ImageSnippetType;
        if (nullptr != pstrErrorMessage)
            pstrErrorMessage->clear();

        return true;
    }

    if (strValue == QString::fromUtf8(g_pszTextSnippetType))
    {
        *pSnippetType = QCSnippetType::TextSnippetType;
        if (nullptr != pstrErrorMessage)
            pstrErrorMessage->clear();

        return true;
    }

    if (strValue == QString::fromUtf8(g_pszCodeSnippetType))
    {
        *pSnippetType = QCSnippetType::CodeSnippetType;
        if (nullptr != pstrErrorMessage)
            pstrErrorMessage->clear();

        return true;
    }

    if (nullptr != pstrErrorMessage)
        *pstrErrorMessage = buildInvalidEnumValueMessage(QString::fromUtf8("QCSnippetType"), strValue);

    return false;
}

bool QCDbEnumCodec::fromDbValue(const QString& strValue,
                                QCCaptureType *pCaptureType,
                                QString *pstrErrorMessage)
{
    if (nullptr == pCaptureType)
    {
        if (nullptr != pstrErrorMessage)
            *pstrErrorMessage = buildNullOutputPointerMessage(QString::fromUtf8("QCCaptureType"));

        return false;
    }

    if (strValue == QString::fromUtf8(g_pszScreenCaptureType))
    {
        *pCaptureType = QCCaptureType::ScreenCaptureType;
        if (nullptr != pstrErrorMessage)
            pstrErrorMessage->clear();

        return true;
    }

    if (strValue == QString::fromUtf8(g_pszManualCaptureType))
    {
        *pCaptureType = QCCaptureType::ManualCaptureType;
        if (nullptr != pstrErrorMessage)
            pstrErrorMessage->clear();

        return true;
    }

    if (strValue == QString::fromUtf8(g_pszImportCaptureType))
    {
        *pCaptureType = QCCaptureType::ImportCaptureType;
        if (nullptr != pstrErrorMessage)
            pstrErrorMessage->clear();

        return true;
    }

    if (nullptr != pstrErrorMessage)
        *pstrErrorMessage = buildInvalidEnumValueMessage(QString::fromUtf8("QCCaptureType"), strValue);

    return false;
}

bool QCDbEnumCodec::fromDbValue(const QString& strValue,
                                QCNoteKind *pNoteKind,
                                QString *pstrErrorMessage)
{
    if (nullptr == pNoteKind)
    {
        if (nullptr != pstrErrorMessage)
            *pstrErrorMessage = buildNullOutputPointerMessage(QString::fromUtf8("QCNoteKind"));

        return false;
    }

    if (strValue == QString::fromUtf8(g_pszConceptNoteKind))
    {
        *pNoteKind = QCNoteKind::ConceptNoteKind;
        if (nullptr != pstrErrorMessage)
            pstrErrorMessage->clear();

        return true;
    }

    if (strValue == QString::fromUtf8(g_pszCodeNoteKind))
    {
        *pNoteKind = QCNoteKind::CodeNoteKind;
        if (nullptr != pstrErrorMessage)
            pstrErrorMessage->clear();

        return true;
    }

    if (strValue == QString::fromUtf8(g_pszHomeworkNoteKind))
    {
        *pNoteKind = QCNoteKind::HomeworkNoteKind;
        if (nullptr != pstrErrorMessage)
            pstrErrorMessage->clear();

        return true;
    }

    if (strValue == QString::fromUtf8(g_pszDefinitionNoteKind))
    {
        *pNoteKind = QCNoteKind::DefinitionNoteKind;
        if (nullptr != pstrErrorMessage)
            pstrErrorMessage->clear();

        return true;
    }

    if (strValue == QString::fromUtf8(g_pszQuestionNoteKind))
    {
        *pNoteKind = QCNoteKind::QuestionNoteKind;
        if (nullptr != pstrErrorMessage)
            pstrErrorMessage->clear();

        return true;
    }

    if (strValue == QString::fromUtf8(g_pszReviewNoteKind))
    {
        *pNoteKind = QCNoteKind::ReviewNoteKind;
        if (nullptr != pstrErrorMessage)
            pstrErrorMessage->clear();

        return true;
    }

    if (nullptr != pstrErrorMessage)
        *pstrErrorMessage = buildInvalidEnumValueMessage(QString::fromUtf8("QCNoteKind"), strValue);

    return false;
}

bool QCDbEnumCodec::fromDbValue(const QString& strValue,
                                QCNoteLevel *pNoteLevel,
                                QString *pstrErrorMessage)
{
    if (nullptr == pNoteLevel)
    {
        if (nullptr != pstrErrorMessage)
            *pstrErrorMessage = buildNullOutputPointerMessage(QString::fromUtf8("QCNoteLevel"));

        return false;
    }

    if (strValue == QString::fromUtf8(g_pszNormalNoteLevel))
    {
        *pNoteLevel = QCNoteLevel::NormalNoteLevel;
        if (nullptr != pstrErrorMessage)
            pstrErrorMessage->clear();

        return true;
    }

    if (strValue == QString::fromUtf8(g_pszImportantNoteLevel))
    {
        *pNoteLevel = QCNoteLevel::ImportantNoteLevel;
        if (nullptr != pstrErrorMessage)
            pstrErrorMessage->clear();

        return true;
    }

    if (strValue == QString::fromUtf8(g_pszReviewNoteLevel))
    {
        *pNoteLevel = QCNoteLevel::ReviewNoteLevel;
        if (nullptr != pstrErrorMessage)
            pstrErrorMessage->clear();

        return true;
    }

    if (nullptr != pstrErrorMessage)
        *pstrErrorMessage = buildInvalidEnumValueMessage(QString::fromUtf8("QCNoteLevel"), strValue);

    return false;
}

bool QCDbEnumCodec::fromDbValue(const QString& strValue,
                                QCAiTaskType *pAiTaskType,
                                QString *pstrErrorMessage)
{
    if (nullptr == pAiTaskType)
    {
        if (nullptr != pstrErrorMessage)
            *pstrErrorMessage = buildNullOutputPointerMessage(QString::fromUtf8("QCAiTaskType"));

        return false;
    }

    if (strValue == QString::fromUtf8(g_pszSnippetTitleTask))
    {
        *pAiTaskType = QCAiTaskType::SnippetTitleTask;
        if (nullptr != pstrErrorMessage)
            pstrErrorMessage->clear();

        return true;
    }

    if (strValue == QString::fromUtf8(g_pszSnippetSummaryTask))
    {
        *pAiTaskType = QCAiTaskType::SnippetSummaryTask;
        if (nullptr != pstrErrorMessage)
            pstrErrorMessage->clear();

        return true;
    }

    if (strValue == QString::fromUtf8(g_pszSnippetTagsTask))
    {
        *pAiTaskType = QCAiTaskType::SnippetTagsTask;
        if (nullptr != pstrErrorMessage)
            pstrErrorMessage->clear();

        return true;
    }

    if (strValue == QString::fromUtf8(g_pszSessionSummaryTask))
    {
        *pAiTaskType = QCAiTaskType::SessionSummaryTask;
        if (nullptr != pstrErrorMessage)
            pstrErrorMessage->clear();

        return true;
    }

    if (nullptr != pstrErrorMessage)
        *pstrErrorMessage = buildInvalidEnumValueMessage(QString::fromUtf8("QCAiTaskType"), strValue);

    return false;
}

QString QCDbEnumCodec::buildInvalidEnumValueMessage(const QString& strEnumName, const QString& strValue)
{
    return QString::fromUtf8("Invalid database enum value for %1: %2")
        .arg(strEnumName, strValue);
}

QString QCDbEnumCodec::buildInvalidEnumOutputMessage(const QString& strEnumName)
{
    return QString::fromUtf8("Unsupported enum value for %1 database encoding.")
        .arg(strEnumName);
}

QString QCDbEnumCodec::buildNullOutputPointerMessage(const QString& strEnumName)
{
    return QString::fromUtf8("%1 output pointer is null.")
        .arg(strEnumName);
}
