// File: qcsettingsservice.cpp
// Author: ZZMI1
// Created: 2026-03-23
// Description: Implements the minimal settings service used by the first QtClip AI workflow.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include "qcsettingsservice.h"

#include <QDateTime>
#include <QDir>
#include <QHash>
#include <QStandardPaths>

namespace
{
const char *g_pszAiUseMockKey = "ai.useMockProvider";
const char *g_pszAiAutoSummarizeImageSnippetKey = "ai.autoSummarizeImageSnippet";
const char *g_pszAiBaseUrlKey = "ai.baseUrl";
const char *g_pszAiApiKeyKey = "ai.apiKey";
const char *g_pszAiModelKey = "ai.model";
const char *g_pszLegacyCaptureOutputDirectoryKey = "capture.outputDir";
const char *g_pszScreenshotSaveDirectoryKey = "screenshot.saveDir";
const char *g_pszExportDirectoryKey = "markdown.exportDir";
const char *g_pszDefaultCopyImportedImageKey = "import.defaultCopyToCaptureDir";

QCAppSetting BuildSetting(const QString& strKey, const QString& strValue)
{
    QCAppSetting appSetting;
    appSetting.setKey(strKey);
    appSetting.setValue(strValue);
    appSetting.setUpdatedAt(QDateTime::currentDateTimeUtc());
    return appSetting;
}

QHash<QString, QString> BuildSettingsMap(const QVector<QCAppSetting>& vecSettings)
{
    QHash<QString, QString> hashValues;
    for (int i = 0; i < vecSettings.size(); ++i)
        hashValues.insert(vecSettings.at(i).key(), vecSettings.at(i).value());

    return hashValues;
}

QString BuildDefaultDirectory(QStandardPaths::StandardLocation location, const QString& strSuffix)
{
    QString strBaseDirectory = QStandardPaths::writableLocation(location);
    if (strBaseDirectory.isEmpty())
        strBaseDirectory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (strBaseDirectory.isEmpty())
        strBaseDirectory = QDir::homePath();
    if (strBaseDirectory.isEmpty())
        return QString();

    return QDir(strBaseDirectory).filePath(strSuffix);
}

QCAiRuntimeSettings BuildDefaultAiSettings()
{
    QCAiRuntimeSettings aiSettings;
    aiSettings.m_bUseMockProvider = true;
    aiSettings.m_bAutoSummarizeImageSnippet = false;
    aiSettings.m_strBaseUrl = QString();
    aiSettings.m_strApiKey = QString();
    aiSettings.m_strModel = QString::fromUtf8("mock-summary-v1");
    return aiSettings;
}

QString DefaultScreenshotSaveDirectory()
{
    return BuildDefaultDirectory(QStandardPaths::PicturesLocation, QString::fromUtf8("QtClip/Captures"));
}

QString DefaultExportDirectory()
{
    return BuildDefaultDirectory(QStandardPaths::DocumentsLocation, QString::fromUtf8("QtClip/Exports"));
}

bool IsSamePath(const QString& strLeftPath, const QString& strRightPath)
{
    return QDir::cleanPath(strLeftPath).compare(QDir::cleanPath(strRightPath), Qt::CaseInsensitive) == 0;
}

bool DeleteSetting(IQCSettingsRepository *pSettingsRepository, const QString& strKey, QString *pstrError)
{
    if (nullptr == pSettingsRepository)
    {
        if (nullptr != pstrError)
            *pstrError = QString::fromUtf8("Settings repository is null.");
        return false;
    }

    if (!pSettingsRepository->deleteSetting(strKey))
    {
        if (nullptr != pstrError)
            *pstrError = QString::fromUtf8("Failed to delete %1.").arg(strKey);
        return false;
    }

    return true;
}

bool SaveSetting(IQCSettingsRepository *pSettingsRepository, const QString& strKey, const QString& strValue, QString *pstrError)
{
    if (nullptr == pSettingsRepository)
    {
        if (nullptr != pstrError)
            *pstrError = QString::fromUtf8("Settings repository is null.");
        return false;
    }

    if (!pSettingsRepository->setSetting(BuildSetting(strKey, strValue)))
    {
        if (nullptr != pstrError)
            *pstrError = QString::fromUtf8("Failed to save %1.").arg(strKey);
        return false;
    }

    return true;
}
}

QCSettingsService::QCSettingsService(IQCSettingsRepository *pSettingsRepository)
    : m_pSettingsRepository(pSettingsRepository)
    , m_strLastError()
{
}

QCSettingsService::~QCSettingsService()
{
}

QCAiRuntimeSettings QCSettingsService::defaultAiSettings() const
{
    return BuildDefaultAiSettings();
}

bool QCSettingsService::loadAiSettings(QCAiRuntimeSettings *pAiSettings) const
{
    clearError();

    if (nullptr == pAiSettings)
    {
        setLastError(QString::fromUtf8("AI settings output pointer is null."));
        return false;
    }

    if (nullptr == m_pSettingsRepository)
    {
        setLastError(QString::fromUtf8("Settings repository is null."));
        return false;
    }

    const QCAiRuntimeSettings defaultSettings = defaultAiSettings();
    const QHash<QString, QString> hashValues = BuildSettingsMap(m_pSettingsRepository->listSettings());
    pAiSettings->m_bUseMockProvider = hashValues.value(QString::fromUtf8(g_pszAiUseMockKey), defaultSettings.m_bUseMockProvider ? QString::fromUtf8("1") : QString::fromUtf8("0")) == QString::fromUtf8("1");
    pAiSettings->m_bAutoSummarizeImageSnippet = hashValues.value(QString::fromUtf8(g_pszAiAutoSummarizeImageSnippetKey), defaultSettings.m_bAutoSummarizeImageSnippet ? QString::fromUtf8("1") : QString::fromUtf8("0")) == QString::fromUtf8("1");
    pAiSettings->m_strBaseUrl = hashValues.value(QString::fromUtf8(g_pszAiBaseUrlKey), defaultSettings.m_strBaseUrl).trimmed();
    pAiSettings->m_strApiKey = hashValues.value(QString::fromUtf8(g_pszAiApiKeyKey), defaultSettings.m_strApiKey).trimmed();
    pAiSettings->m_strModel = hashValues.value(QString::fromUtf8(g_pszAiModelKey), defaultSettings.m_strModel).trimmed();
    if (pAiSettings->m_strModel.isEmpty())
        pAiSettings->m_strModel = defaultSettings.m_strModel;
    return true;
}

bool QCSettingsService::saveAiSettings(const QCAiRuntimeSettings& aiSettings)
{
    clearError();

    if (nullptr == m_pSettingsRepository)
    {
        setLastError(QString::fromUtf8("Settings repository is null."));
        return false;
    }

    const QCAiRuntimeSettings defaultSettings = defaultAiSettings();
    QString strError;

    if (aiSettings.m_bUseMockProvider == defaultSettings.m_bUseMockProvider)
    {
        if (!DeleteSetting(m_pSettingsRepository, QString::fromUtf8(g_pszAiUseMockKey), &strError))
        {
            setLastError(strError);
            return false;
        }
    }
    else if (!SaveSetting(m_pSettingsRepository,
                          QString::fromUtf8(g_pszAiUseMockKey),
                          aiSettings.m_bUseMockProvider ? QString::fromUtf8("1") : QString::fromUtf8("0"),
                          &strError))
    {
        setLastError(strError);
        return false;
    }

    if (aiSettings.m_bAutoSummarizeImageSnippet == defaultSettings.m_bAutoSummarizeImageSnippet)
    {
        if (!DeleteSetting(m_pSettingsRepository, QString::fromUtf8(g_pszAiAutoSummarizeImageSnippetKey), &strError))
        {
            setLastError(strError);
            return false;
        }
    }
    else if (!SaveSetting(m_pSettingsRepository,
                          QString::fromUtf8(g_pszAiAutoSummarizeImageSnippetKey),
                          aiSettings.m_bAutoSummarizeImageSnippet ? QString::fromUtf8("1") : QString::fromUtf8("0"),
                          &strError))
    {
        setLastError(strError);
        return false;
    }

    const QString strBaseUrl = aiSettings.m_strBaseUrl.trimmed();
    if (strBaseUrl.isEmpty())
    {
        if (!DeleteSetting(m_pSettingsRepository, QString::fromUtf8(g_pszAiBaseUrlKey), &strError))
        {
            setLastError(strError);
            return false;
        }
    }
    else if (!SaveSetting(m_pSettingsRepository, QString::fromUtf8(g_pszAiBaseUrlKey), strBaseUrl, &strError))
    {
        setLastError(strError);
        return false;
    }

    const QString strApiKey = aiSettings.m_strApiKey.trimmed();
    if (strApiKey.isEmpty())
    {
        if (!DeleteSetting(m_pSettingsRepository, QString::fromUtf8(g_pszAiApiKeyKey), &strError))
        {
            setLastError(strError);
            return false;
        }
    }
    else if (!SaveSetting(m_pSettingsRepository, QString::fromUtf8(g_pszAiApiKeyKey), strApiKey, &strError))
    {
        setLastError(strError);
        return false;
    }

    const QString strModel = aiSettings.m_strModel.trimmed();
    if (strModel.isEmpty() || strModel == defaultSettings.m_strModel)
    {
        if (!DeleteSetting(m_pSettingsRepository, QString::fromUtf8(g_pszAiModelKey), &strError))
        {
            setLastError(strError);
            return false;
        }
    }
    else if (!SaveSetting(m_pSettingsRepository, QString::fromUtf8(g_pszAiModelKey), strModel, &strError))
    {
        setLastError(strError);
        return false;
    }

    return true;
}

QString QCSettingsService::defaultScreenshotSaveDirectory() const
{
    return DefaultScreenshotSaveDirectory();
}

bool QCSettingsService::getScreenshotSaveDirectory(QString *pstrOutputDirectory) const
{
    clearError();

    if (nullptr == pstrOutputDirectory)
    {
        setLastError(QString::fromUtf8("Screenshot save directory output pointer is null."));
        return false;
    }

    if (nullptr == m_pSettingsRepository)
    {
        setLastError(QString::fromUtf8("Settings repository is null."));
        return false;
    }

    const QHash<QString, QString> hashValues = BuildSettingsMap(m_pSettingsRepository->listSettings());
    QString strDirectory = hashValues.value(QString::fromUtf8(g_pszScreenshotSaveDirectoryKey)).trimmed();
    if (strDirectory.isEmpty())
        strDirectory = hashValues.value(QString::fromUtf8(g_pszLegacyCaptureOutputDirectoryKey)).trimmed();
    if (strDirectory.isEmpty())
        strDirectory = defaultScreenshotSaveDirectory();

    *pstrOutputDirectory = strDirectory;
    return true;
}

bool QCSettingsService::setScreenshotSaveDirectory(const QString& strOutputDirectory)
{
    clearError();

    if (nullptr == m_pSettingsRepository)
    {
        setLastError(QString::fromUtf8("Settings repository is null."));
        return false;
    }

    const QString strNormalizedDirectory = strOutputDirectory.trimmed();
    const QString strDefaultDirectory = defaultScreenshotSaveDirectory();
    QString strError;
    if (strNormalizedDirectory.isEmpty() || IsSamePath(strNormalizedDirectory, strDefaultDirectory))
    {
        if (!DeleteSetting(m_pSettingsRepository, QString::fromUtf8(g_pszScreenshotSaveDirectoryKey), &strError))
        {
            setLastError(QString::fromUtf8("Failed to clear screenshot.saveDir."));
            return false;
        }

        if (!DeleteSetting(m_pSettingsRepository, QString::fromUtf8(g_pszLegacyCaptureOutputDirectoryKey), &strError))
        {
            setLastError(QString::fromUtf8("Failed to clear capture.outputDir."));
            return false;
        }

        return true;
    }

    if (!SaveSetting(m_pSettingsRepository, QString::fromUtf8(g_pszScreenshotSaveDirectoryKey), strNormalizedDirectory, &strError))
    {
        setLastError(QString::fromUtf8("Failed to save screenshot.saveDir."));
        return false;
    }

    if (!SaveSetting(m_pSettingsRepository, QString::fromUtf8(g_pszLegacyCaptureOutputDirectoryKey), strNormalizedDirectory, &strError))
    {
        setLastError(QString::fromUtf8("Failed to save capture.outputDir."));
        return false;
    }

    return true;
}

QString QCSettingsService::defaultExportDirectory() const
{
    return DefaultExportDirectory();
}

bool QCSettingsService::getExportDirectory(QString *pstrOutputDirectory) const
{
    clearError();

    if (nullptr == pstrOutputDirectory)
    {
        setLastError(QString::fromUtf8("Export directory output pointer is null."));
        return false;
    }

    if (nullptr == m_pSettingsRepository)
    {
        setLastError(QString::fromUtf8("Settings repository is null."));
        return false;
    }

    const QHash<QString, QString> hashValues = BuildSettingsMap(m_pSettingsRepository->listSettings());
    QString strDirectory = hashValues.value(QString::fromUtf8(g_pszExportDirectoryKey)).trimmed();
    if (strDirectory.isEmpty())
        strDirectory = defaultExportDirectory();

    *pstrOutputDirectory = strDirectory;
    return true;
}

bool QCSettingsService::setExportDirectory(const QString& strOutputDirectory)
{
    clearError();

    if (nullptr == m_pSettingsRepository)
    {
        setLastError(QString::fromUtf8("Settings repository is null."));
        return false;
    }

    const QString strNormalizedDirectory = strOutputDirectory.trimmed();
    const QString strDefaultDirectory = defaultExportDirectory();
    QString strError;
    if (strNormalizedDirectory.isEmpty() || IsSamePath(strNormalizedDirectory, strDefaultDirectory))
    {
        if (!DeleteSetting(m_pSettingsRepository, QString::fromUtf8(g_pszExportDirectoryKey), &strError))
        {
            setLastError(QString::fromUtf8("Failed to clear markdown.exportDir."));
            return false;
        }

        return true;
    }

    if (!SaveSetting(m_pSettingsRepository, QString::fromUtf8(g_pszExportDirectoryKey), strNormalizedDirectory, &strError))
    {
        setLastError(QString::fromUtf8("Failed to save markdown.exportDir."));
        return false;
    }

    return true;
}

bool QCSettingsService::defaultCopyImportedImageToCaptureDirectory() const
{
    return false;
}

bool QCSettingsService::getDefaultCopyImportedImageToCaptureDirectory(bool *pbEnabled) const
{
    clearError();

    if (nullptr == pbEnabled)
    {
        setLastError(QString::fromUtf8("Default import copy setting output pointer is null."));
        return false;
    }

    if (nullptr == m_pSettingsRepository)
    {
        setLastError(QString::fromUtf8("Settings repository is null."));
        return false;
    }

    const QHash<QString, QString> hashValues = BuildSettingsMap(m_pSettingsRepository->listSettings());
    *pbEnabled = hashValues.value(QString::fromUtf8(g_pszDefaultCopyImportedImageKey), defaultCopyImportedImageToCaptureDirectory() ? QString::fromUtf8("1") : QString::fromUtf8("0")) == QString::fromUtf8("1");
    return true;
}

bool QCSettingsService::setDefaultCopyImportedImageToCaptureDirectory(bool bEnabled)
{
    clearError();

    if (nullptr == m_pSettingsRepository)
    {
        setLastError(QString::fromUtf8("Settings repository is null."));
        return false;
    }

    QString strError;
    if (bEnabled == defaultCopyImportedImageToCaptureDirectory())
    {
        if (!DeleteSetting(m_pSettingsRepository, QString::fromUtf8(g_pszDefaultCopyImportedImageKey), &strError))
        {
            setLastError(QString::fromUtf8("Failed to clear import.defaultCopyToCaptureDir."));
            return false;
        }

        return true;
    }

    if (!SaveSetting(m_pSettingsRepository,
                     QString::fromUtf8(g_pszDefaultCopyImportedImageKey),
                     bEnabled ? QString::fromUtf8("1") : QString::fromUtf8("0"),
                     &strError))
    {
        setLastError(QString::fromUtf8("Failed to save import.defaultCopyToCaptureDir."));
        return false;
    }

    return true;
}

QString QCSettingsService::lastError() const
{
    return m_strLastError;
}

void QCSettingsService::clearError() const
{
    m_strLastError.clear();
}

void QCSettingsService::setLastError(const QString& strError) const
{
    m_strLastError = strError;
}
