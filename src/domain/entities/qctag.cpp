// File: qctag.cpp
// Author: ZZMI1
// Created: 2026-03-23
// Description: Implements the tag entity used by the QtClip domain layer.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include "qctag.h"

QCTag::QCTag()
    : m_nId(0)
    , m_strName()
    , m_strColor()
    , m_dateTimeCreatedAt()
{
}

QCTag::~QCTag()
{
}

qint64 QCTag::id() const
{
    return m_nId;
}

void QCTag::setId(qint64 nId)
{
    m_nId = nId;
}

QString QCTag::name() const
{
    return m_strName;
}

void QCTag::setName(const QString& strName)
{
    m_strName = strName;
}

QString QCTag::color() const
{
    return m_strColor;
}

void QCTag::setColor(const QString& strColor)
{
    m_strColor = strColor;
}

QDateTime QCTag::createdAt() const
{
    return m_dateTimeCreatedAt;
}

void QCTag::setCreatedAt(const QDateTime& dateTimeCreatedAt)
{
    m_dateTimeCreatedAt = dateTimeCreatedAt;
}
