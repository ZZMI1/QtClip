#ifndef QTCLIP_QCSESSIONREPOSITORYSQLITE_H_
#define QTCLIP_QCSESSIONREPOSITORYSQLITE_H_

// File: qcsessionrepositorysqlite.h
// Author: ZZMI1
// Created: 2026-03-23
// Description: Declares the SQLite session repository implementation used by QtClip.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include "../../core/database/qcdatabasemanager.h"
#include "../../domain/interfaces/iqcsessionrepository.h"

class QSqlQuery;

class QCSessionRepositorySqlite : public IQCSessionRepository
{
public:
    explicit QCSessionRepositorySqlite(QCDatabaseManager *pDatabaseManager);
    virtual ~QCSessionRepositorySqlite() override;

    virtual bool createSession(const QCStudySession& session, qint64 *pnSessionId) override;
    virtual bool updateSession(const QCStudySession& session) override;
    virtual bool finishSession(qint64 nSessionId, const QDateTime& dateTimeEndedAt) override;
    virtual bool getSessionById(qint64 nSessionId, QCStudySession *pSession) const override;
    virtual bool getActiveSession(QCStudySession *pSession) const override;
    virtual QVector<QCStudySession> listSessions() const override;
    virtual bool deleteSession(qint64 nSessionId) override;

    QString lastError() const;
    QString lastFailedSql() const;

private:
    QCSessionRepositorySqlite(const QCSessionRepositorySqlite& other);
    QCSessionRepositorySqlite& operator=(const QCSessionRepositorySqlite& other);

    bool ensureDatabaseReady() const;
    bool mapSession(QSqlQuery *pQuery, QCStudySession *pSession) const;
    bool parseRequiredDateTime(const QString& strFieldName,
                               const QString& strValue,
                               QDateTime *pDateTime) const;
    bool parseOptionalDateTime(const QString& strFieldName,
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

#endif // QTCLIP_QCSESSIONREPOSITORYSQLITE_H_
