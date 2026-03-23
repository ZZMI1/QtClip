#ifndef QTCLIP_IQCSETTINGSREPOSITORY_H_
#define QTCLIP_IQCSETTINGSREPOSITORY_H_

// File: iqcsettingsrepository.h
// Author: ZZMI1
// Created: 2026-03-23
// Description: Declares the settings repository abstraction used by the QtClip domain layer.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include <QVector>

#include "../entities/qcappsetting.h"

class IQCSettingsRepository
{
public:
    virtual ~IQCSettingsRepository() = default;

    virtual bool setSetting(const QCAppSetting& appSetting) = 0;
    virtual bool getSettingByKey(const QString& strKey, QCAppSetting *pAppSetting) const = 0;
    virtual QVector<QCAppSetting> listSettings() const = 0;
    virtual bool deleteSetting(const QString& strKey) = 0;

protected:
    IQCSettingsRepository() = default;

private:
    IQCSettingsRepository(const IQCSettingsRepository& other);
    IQCSettingsRepository& operator=(const IQCSettingsRepository& other);
};

#endif // QTCLIP_IQCSETTINGSREPOSITORY_H_
