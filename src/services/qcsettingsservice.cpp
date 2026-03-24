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
const char *g_pszAiActiveProfileIndexKey = "ai.activeProfileIndex";
const char *g_pszLegacyCaptureOutputDirectoryKey = "capture.outputDir";
const char *g_pszScreenshotSaveDirectoryKey = "screenshot.saveDir";
const char *g_pszExportDirectoryKey = "markdown.exportDir";
const char *g_pszDefaultCopyImportedImageKey = "import.defaultCopyToCaptureDir";
const int g_nAiSettingsProfileCount = 3;

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

QVector<QCAiRuntimeSettings> BuildDefaultAiSettingsProfiles()
{
    QVector<QCAiRuntimeSettings> vecAiSettingsProfiles;
    vecAiSettingsProfiles.reserve(g_nAiSettingsProfileCount);
    for (int i = 0; i < g_nAiSettingsProfileCount; ++i)
        vecAiSettingsProfiles.append(BuildDefaultAiSettings());

    return vecAiSettingsProfiles;
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

int NormalizeAiProfileIndex(int nProfileIndex)
{
    if (nProfileIndex < 0 || nProfileIndex >= g_nAiSettingsProfileCount)
        return 0;

    return nProfileIndex;
}

QString BuildAiProfileKey(int nProfileIndex, const QString& strSuffix)
{
    return QString::fromUtf8("ai.profile.%1.%2").arg(nProfileIndex + 1).arg(strSuffix);
}

QString BuildLegacyAiKeyForProfileIndex(int nProfileIndex, const QString& strSuffix)
{
    if (nProfileIndex != 0)
        return QString();

    if (strSuffix == QString::fromUtf8("useMockProvider"))
        return QString::fromUtf8(g_pszAiUseMockKey);
    if (strSuffix == QString::fromUtf8("autoSummarizeImageSnippet"))
        return QString::fromUtf8(g_pszAiAutoSummarizeImageSnippetKey);
    if (strSuffix == QString::fromUtf8("baseUrl"))
        return QString::fromUtf8(g_pszAiBaseUrlKey);
    if (strSuffix == QString::fromUtf8("apiKey"))
        return QString::fromUtf8(g_pszAiApiKeyKey);
    if (strSuffix == QString::fromUtf8("model"))
        return QString::fromUtf8(g_pszAiModelKey);

    return QString();
}

bool ReadBoolSetting(const QHash<QString, QString>& hashValues,
                     const QString& strPrimaryKey,
                     const QString& strFallbackKey,
                     bool bDefaultValue)
{
    QString strValue = hashValues.value(strPrimaryKey);
    if (strValue.trimmed().isEmpty() && !strFallbackKey.isEmpty())
        strValue = hashValues.value(strFallbackKey);
    if (strValue.trimmed().isEmpty())
        return bDefaultValue;

    return strValue.trimmed() == QString::fromUtf8("1");
}

QString ReadStringSetting(const QHash<QString, QString>& hashValues,
                          const QString& strPrimaryKey,
                          const QString& strFallbackKey,
                          const QString& strDefaultValue)
{
    QString strValue = hashValues.value(strPrimaryKey).trimmed();
    if (strValue.isEmpty() && !strFallbackKey.isEmpty())
        strValue = hashValues.value(strFallbackKey).trimmed();
    if (strValue.isEmpty())
        strValue = strDefaultValue;

    return strValue.trimmed();
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

bool SaveOptionalBoolSetting(IQCSettingsRepository *pSettingsRepository,
                             const QString& strKey,
                             bool bValue,
                             bool bDefaultValue,
                             QString *pstrError)
{
    if (bValue == bDefaultValue)
        return DeleteSetting(pSettingsRepository, strKey, pstrError);

    return SaveSetting(pSettingsRepository,
                       strKey,
                       bValue ? QString::fromUtf8("1") : QString::fromUtf8("0"),
                       pstrError);
}

bool SaveOptionalStringSetting(IQCSettingsRepository *pSettingsRepository,
                               const QString& strKey,
                               const QString& strValue,
                               const QString& strDefaultValue,
                               QString *pstrError)
{
    const QString strNormalizedValue = strValue.trimmed();
    if (strNormalizedValue.isEmpty() || strNormalizedValue == strDefaultValue.trimmed())
        return DeleteSetting(pSettingsRepository, strKey, pstrError);

    return SaveSetting(pSettingsRepository, strKey, strNormalizedValue, pstrError);
}

bool SaveLegacyAiSettings(IQCSettingsRepository *pSettingsRepository,
                          const QCAiRuntimeSettings& aiSettings,
                          const QCAiRuntimeSettings& defaultSettings,
                          QString *pstrError)
{
    if (!SaveOptionalBoolSetting(pSettingsRepository,
                                 QString::fromUtf8(g_pszAiUseMockKey),
                                 aiSettings.m_bUseMockProvider,
                                 defaultSettings.m_bUseMockProvider,
                                 pstrError))
    {
        return false;
    }

    if (!SaveOptionalBoolSetting(pSettingsRepository,
                                 QString::fromUtf8(g_pszAiAutoSummarizeImageSnippetKey),
                                 aiSettings.m_bAutoSummarizeImageSnippet,
                                 defaultSettings.m_bAutoSummarizeImageSnippet,
                                 pstrError))
    {
        return false;
    }

    if (!SaveOptionalStringSetting(pSettingsRepository,
                                   QString::fromUtf8(g_pszAiBaseUrlKey),
                                   aiSettings.m_strBaseUrl,
                                   defaultSettings.m_strBaseUrl,
                                   pstrError))
    {
        return false;
    }

    if (!SaveOptionalStringSetting(pSettingsRepository,
                                   QString::fromUtf8(g_pszAiApiKeyKey),
                                   aiSettings.m_strApiKey,
                                   defaultSettings.m_strApiKey,
                                   pstrError))
    {
        return false;
    }

    if (!SaveOptionalStringSetting(pSettingsRepository,
                                   QString::fromUtf8(g_pszAiModelKey),
                                   aiSettings.m_strModel,
                                   defaultSettings.m_strModel,
                                   pstrError))
    {
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

int QCSettingsService::aiSettingsProfileCount() const
{
    return g_nAiSettingsProfileCount;
}

int QCSettingsService::defaultAiProfileIndex() const
{
    return 0;
}

QCAiRuntimeSettings QCSettingsService::defaultAiSettings() const
{
    return BuildDefaultAiSettings();
}

QVector<QCAiRuntimeSettings> QCSettingsService::defaultAiSettingsProfiles() const
{
    return BuildDefaultAiSettingsProfiles();
}

bool QCSettingsService::getActiveAiProfileIndex(int *pnProfileIndex) const
{
    clearError();

    if (nullptr == pnProfileIndex)
    {
        setLastError(QString::fromUtf8("AI profile index output pointer is null."));
        return false;
    }

    if (nullptr == m_pSettingsRepository)
    {
        setLastError(QString::fromUtf8("Settings repository is null."));
        return false;
    }

    const QHash<QString, QString> hashValues = BuildSettingsMap(m_pSettingsRepository->listSettings());
    bool bOk = false;
    const int nStoredIndex = hashValues.value(QString::fromUtf8(g_pszAiActiveProfileIndexKey)).trimmed().toInt(&bOk);
    *pnProfileIndex = bOk ? NormalizeAiProfileIndex(nStoredIndex) : defaultAiProfileIndex();
    return true;
}

bool QCSettingsService::setActiveAiProfileIndex(int nProfileIndex)
{
    clearError();

    if (nullptr == m_pSettingsRepository)
    {
        setLastError(QString::fromUtf8("Settings repository is null."));
        return false;
    }

    const int nNormalizedProfileIndex = NormalizeAiProfileIndex(nProfileIndex);
    QString strError;
    if (nNormalizedProfileIndex == defaultAiProfileIndex())
    {
        if (!DeleteSetting(m_pSettingsRepository, QString::fromUtf8(g_pszAiActiveProfileIndexKey), &strError))
        {
            setLastError(strError);
            return false;
        }

        return true;
    }

    if (!SaveSetting(m_pSettingsRepository,
                     QString::fromUtf8(g_pszAiActiveProfileIndexKey),
                     QString::number(nNormalizedProfileIndex),
                     &strError))
    {
        setLastError(strError);
        return false;
    }

    return true;
}

bool QCSettingsService::loadAiSettingsProfiles(QVector<QCAiRuntimeSettings> *pvecAiSettingsProfiles) const
{
    clearError();

    if (nullptr == pvecAiSettingsProfiles)
    {
        setLastError(QString::fromUtf8("AI settings profiles output pointer is null."));
        return false;
    }

    if (nullptr == m_pSettingsRepository)
    {
        setLastError(QString::fromUtf8("Settings repository is null."));
        return false;
    }

    const QHash<QString, QString> hashValues = BuildSettingsMap(m_pSettingsRepository->listSettings());
    QVector<QCAiRuntimeSettings> vecAiSettingsProfiles = defaultAiSettingsProfiles();
    const QCAiRuntimeSettings defaultSettings = defaultAiSettings();

    for (int i = 0; i < vecAiSettingsProfiles.size(); ++i)
    {
        QCAiRuntimeSettings aiSettings = defaultSettings;
        const QString strUseMockFallbackKey = BuildLegacyAiKeyForProfileIndex(i, QString::fromUtf8("useMockProvider"));
        const QString strAutoSummaryFallbackKey = BuildLegacyAiKeyForProfileIndex(i, QString::fromUtf8("autoSummarizeImageSnippet"));
        const QString strBaseUrlFallbackKey = BuildLegacyAiKeyForProfileIndex(i, QString::fromUtf8("baseUrl"));
        const QString strApiKeyFallbackKey = BuildLegacyAiKeyForProfileIndex(i, QString::fromUtf8("apiKey"));
        const QString strModelFallbackKey = BuildLegacyAiKeyForProfileIndex(i, QString::fromUtf8("model"));

        aiSettings.m_bUseMockProvider = ReadBoolSetting(hashValues,
                                                        BuildAiProfileKey(i, QString::fromUtf8("useMockProvider")),
                                                        strUseMockFallbackKey,
                                                        defaultSettings.m_bUseMockProvider);
        aiSettings.m_bAutoSummarizeImageSnippet = ReadBoolSetting(hashValues,
                                                                  BuildAiProfileKey(i, QString::fromUtf8("autoSummarizeImageSnippet")),
                                                                  strAutoSummaryFallbackKey,
                                                                  defaultSettings.m_bAutoSummarizeImageSnippet);
        aiSettings.m_strBaseUrl = ReadStringSetting(hashValues,
                                                    BuildAiProfileKey(i, QString::fromUtf8("baseUrl")),
                                                    strBaseUrlFallbackKey,
                                                    defaultSettings.m_strBaseUrl);
        aiSettings.m_strApiKey = ReadStringSetting(hashValues,
                                                   BuildAiProfileKey(i, QString::fromUtf8("apiKey")),
                                                   strApiKeyFallbackKey,
                                                   defaultSettings.m_strApiKey);
        aiSettings.m_strModel = ReadStringSetting(hashValues,
                                                  BuildAiProfileKey(i, QString::fromUtf8("model")),
                                                  strModelFallbackKey,
                                                  defaultSettings.m_strModel);
        if (aiSettings.m_strModel.isEmpty())
            aiSettings.m_strModel = defaultSettings.m_strModel;

        vecAiSettingsProfiles[i] = aiSettings;
    }

    *pvecAiSettingsProfiles = vecAiSettingsProfiles;
    return true;
}

bool QCSettingsService::saveAiSettingsProfiles(const QVector<QCAiRuntimeSettings>& vecAiSettingsProfiles)
{
    clearError();

    if (nullptr == m_pSettingsRepository)
    {
        setLastError(QString::fromUtf8("Settings repository is null."));
        return false;
    }

    if (vecAiSettingsProfiles.size() != aiSettingsProfileCount())
    {
        setLastError(QString::fromUtf8("AI settings profile count is invalid."));
        return false;
    }

    const QCAiRuntimeSettings defaultSettings = defaultAiSettings();
    QString strError;
    for (int i = 0; i < vecAiSettingsProfiles.size(); ++i)
    {
        const QCAiRuntimeSettings& aiSettings = vecAiSettingsProfiles.at(i);
        if (!SaveOptionalBoolSetting(m_pSettingsRepository,
                                     BuildAiProfileKey(i, QString::fromUtf8("useMockProvider")),
                                     aiSettings.m_bUseMockProvider,
                                     defaultSettings.m_bUseMockProvider,
                                     &strError))
        {
            setLastError(strError);
            return false;
        }

        if (!SaveOptionalBoolSetting(m_pSettingsRepository,
                                     BuildAiProfileKey(i, QString::fromUtf8("autoSummarizeImageSnippet")),
                                     aiSettings.m_bAutoSummarizeImageSnippet,
                                     defaultSettings.m_bAutoSummarizeImageSnippet,
                                     &strError))
        {
            setLastError(strError);
            return false;
        }

        if (!SaveOptionalStringSetting(m_pSettingsRepository,
                                       BuildAiProfileKey(i, QString::fromUtf8("baseUrl")),
                                       aiSettings.m_strBaseUrl,
                                       defaultSettings.m_strBaseUrl,
                                       &strError))
        {
            setLastError(strError);
            return false;
        }

        if (!SaveOptionalStringSetting(m_pSettingsRepository,
                                       BuildAiProfileKey(i, QString::fromUtf8("apiKey")),
                                       aiSettings.m_strApiKey,
                                       defaultSettings.m_strApiKey,
                                       &strError))
        {
            setLastError(strError);
            return false;
        }

        if (!SaveOptionalStringSetting(m_pSettingsRepository,
                                       BuildAiProfileKey(i, QString::fromUtf8("model")),
                                       aiSettings.m_strModel,
                                       defaultSettings.m_strModel,
                                       &strError))
        {
            setLastError(strError);
            return false;
        }
    }

    if (!SaveLegacyAiSettings(m_pSettingsRepository, vecAiSettingsProfiles.at(0), defaultSettings, &strError))
    {
        setLastError(strError);
        return false;
    }

    return true;
}

bool QCSettingsService::loadAiSettings(QCAiRuntimeSettings *pAiSettings) const
{
    clearError();

    if (nullptr == pAiSettings)
    {
        setLastError(QString::fromUtf8("AI settings output pointer is null."));
        return false;
    }

    QVector<QCAiRuntimeSettings> vecAiSettingsProfiles;
    if (!loadAiSettingsProfiles(&vecAiSettingsProfiles))
        return false;

    int nActiveProfileIndex = defaultAiProfileIndex();
    if (!getActiveAiProfileIndex(&nActiveProfileIndex))
        return false;

    *pAiSettings = vecAiSettingsProfiles.at(NormalizeAiProfileIndex(nActiveProfileIndex));
    return true;
}

bool QCSettingsService::saveAiSettings(const QCAiRuntimeSettings& aiSettings)
{
    clearError();

    QVector<QCAiRuntimeSettings> vecAiSettingsProfiles;
    if (!loadAiSettingsProfiles(&vecAiSettingsProfiles))
        return false;

    int nActiveProfileIndex = defaultAiProfileIndex();
    if (!getActiveAiProfileIndex(&nActiveProfileIndex))
        return false;

    vecAiSettingsProfiles[NormalizeAiProfileIndex(nActiveProfileIndex)] = aiSettings;
    return saveAiSettingsProfiles(vecAiSettingsProfiles);
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
