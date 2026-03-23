// File: qcdatabasemanager.cpp
// Author: ZZMI1
// Created: 2026-03-23
// Description: Implements the SQLite database manager used by QtClip infrastructure.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include "qcdatabasemanager.h"

#include <QDateTime>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>
#include <QVariant>

#include "qcdatabaseschema.h"

namespace
{
static const char *g_pszSqliteDriverName = "QSQLITE";
static const char *g_pszEnableForeignKeys = "PRAGMA foreign_keys = ON;";
}

QCDatabaseManager::QCDatabaseManager()
    : m_strConnectionName()
    , m_database()
    , m_strLastError()
    , m_strLastFailedSql()
{
}

QCDatabaseManager::~QCDatabaseManager()
{
    if (m_database.isValid())
    {
        const QString strConnectionName = m_strConnectionName;

        if (m_database.isOpen())
            m_database.close();

        m_database = QSqlDatabase();

        if (!strConnectionName.isEmpty())
            QSqlDatabase::removeDatabase(strConnectionName);
    }
}

bool QCDatabaseManager::open(const QString& strDatabasePath)
{
    clearLastError();
    clearLastFailedSql();

    if (strDatabasePath.isEmpty())
    {
        setLastError(QString::fromUtf8("Database path is empty."));
        return false;
    }

    if (m_database.isValid())
    {
        if (m_database.isOpen())
            m_database.close();

        m_database = QSqlDatabase();

        if (!m_strConnectionName.isEmpty())
            QSqlDatabase::removeDatabase(m_strConnectionName);
    }

    m_strConnectionName = QString::fromUtf8("qtclip_connection_%1")
        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));

    m_database = QSqlDatabase::addDatabase(QString::fromUtf8(g_pszSqliteDriverName), m_strConnectionName);
    m_database.setDatabaseName(strDatabasePath);

    if (!m_database.open())
    {
        setLastError(m_database.lastError().text());
        return false;
    }

    if (!enableForeignKeys())
        return false;

    return true;
}

bool QCDatabaseManager::initialize()
{
    clearLastError();
    clearLastFailedSql();

    if (!isOpen())
    {
        setLastError(QString::fromUtf8("Database connection is not open."));
        return false;
    }

    const QStringList listStatements = QCDatabaseSchema::allStatements();
    for (const QString& strStatement : listStatements)
    {
        if (!executeStatement(strStatement))
            return false;
    }

    return setSchemaVersion(QCDatabaseSchema::currentSchemaVersion());
}

bool QCDatabaseManager::migrate()
{
    clearLastError();
    clearLastFailedSql();

    if (!isOpen())
    {
        setLastError(QString::fromUtf8("Database connection is not open."));
        return false;
    }

    const int nCurrentVersion = schemaVersion();
    if (nCurrentVersion < 0)
    {
        setLastError(QString::fromUtf8("Failed to read the database schema version."));
        return false;
    }

    if (nCurrentVersion == 0)
        return initialize();

    if (nCurrentVersion > QCDatabaseSchema::currentSchemaVersion())
    {
        setLastError(QString::fromUtf8("Database schema version is newer than the application schema version."));
        return false;
    }

    if (nCurrentVersion == QCDatabaseSchema::currentSchemaVersion())
        return true;

    setLastError(QString::fromUtf8("Database migration steps are not implemented yet."));
    return false;
}

bool QCDatabaseManager::isOpen() const
{
    return m_database.isValid() && m_database.isOpen();
}

QString QCDatabaseManager::lastError() const
{
    return m_strLastError;
}

QString QCDatabaseManager::lastFailedSql() const
{
    return m_strLastFailedSql;
}

int QCDatabaseManager::schemaVersion() const
{
    if (!isOpen())
        return -1;

    QSqlQuery query(m_database);
    query.prepare(QString::fromUtf8("SELECT value FROM app_settings WHERE key = :key LIMIT 1;"));
    query.bindValue(QString::fromUtf8(":key"), QVariant(QCDatabaseSchema::schemaVersionKey()));

    if (!query.exec())
        return 0;

    if (!query.next())
        return 0;

    bool bOk = false;
    const int nSchemaVersion = query.value(0).toString().toInt(&bOk);
    if (!bOk)
        return -1;

    return nSchemaVersion;
}

bool QCDatabaseManager::setSchemaVersion(int nSchemaVersion)
{
    static const QString strUpsertSchemaVersionSql = QString::fromUtf8(
        "INSERT INTO app_settings(key, value, updated_at) VALUES(:key, :value, :updated_at) "
        "ON CONFLICT(key) DO UPDATE SET value = excluded.value, updated_at = excluded.updated_at;");

    clearLastError();
    clearLastFailedSql();

    if (!isOpen())
    {
        setLastError(QString::fromUtf8("Database connection is not open."));
        return false;
    }

    QSqlQuery query(m_database);
    query.prepare(strUpsertSchemaVersionSql);
    query.bindValue(QString::fromUtf8(":key"), QVariant(QCDatabaseSchema::schemaVersionKey()));
    query.bindValue(QString::fromUtf8(":value"), QVariant(QString::number(nSchemaVersion)));
    query.bindValue(QString::fromUtf8(":updated_at"), QVariant(QDateTime::currentDateTimeUtc().toString(Qt::ISODate)));

    setLastFailedSql(strUpsertSchemaVersionSql);
    if (!query.exec())
    {
        setLastError(query.lastError().text());
        return false;
    }

    clearLastFailedSql();
    return true;
}

QSqlDatabase QCDatabaseManager::database() const
{
    return m_database;
}

bool QCDatabaseManager::beginTransaction()
{
    clearLastError();
    clearLastFailedSql();

    if (!isOpen())
    {
        setLastError(QString::fromUtf8("Database connection is not open."));
        return false;
    }

    if (!m_database.transaction())
    {
        setLastError(m_database.lastError().text());
        return false;
    }

    return true;
}

bool QCDatabaseManager::commitTransaction()
{
    clearLastError();
    clearLastFailedSql();

    if (!isOpen())
    {
        setLastError(QString::fromUtf8("Database connection is not open."));
        return false;
    }

    if (!m_database.commit())
    {
        setLastError(m_database.lastError().text());
        return false;
    }

    return true;
}

bool QCDatabaseManager::rollbackTransaction()
{
    clearLastError();
    clearLastFailedSql();

    if (!isOpen())
    {
        setLastError(QString::fromUtf8("Database connection is not open."));
        return false;
    }

    if (!m_database.rollback())
    {
        setLastError(m_database.lastError().text());
        return false;
    }

    return true;
}

bool QCDatabaseManager::executeInTransaction(const std::function<bool()>& fnAction)
{
    clearLastError();
    clearLastFailedSql();

    if (!fnAction)
    {
        setLastError(QString::fromUtf8("Transaction action is invalid."));
        return false;
    }

    if (!beginTransaction())
        return false;

    if (!fnAction())
    {
        const QString strActionError = m_strLastError;
        const QString strActionSql = m_strLastFailedSql;
        if (!rollbackTransaction())
            return false;

        if (!strActionError.isEmpty())
            setLastError(strActionError);

        if (!strActionSql.isEmpty())
            setLastFailedSql(strActionSql);

        return false;
    }

    if (!commitTransaction())
    {
        const QString strCommitError = m_strLastError;
        const QString strCommitSql = m_strLastFailedSql;
        if (!rollbackTransaction())
            return false;

        setLastError(strCommitError);
        setLastFailedSql(strCommitSql);
        return false;
    }

    return true;
}

bool QCDatabaseManager::enableForeignKeys()
{
    return executeStatement(QString::fromUtf8(g_pszEnableForeignKeys));
}

bool QCDatabaseManager::executeStatement(const QString& strSql)
{
    clearLastFailedSql();
    setLastFailedSql(strSql);

    if (!isOpen())
    {
        setLastError(QString::fromUtf8("Database connection is not open."));
        return false;
    }

    QSqlQuery query(m_database);
    if (!query.exec(strSql))
    {
        setLastError(query.lastError().text());
        return false;
    }

    clearLastFailedSql();
    return true;
}

void QCDatabaseManager::setLastError(const QString& strError)
{
    m_strLastError = strError;
}

void QCDatabaseManager::clearLastError()
{
    m_strLastError.clear();
}

void QCDatabaseManager::setLastFailedSql(const QString& strSql)
{
    m_strLastFailedSql = strSql;
}

void QCDatabaseManager::clearLastFailedSql()
{
    m_strLastFailedSql.clear();
}
