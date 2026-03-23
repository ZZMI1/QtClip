// File: qcairecordrepositorysqlite.cpp
// Author: ZZMI1
// Created: 2026-03-23
// Description: Implements the SQLite AI record repository used by QtClip.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include "qcairecordrepositorysqlite.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>

#include "../../core/database/qcdbenumcodec.h"

namespace
{
static const char *g_pszInsertAiRecordSql =
    "INSERT INTO ai_records("
    "session_id, snippet_id, task_type, provider_name, model_name, prompt_text, response_text, status, error_message, created_at"
    ") VALUES("
    ":session_id, :snippet_id, :task_type, :provider_name, :model_name, :prompt_text, :response_text, :status, :error_message, :created_at"
    ");";

static const char *g_pszUpdateAiRecordSql =
    "UPDATE ai_records SET "
    "session_id = :session_id, "
    "snippet_id = :snippet_id, "
    "task_type = :task_type, "
    "provider_name = :provider_name, "
    "model_name = :model_name, "
    "prompt_text = :prompt_text, "
    "response_text = :response_text, "
    "status = :status, "
    "error_message = :error_message, "
    "created_at = :created_at "
    "WHERE id = :id;";

static const char *g_pszSelectAiRecordByIdSql =
    "SELECT id, session_id, snippet_id, task_type, provider_name, model_name, prompt_text, response_text, status, error_message, created_at "
    "FROM ai_records WHERE id = :id LIMIT 1;";

static const char *g_pszSelectAiRecordsBySessionSql =
    "SELECT id, session_id, snippet_id, task_type, provider_name, model_name, prompt_text, response_text, status, error_message, created_at "
    "FROM ai_records WHERE session_id = :session_id ORDER BY created_at DESC, id DESC;";

static const char *g_pszSelectAiRecordsBySnippetSql =
    "SELECT id, session_id, snippet_id, task_type, provider_name, model_name, prompt_text, response_text, status, error_message, created_at "
    "FROM ai_records WHERE snippet_id = :snippet_id ORDER BY created_at DESC, id DESC;";

static const char *g_pszDeleteAiRecordSql =
    "DELETE FROM ai_records WHERE id = :id;";
}

QCAiRecordRepositorySqlite::QCAiRecordRepositorySqlite(QCDatabaseManager *pDatabaseManager)
    : m_pDatabaseManager(pDatabaseManager)
    , m_strLastError()
    , m_strLastFailedSql()
{
}

QCAiRecordRepositorySqlite::~QCAiRecordRepositorySqlite()
{
}

bool QCAiRecordRepositorySqlite::createAiRecord(const QCAiRecord& aiRecord, qint64 *pnAiRecordId)
{
    clearErrors();

    if (!ensureDatabaseReady())
        return false;

    QSqlQuery query(m_pDatabaseManager->database());
    const QString strSql = QString::fromUtf8(g_pszInsertAiRecordSql);
    setLastFailedSql(strSql);
    query.prepare(strSql);

    if (!bindAiRecordValues(&query, aiRecord))
        return false;

    if (!query.exec())
    {
        setLastError(query.lastError().text());
        return false;
    }

    if (nullptr != pnAiRecordId)
        *pnAiRecordId = query.lastInsertId().toLongLong();

    setLastFailedSql(QString());
    return true;
}

bool QCAiRecordRepositorySqlite::updateAiRecord(const QCAiRecord& aiRecord)
{
    clearErrors();

    if (!ensureDatabaseReady())
        return false;

    QSqlQuery query(m_pDatabaseManager->database());
    const QString strSql = QString::fromUtf8(g_pszUpdateAiRecordSql);
    setLastFailedSql(strSql);
    query.prepare(strSql);

    if (!bindAiRecordValues(&query, aiRecord))
        return false;

    query.bindValue(QString::fromUtf8(":id"), QVariant(aiRecord.id()));

    if (!query.exec())
    {
        setLastError(query.lastError().text());
        return false;
    }

    if (query.numRowsAffected() <= 0)
    {
        setLastError(QString::fromUtf8("No AI record was updated."));
        return false;
    }

    setLastFailedSql(QString());
    return true;
}

bool QCAiRecordRepositorySqlite::getAiRecordById(qint64 nAiRecordId, QCAiRecord *pAiRecord) const
{
    clearErrors();

    if (!ensureDatabaseReady())
        return false;

    if (nullptr == pAiRecord)
    {
        setLastError(QString::fromUtf8("QCAiRecord output pointer is null."));
        return false;
    }

    QSqlQuery query(m_pDatabaseManager->database());
    const QString strSql = QString::fromUtf8(g_pszSelectAiRecordByIdSql);
    setLastFailedSql(strSql);
    query.prepare(strSql);
    query.bindValue(QString::fromUtf8(":id"), QVariant(nAiRecordId));

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

    if (!mapAiRecord(&query, pAiRecord))
        return false;

    setLastFailedSql(QString());
    return true;
}

QVector<QCAiRecord> QCAiRecordRepositorySqlite::listAiRecordsBySession(qint64 nSessionId) const
{
    clearErrors();

    QVector<QCAiRecord> vecAiRecords;
    if (!ensureDatabaseReady())
        return vecAiRecords;

    QSqlQuery query(m_pDatabaseManager->database());
    const QString strSql = QString::fromUtf8(g_pszSelectAiRecordsBySessionSql);
    setLastFailedSql(strSql);
    query.prepare(strSql);
    query.bindValue(QString::fromUtf8(":session_id"), QVariant(nSessionId));

    if (!query.exec())
    {
        setLastError(query.lastError().text());
        return vecAiRecords;
    }

    while (query.next())
    {
        QCAiRecord aiRecord;
        if (!mapAiRecord(&query, &aiRecord))
        {
            vecAiRecords.clear();
            return vecAiRecords;
        }

        vecAiRecords.append(aiRecord);
    }

    setLastFailedSql(QString());
    return vecAiRecords;
}

QVector<QCAiRecord> QCAiRecordRepositorySqlite::listAiRecordsBySnippet(qint64 nSnippetId) const
{
    clearErrors();

    QVector<QCAiRecord> vecAiRecords;
    if (!ensureDatabaseReady())
        return vecAiRecords;

    QSqlQuery query(m_pDatabaseManager->database());
    const QString strSql = QString::fromUtf8(g_pszSelectAiRecordsBySnippetSql);
    setLastFailedSql(strSql);
    query.prepare(strSql);
    query.bindValue(QString::fromUtf8(":snippet_id"), QVariant(nSnippetId));

    if (!query.exec())
    {
        setLastError(query.lastError().text());
        return vecAiRecords;
    }

    while (query.next())
    {
        QCAiRecord aiRecord;
        if (!mapAiRecord(&query, &aiRecord))
        {
            vecAiRecords.clear();
            return vecAiRecords;
        }

        vecAiRecords.append(aiRecord);
    }

    setLastFailedSql(QString());
    return vecAiRecords;
}

bool QCAiRecordRepositorySqlite::deleteAiRecord(qint64 nAiRecordId)
{
    clearErrors();

    if (!ensureDatabaseReady())
        return false;

    QSqlQuery query(m_pDatabaseManager->database());
    const QString strSql = QString::fromUtf8(g_pszDeleteAiRecordSql);
    setLastFailedSql(strSql);
    query.prepare(strSql);
    query.bindValue(QString::fromUtf8(":id"), QVariant(nAiRecordId));

    if (!query.exec())
    {
        setLastError(query.lastError().text());
        return false;
    }

    if (query.numRowsAffected() <= 0)
    {
        setLastError(QString::fromUtf8("No AI record was deleted."));
        return false;
    }

    setLastFailedSql(QString());
    return true;
}

QString QCAiRecordRepositorySqlite::lastError() const
{
    return m_strLastError;
}

QString QCAiRecordRepositorySqlite::lastFailedSql() const
{
    return m_strLastFailedSql;
}

bool QCAiRecordRepositorySqlite::ensureDatabaseReady() const
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

bool QCAiRecordRepositorySqlite::bindAiRecordValues(QSqlQuery *pQuery, const QCAiRecord& aiRecord) const
{
    if (nullptr == pQuery)
    {
        setLastError(QString::fromUtf8("AI record query pointer is null."));
        return false;
    }

    if (0 == aiRecord.sessionId() && 0 == aiRecord.snippetId())
    {
        setLastError(QString::fromUtf8("AI record requires sessionId or snippetId."));
        return false;
    }

    if (!aiRecord.createdAt().isValid())
    {
        setLastError(QString::fromUtf8("AI record createdAt is invalid."));
        return false;
    }

    QString strTaskTypeValue;
    QString strTaskTypeError;
    if (!QCDbEnumCodec::toDbValue(aiRecord.taskType(), &strTaskTypeValue, &strTaskTypeError))
    {
        setLastError(strTaskTypeError);
        return false;
    }

    const QVariant varSessionId = 0 != aiRecord.sessionId() ? QVariant(aiRecord.sessionId()) : QVariant(QVariant::LongLong);
    const QVariant varSnippetId = 0 != aiRecord.snippetId() ? QVariant(aiRecord.snippetId()) : QVariant(QVariant::LongLong);

    pQuery->bindValue(QString::fromUtf8(":session_id"), varSessionId);
    pQuery->bindValue(QString::fromUtf8(":snippet_id"), varSnippetId);
    pQuery->bindValue(QString::fromUtf8(":task_type"), QVariant(strTaskTypeValue));
    pQuery->bindValue(QString::fromUtf8(":provider_name"), QVariant(aiRecord.providerName()));
    pQuery->bindValue(QString::fromUtf8(":model_name"), QVariant(aiRecord.modelName()));
    pQuery->bindValue(QString::fromUtf8(":prompt_text"), QVariant(aiRecord.promptText()));
    pQuery->bindValue(QString::fromUtf8(":response_text"), QVariant(aiRecord.responseText()));
    pQuery->bindValue(QString::fromUtf8(":status"), QVariant(aiRecord.status()));
    pQuery->bindValue(QString::fromUtf8(":error_message"), QVariant(aiRecord.errorMessage()));
    pQuery->bindValue(QString::fromUtf8(":created_at"), QVariant(aiRecord.createdAt().toString(Qt::ISODate)));

    return true;
}

bool QCAiRecordRepositorySqlite::mapAiRecord(QSqlQuery *pQuery, QCAiRecord *pAiRecord) const
{
    if (nullptr == pQuery || nullptr == pAiRecord)
    {
        setLastError(QString::fromUtf8("AI record mapping input is invalid."));
        return false;
    }

    const QSqlRecord record = pQuery->record();
    const int nIdIndex = record.indexOf(QString::fromUtf8("id"));
    const int nSessionIdIndex = record.indexOf(QString::fromUtf8("session_id"));
    const int nSnippetIdIndex = record.indexOf(QString::fromUtf8("snippet_id"));
    const int nTaskTypeIndex = record.indexOf(QString::fromUtf8("task_type"));
    const int nProviderNameIndex = record.indexOf(QString::fromUtf8("provider_name"));
    const int nModelNameIndex = record.indexOf(QString::fromUtf8("model_name"));
    const int nPromptTextIndex = record.indexOf(QString::fromUtf8("prompt_text"));
    const int nResponseTextIndex = record.indexOf(QString::fromUtf8("response_text"));
    const int nStatusIndex = record.indexOf(QString::fromUtf8("status"));
    const int nErrorMessageIndex = record.indexOf(QString::fromUtf8("error_message"));
    const int nCreatedAtIndex = record.indexOf(QString::fromUtf8("created_at"));

    if (nIdIndex < 0 || nSessionIdIndex < 0 || nSnippetIdIndex < 0 || nTaskTypeIndex < 0
        || nProviderNameIndex < 0 || nModelNameIndex < 0 || nPromptTextIndex < 0
        || nResponseTextIndex < 0 || nStatusIndex < 0 || nErrorMessageIndex < 0
        || nCreatedAtIndex < 0)
    {
        setLastError(QString::fromUtf8("AI record query result is missing required fields."));
        return false;
    }

    QCAiTaskType aiTaskType = QCAiTaskType::SnippetSummaryTask;
    QString strTaskTypeError;
    if (!QCDbEnumCodec::fromDbValue(pQuery->value(nTaskTypeIndex).toString(), &aiTaskType, &strTaskTypeError))
    {
        setLastError(strTaskTypeError);
        return false;
    }

    if (pQuery->value(nSessionIdIndex).isNull() && pQuery->value(nSnippetIdIndex).isNull())
    {
        setLastError(QString::fromUtf8("AI record row has neither session_id nor snippet_id."));
        return false;
    }

    QDateTime dateTimeCreatedAt;
    if (!parseRequiredDateTime(QString::fromUtf8("created_at"), pQuery->value(nCreatedAtIndex).toString(), &dateTimeCreatedAt))
        return false;

    // Map the current query row into a pure AI record domain object.
    pAiRecord->setId(pQuery->value(nIdIndex).toLongLong());
    pAiRecord->setSessionId(pQuery->value(nSessionIdIndex).isNull() ? 0 : pQuery->value(nSessionIdIndex).toLongLong());
    pAiRecord->setSnippetId(pQuery->value(nSnippetIdIndex).isNull() ? 0 : pQuery->value(nSnippetIdIndex).toLongLong());
    pAiRecord->setTaskType(aiTaskType);
    pAiRecord->setProviderName(pQuery->value(nProviderNameIndex).toString());
    pAiRecord->setModelName(pQuery->value(nModelNameIndex).toString());
    pAiRecord->setPromptText(pQuery->value(nPromptTextIndex).toString());
    pAiRecord->setResponseText(pQuery->value(nResponseTextIndex).toString());
    pAiRecord->setStatus(pQuery->value(nStatusIndex).toString());
    pAiRecord->setErrorMessage(pQuery->value(nErrorMessageIndex).toString());
    pAiRecord->setCreatedAt(dateTimeCreatedAt);

    return true;
}

bool QCAiRecordRepositorySqlite::parseRequiredDateTime(const QString& strFieldName,
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

void QCAiRecordRepositorySqlite::setLastError(const QString& strError) const
{
    m_strLastError = strError;
}

void QCAiRecordRepositorySqlite::setLastFailedSql(const QString& strSql) const
{
    m_strLastFailedSql = strSql;
}

void QCAiRecordRepositorySqlite::clearErrors() const
{
    m_strLastError.clear();
    m_strLastFailedSql.clear();
}
