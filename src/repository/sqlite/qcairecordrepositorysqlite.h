#ifndef QTCLIP_QCAIRECORDREPOSITORYSQLITE_H_
#define QTCLIP_QCAIRECORDREPOSITORYSQLITE_H_

// File: qcairecordrepositorysqlite.h
// Author: ZZMI1
// Created: 2026-03-23
// Description: Declares the SQLite AI record repository implementation used by QtClip.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include "../../core/database/qcdatabasemanager.h"
#include "../../domain/interfaces/iqcairecordrepository.h"

class QSqlQuery;

class QCAiRecordRepositorySqlite : public IQCAiRecordRepository
{
public:
    explicit QCAiRecordRepositorySqlite(QCDatabaseManager *pDatabaseManager);
    virtual ~QCAiRecordRepositorySqlite() override;

    virtual bool createAiRecord(const QCAiRecord& aiRecord, qint64 *pnAiRecordId) override;
    virtual bool updateAiRecord(const QCAiRecord& aiRecord) override;
    virtual bool getAiRecordById(qint64 nAiRecordId, QCAiRecord *pAiRecord) const override;
    virtual QVector<QCAiRecord> listAiRecordsBySession(qint64 nSessionId) const override;
    virtual QVector<QCAiRecord> listAiRecordsBySnippet(qint64 nSnippetId) const override;
    virtual bool deleteAiRecord(qint64 nAiRecordId) override;

    QString lastError() const;
    QString lastFailedSql() const;

private:
    QCAiRecordRepositorySqlite(const QCAiRecordRepositorySqlite& other);
    QCAiRecordRepositorySqlite& operator=(const QCAiRecordRepositorySqlite& other);

    bool ensureDatabaseReady() const;
    bool bindAiRecordValues(QSqlQuery *pQuery, const QCAiRecord& aiRecord) const;
    bool mapAiRecord(QSqlQuery *pQuery, QCAiRecord *pAiRecord) const;
    bool parseRequiredDateTime(const QString& strFieldName,
                               const QString& strValue,
                               QDateTime *pDateTime) const;
    void setLastError(const QString& strError) const;
    void setLastFailedSql(const QString& strSql) const;
    void clearErrors() const;

private:
    QCDatabaseManager *m_pDatabaseManager;
    mutable QString m_strLastError;
    mutable QString m_strLastFailedSql;
};

#endif // QTCLIP_QCAIRECORDREPOSITORYSQLITE_H_
