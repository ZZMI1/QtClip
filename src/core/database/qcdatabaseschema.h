#ifndef QTCLIP_QCDATABASESCHEMA_H_
#define QTCLIP_QCDATABASESCHEMA_H_

// File: qcdatabaseschema.h
// Author: ZZMI1
// Created: 2026-03-23
// Description: Declares database schema statements for QtClip SQLite initialization.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include <QString>
#include <QStringList>

class QCDatabaseSchema
{
public:
    QCDatabaseSchema() = delete;
    ~QCDatabaseSchema() = delete;

    static QString schemaVersionKey();
    static int currentSchemaVersion();
    static QStringList allStatements();
};

#endif // QTCLIP_QCDATABASESCHEMA_H_
