#ifndef QTCLIP_QCAPPSETTING_H_
#define QTCLIP_QCAPPSETTING_H_

// File: qcappsetting.h
// Author: ZZMI1
// Created: 2026-03-23
// Description: Declares the app setting entity used by the QtClip domain layer.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include <QDateTime>
#include <QString>

class QCAppSetting
{
public:
    QCAppSetting();
    ~QCAppSetting();

    QString key() const;
    void setKey(const QString& strKey);

    QString value() const;
    void setValue(const QString& strValue);

    QDateTime updatedAt() const;
    void setUpdatedAt(const QDateTime& dateTimeUpdatedAt);

private:
    QString m_strKey;
    QString m_strValue;
    QDateTime m_dateTimeUpdatedAt;
};

#endif // QTCLIP_QCAPPSETTING_H_
