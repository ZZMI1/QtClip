#ifndef QTCLIP_QCSETTINGSSERVICE_H_
#define QTCLIP_QCSETTINGSSERVICE_H_

// File: qcsettingsservice.h
// Author: ZZMI1
// Created: 2026-03-23
// Description: Declares the minimal settings service used by the first QtClip AI workflow.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include <QString>
#include <QVector>

#include "../domain/interfaces/iqcsettingsrepository.h"

struct QCAiRuntimeSettings
{
    bool m_bUseMockProvider;
    bool m_bAutoSummarizeImageSnippet;
    QString m_strBaseUrl;
    QString m_strApiKey;
    QString m_strModel;
};

class QCSettingsService
{
public:
    explicit QCSettingsService(IQCSettingsRepository *pSettingsRepository);
    ~QCSettingsService();

    int aiSettingsProfileCount() const;
    int defaultAiProfileIndex() const;
    QCAiRuntimeSettings defaultAiSettings() const;
    QVector<QCAiRuntimeSettings> defaultAiSettingsProfiles() const;
    bool getActiveAiProfileIndex(int *pnProfileIndex) const;
    bool setActiveAiProfileIndex(int nProfileIndex);
    bool loadAiSettingsProfiles(QVector<QCAiRuntimeSettings> *pvecAiSettingsProfiles) const;
    bool saveAiSettingsProfiles(const QVector<QCAiRuntimeSettings>& vecAiSettingsProfiles);
    bool loadAiSettings(QCAiRuntimeSettings *pAiSettings) const;
    bool saveAiSettings(const QCAiRuntimeSettings& aiSettings);
    QString defaultScreenshotSaveDirectory() const;
    bool getScreenshotSaveDirectory(QString *pstrOutputDirectory) const;
    bool setScreenshotSaveDirectory(const QString& strOutputDirectory);
    QString defaultExportDirectory() const;
    bool getExportDirectory(QString *pstrOutputDirectory) const;
    bool setExportDirectory(const QString& strOutputDirectory);
    bool defaultCopyImportedImageToCaptureDirectory() const;
    bool getDefaultCopyImportedImageToCaptureDirectory(bool *pbEnabled) const;
    bool setDefaultCopyImportedImageToCaptureDirectory(bool bEnabled);

    QString lastError() const;
    void clearError() const;

private:
    QCSettingsService(const QCSettingsService& other);
    QCSettingsService& operator=(const QCSettingsService& other);

    void setLastError(const QString& strError) const;

private:
    IQCSettingsRepository *m_pSettingsRepository;
    mutable QString m_strLastError;
};

#endif // QTCLIP_QCSETTINGSSERVICE_H_
