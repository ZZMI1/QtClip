#ifndef QTCLIP_QCDATABASEMANAGER_H_
#define QTCLIP_QCDATABASEMANAGER_H_

// File: qcdatabasemanager.h
// Author: ZZMI1
// Created: 2026-03-23
// Description: Declares the SQLite database manager used by QtClip infrastructure.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include <functional>

#include <QSqlDatabase>
#include <QString>

class QCDatabaseManager
{
public:
    QCDatabaseManager();
    ~QCDatabaseManager();

    bool open(const QString& strDatabasePath);
    bool initialize();
    bool migrate();
    bool isOpen() const;
    QString lastError() const;
    QString lastFailedSql() const;
    int schemaVersion() const;
    bool setSchemaVersion(int nSchemaVersion);
    QSqlDatabase database() const;
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();
    bool executeInTransaction(const std::function<bool()>& fnAction);

private:
    QCDatabaseManager(const QCDatabaseManager& other);
    QCDatabaseManager& operator=(const QCDatabaseManager& other);

    bool enableForeignKeys();
    bool executeStatement(const QString& strSql);
    void setLastError(const QString& strError);
    void clearLastError();
    void setLastFailedSql(const QString& strSql);
    void clearLastFailedSql();

private:
    QString m_strConnectionName;
    QSqlDatabase m_database;
    QString m_strLastError;
    QString m_strLastFailedSql;
};

#endif // QTCLIP_QCDATABASEMANAGER_H_
