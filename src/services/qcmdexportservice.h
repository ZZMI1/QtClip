#ifndef QTCLIP_QCMDEXPORTSERVICE_H_
#define QTCLIP_QCMDEXPORTSERVICE_H_

// File: qcmdexportservice.h
// Author: ZZMI1
// Created: 2026-03-23
// Description: Declares the Markdown export service used by the first QtClip export workflow.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include <QString>
#include <QVector>

#include "qcexportdataservice.h"

class QCExportDataService;
class QCMdExportRenderer;

struct QCMdExportPreview
{
    QString m_strSessionTitle;
    QString m_strCourseName;
    int m_nSnippetCount;
    int m_nImageSnippetCount;
    int m_nArchivedSnippetCount;
    int m_nFavoriteSnippetCount;
    int m_nSummarizedSnippetCount;
    bool m_bHasSessionSummary;
};

class QCMdExportService
{
public:
    QCMdExportService(QCExportDataService *pExportDataService,
                      QCMdExportRenderer *pMdExportRenderer);
    ~QCMdExportService();

    bool buildExportPreview(qint64 nSessionId, QCMdExportPreview *pExportPreview) const;
    bool buildExportPreviewForSnippets(qint64 nSessionId, const QVector<qint64>& vecSnippetIds, QCMdExportPreview *pExportPreview) const;
    bool exportSessionToFile(qint64 nSessionId, const QString& strOutputFilePath);
    bool exportSnippetsToFile(qint64 nSessionId, const QVector<qint64>& vecSnippetIds, const QString& strOutputFilePath);

    QString lastError() const;
    void clearError() const;

private:
    QCMdExportService(const QCMdExportService& other);
    QCMdExportService& operator=(const QCMdExportService& other);

    void setLastError(const QString& strError) const;

private:
    QCExportDataService *m_pExportDataService;
    QCMdExportRenderer *m_pMdExportRenderer;
    mutable QString m_strLastError;
};

#endif // QTCLIP_QCMDEXPORTSERVICE_H_
