// File: qcexportdataservice.cpp
// Author: ZZMI1
// Created: 2026-03-23
// Description: Implements the export data assembly service used by the first QtClip Markdown export workflow.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include "qcexportdataservice.h"

#include "qcaiservice.h"
#include "qcsessionservice.h"
#include "qcsnippetservice.h"

QCExportDataService::QCExportDataService(QCSessionService *pSessionService,
                                         QCSnippetService *pSnippetService,
                                         QCAiService *pAiService)
    : m_pSessionService(pSessionService)
    , m_pSnippetService(pSnippetService)
    , m_pAiService(pAiService)
    , m_strLastError()
{
}

QCExportDataService::~QCExportDataService()
{
}

bool QCExportDataService::buildExportContext(qint64 nSessionId, QCExportContext *pExportContext) const
{
    clearError();

    if (nullptr == pExportContext)
    {
        setLastError(QString::fromUtf8("QCExportContext output pointer is null."));
        return false;
    }

    if (nullptr == m_pSessionService || nullptr == m_pSnippetService || nullptr == m_pAiService)
    {
        setLastError(QString::fromUtf8("Export data service dependencies are not ready."));
        return false;
    }

    if (nSessionId <= 0)
    {
        setLastError(QString::fromUtf8("Study session id is invalid."));
        return false;
    }

    QCExportContext exportContext;
    if (!m_pSessionService->getSessionById(nSessionId, &exportContext.m_session))
    {
        setLastError(m_pSessionService->lastError());
        return false;
    }

    exportContext.m_vecSessionAiRecords = m_pAiService->listAiRecordsBySession(nSessionId);
    if (!m_pAiService->lastError().isEmpty())
    {
        setLastError(m_pAiService->lastError());
        return false;
    }

    const QVector<QCSnippet> vecSnippets = m_pSnippetService->listSnippetsBySession(nSessionId);
    if (!m_pSnippetService->lastError().isEmpty())
    {
        setLastError(m_pSnippetService->lastError());
        return false;
    }

    for (int i = 0; i < vecSnippets.size(); ++i)
    {
        QCExportSnippetContext snippetContext;
        snippetContext.m_snippet = vecSnippets.at(i);
        snippetContext.m_bHasPrimaryAttachment = false;

        if (snippetContext.m_snippet.type() == QCSnippetType::ImageSnippetType)
        {
            if (!m_pSnippetService->getPrimaryAttachmentBySnippetId(snippetContext.m_snippet.id(),
                                                                    &snippetContext.m_primaryAttachment))
            {
                setLastError(m_pSnippetService->lastError());
                return false;
            }

            snippetContext.m_bHasPrimaryAttachment = true;
        }

        snippetContext.m_vecAiRecords = m_pAiService->listAiRecordsBySnippet(snippetContext.m_snippet.id());
        if (!m_pAiService->lastError().isEmpty())
        {
            setLastError(m_pAiService->lastError());
            return false;
        }

        exportContext.m_vecSnippetContexts.append(snippetContext);
    }

    *pExportContext = exportContext;
    return true;
}

QString QCExportDataService::lastError() const
{
    return m_strLastError;
}

void QCExportDataService::clearError() const
{
    m_strLastError.clear();
}

void QCExportDataService::setLastError(const QString& strError) const
{
    m_strLastError = strError;
}
