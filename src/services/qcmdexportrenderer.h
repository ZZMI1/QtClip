#ifndef QTCLIP_QCMDEXPORTRENDERER_H_
#define QTCLIP_QCMDEXPORTRENDERER_H_

// File: qcmdexportrenderer.h
// Author: ZZMI1
// Created: 2026-03-23
// Description: Declares the Markdown renderer used by the first QtClip export workflow.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include <QString>

#include "qcexportdataservice.h"

class QCMdExportRenderer
{
public:
    QCMdExportRenderer();
    ~QCMdExportRenderer();

    bool renderMarkdown(const QCExportContext& exportContext, QString *pstrMarkdown) const;

    QString lastError() const;
    void clearError() const;

private:
    QCMdExportRenderer(const QCMdExportRenderer& other);
    QCMdExportRenderer& operator=(const QCMdExportRenderer& other);

    QString formatDateTime(const QDateTime& dateTimeValue) const;
    QString findSnippetSummary(const QCExportSnippetContext& snippetContext) const;
    QString findSessionSummary(const QCExportContext& exportContext) const;
    QString taskTypeLabel(QCAiTaskType aiTaskType) const;
    void setLastError(const QString& strError) const;

private:
    mutable QString m_strLastError;
};

#endif // QTCLIP_QCMDEXPORTRENDERER_H_
