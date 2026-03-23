#ifndef QTCLIP_QCSNIPPETSERVICE_H_
#define QTCLIP_QCSNIPPETSERVICE_H_

// File: qcsnippetservice.h
// Author: ZZMI1
// Created: 2026-03-23
// Description: Declares the thin snippet service used to drive the first QtClip workflow.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include <QString>
#include <QVector>

#include "../domain/entities/qcattachment.h"
#include "../domain/entities/qcsnippet.h"
#include "../domain/interfaces/iqcsnippetrepository.h"

class QCSnippetService
{
public:
    explicit QCSnippetService(IQCSnippetRepository *pSnippetRepository);
    ~QCSnippetService();

    bool createTextSnippet(QCSnippet *pSnippet);
    bool createCodeSnippet(QCSnippet *pSnippet);
    bool createImageSnippetWithPrimaryAttachment(QCSnippet *pSnippet, QCAttachment *pPrimaryAttachment);
    bool updateSnippet(QCSnippet *pSnippet);
    bool getSnippetById(qint64 nSnippetId, QCSnippet *pSnippet) const;
    bool getPrimaryAttachmentBySnippetId(qint64 nSnippetId, QCAttachment *pPrimaryAttachment) const;
    QVector<QCSnippet> listSnippetsBySession(qint64 nSessionId) const;
    QVector<QCSnippet> querySnippets(qint64 nSessionId,
                                    const QString& strKeyword,
                                    bool bFavoriteOnly,
                                    bool bReviewOnly,
                                    qint64 nTagId) const;
    bool setFavorite(qint64 nSnippetId, bool bIsFavorite);
    bool setArchived(qint64 nSnippetId, bool bIsArchived);
    bool setReviewState(qint64 nSnippetId, bool bIsReview);

    QString lastError() const;
    void clearError() const;

private:
    QCSnippetService(const QCSnippetService& other);
    QCSnippetService& operator=(const QCSnippetService& other);

    bool validateSnippetInput(const QCSnippet& snippet) const;
    void normalizeSnippetForCreate(QCSnippet *pSnippet) const;
    void setLastError(const QString& strError) const;

private:
    IQCSnippetRepository *m_pSnippetRepository;
    mutable QString m_strLastError;
};

#endif // QTCLIP_QCSNIPPETSERVICE_H_
