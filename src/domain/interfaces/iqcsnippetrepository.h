#ifndef QTCLIP_IQCSNIPPETREPOSITORY_H_
#define QTCLIP_IQCSNIPPETREPOSITORY_H_
// File: iqcsnippetrepository.h
// Author: ZZMI1
// Created: 2026-03-23
// Description: Declares the snippet repository abstraction used by the QtClip domain layer.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.
#include <QString>
#include <QVector>
#include "../entities/qcattachment.h"
#include "../entities/qcsnippet.h"
class IQCSnippetRepository
{
public:
    virtual ~IQCSnippetRepository() = default;
    virtual bool createSnippet(const QCSnippet& snippet, qint64 *pnSnippetId) = 0;
    virtual bool createSnippetWithPrimaryAttachment(const QCSnippet& snippet,
                                                    const QCAttachment& primaryAttachment,
                                                    qint64 *pnSnippetId) = 0;
    virtual bool updateSnippet(const QCSnippet& snippet) = 0;
    virtual bool deleteSnippet(qint64 nSnippetId) = 0;
    virtual bool getSnippetById(qint64 nSnippetId, QCSnippet *pSnippet) const = 0;
    virtual bool getPrimaryAttachmentBySnippetId(qint64 nSnippetId, QCAttachment *pPrimaryAttachment) const = 0;
    virtual QVector<QCSnippet> listSnippetsBySession(qint64 nSessionId) const = 0;
    virtual QVector<QCSnippet> querySnippets(qint64 nSessionId,
                                            const QString& strKeyword,
                                            bool bFavoriteOnly,
                                            bool bReviewOnly,
                                            qint64 nTagId) const = 0;
    virtual QVector<QCSnippet> searchSnippets(const QString& strKeyword) const = 0;
    virtual bool setFavorite(qint64 nSnippetId, bool bIsFavorite) = 0;
    virtual bool setArchived(qint64 nSnippetId, bool bIsArchived) = 0;
protected:
    IQCSnippetRepository() = default;
private:
    IQCSnippetRepository(const IQCSnippetRepository& other);
    IQCSnippetRepository& operator=(const IQCSnippetRepository& other);
};
#endif // QTCLIP_IQCSNIPPETREPOSITORY_H_
