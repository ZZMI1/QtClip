#ifndef QTCLIP_QCEXPORTRECORD_H_
#define QTCLIP_QCEXPORTRECORD_H_

// File: qcexportrecord.h
// Author: ZZMI1
// Created: 2026-03-23
// Description: Declares the export record entity used by the QtClip domain layer.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include <QDateTime>
#include <QString>

class QCExportRecord
{
public:
    QCExportRecord();
    ~QCExportRecord();

    qint64 id() const;
    void setId(qint64 nId);

    qint64 sessionId() const;
    void setSessionId(qint64 nSessionId);

    QString exportType() const;
    void setExportType(const QString& strExportType);

    QString filePath() const;
    void setFilePath(const QString& strFilePath);

    QDateTime createdAt() const;
    void setCreatedAt(const QDateTime& dateTimeCreatedAt);

private:
    qint64 m_nId;
    qint64 m_nSessionId;
    QString m_strExportType;
    QString m_strFilePath;
    QDateTime m_dateTimeCreatedAt;
};

#endif // QTCLIP_QCEXPORTRECORD_H_
