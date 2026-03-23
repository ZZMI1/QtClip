#ifndef QTCLIP_QCTAG_H_
#define QTCLIP_QCTAG_H_

// File: qctag.h
// Author: ZZMI1
// Created: 2026-03-23
// Description: Declares the tag entity used by the QtClip domain layer.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include <QDateTime>
#include <QString>

class QCTag
{
public:
    QCTag();
    ~QCTag();

    qint64 id() const;
    void setId(qint64 nId);

    QString name() const;
    void setName(const QString& strName);

    QString color() const;
    void setColor(const QString& strColor);

    QDateTime createdAt() const;
    void setCreatedAt(const QDateTime& dateTimeCreatedAt);

private:
    qint64 m_nId;
    QString m_strName;
    QString m_strColor;
    QDateTime m_dateTimeCreatedAt;
};

#endif // QTCLIP_QCTAG_H_
