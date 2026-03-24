#ifndef QTCLIP_QCTAGREPOSITORYSQLITE_H_
#define QTCLIP_QCTAGREPOSITORYSQLITE_H_

// File: qctagrepositorysqlite.h
// Author: ZZMI1
// Created: 2026-03-23
// Description: Declares the SQLite tag repository implementation used by QtClip.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include "../../core/database/qcdatabasemanager.h"
#include "../../domain/interfaces/iqctagrepository.h"

class QSqlQuery;

class QCTagRepositorySqlite : public IQCTagRepository
{
public:
    explicit QCTagRepositorySqlite(QCDatabaseManager *pDatabaseManager);
    virtual ~QCTagRepositorySqlite() override;

    virtual bool createTag(const QCTag& tag, qint64 *pnTagId) override;
    virtual bool updateTag(const QCTag& tag) override;
    virtual bool deleteTag(qint64 nTagId) override;
    virtual bool getTagById(qint64 nTagId, QCTag *pTag) const override;
    virtual bool getTagByName(const QString& strName, QCTag *pTag) const override;
    virtual QVector<QCTag> listTags() const override;
    virtual QVector<QCTag> listTagsBySnippet(qint64 nSnippetId) const override;
    virtual int countSnippetsByTag(qint64 nTagId) const override;
    virtual bool replaceSnippetTags(qint64 nSnippetId, const QVector<qint64>& vecTagIds) override;

    QString lastError() const;
    QString lastFailedSql() const;

private:
    QCTagRepositorySqlite(const QCTagRepositorySqlite& other);
    QCTagRepositorySqlite& operator=(const QCTagRepositorySqlite& other);

    bool ensureDatabaseReady() const;
    bool mapTag(QSqlQuery *pQuery, QCTag *pTag) const;
    bool parseRequiredDateTime(const QString& strValue, QDateTime *pDateTime) const;
    void setLastError(const QString& strError) const;
    void setLastFailedSql(const QString& strSql) const;
    void clearErrors() const;

private:
    QCDatabaseManager *m_pDatabaseManager;
    mutable QString m_strLastError;
    mutable QString m_strLastFailedSql;
};

#endif // QTCLIP_QCTAGREPOSITORYSQLITE_H_
