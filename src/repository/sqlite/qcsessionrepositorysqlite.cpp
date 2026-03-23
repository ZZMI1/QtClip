// File: qcsessionrepositorysqlite.cpp
// Author: ZZMI1
// Created: 2026-03-23
// Description: Implements the SQLite session repository used by QtClip.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include "qcsessionrepositorysqlite.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>

#include "../../core/database/qcdbenumcodec.h"

namespace
{
static const char *g_pszInsertSessionSql =
    "INSERT INTO study_sessions("
    "title, course_name, description, status, started_at, ended_at, created_at, updated_at"
    ") VALUES("
    ":title, :course_name, :description, :status, :started_at, :ended_at, :created_at, :updated_at"
    ");";

static const char *g_pszUpdateSessionSql =
    "UPDATE study_sessions SET "
    "title = :title, "
    "course_name = :course_name, "
    "description = :description, "
    "status = :status, "
    "started_at = :started_at, "
    "ended_at = :ended_at, "
    "created_at = :created_at, "
    "updated_at = :updated_at "
    "WHERE id = :id;";

static const char *g_pszFinishSessionSql =
    "UPDATE study_sessions SET "
    "status = :status, "
    "ended_at = :ended_at, "
    "updated_at = :updated_at "
    "WHERE id = :id;";

static const char *g_pszSelectSessionByIdSql =
    "SELECT id, title, course_name, description, status, started_at, ended_at, created_at, updated_at "
    "FROM study_sessions WHERE id = :id LIMIT 1;";

static const char *g_pszSelectActiveSessionSql =
    "SELECT id, title, course_name, description, status, started_at, ended_at, created_at, updated_at "
    "FROM study_sessions WHERE status = :status ORDER BY started_at DESC LIMIT 1;";

static const char *g_pszSelectSessionsSql =
    "SELECT id, title, course_name, description, status, started_at, ended_at, created_at, updated_at "
    "FROM study_sessions ORDER BY started_at DESC, id DESC;";

static const char *g_pszDeleteSessionSql =
    "DELETE FROM study_sessions WHERE id = :id;";
}

QCSessionRepositorySqlite::QCSessionRepositorySqlite(QCDatabaseManager *pDatabaseManager)
    : m_pDatabaseManager(pDatabaseManager)
    , m_strLastError()
    , m_strLastFailedSql()
{
}

QCSessionRepositorySqlite::~QCSessionRepositorySqlite()
{
}

bool QCSessionRepositorySqlite::createSession(const QCStudySession& session, qint64 *pnSessionId)
{
    clearErrors();

    if (!ensureDatabaseReady())
        return false;

    QString strStatusValue;
    QString strStatusError;
    if (!QCDbEnumCodec::toDbValue(session.status(), &strStatusValue, &strStatusError))
    {
        setLastError(strStatusError);
        return false;
    }

    QSqlQuery query(m_pDatabaseManager->database());
    const QString strSql = QString::fromUtf8(g_pszInsertSessionSql);
    setLastFailedSql(strSql);
    query.prepare(strSql);
    query.bindValue(QString::fromUtf8(":title"), session.title());
    query.bindValue(QString::fromUtf8(":course_name"), session.courseName());
    query.bindValue(QString::fromUtf8(":description"), session.description());
    query.bindValue(QString::fromUtf8(":status"), strStatusValue);
    query.bindValue(QString::fromUtf8(":started_at"), session.startedAt().toString(Qt::ISODate));
    query.bindValue(QString::fromUtf8(":ended_at"), session.endedAt().isValid() ? session.endedAt().toString(Qt::ISODate) : QVariant(QVariant::String));
    query.bindValue(QString::fromUtf8(":created_at"), session.createdAt().toString(Qt::ISODate));
    query.bindValue(QString::fromUtf8(":updated_at"), session.updatedAt().toString(Qt::ISODate));

    if (!query.exec())
    {
        setLastError(query.lastError().text());
        return false;
    }

    if (nullptr != pnSessionId)
        *pnSessionId = query.lastInsertId().toLongLong();

    setLastFailedSql(QString());
    return true;
}

bool QCSessionRepositorySqlite::updateSession(const QCStudySession& session)
{
    clearErrors();

    if (!ensureDatabaseReady())
        return false;

    QString strStatusValue;
    QString strStatusError;
    if (!QCDbEnumCodec::toDbValue(session.status(), &strStatusValue, &strStatusError))
    {
        setLastError(strStatusError);
        return false;
    }

    QSqlQuery query(m_pDatabaseManager->database());
    const QString strSql = QString::fromUtf8(g_pszUpdateSessionSql);
    setLastFailedSql(strSql);
    query.prepare(strSql);
    query.bindValue(QString::fromUtf8(":title"), session.title());
    query.bindValue(QString::fromUtf8(":course_name"), session.courseName());
    query.bindValue(QString::fromUtf8(":description"), session.description());
    query.bindValue(QString::fromUtf8(":status"), strStatusValue);
    query.bindValue(QString::fromUtf8(":started_at"), session.startedAt().toString(Qt::ISODate));
    query.bindValue(QString::fromUtf8(":ended_at"), session.endedAt().isValid() ? session.endedAt().toString(Qt::ISODate) : QVariant(QVariant::String));
    query.bindValue(QString::fromUtf8(":created_at"), session.createdAt().toString(Qt::ISODate));
    query.bindValue(QString::fromUtf8(":updated_at"), session.updatedAt().toString(Qt::ISODate));
    query.bindValue(QString::fromUtf8(":id"), session.id());

    if (!query.exec())
    {
        setLastError(query.lastError().text());
        return false;
    }

    if (query.numRowsAffected() <= 0)
    {
        setLastError(QString::fromUtf8("No study session was updated."));
        return false;
    }

    setLastFailedSql(QString());
    return true;
}

bool QCSessionRepositorySqlite::finishSession(qint64 nSessionId, const QDateTime& dateTimeEndedAt)
{
    clearErrors();

    if (!ensureDatabaseReady())
        return false;

    QString strStatusValue;
    QString strStatusError;
    if (!QCDbEnumCodec::toDbValue(QCSessionStatus::FinishedSessionStatus, &strStatusValue, &strStatusError))
    {
        setLastError(strStatusError);
        return false;
    }

    QSqlQuery query(m_pDatabaseManager->database());
    const QString strSql = QString::fromUtf8(g_pszFinishSessionSql);
    setLastFailedSql(strSql);
    query.prepare(strSql);
    query.bindValue(QString::fromUtf8(":status"), strStatusValue);
    query.bindValue(QString::fromUtf8(":ended_at"), dateTimeEndedAt.toString(Qt::ISODate));
    query.bindValue(QString::fromUtf8(":updated_at"), dateTimeEndedAt.toString(Qt::ISODate));
    query.bindValue(QString::fromUtf8(":id"), nSessionId);

    if (!query.exec())
    {
        setLastError(query.lastError().text());
        return false;
    }

    if (query.numRowsAffected() <= 0)
    {
        setLastError(QString::fromUtf8("No study session was finished."));
        return false;
    }

    setLastFailedSql(QString());
    return true;
}

bool QCSessionRepositorySqlite::getSessionById(qint64 nSessionId, QCStudySession *pSession) const
{
    clearErrors();

    if (!ensureDatabaseReady())
        return false;

    if (nullptr == pSession)
    {
        setLastError(QString::fromUtf8("QCStudySession output pointer is null."));
        return false;
    }

    QSqlQuery query(m_pDatabaseManager->database());
    const QString strSql = QString::fromUtf8(g_pszSelectSessionByIdSql);
    setLastFailedSql(strSql);
    query.prepare(strSql);
    query.bindValue(QString::fromUtf8(":id"), nSessionId);

    if (!query.exec())
    {
        setLastError(query.lastError().text());
        return false;
    }

    if (!query.next())
    {
        setLastFailedSql(QString());
        return false;
    }

    if (!mapSession(&query, pSession))
        return false;

    setLastFailedSql(QString());
    return true;
}

bool QCSessionRepositorySqlite::getActiveSession(QCStudySession *pSession) const
{
    clearErrors();

    if (!ensureDatabaseReady())
        return false;

    if (nullptr == pSession)
    {
        setLastError(QString::fromUtf8("QCStudySession output pointer is null."));
        return false;
    }

    QString strStatusValue;
    QString strStatusError;
    if (!QCDbEnumCodec::toDbValue(QCSessionStatus::ActiveSessionStatus, &strStatusValue, &strStatusError))
    {
        setLastError(strStatusError);
        return false;
    }

    QSqlQuery query(m_pDatabaseManager->database());
    const QString strSql = QString::fromUtf8(g_pszSelectActiveSessionSql);
    setLastFailedSql(strSql);
    query.prepare(strSql);
    query.bindValue(QString::fromUtf8(":status"), strStatusValue);

    if (!query.exec())
    {
        setLastError(query.lastError().text());
        return false;
    }

    if (!query.next())
    {
        setLastFailedSql(QString());
        return false;
    }

    if (!mapSession(&query, pSession))
        return false;

    setLastFailedSql(QString());
    return true;
}

QVector<QCStudySession> QCSessionRepositorySqlite::listSessions() const
{
    clearErrors();

    QVector<QCStudySession> vecSessions;
    if (!ensureDatabaseReady())
        return vecSessions;

    QSqlQuery query(m_pDatabaseManager->database());
    const QString strSql = QString::fromUtf8(g_pszSelectSessionsSql);
    setLastFailedSql(strSql);

    if (!query.exec(strSql))
    {
        setLastError(query.lastError().text());
        return vecSessions;
    }

    while (query.next())
    {
        QCStudySession session;
        if (!mapSession(&query, &session))
        {
            vecSessions.clear();
            return vecSessions;
        }

        vecSessions.append(session);
    }

    setLastFailedSql(QString());
    return vecSessions;
}

bool QCSessionRepositorySqlite::deleteSession(qint64 nSessionId)
{
    clearErrors();

    if (!ensureDatabaseReady())
        return false;

    QSqlQuery query(m_pDatabaseManager->database());
    const QString strSql = QString::fromUtf8(g_pszDeleteSessionSql);
    setLastFailedSql(strSql);
    query.prepare(strSql);
    query.bindValue(QString::fromUtf8(":id"), nSessionId);

    if (!query.exec())
    {
        setLastError(query.lastError().text());
        return false;
    }

    if (query.numRowsAffected() <= 0)
    {
        setLastError(QString::fromUtf8("No study session was deleted."));
        return false;
    }

    setLastFailedSql(QString());
    return true;
}

QString QCSessionRepositorySqlite::lastError() const
{
    return m_strLastError;
}

QString QCSessionRepositorySqlite::lastFailedSql() const
{
    return m_strLastFailedSql;
}

bool QCSessionRepositorySqlite::ensureDatabaseReady() const
{
    if (nullptr == m_pDatabaseManager)
    {
        setLastError(QString::fromUtf8("Database manager is null."));
        return false;
    }

    if (!m_pDatabaseManager->isOpen())
    {
        setLastError(QString::fromUtf8("Database connection is not open."));
        return false;
    }

    return true;
}

bool QCSessionRepositorySqlite::mapSession(QSqlQuery *pQuery, QCStudySession *pSession) const
{
    if (nullptr == pQuery || nullptr == pSession)
    {
        setLastError(QString::fromUtf8("Study session mapping input is invalid."));
        return false;
    }

    const QSqlRecord record = pQuery->record();
    const int nIdIndex = record.indexOf(QString::fromUtf8("id"));
    const int nTitleIndex = record.indexOf(QString::fromUtf8("title"));
    const int nCourseNameIndex = record.indexOf(QString::fromUtf8("course_name"));
    const int nDescriptionIndex = record.indexOf(QString::fromUtf8("description"));
    const int nStatusIndex = record.indexOf(QString::fromUtf8("status"));
    const int nStartedAtIndex = record.indexOf(QString::fromUtf8("started_at"));
    const int nEndedAtIndex = record.indexOf(QString::fromUtf8("ended_at"));
    const int nCreatedAtIndex = record.indexOf(QString::fromUtf8("created_at"));
    const int nUpdatedAtIndex = record.indexOf(QString::fromUtf8("updated_at"));

    if (nIdIndex < 0 || nTitleIndex < 0 || nCourseNameIndex < 0 || nDescriptionIndex < 0
        || nStatusIndex < 0 || nStartedAtIndex < 0 || nEndedAtIndex < 0
        || nCreatedAtIndex < 0 || nUpdatedAtIndex < 0)
    {
        setLastError(QString::fromUtf8("Study session query result is missing required fields."));
        return false;
    }

    QCSessionStatus sessionStatus = QCSessionStatus::ActiveSessionStatus;
    QString strStatusError;
    if (!QCDbEnumCodec::fromDbValue(pQuery->value(nStatusIndex).toString(), &sessionStatus, &strStatusError))
    {
        setLastError(strStatusError);
        return false;
    }

    QDateTime dateTimeStartedAt;
    if (!parseRequiredDateTime(QString::fromUtf8("started_at"), pQuery->value(nStartedAtIndex).toString(), &dateTimeStartedAt))
        return false;

    QDateTime dateTimeEndedAt;
    if (!parseOptionalDateTime(QString::fromUtf8("ended_at"), pQuery->value(nEndedAtIndex).toString(), &dateTimeEndedAt))
        return false;

    QDateTime dateTimeCreatedAt;
    if (!parseRequiredDateTime(QString::fromUtf8("created_at"), pQuery->value(nCreatedAtIndex).toString(), &dateTimeCreatedAt))
        return false;

    QDateTime dateTimeUpdatedAt;
    if (!parseRequiredDateTime(QString::fromUtf8("updated_at"), pQuery->value(nUpdatedAtIndex).toString(), &dateTimeUpdatedAt))
        return false;

    // Map the current query row into a pure domain object.
    pSession->setId(pQuery->value(nIdIndex).toLongLong());
    pSession->setTitle(pQuery->value(nTitleIndex).toString());
    pSession->setCourseName(pQuery->value(nCourseNameIndex).toString());
    pSession->setDescription(pQuery->value(nDescriptionIndex).toString());
    pSession->setStatus(sessionStatus);
    pSession->setStartedAt(dateTimeStartedAt);
    pSession->setEndedAt(dateTimeEndedAt);
    pSession->setCreatedAt(dateTimeCreatedAt);
    pSession->setUpdatedAt(dateTimeUpdatedAt);

    return true;
}

bool QCSessionRepositorySqlite::parseRequiredDateTime(const QString& strFieldName,
                                                      const QString& strValue,
                                                      QDateTime *pDateTime) const
{
    if (nullptr == pDateTime)
    {
        setLastError(QString::fromUtf8("Required date time output pointer is null."));
        return false;
    }

    const QDateTime dateTime = QDateTime::fromString(strValue, Qt::ISODate);
    if (!dateTime.isValid())
    {
        setLastError(QString::fromUtf8("Invalid ISO date time value in field %1: %2")
            .arg(strFieldName, strValue));
        return false;
    }

    *pDateTime = dateTime;
    return true;
}

bool QCSessionRepositorySqlite::parseOptionalDateTime(const QString& strFieldName,
                                                      const QString& strValue,
                                                      QDateTime *pDateTime) const
{
    if (nullptr == pDateTime)
    {
        setLastError(QString::fromUtf8("Optional date time output pointer is null."));
        return false;
    }

    if (strValue.isEmpty())
    {
        *pDateTime = QDateTime();
        return true;
    }

    return parseRequiredDateTime(strFieldName, strValue, pDateTime);
}

void QCSessionRepositorySqlite::setLastError(const QString& strError) const
{
    m_strLastError = strError;
}

void QCSessionRepositorySqlite::setLastFailedSql(const QString& strSql) const
{
    m_strLastFailedSql = strSql;
}

void QCSessionRepositorySqlite::clearErrors() const
{
    m_strLastError.clear();
    m_strLastFailedSql.clear();
}
