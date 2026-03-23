// File: qcexportrecord.cpp
// Author: ZZMI1
// Created: 2026-03-23
// Description: Implements the export record entity used by the QtClip domain layer.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include "qcexportrecord.h"

QCExportRecord::QCExportRecord()
    : m_nId(0)
    , m_nSessionId(0)
    , m_strExportType()
    , m_strFilePath()
    , m_dateTimeCreatedAt()
{
}

QCExportRecord::~QCExportRecord()
{
}

qint64 QCExportRecord::id() const
{
    return m_nId;
}

void QCExportRecord::setId(qint64 nId)
{
    m_nId = nId;
}

qint64 QCExportRecord::sessionId() const
{
    return m_nSessionId;
}

void QCExportRecord::setSessionId(qint64 nSessionId)
{
    m_nSessionId = nSessionId;
}

QString QCExportRecord::exportType() const
{
    return m_strExportType;
}

void QCExportRecord::setExportType(const QString& strExportType)
{
    m_strExportType = strExportType;
}

QString QCExportRecord::filePath() const
{
    return m_strFilePath;
}

void QCExportRecord::setFilePath(const QString& strFilePath)
{
    m_strFilePath = strFilePath;
}

QDateTime QCExportRecord::createdAt() const
{
    return m_dateTimeCreatedAt;
}

void QCExportRecord::setCreatedAt(const QDateTime& dateTimeCreatedAt)
{
    m_dateTimeCreatedAt = dateTimeCreatedAt;
}
