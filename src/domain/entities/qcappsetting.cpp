// File: qcappsetting.cpp
// Author: ZZMI1
// Created: 2026-03-23
// Description: Implements the app setting entity used by the QtClip domain layer.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include "qcappsetting.h"

QCAppSetting::QCAppSetting()
    : m_strKey()
    , m_strValue()
    , m_dateTimeUpdatedAt()
{
}

QCAppSetting::~QCAppSetting()
{
}

QString QCAppSetting::key() const
{
    return m_strKey;
}

void QCAppSetting::setKey(const QString& strKey)
{
    m_strKey = strKey;
}

QString QCAppSetting::value() const
{
    return m_strValue;
}

void QCAppSetting::setValue(const QString& strValue)
{
    m_strValue = strValue;
}

QDateTime QCAppSetting::updatedAt() const
{
    return m_dateTimeUpdatedAt;
}

void QCAppSetting::setUpdatedAt(const QDateTime& dateTimeUpdatedAt)
{
    m_dateTimeUpdatedAt = dateTimeUpdatedAt;
}
