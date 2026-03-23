// File: qcsettingsrepositorysqlite.cpp
// Author: ZZMI1
// Created: 2026-03-23
// Description: Implements the SQLite settings repository implementation used by QtClip.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include "qcsettingsrepositorysqlite.h"

#include <QDateTime>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>

namespace
{
const char *g_pszUpsertSettingSql =
    "INSERT OR REPLACE INTO app_settings (key, value, updated_at) VALUES (:key, :value, :updated_at);";

const char *g_pszSelectSettingByKeySql =
    "SELECT key, value, updated_at FROM app_settings WHERE key = :key LIMIT 1;";

const char *g_pszListSettingsSql =
    "SELECT key, value, updated_at FROM app_settings ORDER BY key ASC;";

const char *g_pszDeleteSettingSql =
    "DELETE FROM app_settings WHERE key = :key;";
}

QCSettingsRepositorySqlite::QCSettingsRepositorySqlite(QCDatabaseManager *pDatabaseManager)
    : m_pDatabaseManager(pDatabaseManager)
    , m_strLastError()
    , m_strLastFailedSql()
{
}

QCSettingsRepositorySqlite::~QCSettingsRepositorySqlite()
{
}

bool QCSettingsRepositorySqlite::setSetting(const QCAppSetting& appSetting)
{
    clearErrors();

    if (!ensureDatabaseReady())
        return false;

    if (appSetting.key().trimmed().isEmpty())
    {
        setLastError(QString::fromUtf8("Setting key is empty."));
        return false;
    }

    QSqlQuery query(m_pDatabaseManager->database());
    const QString strSql = QString::fromUtf8(g_pszUpsertSettingSql);
    setLastFailedSql(strSql);

    if (!query.prepare(strSql))
    {
        setLastError(query.lastError().text());
        return false;
    }

    const QDateTime dateTimeUpdatedAt = appSetting.updatedAt().isValid()
        ? appSetting.updatedAt()
        : QDateTime::currentDateTimeUtc();
    query.bindValue(QString::fromUtf8(":key"), appSetting.key());
    query.bindValue(QString::fromUtf8(":value"), appSetting.value());
    query.bindValue(QString::fromUtf8(":updated_at"), dateTimeUpdatedAt.toString(Qt::ISODate));

    if (!query.exec())
    {
        setLastError(query.lastError().text());
        return false;
    }

    clearErrors();
    return true;
}

bool QCSettingsRepositorySqlite::getSettingByKey(const QString& strKey, QCAppSetting *pAppSetting) const
{
    clearErrors();

    if (nullptr == pAppSetting)
    {
        setLastError(QString::fromUtf8("QCAppSetting output pointer is null."));
        return false;
    }

    if (!ensureDatabaseReady())
        return false;

    QSqlQuery query(m_pDatabaseManager->database());
    const QString strSql = QString::fromUtf8(g_pszSelectSettingByKeySql);
    setLastFailedSql(strSql);

    if (!query.prepare(strSql))
    {
        setLastError(query.lastError().text());
        return false;
    }

    query.bindValue(QString::fromUtf8(":key"), strKey);
    if (!query.exec())
    {
        setLastError(query.lastError().text());
        return false;
    }

    if (!query.next())
        return false;

    return mapSetting(&query, pAppSetting);
}

QVector<QCAppSetting> QCSettingsRepositorySqlite::listSettings() const
{
    clearErrors();
    QVector<QCAppSetting> vecSettings;

    if (!ensureDatabaseReady())
        return vecSettings;

    QSqlQuery query(m_pDatabaseManager->database());
    const QString strSql = QString::fromUtf8(g_pszListSettingsSql);
    setLastFailedSql(strSql);
    if (!query.exec(strSql))
    {
        setLastError(query.lastError().text());
        return vecSettings;
    }

    while (query.next())
    {
        QCAppSetting appSetting;
        if (!mapSetting(&query, &appSetting))
        {
            vecSettings.clear();
            return vecSettings;
        }

        vecSettings.append(appSetting);
    }

    clearErrors();
    return vecSettings;
}

bool QCSettingsRepositorySqlite::deleteSetting(const QString& strKey)
{
    clearErrors();

    if (!ensureDatabaseReady())
        return false;

    QSqlQuery query(m_pDatabaseManager->database());
    const QString strSql = QString::fromUtf8(g_pszDeleteSettingSql);
    setLastFailedSql(strSql);

    if (!query.prepare(strSql))
    {
        setLastError(query.lastError().text());
        return false;
    }

    query.bindValue(QString::fromUtf8(":key"), strKey);
    if (!query.exec())
    {
        setLastError(query.lastError().text());
        return false;
    }

    clearErrors();
    return true;
}

QString QCSettingsRepositorySqlite::lastError() const
{
    return m_strLastError;
}

QString QCSettingsRepositorySqlite::lastFailedSql() const
{
    return m_strLastFailedSql;
}

bool QCSettingsRepositorySqlite::ensureDatabaseReady() const
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

bool QCSettingsRepositorySqlite::mapSetting(QSqlQuery *pQuery, QCAppSetting *pAppSetting) const
{
    if (nullptr == pQuery || nullptr == pAppSetting)
    {
        setLastError(QString::fromUtf8("Setting mapping input is invalid."));
        return false;
    }

    const QSqlRecord record = pQuery->record();
    pAppSetting->setKey(record.value(QString::fromUtf8("key")).toString());
    pAppSetting->setValue(record.value(QString::fromUtf8("value")).toString());

    QDateTime dateTimeUpdatedAt;
    if (!parseRequiredDateTime(record.value(QString::fromUtf8("updated_at")).toString(), &dateTimeUpdatedAt))
        return false;

    pAppSetting->setUpdatedAt(dateTimeUpdatedAt);
    return true;
}

bool QCSettingsRepositorySqlite::parseRequiredDateTime(const QString& strValue, QDateTime *pDateTime) const
{
    if (nullptr == pDateTime)
    {
        setLastError(QString::fromUtf8("DateTime output pointer is null."));
        return false;
    }

    const QDateTime dateTimeValue = QDateTime::fromString(strValue, Qt::ISODate);
    if (!dateTimeValue.isValid())
    {
        setLastError(QString::fromUtf8("Failed to parse setting updated_at value."));
        return false;
    }

    *pDateTime = dateTimeValue;
    return true;
}

void QCSettingsRepositorySqlite::setLastError(const QString& strError) const
{
    m_strLastError = strError;
}

void QCSettingsRepositorySqlite::setLastFailedSql(const QString& strSql) const
{
    m_strLastFailedSql = strSql;
}

void QCSettingsRepositorySqlite::clearErrors() const
{
    m_strLastError.clear();
    m_strLastFailedSql.clear();
}
