// File: main.cpp
// Author: ZZMI1
// Created: 2026-03-23
// Description: Provides the minimal Qt Widgets application entry used by the QtClip demo application.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include <QApplication>
#include <QtConcurrent/QtConcurrent>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QMessageBox>
#include <QScreen>
#include <QStandardPaths>
#include <QTextStream>

#include "../core/database/qcdatabasemanager.h"
#include "../repository/sqlite/qcairecordrepositorysqlite.h"
#include "../repository/sqlite/qcsessionrepositorysqlite.h"
#include "../repository/sqlite/qcsettingsrepositorysqlite.h"
#include "../repository/sqlite/qcsnippetrepositorysqlite.h"
#include "../repository/sqlite/qctagrepositorysqlite.h"
#include "../services/qcaiprocessservice.h"
#include "../services/qcaiservice.h"
#include "../services/qcexportdataservice.h"
#include "../services/qcmdexportrenderer.h"
#include "../services/qcmdexportservice.h"
#include "../services/qcscreencaptureservice.h"
#include "../services/qcsessionservice.h"
#include "../services/qcsettingsservice.h"
#include "../services/qcsnippetservice.h"
#include "../services/qctagservice.h"
#include "../ui/dialogs/qccreateimagesnippetdialog.h"
#include "../ui/mainwindow/qcmainwindow.h"

namespace
{
QCAiTaskExecutionResult RunAiTaskInBackground(QCAiProcessService *pAiProcessService,
                                              const QCAiTaskExecutionContext& executionContext)
{
    QCAiTaskExecutionResult executionResult;
    executionResult.m_bSuccess = false;
    executionResult.m_context = executionContext;
    executionResult.m_aiResponse = QCAiProviderResponse();
    executionResult.m_strErrorMessage = QString::fromUtf8("AI process service is unavailable.");

    if (nullptr == pAiProcessService)
        return executionResult;

    pAiProcessService->executePreparedTask(executionContext, &executionResult);
    return executionResult;
}

void PrintError(const QString& strMessage)
{
    QTextStream stderrStream(stderr);
    stderrStream << strMessage << Qt::endl;
}

void PrintInfo(const QString& strMessage)
{
    QTextStream stdoutStream(stdout);
    stdoutStream << strMessage << Qt::endl;
}

QString ResolveAppDataPath()
{
    QString strDataDirectory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (strDataDirectory.isEmpty())
        strDataDirectory = QDir::tempPath();

    QDir dataDirectory(strDataDirectory);
    if (!dataDirectory.exists())
        dataDirectory.mkpath(QString::fromUtf8("."));

    return dataDirectory.filePath(QString::fromUtf8("qtclip.sqlite"));
}

QString BuildPreviewText(const QString& strText, int nMaxLength = 240)
{
    QString strPreview = strText;
    strPreview.replace(QString::fromUtf8("\r"), QString());
    strPreview.replace(QString::fromUtf8("\n"), QString::fromUtf8(" "));
    strPreview = strPreview.trimmed();
    if (strPreview.size() > nMaxLength)
        strPreview = strPreview.left(nMaxLength) + QString::fromUtf8("...");

    return strPreview;
}

int RunSmokeWorkflow()
{
    QString strTempDirectory = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (strTempDirectory.isEmpty())
        strTempDirectory = QDir::tempPath();

    const QString strDatabasePath = QDir(strTempDirectory).filePath(QString::fromUtf8("qtclip_smoke.sqlite"));
    const QString strSmokeScreenshotDirectory = QDir(strTempDirectory).filePath(QString::fromUtf8("qtclip_smoke_captures"));
    const QString strSmokeExportDirectory = QDir(strTempDirectory).filePath(QString::fromUtf8("qtclip_smoke_exports"));
    const QString strMarkdownPath = QDir(strSmokeExportDirectory).filePath(QString::fromUtf8("qtclip_smoke_export.md"));
    QFile::remove(strDatabasePath);
    QFile::remove(strMarkdownPath);

    QCDatabaseManager databaseManager;
    if (!databaseManager.open(strDatabasePath))
    {
        PrintError(databaseManager.lastError());
        return 1;
    }

    if (!databaseManager.initialize())
    {
        PrintError(databaseManager.lastError());
        return 2;
    }

    QCSessionRepositorySqlite sessionRepository(&databaseManager);
    QCSnippetRepositorySqlite snippetRepository(&databaseManager);
    QCAiRecordRepositorySqlite aiRecordRepository(&databaseManager);
    QCSettingsRepositorySqlite settingsRepository(&databaseManager);
    QCTagRepositorySqlite tagRepository(&databaseManager);
    QCSessionService sessionService(&sessionRepository);
    QCSnippetService snippetService(&snippetRepository);
    QCAiService aiService(&aiRecordRepository);
    QCSettingsService settingsService(&settingsRepository);
    QCTagService tagService(&tagRepository);
    QCAiProcessService aiProcessService(&sessionService, &snippetService, &aiService, &settingsService);
    QCExportDataService exportDataService(&sessionService, &snippetService, &aiService);
    QCMdExportRenderer mdExportRenderer;
    QCMdExportService mdExportService(&exportDataService, &mdExportRenderer);
    QCScreenCaptureService screenCaptureService(&settingsService);

    QCAiRuntimeSettings aiSettings;
    aiSettings.m_bUseMockProvider = true;
    aiSettings.m_bAutoSummarizeImageSnippet = false;
    aiSettings.m_strModel = QString::fromUtf8("mock-summary-v1");
    if (!settingsService.saveAiSettings(aiSettings))
    {
        PrintError(settingsService.lastError());
        return 3;
    }

    QCAiConnectionTestResult mockConnectionTestResult;
    if (!aiProcessService.testConnection(aiSettings, &mockConnectionTestResult) || !mockConnectionTestResult.m_bSuccess)
    {
        PrintError(mockConnectionTestResult.m_strMessage.isEmpty() ? QString::fromUtf8("Mock AI connection test failed.") : mockConnectionTestResult.m_strMessage);
        return 30;
    }

    QCAiRuntimeSettings invalidRealAiSettings;
    invalidRealAiSettings.m_bUseMockProvider = false;
    invalidRealAiSettings.m_strModel = QString::fromUtf8("gpt-test");
    QCAiConnectionTestResult invalidRealConnectionTestResult;
    if (aiProcessService.testConnection(invalidRealAiSettings, &invalidRealConnectionTestResult))
    {
        PrintError(QString::fromUtf8("Invalid real AI configuration test should have failed."));
        return 301;
    }

    if (!invalidRealConnectionTestResult.m_strMessage.contains(QString::fromUtf8("missing"), Qt::CaseInsensitive))
    {
        PrintError(QString::fromUtf8("Invalid real AI configuration error verification failed."));
        return 302;
    }

    QVector<QCAiRuntimeSettings> vecAiSettingsProfiles = settingsService.defaultAiSettingsProfiles();
    vecAiSettingsProfiles[0].m_bUseMockProvider = true;
    vecAiSettingsProfiles[0].m_strModel = QString::fromUtf8("mock-profile-1");
    vecAiSettingsProfiles[1].m_bUseMockProvider = false;
    vecAiSettingsProfiles[1].m_strBaseUrl = QString::fromUtf8("https://example.com/v1");
    vecAiSettingsProfiles[1].m_strApiKey = QString::fromUtf8("test-key-2");
    vecAiSettingsProfiles[1].m_strModel = QString::fromUtf8("gpt-profile-2");
    vecAiSettingsProfiles[2].m_bUseMockProvider = false;
    vecAiSettingsProfiles[2].m_strBaseUrl = QString::fromUtf8("https://example.org/v1");
    vecAiSettingsProfiles[2].m_strApiKey = QString::fromUtf8("test-key-3");
    vecAiSettingsProfiles[2].m_strModel = QString::fromUtf8("gpt-profile-3");
    if (!settingsService.saveAiSettingsProfiles(vecAiSettingsProfiles))
    {
        PrintError(settingsService.lastError());
        return 303;
    }

    if (!settingsService.setActiveAiProfileIndex(2))
    {
        PrintError(settingsService.lastError());
        return 304;
    }

    QVector<QCAiRuntimeSettings> vecLoadedAiSettingsProfiles;
    if (!settingsService.loadAiSettingsProfiles(&vecLoadedAiSettingsProfiles))
    {
        PrintError(settingsService.lastError());
        return 305;
    }

    int nLoadedActiveAiProfileIndex = 0;
    if (!settingsService.getActiveAiProfileIndex(&nLoadedActiveAiProfileIndex))
    {
        PrintError(settingsService.lastError());
        return 306;
    }

    QCAiRuntimeSettings loadedActiveAiSettings;
    if (!settingsService.loadAiSettings(&loadedActiveAiSettings))
    {
        PrintError(settingsService.lastError());
        return 307;
    }

    if (vecLoadedAiSettingsProfiles.size() != settingsService.aiSettingsProfileCount()
        || nLoadedActiveAiProfileIndex != 2
        || vecLoadedAiSettingsProfiles.at(1).m_strModel != QString::fromUtf8("gpt-profile-2")
        || vecLoadedAiSettingsProfiles.at(2).m_strModel != QString::fromUtf8("gpt-profile-3")
        || loadedActiveAiSettings.m_strModel != QString::fromUtf8("gpt-profile-3"))
    {
        PrintError(QString::fromUtf8("AI settings profile round-trip verification failed."));
        return 308;
    }

    if (!settingsService.setActiveAiProfileIndex(settingsService.defaultAiProfileIndex()))
    {
        PrintError(settingsService.lastError());
        return 309;
    }

    if (!settingsService.setScreenshotSaveDirectory(strSmokeScreenshotDirectory))
    {
        PrintError(settingsService.lastError());
        return 31;
    }

    if (!settingsService.setExportDirectory(strSmokeExportDirectory))
    {
        PrintError(settingsService.lastError());
        return 32;
    }


    if (!settingsService.setDefaultCopyImportedImageToCaptureDirectory(true))
    {
        PrintError(settingsService.lastError());
        return 38;
    }
    QString strLoadedScreenshotDirectory;
    if (!settingsService.getScreenshotSaveDirectory(&strLoadedScreenshotDirectory))
    {
        PrintError(settingsService.lastError());
        return 33;
    }

    QString strLoadedExportDirectory;
    if (!settingsService.getExportDirectory(&strLoadedExportDirectory))
    {
        PrintError(settingsService.lastError());
        return 34;
    }


    bool bLoadedDefaultCopyImportedImage = false;
    if (!settingsService.getDefaultCopyImportedImageToCaptureDirectory(&bLoadedDefaultCopyImportedImage))
    {
        PrintError(settingsService.lastError());
        return 39;
    }
    if (QDir::cleanPath(strLoadedScreenshotDirectory) != QDir::cleanPath(strSmokeScreenshotDirectory)
        || QDir::cleanPath(strLoadedExportDirectory) != QDir::cleanPath(strSmokeExportDirectory)
        || !bLoadedDefaultCopyImportedImage)
    {
        PrintError(QString::fromUtf8("Settings directory round-trip verification failed."));
        return 35;
    }

    if (!settingsService.setScreenshotSaveDirectory(settingsService.defaultScreenshotSaveDirectory()))
    {
        PrintError(settingsService.lastError());
        return 41;
    }

    if (!settingsService.setExportDirectory(settingsService.defaultExportDirectory()))
    {
        PrintError(settingsService.lastError());
        return 42;
    }

    if (!settingsService.setDefaultCopyImportedImageToCaptureDirectory(settingsService.defaultCopyImportedImageToCaptureDirectory()))
    {
        PrintError(settingsService.lastError());
        return 43;
    }

    QString strRestoredScreenshotDirectory;
    if (!settingsService.getScreenshotSaveDirectory(&strRestoredScreenshotDirectory))
    {
        PrintError(settingsService.lastError());
        return 44;
    }

    QString strRestoredExportDirectory;
    if (!settingsService.getExportDirectory(&strRestoredExportDirectory))
    {
        PrintError(settingsService.lastError());
        return 45;
    }

    bool bRestoredDefaultCopyImportedImage = true;
    if (!settingsService.getDefaultCopyImportedImageToCaptureDirectory(&bRestoredDefaultCopyImportedImage))
    {
        PrintError(settingsService.lastError());
        return 46;
    }

    if (QDir::cleanPath(strRestoredScreenshotDirectory) != QDir::cleanPath(settingsService.defaultScreenshotSaveDirectory())
        || QDir::cleanPath(strRestoredExportDirectory) != QDir::cleanPath(settingsService.defaultExportDirectory())
        || bRestoredDefaultCopyImportedImage != settingsService.defaultCopyImportedImageToCaptureDirectory())
    {
        PrintError(QString::fromUtf8("Settings restore defaults verification failed."));
        return 47;
    }

    if (!settingsService.setScreenshotSaveDirectory(strSmokeScreenshotDirectory))
    {
        PrintError(settingsService.lastError());
        return 48;
    }

    if (!settingsService.setExportDirectory(strSmokeExportDirectory))
    {
        PrintError(settingsService.lastError());
        return 49;
    }

    if (!settingsService.setDefaultCopyImportedImageToCaptureDirectory(true))
    {
        PrintError(settingsService.lastError());
        return 50;
    }

    QCCreateImageSnippetDialog importImageDialog(strLoadedScreenshotDirectory,
                                                 bLoadedDefaultCopyImportedImage,
                                                 strLoadedScreenshotDirectory,
                                                 &screenCaptureService);
    if (!importImageDialog.shouldCopyImportedImageToDefaultCaptureDirectory())
    {
        PrintError(QString::fromUtf8("Import Image default copy setting verification failed."));
        return 40;
    }
    QCStudySession session;
    session.setTitle(QString::fromUtf8("QtClip Smoke Session"));
    session.setCourseName(QString::fromUtf8("Software Architecture"));
    session.setDescription(QString::fromUtf8("Minimal AI workflow verification."));
    if (!sessionService.createSession(&session))
    {
        PrintError(sessionService.lastError());
        return 4;
    }

    QCSnippet textSnippet;
    textSnippet.setSessionId(session.id());
    textSnippet.setCaptureType(QCCaptureType::ManualCaptureType);
    textSnippet.setTitle(QString::fromUtf8("Key concept"));
    textSnippet.setContentText(QString::fromUtf8("Repositories isolate SQLite details from the domain layer."));
    textSnippet.setNote(QString::fromUtf8("Use thin services before wiring UI."));
    textSnippet.setNoteKind(QCNoteKind::ConceptNoteKind);
    textSnippet.setNoteLevel(QCNoteLevel::ImportantNoteLevel);
    textSnippet.setSource(QString::fromUtf8("lecture"));
    textSnippet.setLanguage(QString::fromUtf8("en"));
    if (!snippetService.createTextSnippet(&textSnippet))
    {
        PrintError(snippetService.lastError());
        return 5;
    }

    if (!snippetService.setFavorite(textSnippet.id(), true))
    {
        PrintError(snippetService.lastError());
        return 6;
    }

    if (!snippetService.setFavorite(textSnippet.id(), false))
    {
        PrintError(snippetService.lastError());
        return 7;
    }

    if (!snippetService.setFavorite(textSnippet.id(), true))
    {
        PrintError(snippetService.lastError());
        return 8;
    }

    QCTag tag;
    tag.setName(QString::fromUtf8("architecture"));
    if (!tagService.createTag(&tag))
    {
        PrintError(tagService.lastError());
        return 6;
    }

    QVector<qint64> vecTagIds;
    vecTagIds.append(tag.id());
    if (!tagService.replaceSnippetTags(textSnippet.id(), vecTagIds))
    {
        PrintError(tagService.lastError());
        return 7;
    }

    const QVector<QCTag> vecSnippetTags = tagService.listTagsBySnippet(textSnippet.id());
    if (!tagService.lastError().isEmpty() || vecSnippetTags.isEmpty())
    {
        PrintError(tagService.lastError().isEmpty() ? QString::fromUtf8("Tag binding verification failed.") : tagService.lastError());
        return 8;
    }

    QScreen *pPrimaryScreen = QGuiApplication::primaryScreen();
    if (nullptr == pPrimaryScreen)
    {
        PrintError(QString::fromUtf8("Primary screen is unavailable."));
        return 9;
    }

    const QRect rectScreenGeometry = pPrimaryScreen->geometry();
    const int nCaptureWidth = qMax(240, rectScreenGeometry.width() / 3);
    const int nCaptureHeight = qMax(180, rectScreenGeometry.height() / 3);
    const QRect rectCaptureRegion(rectScreenGeometry.left() + 80,
                                  rectScreenGeometry.top() + 80,
                                  nCaptureWidth,
                                  nCaptureHeight);

    QCScreenCaptureResult captureResult;
    if (!screenCaptureService.capturePrimaryScreenRegion(rectCaptureRegion, &captureResult))
    {
        PrintError(screenCaptureService.lastError());
        return 10;
    }

    const QRect rectReverseCaptureRegion(rectCaptureRegion.bottomRight(),
                                         rectCaptureRegion.topLeft());
    QCScreenCaptureResult reverseCaptureResult;
    if (!screenCaptureService.capturePrimaryScreenRegion(rectReverseCaptureRegion, &reverseCaptureResult))
    {
        PrintError(screenCaptureService.lastError());
        return 51;
    }

    if (qAbs(reverseCaptureResult.m_nWidth - captureResult.m_nWidth) > 1
        || qAbs(reverseCaptureResult.m_nHeight - captureResult.m_nHeight) > 1)
    {
        PrintError(QString::fromUtf8("Reverse region capture normalization verification failed."));
        return 52;
    }

    QCScreenCaptureResult tinyCaptureResult;
    if (screenCaptureService.capturePrimaryScreenRegion(QRect(rectScreenGeometry.topLeft(), QSize(4, 4)), &tinyCaptureResult))
    {
        PrintError(QString::fromUtf8("Tiny region capture should have failed."));
        return 53;
    }

    if (!screenCaptureService.lastError().contains(QString::fromUtf8("too small"), Qt::CaseInsensitive))
    {
        PrintError(QString::fromUtf8("Tiny region capture error verification failed."));
        return 54;
    }

    if (!QDir::cleanPath(captureResult.m_strFilePath).startsWith(QDir::cleanPath(strSmokeScreenshotDirectory), Qt::CaseInsensitive))
    {
        PrintError(QString::fromUtf8("Screenshot save directory verification failed."));
        return 36;
    }
    QCSnippet imageSnippet;
    imageSnippet.setSessionId(session.id());
    imageSnippet.setType(QCSnippetType::ImageSnippetType);
    imageSnippet.setCaptureType(QCCaptureType::ScreenCaptureType);
    imageSnippet.setTitle(QString::fromUtf8("Smoke Region Capture"));
    imageSnippet.setNote(QString::fromUtf8("Primary screen region captured by QCScreenCaptureService."));
    imageSnippet.setNoteKind(QCNoteKind::ReviewNoteKind);
    imageSnippet.setNoteLevel(QCNoteLevel::ReviewNoteLevel);
    imageSnippet.setSource(QString::fromUtf8("region"));
    imageSnippet.setCapturedAt(captureResult.m_dateTimeCreatedAt);
    imageSnippet.setCreatedAt(captureResult.m_dateTimeCreatedAt);
    imageSnippet.setUpdatedAt(captureResult.m_dateTimeCreatedAt);

    QCAttachment primaryAttachment;
    primaryAttachment.setFilePath(captureResult.m_strFilePath);
    primaryAttachment.setFileName(QFileInfo(captureResult.m_strFilePath).fileName());
    primaryAttachment.setMimeType(QString::fromUtf8("image/png"));
    primaryAttachment.setCreatedAt(captureResult.m_dateTimeCreatedAt);
    if (!snippetService.createImageSnippetWithPrimaryAttachment(&imageSnippet, &primaryAttachment))
    {
        PrintError(snippetService.lastError());
        return 11;
    }

    QCAiTaskExecutionContext snippetExecutionContext;
    if (!aiProcessService.prepareSnippetSummary(textSnippet.id(), &snippetExecutionContext))
    {
        PrintError(aiProcessService.lastError());
        return 12;
    }

    QCSnippet importedOriginalSnippet;
    importedOriginalSnippet.setSessionId(session.id());
    importedOriginalSnippet.setType(QCSnippetType::ImageSnippetType);
    importedOriginalSnippet.setCaptureType(QCCaptureType::ImportCaptureType);
    importedOriginalSnippet.setTitle(QString::fromUtf8("Imported Original Image"));
    importedOriginalSnippet.setNote(QString::fromUtf8("Import image without copying keeps the original file path."));
    importedOriginalSnippet.setNoteKind(QCNoteKind::ReviewNoteKind);
    importedOriginalSnippet.setNoteLevel(QCNoteLevel::ImportantNoteLevel);
    importedOriginalSnippet.setSource(QString::fromUtf8("file"));

    QCAttachment importedOriginalAttachment;
    importedOriginalAttachment.setFilePath(captureResult.m_strFilePath);
    importedOriginalAttachment.setFileName(QFileInfo(captureResult.m_strFilePath).fileName());
    importedOriginalAttachment.setMimeType(QString::fromUtf8("image/png"));
    importedOriginalAttachment.setCreatedAt(QDateTime::currentDateTimeUtc());
    if (!snippetService.createImageSnippetWithPrimaryAttachment(&importedOriginalSnippet, &importedOriginalAttachment))
    {
        PrintError(snippetService.lastError());
        return 111;
    }
    QString strPreviewCopiedImportImagePath;
    if (!screenCaptureService.previewImportedImageCopyPath(captureResult.m_strFilePath, &strPreviewCopiedImportImagePath))
    {
        PrintError(screenCaptureService.lastError());
        return 112;
    }

    QString strCopiedImportImagePath;
    if (!screenCaptureService.copyImportedImageToCaptureDirectory(captureResult.m_strFilePath, &strCopiedImportImagePath))
    {
        PrintError(screenCaptureService.lastError());
        return 112;
    }

    if (!QFileInfo::exists(strCopiedImportImagePath)
        || !QDir::cleanPath(strCopiedImportImagePath).startsWith(QDir::cleanPath(strSmokeScreenshotDirectory), Qt::CaseInsensitive)
        || QDir::cleanPath(strCopiedImportImagePath).compare(QDir::cleanPath(captureResult.m_strFilePath), Qt::CaseInsensitive) == 0)
    {
        PrintError(QString::fromUtf8("Copied import image verification failed."));
        return 113;
    }

    if (QDir::cleanPath(strPreviewCopiedImportImagePath).compare(QDir::cleanPath(strCopiedImportImagePath), Qt::CaseInsensitive) != 0)
    {
        PrintError(QString::fromUtf8("Copied import image preview verification failed."));
        return 115;
    }

    QCSnippet importedCopiedSnippet;
    importedCopiedSnippet.setSessionId(session.id());
    importedCopiedSnippet.setType(QCSnippetType::ImageSnippetType);
    importedCopiedSnippet.setCaptureType(QCCaptureType::ImportCaptureType);
    importedCopiedSnippet.setTitle(QString::fromUtf8("Imported Copied Image"));
    importedCopiedSnippet.setNote(QString::fromUtf8("Import image with copying stores a new file under the default capture directory."));
    importedCopiedSnippet.setNoteKind(QCNoteKind::ReviewNoteKind);
    importedCopiedSnippet.setNoteLevel(QCNoteLevel::ImportantNoteLevel);
    importedCopiedSnippet.setSource(QString::fromUtf8("file"));

    QCAttachment importedCopiedAttachment;
    importedCopiedAttachment.setFilePath(strCopiedImportImagePath);
    importedCopiedAttachment.setFileName(QFileInfo(strCopiedImportImagePath).fileName());
    importedCopiedAttachment.setMimeType(QString::fromUtf8("image/png"));
    importedCopiedAttachment.setCreatedAt(QDateTime::currentDateTimeUtc());
    if (!snippetService.createImageSnippetWithPrimaryAttachment(&importedCopiedSnippet, &importedCopiedAttachment))
    {
        PrintError(snippetService.lastError());
        return 114;
    }

    QFuture<QCAiTaskExecutionResult> snippetFuture = QtConcurrent::run(RunAiTaskInBackground,
                                                                       &aiProcessService,
                                                                       snippetExecutionContext);
    snippetFuture.waitForFinished();
    if (!aiProcessService.applyTaskResult(snippetFuture.result()))
    {
        PrintError(aiProcessService.lastError());
        return 13;
    }

    QCAiTaskExecutionContext sessionExecutionContext;
    if (!aiProcessService.prepareSessionSummary(session.id(), &sessionExecutionContext))
    {
        PrintError(aiProcessService.lastError());
        return 14;
    }

    QFuture<QCAiTaskExecutionResult> sessionFuture = QtConcurrent::run(RunAiTaskInBackground,
                                                                       &aiProcessService,
                                                                       sessionExecutionContext);
    sessionFuture.waitForFinished();
    if (!aiProcessService.applyTaskResult(sessionFuture.result()))
    {
        PrintError(aiProcessService.lastError());
        return 15;
    }

    if (!snippetService.setReviewState(textSnippet.id(), true))
    {
        PrintError(snippetService.lastError());
        return 17;
    }

    const QVector<QCSnippet> vecReviewKeywordResults = snippetService.querySnippets(session.id(),
                                                                                     QString::fromUtf8("Repositories"),
                                                                                     false,
                                                                                     true,
                                                                                     0);
    if (!snippetService.lastError().isEmpty() || vecReviewKeywordResults.isEmpty())
    {
        PrintError(snippetService.lastError().isEmpty() ? QString::fromUtf8("Review state enable verification failed.") : snippetService.lastError());
        return 18;
    }

    if (!snippetService.setReviewState(textSnippet.id(), false))
    {
        PrintError(snippetService.lastError());
        return 19;
    }

    const QVector<QCSnippet> vecReviewKeywordClearedResults = snippetService.querySnippets(session.id(),
                                                                                            QString::fromUtf8("Repositories"),
                                                                                            false,
                                                                                            true,
                                                                                            tag.id());
    if (!snippetService.lastError().isEmpty() || !vecReviewKeywordClearedResults.isEmpty())
    {
        PrintError(snippetService.lastError().isEmpty() ? QString::fromUtf8("Review state clear verification failed.") : snippetService.lastError());
        return 20;
    }

    const QVector<QCSnippet> vecKeywordResults = snippetService.querySnippets(session.id(),
                                                                              QString::fromUtf8("Repositories"),
                                                                              false,
                                                                              false,
                                                                              0);
    if (!snippetService.lastError().isEmpty() || vecKeywordResults.isEmpty())
    {
        PrintError(snippetService.lastError().isEmpty() ? QString::fromUtf8("Keyword query verification failed.") : snippetService.lastError());
        return 17;
    }

    const QVector<QCSnippet> vecFavoriteResults = snippetService.querySnippets(session.id(), QString(), true, false, 0);
    if (!snippetService.lastError().isEmpty() || vecFavoriteResults.isEmpty())
    {
        PrintError(snippetService.lastError().isEmpty() ? QString::fromUtf8("Favorite filter verification failed.") : snippetService.lastError());
        return 18;
    }

    const QVector<QCSnippet> vecReviewResults = snippetService.querySnippets(session.id(), QString(), false, true, 0);
    if (!snippetService.lastError().isEmpty() || vecReviewResults.isEmpty())
    {
        PrintError(snippetService.lastError().isEmpty() ? QString::fromUtf8("Review filter verification failed.") : snippetService.lastError());
        return 19;
    }

    const QVector<QCSnippet> vecTagResults = snippetService.querySnippets(session.id(), QString(), false, false, tag.id());
    if (!snippetService.lastError().isEmpty() || vecTagResults.isEmpty())
    {
        PrintError(snippetService.lastError().isEmpty() ? QString::fromUtf8("Tag filter verification failed.") : snippetService.lastError());
        return 20;
    }

    const QVector<QCSnippet> vecCombinedResults = snippetService.querySnippets(session.id(),
                                                                               QString::fromUtf8("Repositories"),
                                                                               true,
                                                                               false,
                                                                               tag.id());
    if (!snippetService.lastError().isEmpty() || vecCombinedResults.isEmpty())
    {
        PrintError(snippetService.lastError().isEmpty() ? QString::fromUtf8("Combined filter verification failed.") : snippetService.lastError());
        return 21;
    }

    QCSnippet summarizedSnippet;
    if (!snippetService.getSnippetById(textSnippet.id(), &summarizedSnippet))
    {
        PrintError(snippetService.lastError());
        return 16;
    }

    const QVector<QCAiRecord> vecSessionAiRecords = aiService.listAiRecordsBySession(session.id());
    if (!aiService.lastError().isEmpty())
    {
        PrintError(aiService.lastError());
        return 17;
    }

    if (!mdExportService.exportSessionToFile(session.id(), strMarkdownPath))
    {
        PrintError(mdExportService.lastError());
        return 18;
    }


    if (!QDir::cleanPath(strMarkdownPath).startsWith(QDir::cleanPath(strSmokeExportDirectory), Qt::CaseInsensitive))
    {
        PrintError(QString::fromUtf8("Export directory verification failed."));
        return 37;
    }
    QFile markdownFile(strMarkdownPath);
    if (!markdownFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        PrintError(QString::fromUtf8("Failed to open Markdown export file."));
        return 19;
    }

    const QByteArray byteArrayMarkdown = markdownFile.readAll();
    markdownFile.close();
    if (byteArrayMarkdown.isEmpty())
    {
        PrintError(QString::fromUtf8("Markdown export file is empty."));
        return 20;
    }

    PrintInfo(QString::fromUtf8("Session created: %1").arg(session.id()));
    PrintInfo(QString::fromUtf8("Tag bound to snippet: %1").arg(vecSnippetTags.at(0).name()));
    PrintInfo(QString::fromUtf8("Keyword search count: %1").arg(vecKeywordResults.size()));
    PrintInfo(QString::fromUtf8("Review-on keyword count: %1").arg(vecReviewKeywordResults.size()));
    PrintInfo(QString::fromUtf8("Review-cleared keyword+tag count: %1").arg(vecReviewKeywordClearedResults.size()));
    PrintInfo(QString::fromUtf8("Favorite filter count: %1").arg(vecFavoriteResults.size()));
    PrintInfo(QString::fromUtf8("Review filter count: %1").arg(vecReviewResults.size()));
    PrintInfo(QString::fromUtf8("Tag filter count: %1").arg(vecTagResults.size()));
    PrintInfo(QString::fromUtf8("Combined filter count: %1").arg(vecCombinedResults.size()));
    PrintInfo(QString::fromUtf8("Screenshot directory: %1").arg(strLoadedScreenshotDirectory));
    PrintInfo(QString::fromUtf8("Export directory: %1").arg(strLoadedExportDirectory));
    PrintInfo(QString::fromUtf8("Import default copy enabled: %1").arg(bLoadedDefaultCopyImportedImage ? QString::fromUtf8("true") : QString::fromUtf8("false")));
    PrintInfo(QString::fromUtf8("Region capture saved to: %1").arg(captureResult.m_strFilePath));
    PrintInfo(QString::fromUtf8("Import original path: %1").arg(importedOriginalAttachment.filePath()));
    PrintInfo(QString::fromUtf8("Import copied preview path: %1").arg(strPreviewCopiedImportImagePath));
    PrintInfo(QString::fromUtf8("Import copied path: %1").arg(strCopiedImportImagePath));
    PrintInfo(QString::fromUtf8("Snippet summary: %1").arg(summarizedSnippet.summary()));
    if (!vecSessionAiRecords.isEmpty())
        PrintInfo(QString::fromUtf8("Session summary record: %1").arg(vecSessionAiRecords.at(0).responseText()));
    PrintInfo(QString::fromUtf8("Markdown exported to: %1").arg(strMarkdownPath));
    PrintInfo(QString::fromUtf8("Markdown size: %1 bytes").arg(byteArrayMarkdown.size()));
    return 0;
}
}

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    const QStringList vecArguments = application.arguments();

    if (vecArguments.contains(QString::fromUtf8("--smoke")))
        return RunSmokeWorkflow();

    const QString strDatabasePath = ResolveAppDataPath();
    QCDatabaseManager databaseManager;
    if (!databaseManager.open(strDatabasePath))
    {
        QMessageBox::critical(nullptr, QString::fromUtf8("QtClip"), databaseManager.lastError());
        return 1;
    }

    if (!databaseManager.initialize())
    {
        QMessageBox::critical(nullptr, QString::fromUtf8("QtClip"), databaseManager.lastError());
        return 2;
    }

    QCSessionRepositorySqlite sessionRepository(&databaseManager);
    QCSnippetRepositorySqlite snippetRepository(&databaseManager);
    QCAiRecordRepositorySqlite aiRecordRepository(&databaseManager);
    QCSettingsRepositorySqlite settingsRepository(&databaseManager);
    QCTagRepositorySqlite tagRepository(&databaseManager);
    QCSessionService sessionService(&sessionRepository);
    QCSnippetService snippetService(&snippetRepository);
    QCAiService aiService(&aiRecordRepository);
    QCSettingsService settingsService(&settingsRepository);
    QCTagService tagService(&tagRepository);
    QCAiProcessService aiProcessService(&sessionService, &snippetService, &aiService, &settingsService);
    QCExportDataService exportDataService(&sessionService, &snippetService, &aiService);
    QCMdExportRenderer mdExportRenderer;
    QCMdExportService mdExportService(&exportDataService, &mdExportRenderer);
    QCScreenCaptureService screenCaptureService(&settingsService);

    if (vecArguments.contains(QString::fromUtf8("--test-ai-config")))
    {
        QCAiRuntimeSettings aiSettings;
        if (!settingsService.loadAiSettings(&aiSettings))
        {
            PrintError(settingsService.lastError());
            return 11;
        }

        QCAiConnectionTestResult connectionTestResult;
        if (!aiProcessService.testConnection(aiSettings, &connectionTestResult) || !connectionTestResult.m_bSuccess)
        {
            PrintError(connectionTestResult.m_strMessage.trimmed().isEmpty()
                ? QString::fromUtf8("AI connection test failed.")
                : connectionTestResult.m_strMessage);
            return 12;
        }

        PrintInfo(connectionTestResult.m_strMessage);
        return 0;
    }

    const int nSummarizeSnippetFlagIndex = vecArguments.indexOf(QString::fromUtf8("--summarize-snippet"));
    if (nSummarizeSnippetFlagIndex >= 0)
    {
        if (nSummarizeSnippetFlagIndex + 1 >= vecArguments.size())
        {
            PrintError(QString::fromUtf8("Missing snippet id after --summarize-snippet."));
            return 13;
        }

        bool bOk = false;
        const qint64 nSnippetId = vecArguments.at(nSummarizeSnippetFlagIndex + 1).toLongLong(&bOk);
        if (!bOk || nSnippetId <= 0)
        {
            PrintError(QString::fromUtf8("Snippet id is invalid."));
            return 14;
        }

        if (!aiProcessService.summarizeSnippet(nSnippetId))
        {
            PrintError(aiProcessService.lastError().trimmed().isEmpty()
                ? QString::fromUtf8("Summarize Snippet failed.")
                : aiProcessService.lastError());
            return 15;
        }

        QCSnippet snippet;
        if (!snippetService.getSnippetById(nSnippetId, &snippet))
        {
            PrintError(snippetService.lastError());
            return 16;
        }

        PrintInfo(QString::fromUtf8("Snippet %1 summarized.").arg(nSnippetId));
        PrintInfo(QString::fromUtf8("Summary Preview: %1").arg(BuildPreviewText(snippet.summary())));
        return 0;
    }

    QCMainWindow mainWindow(&sessionService,
                            &snippetService,
                            &tagService,
                            &aiService,
                            &aiProcessService,
                            &settingsService,
                            &mdExportService,
                            &screenCaptureService);
    mainWindow.show();

    return application.exec();
}







