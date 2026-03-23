#ifndef QTCLIP_QCDBENUMCODEC_H_
#define QTCLIP_QCDBENUMCODEC_H_

// File: qcdbenumcodec.h
// Author: ZZMI1
// Created: 2026-03-23
// Description: Declares enum to database text conversion helpers used by QtClip repositories.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include <QString>

#include "../../domain/entities/qcairecord.h"
#include "../../domain/entities/qcsnippet.h"
#include "../../domain/entities/qcstudysession.h"

class QCDbEnumCodec
{
public:
    QCDbEnumCodec() = delete;
    ~QCDbEnumCodec() = delete;

    static QString toDbValue(QCSessionStatus sessionStatus);
    static QString toDbValue(QCSnippetType snippetType);
    static QString toDbValue(QCCaptureType captureType);
    static QString toDbValue(QCNoteKind noteKind);
    static QString toDbValue(QCNoteLevel noteLevel);
    static QString toDbValue(QCAiTaskType aiTaskType);

    static bool toDbValue(QCSessionStatus sessionStatus, QString *pstrValue, QString *pstrErrorMessage);
    static bool toDbValue(QCSnippetType snippetType, QString *pstrValue, QString *pstrErrorMessage);
    static bool toDbValue(QCCaptureType captureType, QString *pstrValue, QString *pstrErrorMessage);
    static bool toDbValue(QCNoteKind noteKind, QString *pstrValue, QString *pstrErrorMessage);
    static bool toDbValue(QCNoteLevel noteLevel, QString *pstrValue, QString *pstrErrorMessage);
    static bool toDbValue(QCAiTaskType aiTaskType, QString *pstrValue, QString *pstrErrorMessage);

    static bool fromDbValue(const QString& strValue, QCSessionStatus *pSessionStatus, QString *pstrErrorMessage);
    static bool fromDbValue(const QString& strValue, QCSnippetType *pSnippetType, QString *pstrErrorMessage);
    static bool fromDbValue(const QString& strValue, QCCaptureType *pCaptureType, QString *pstrErrorMessage);
    static bool fromDbValue(const QString& strValue, QCNoteKind *pNoteKind, QString *pstrErrorMessage);
    static bool fromDbValue(const QString& strValue, QCNoteLevel *pNoteLevel, QString *pstrErrorMessage);
    static bool fromDbValue(const QString& strValue, QCAiTaskType *pAiTaskType, QString *pstrErrorMessage);

private:
    static QString buildInvalidEnumValueMessage(const QString& strEnumName, const QString& strValue);
    static QString buildInvalidEnumOutputMessage(const QString& strEnumName);
    static QString buildNullOutputPointerMessage(const QString& strEnumName);
};

#endif // QTCLIP_QCDBENUMCODEC_H_
