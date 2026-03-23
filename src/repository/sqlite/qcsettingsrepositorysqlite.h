#ifndef QTCLIP_QCSETTINGSREPOSITORYSQLITE_H_
#define QTCLIP_QCSETTINGSREPOSITORYSQLITE_H_

// File: qcsettingsrepositorysqlite.h
// Author: ZZMI1
// Created: 2026-03-23
// Description: Declares the SQLite settings repository implementation used by QtClip.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include "../../core/database/qcdatabasemanager.h"
#include "../../domain/interfaces/iqcsettingsrepository.h"

class QSqlQuery;

class QCSettingsRepositorySqlite : public IQCSettingsRepository
{
public:
    explicit QCSettingsRepositorySqlite(QCDatabaseManager *pDatabaseManager);
    virtual ~QCSettingsRepositorySqlite() override;

    virtual bool setSetting(const QCAppSetting& appSetting) override;
    virtual bool getSettingByKey(const QString& strKey, QCAppSetting *pAppSetting) const override;
    virtual QVector<QCAppSetting> listSettings() const override;
    virtual bool deleteSetting(const QString& strKey) override;

    QString lastError() const;
    QString lastFailedSql() const;

private:
    QCSettingsRepositorySqlite(const QCSettingsRepositorySqlite& other);
    QCSettingsRepositorySqlite& operator=(const QCSettingsRepositorySqlite& other);

    bool ensureDatabaseReady() const;
    bool mapSetting(QSqlQuery *pQuery, QCAppSetting *pAppSetting) const;
    bool parseRequiredDateTime(const QString& strValue, QDateTime *pDateTime) const;
    void setLastError(const QString& strError) const;
    void setLastFailedSql(const QString& strSql) const;
    void clearErrors() const;

private:
    QCDatabaseManager *m_pDatabaseManager;
    mutable QString m_strLastError;
    mutable QString m_strLastFailedSql;
};

#endif // QTCLIP_QCSETTINGSREPOSITORYSQLITE_H_
