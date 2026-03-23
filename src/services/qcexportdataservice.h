#ifndef QTCLIP_QCEXPORTDATASERVICE_H_
#define QTCLIP_QCEXPORTDATASERVICE_H_

// File: qcexportdataservice.h
// Author: ZZMI1
// Created: 2026-03-23
// Description: Declares the export data assembly service used by the first QtClip Markdown export workflow.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include <QString>
#include <QVector>

#include "../domain/entities/qcattachment.h"
#include "../domain/entities/qcairecord.h"
#include "../domain/entities/qcsnippet.h"
#include "../domain/entities/qcstudysession.h"

class QCAiService;
class QCSessionService;
class QCSnippetService;

struct QCExportSnippetContext
{
    QCSnippet m_snippet;
    bool m_bHasPrimaryAttachment;
    QCAttachment m_primaryAttachment;
    QVector<QCAiRecord> m_vecAiRecords;
};

struct QCExportContext
{
    QCStudySession m_session;
    QVector<QCAiRecord> m_vecSessionAiRecords;
    QVector<QCExportSnippetContext> m_vecSnippetContexts;
};

class QCExportDataService
{
public:
    QCExportDataService(QCSessionService *pSessionService,
                        QCSnippetService *pSnippetService,
                        QCAiService *pAiService);
    ~QCExportDataService();

    bool buildExportContext(qint64 nSessionId, QCExportContext *pExportContext) const;

    QString lastError() const;
    void clearError() const;

private:
    QCExportDataService(const QCExportDataService& other);
    QCExportDataService& operator=(const QCExportDataService& other);

    void setLastError(const QString& strError) const;

private:
    QCSessionService *m_pSessionService;
    QCSnippetService *m_pSnippetService;
    QCAiService *m_pAiService;
    mutable QString m_strLastError;
};

#endif // QTCLIP_QCEXPORTDATASERVICE_H_
