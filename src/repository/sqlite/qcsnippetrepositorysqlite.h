#ifndef QTCLIP_QCSNIPPETREPOSITORYSQLITE_H_
#define QTCLIP_QCSNIPPETREPOSITORYSQLITE_H_

// File: qcsnippetrepositorysqlite.h
// Author: ZZMI1
// Created: 2026-03-23
// Description: Declares the SQLite snippet repository implementation used by QtClip.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include "../../core/database/qcdatabasemanager.h"
#include "../../domain/interfaces/iqcsnippetrepository.h"

class QSqlQuery;

class QCSnippetRepositorySqlite : public IQCSnippetRepository
{
public:
    explicit QCSnippetRepositorySqlite(QCDatabaseManager *pDatabaseManager);
    virtual ~QCSnippetRepositorySqlite() override;

    virtual bool createSnippet(const QCSnippet& snippet, qint64 *pnSnippetId) override;
    virtual bool createSnippetWithPrimaryAttachment(const QCSnippet& snippet,
                                                    const QCAttachment& primaryAttachment,
                                                    qint64 *pnSnippetId) override;
    virtual bool updateSnippet(const QCSnippet& snippet) override;
    virtual bool deleteSnippet(qint64 nSnippetId) override;
    virtual bool getSnippetById(qint64 nSnippetId, QCSnippet *pSnippet) const override;
    virtual bool getPrimaryAttachmentBySnippetId(qint64 nSnippetId,
                                                 QCAttachment *pPrimaryAttachment) const override;
    virtual QVector<QCSnippet> listSnippetsBySession(qint64 nSessionId) const override;
    virtual QVector<QCSnippet> querySnippets(qint64 nSessionId,
                                             const QString& strKeyword,
                                             bool bFavoriteOnly,
                                             bool bReviewOnly,
                                             qint64 nTagId) const override;
    virtual QVector<QCSnippet> searchSnippets(const QString& strKeyword) const override;
    virtual bool setFavorite(qint64 nSnippetId, bool bIsFavorite) override;
    virtual bool setArchived(qint64 nSnippetId, bool bIsArchived) override;

    QString lastError() const;
    QString lastFailedSql() const;

private:
    QCSnippetRepositorySqlite(const QCSnippetRepositorySqlite& other);
    QCSnippetRepositorySqlite& operator=(const QCSnippetRepositorySqlite& other);

    bool ensureDatabaseReady() const;
    bool bindSnippetValues(QSqlQuery *pQuery, const QCSnippet& snippet) const;
    bool mapSnippet(QSqlQuery *pQuery, QCSnippet *pSnippet) const;
    bool mapAttachment(QSqlQuery *pQuery, QCAttachment *pAttachment) const;
    bool parseRequiredDateTime(const QString& strFieldName,
                               const QString& strValue,
                               QDateTime *pDateTime) const;
    bool parseOptionalDateTime(const QString& strFieldName,
                               const QString& strValue,
                               QDateTime *pDateTime) const;
    bool rollbackTransactionPreserveError(const QString& strOriginalError,
                                          const QString& strOriginalSql);
    void setLastError(const QString& strError) const;
    void setLastFailedSql(const QString& strSql) const;
    void clearErrors() const;

private:
    QCDatabaseManager *m_pDatabaseManager;
    mutable QString m_strLastError;
    mutable QString m_strLastFailedSql;
};

#endif // QTCLIP_QCSNIPPETREPOSITORYSQLITE_H_
