// File: main.cpp
// Author: ZZMI1
// Created: 2026-03-23
// Description: Provides the minimal Qt Widgets application entry used by the QtClip demo application.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QtConcurrent/QtConcurrent>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
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


QString CreateVerificationImageFile(const QString& strFilePath)
{
    QImage image(96, 64, QImage::Format_ARGB32);
    image.fill(qRgb(245, 248, 252));
    for (int y = 0; y < image.height(); ++y)
    {
        for (int x = 0; x < image.width(); ++x)
        {
            if ((x + y) % 9 == 0)
                image.setPixelColor(x, y, QColor(80, 130, 220));
        }
    }

    if (!image.save(strFilePath))
        return QString();

    return strFilePath;
}

bool ActionHasShortcut(const QAction *pAction, const QKeySequence& keySequence)
{
    if (nullptr == pAction)
        return false;

    if (pAction->shortcut() == keySequence)
        return true;

    return pAction->shortcuts().contains(keySequence);
}

bool SelectListItemById(QListWidget *pListWidget, qint64 nId)
{
    if (nullptr == pListWidget)
        return false;

    for (int i = 0; i < pListWidget->count(); ++i)
    {
        QListWidgetItem *pItem = pListWidget->item(i);
        if (nullptr == pItem)
            continue;

        if (pItem->data(Qt::UserRole).toLongLong() == nId)
        {
            pListWidget->setCurrentItem(pItem);
            QApplication::processEvents();
            return true;
        }
    }

    return false;
}

bool SelectListItemsByIds(QListWidget *pListWidget, const QVector<qint64>& vecIds)
{
    if (nullptr == pListWidget || vecIds.isEmpty())
        return false;

    bool bSelectedAny = false;
    QListWidgetItem *pCurrentItem = nullptr;
    for (int i = 0; i < pListWidget->count(); ++i)
    {
        QListWidgetItem *pItem = pListWidget->item(i);
        if (nullptr == pItem)
            continue;

        const bool bShouldSelect = vecIds.contains(pItem->data(Qt::UserRole).toLongLong());
        pItem->setSelected(bShouldSelect);
        if (bShouldSelect && nullptr == pCurrentItem)
            pCurrentItem = pItem;
        bSelectedAny = bSelectedAny || bShouldSelect;
    }

    if (1 == vecIds.size() && nullptr != pCurrentItem)
        pListWidget->setCurrentItem(pCurrentItem);
    QApplication::processEvents();
    return bSelectedAny;
}

void ClearCurrentSelection(QListWidget *pListWidget)
{
    if (nullptr == pListWidget)
        return;

    pListWidget->clearSelection();
    pListWidget->setCurrentItem(nullptr);
    QApplication::processEvents();
}

int RunManagementVerificationWorkflow()
{
    QString strTempDirectory = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (strTempDirectory.isEmpty())
        strTempDirectory = QDir::tempPath();

    const QString strDatabasePath = QDir(strTempDirectory).filePath(QString::fromUtf8("qtclip_management_verify.sqlite"));
    const QString strImagePath = QDir(strTempDirectory).filePath(QString::fromUtf8("qtclip_management_verify.png"));
    QFile::remove(strDatabasePath);
    QFile::remove(strImagePath);

    QCDatabaseManager databaseManager;
    if (!databaseManager.open(strDatabasePath) || !databaseManager.initialize())
    {
        PrintError(databaseManager.lastError());
        return 501;
    }

    if (CreateVerificationImageFile(strImagePath).isEmpty())
    {
        PrintError(QString::fromUtf8("Failed to create verification image file."));
        return 502;
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
    Q_UNUSED(tagService);
    Q_UNUSED(aiProcessService);

    QCAiRuntimeSettings aiSettings;
    aiSettings.m_bUseMockProvider = true;
    aiSettings.m_strModel = QString::fromUtf8("mock-summary-v1");
    if (!settingsService.saveAiSettings(aiSettings))
    {
        PrintError(settingsService.lastError());
        return 503;
    }

    QCStudySession session;
    session.setTitle(QString::fromUtf8("Management Verify Session"));
    session.setCourseName(QString::fromUtf8("Architecture"));
    session.setDescription(QString::fromUtf8("Created for management verification."));
    if (!sessionService.createSession(&session))
    {
        PrintError(sessionService.lastError());
        return 504;
    }

    QCStudySession activeSession;
    if (!sessionService.getActiveSession(&activeSession) || activeSession.id() != session.id())
    {
        PrintError(sessionService.lastError().isEmpty() ? QString::fromUtf8("Active session verification failed.") : sessionService.lastError());
        return 505;
    }

    session.setTitle(QString::fromUtf8("Edited Management Session"));
    session.setCourseName(QString::fromUtf8("Advanced Architecture"));
    session.setDescription(QString::fromUtf8("Updated session description."));
    if (!sessionService.updateSession(&session))
    {
        PrintError(sessionService.lastError());
        return 506;
    }

    QCStudySession updatedSession;
    if (!sessionService.getSessionById(session.id(), &updatedSession)
        || updatedSession.title() != QString::fromUtf8("Edited Management Session")
        || updatedSession.courseName() != QString::fromUtf8("Advanced Architecture")
        || updatedSession.description() != QString::fromUtf8("Updated session description."))
    {
        PrintError(sessionService.lastError().isEmpty() ? QString::fromUtf8("Session update verification failed.") : sessionService.lastError());
        return 507;
    }

    QCSnippet textSnippet;
    textSnippet.setSessionId(session.id());
    textSnippet.setCaptureType(QCCaptureType::ManualCaptureType);
    textSnippet.setTitle(QString::fromUtf8("Original Text"));
    textSnippet.setNote(QString::fromUtf8("Original text note."));
    textSnippet.setContentText(QString::fromUtf8("Repositories stay searchable after update."));
    textSnippet.setNoteKind(QCNoteKind::ConceptNoteKind);
    textSnippet.setNoteLevel(QCNoteLevel::ImportantNoteLevel);
    textSnippet.setSource(QString::fromUtf8("verify"));
    textSnippet.setLanguage(QString::fromUtf8("en"));
    if (!snippetService.createTextSnippet(&textSnippet))
    {
        PrintError(snippetService.lastError());
        return 508;
    }

    textSnippet.setTitle(QString::fromUtf8("Edited Text"));
    textSnippet.setNote(QString::fromUtf8("Edited text note."));
    textSnippet.setContentText(QString::fromUtf8("Repositories stay searchable after edited text update."));
    if (!snippetService.updateSnippet(&textSnippet))
    {
        PrintError(snippetService.lastError());
        return 509;
    }

    QCSnippet updatedTextSnippet;
    if (!snippetService.getSnippetById(textSnippet.id(), &updatedTextSnippet)
        || updatedTextSnippet.title() != QString::fromUtf8("Edited Text")
        || updatedTextSnippet.note() != QString::fromUtf8("Edited text note."))
    {
        PrintError(snippetService.lastError().isEmpty() ? QString::fromUtf8("Text snippet update verification failed.") : snippetService.lastError());
        return 510;
    }

    QCSnippet imageSnippet;
    imageSnippet.setSessionId(session.id());
    imageSnippet.setType(QCSnippetType::ImageSnippetType);
    imageSnippet.setCaptureType(QCCaptureType::ImportCaptureType);
    imageSnippet.setTitle(QString::fromUtf8("Original Image"));
    imageSnippet.setNote(QString::fromUtf8("Original image note."));
    imageSnippet.setNoteKind(QCNoteKind::ReviewNoteKind);
    imageSnippet.setNoteLevel(QCNoteLevel::ImportantNoteLevel);
    imageSnippet.setSource(QString::fromUtf8("file"));

    QCAttachment primaryAttachment;
    primaryAttachment.setFilePath(strImagePath);
    primaryAttachment.setFileName(QFileInfo(strImagePath).fileName());
    primaryAttachment.setMimeType(QString::fromUtf8("image/png"));
    primaryAttachment.setCreatedAt(QDateTime::currentDateTimeUtc());
    if (!snippetService.createImageSnippetWithPrimaryAttachment(&imageSnippet, &primaryAttachment))
    {
        PrintError(snippetService.lastError());
        return 511;
    }

    imageSnippet.setTitle(QString::fromUtf8("Edited Image"));
    imageSnippet.setNote(QString::fromUtf8("Edited image note."));
    if (!snippetService.updateSnippet(&imageSnippet))
    {
        PrintError(snippetService.lastError());
        return 512;
    }

    QCSnippet updatedImageSnippet;
    QCAttachment loadedPrimaryAttachment;
    if (!snippetService.getSnippetById(imageSnippet.id(), &updatedImageSnippet)
        || !snippetService.getPrimaryAttachmentBySnippetId(imageSnippet.id(), &loadedPrimaryAttachment)
        || updatedImageSnippet.title() != QString::fromUtf8("Edited Image")
        || updatedImageSnippet.note() != QString::fromUtf8("Edited image note.")
        || QFileInfo(loadedPrimaryAttachment.filePath()).fileName() != QFileInfo(strImagePath).fileName())
    {
        PrintError(snippetService.lastError().isEmpty() ? QString::fromUtf8("Image snippet update verification failed.") : snippetService.lastError());
        return 513;
    }

    QCTag managementTag;
    managementTag.setName(QString::fromUtf8("architecture"));
    if (!tagService.createTag(&managementTag))
    {
        PrintError(tagService.lastError());
        return 5131;
    }

    QVector<qint64> vecManagementTextTagIds;
    vecManagementTextTagIds.append(managementTag.id());
    if (!tagService.replaceSnippetTags(textSnippet.id(), vecManagementTextTagIds))
    {
        PrintError(tagService.lastError());
        return 5132;
    }

    if (tagService.countSnippetsByTag(managementTag.id()) != 1)
    {
        PrintError(QString::fromUtf8("Tag usage count verification failed after first binding."));
        return 5133;
    }

    QCTag imageTag;
    imageTag.setName(QString::fromUtf8("image-reference"));
    if (!tagService.createTag(&imageTag))
    {
        PrintError(tagService.lastError());
        return 5134;
    }

    QVector<qint64> vecImageTagIds;
    vecImageTagIds.append(managementTag.id());
    vecImageTagIds.append(imageTag.id());
    if (!tagService.replaceSnippetTags(imageSnippet.id(), vecImageTagIds))
    {
        PrintError(tagService.lastError());
        return 5135;
    }

    if (tagService.countSnippetsByTag(managementTag.id()) != 2 || tagService.countSnippetsByTag(imageTag.id()) != 1)
    {
        PrintError(QString::fromUtf8("Tag usage count verification failed after multi-snippet binding."));
        return 5136;
    }

    QVector<qint64> vecImageOnlyTagIds;
    vecImageOnlyTagIds.append(imageTag.id());
    if (!tagService.replaceSnippetTags(imageSnippet.id(), vecImageOnlyTagIds))
    {
        PrintError(tagService.lastError());
        return 5137;
    }

    if (tagService.countSnippetsByTag(managementTag.id()) != 1 || tagService.countSnippetsByTag(imageTag.id()) != 1)
    {
        PrintError(QString::fromUtf8("Tag usage count verification failed after tag removal."));
        return 5138;
    }

    if (!snippetService.setArchived(textSnippet.id(), true))
    {
        PrintError(snippetService.lastError());
        return 514;
    }

    if (!snippetService.getSnippetById(textSnippet.id(), &updatedTextSnippet) || !updatedTextSnippet.isArchived())
    {
        PrintError(snippetService.lastError().isEmpty() ? QString::fromUtf8("Archive verification failed.") : snippetService.lastError());
        return 515;
    }

    if (!snippetService.setArchived(textSnippet.id(), false))
    {
        PrintError(snippetService.lastError());
        return 516;
    }

        if (!snippetService.getSnippetById(textSnippet.id(), &updatedTextSnippet) || updatedTextSnippet.isArchived())
    {
        PrintError(snippetService.lastError().isEmpty() ? QString::fromUtf8("Restore verification failed.") : snippetService.lastError());
        return 517;
    }

    qint64 nDuplicatedSnippetId = 0;
    if (!snippetService.duplicateSnippet(textSnippet.id(), session.id(), &nDuplicatedSnippetId) || nDuplicatedSnippetId <= 0)
    {
        PrintError(snippetService.lastError().isEmpty() ? QString::fromUtf8("Duplicate snippet verification failed.") : snippetService.lastError());
        return 518;
    }

    QCSnippet duplicatedSnippet;
    if (!snippetService.getSnippetById(nDuplicatedSnippetId, &duplicatedSnippet)
        || duplicatedSnippet.sessionId() != session.id()
        || !duplicatedSnippet.title().contains(QString::fromUtf8("Copy")))
    {
        PrintError(snippetService.lastError().isEmpty() ? QString::fromUtf8("Duplicated snippet readback verification failed.") : snippetService.lastError());
        return 519;
    }

    QCStudySession moveTargetSession;
    moveTargetSession.setTitle(QString::fromUtf8("Move Target Session"));
    if (!sessionService.createSession(&moveTargetSession))
    {
        PrintError(sessionService.lastError());
        return 520;
    }

    QVector<qint64> vecSnippetIdsToMove;
    vecSnippetIdsToMove.append(textSnippet.id());
    vecSnippetIdsToMove.append(nDuplicatedSnippetId);
    if (!snippetService.moveSnippetsToSession(vecSnippetIdsToMove, moveTargetSession.id()))
    {
        PrintError(snippetService.lastError());
        return 521;
    }

    textSnippet.setSessionId(moveTargetSession.id());
    if (!snippetService.updateSnippet(&textSnippet))
    {
        PrintError(snippetService.lastError());
        return 522;
    }

    const QVector<QCSnippet> vecMovedSourceSnippets = snippetService.listSnippetsBySession(session.id());
    if (!snippetService.lastError().isEmpty())
    {
        PrintError(snippetService.lastError());
        return 523;
    }

    const QVector<QCSnippet> vecMovedTargetSnippets = snippetService.listSnippetsBySession(moveTargetSession.id());
    if (!snippetService.lastError().isEmpty())
    {
        PrintError(snippetService.lastError());
        return 524;
    }

    bool bMovedTextSnippetInTarget = false;
    bool bMovedDuplicateSnippetInTarget = false;
    for (int i = 0; i < vecMovedTargetSnippets.size(); ++i)
    {
        if (vecMovedTargetSnippets.at(i).id() == textSnippet.id())
            bMovedTextSnippetInTarget = true;
        if (vecMovedTargetSnippets.at(i).id() == nDuplicatedSnippetId)
            bMovedDuplicateSnippetInTarget = true;
    }

    if (vecMovedSourceSnippets.size() != 1 || !bMovedTextSnippetInTarget || !bMovedDuplicateSnippetInTarget)
    {
        PrintError(QString::fromUtf8("Move snippet verification failed."));
        return 525;
    }

    const QDateTime dateTimeEndedAt = QDateTime::currentDateTimeUtc();
    if (!sessionService.finishSession(session.id(), dateTimeEndedAt))
    {
        PrintError(sessionService.lastError());
        return 534;
    }

    QCStudySession finishedSession;
    if (!sessionService.getSessionById(session.id(), &finishedSession)
        || finishedSession.status() != QCSessionStatus::FinishedSessionStatus
        || !finishedSession.endedAt().isValid())
    {
        PrintError(sessionService.lastError().isEmpty() ? QString::fromUtf8("Finish session verification failed.") : sessionService.lastError());
        return 527;
    }

    finishedSession.setStatus(QCSessionStatus::ActiveSessionStatus);
    finishedSession.setEndedAt(QDateTime());
    if (!sessionService.updateSession(&finishedSession))
    {
        PrintError(sessionService.lastError());
        return 528;
    }

    QCStudySession reopenedSession;
    if (!sessionService.getSessionById(session.id(), &reopenedSession)
        || reopenedSession.status() != QCSessionStatus::ActiveSessionStatus
        || reopenedSession.endedAt().isValid())
    {
        PrintError(sessionService.lastError().isEmpty() ? QString::fromUtf8("Reopen session verification failed.") : sessionService.lastError());
        return 529;
    }

    if (!snippetService.deleteSnippet(imageSnippet.id()))
    {
        PrintError(snippetService.lastError());
        return 530;
    }

    if (snippetService.getSnippetById(imageSnippet.id(), &updatedImageSnippet))
    {
        PrintError(QString::fromUtf8("Deleted image snippet should not load."));
        return 531;
    }

    QCStudySession deleteSession;
    deleteSession.setTitle(QString::fromUtf8("Delete Session Verify"));
    if (!sessionService.createSession(&deleteSession))
    {
        PrintError(sessionService.lastError());
        return 532;
    }

    if (!sessionService.deleteSession(deleteSession.id()))
    {
        PrintError(sessionService.lastError());
        return 533;
    }

    QCStudySession deletedSession;
    if (sessionService.getSessionById(deleteSession.id(), &deletedSession))
    {
        PrintError(QString::fromUtf8("Deleted session should not load."));
        return 534;
    }

    PrintInfo(QString::fromUtf8("Management verification passed."));
    PrintInfo(QString::fromUtf8("Session Id: %1").arg(session.id()));
    PrintInfo(QString::fromUtf8("Text Snippet Id: %1").arg(textSnippet.id()));
    PrintInfo(QString::fromUtf8("Image Snippet Deleted: %1").arg(imageSnippet.id()));
    return 0;
}

int RunMainWindowStateVerificationWorkflow()
{
    QString strTempDirectory = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (strTempDirectory.isEmpty())
        strTempDirectory = QDir::tempPath();

    const QString strDatabasePath = QDir(strTempDirectory).filePath(QString::fromUtf8("qtclip_mainwindow_verify.sqlite"));
    const QString strImagePath = QDir(strTempDirectory).filePath(QString::fromUtf8("qtclip_mainwindow_verify.png"));
    QFile::remove(strDatabasePath);
    QFile::remove(strImagePath);

    QCDatabaseManager databaseManager;
    if (!databaseManager.open(strDatabasePath) || !databaseManager.initialize())
    {
        PrintError(databaseManager.lastError());
        return 601;
    }

    if (CreateVerificationImageFile(strImagePath).isEmpty())
    {
        PrintError(QString::fromUtf8("Failed to create main window verification image file."));
        return 602;
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
    Q_UNUSED(tagService);

    QCAiRuntimeSettings aiSettings;
    aiSettings.m_bUseMockProvider = true;
    aiSettings.m_strModel = QString::fromUtf8("mock-summary-v1");
    if (!settingsService.saveAiSettings(aiSettings))
    {
        PrintError(settingsService.lastError());
        return 603;
    }

    QCStudySession activeSession;
    activeSession.setTitle(QString::fromUtf8("Active Verify Session"));
    activeSession.setCourseName(QString::fromUtf8("Verification"));
    if (!sessionService.createSession(&activeSession))
    {
        PrintError(sessionService.lastError());
        return 604;
    }

    QCSnippet textSnippet;
    textSnippet.setSessionId(activeSession.id());
    textSnippet.setCaptureType(QCCaptureType::ManualCaptureType);
    textSnippet.setTitle(QString::fromUtf8("Visible Text Snippet"));
    textSnippet.setContentText(QString::fromUtf8("Visible snippet content."));
    textSnippet.setNote(QString::fromUtf8("Visible snippet note."));
    textSnippet.setNoteKind(QCNoteKind::ConceptNoteKind);
    textSnippet.setNoteLevel(QCNoteLevel::ImportantNoteLevel);
    if (!snippetService.createTextSnippet(&textSnippet))
    {
        PrintError(snippetService.lastError());
        return 605;
    }

    QCSnippet archivedSnippet;
    archivedSnippet.setSessionId(activeSession.id());
    archivedSnippet.setType(QCSnippetType::ImageSnippetType);
    archivedSnippet.setCaptureType(QCCaptureType::ImportCaptureType);
    archivedSnippet.setTitle(QString::fromUtf8("Archived Image Snippet"));
    archivedSnippet.setNote(QString::fromUtf8("Archived snippet note."));
    archivedSnippet.setNoteKind(QCNoteKind::ReviewNoteKind);
    archivedSnippet.setNoteLevel(QCNoteLevel::ImportantNoteLevel);
    QCAttachment attachment;
    attachment.setFilePath(strImagePath);
    attachment.setFileName(QFileInfo(strImagePath).fileName());
    attachment.setMimeType(QString::fromUtf8("image/png"));
    attachment.setCreatedAt(QDateTime::currentDateTimeUtc());
    if (!snippetService.createImageSnippetWithPrimaryAttachment(&archivedSnippet, &attachment))
    {
        PrintError(snippetService.lastError());
        return 606;
    }

    if (!snippetService.setArchived(archivedSnippet.id(), true))
    {
        PrintError(snippetService.lastError());
        return 607;
    }

    QCStudySession finishedSession;
    finishedSession.setTitle(QString::fromUtf8("Finished Verify Session"));
    finishedSession.setCourseName(QString::fromUtf8("Verification"));
    if (!sessionService.createSession(&finishedSession))
    {
        PrintError(sessionService.lastError());
        return 608;
    }

    if (!sessionService.finishSession(finishedSession.id(), QDateTime::currentDateTimeUtc()))
    {
        PrintError(sessionService.lastError());
        return 609;
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
    QApplication::processEvents();

    QListWidget *pSessionListWidget = mainWindow.findChild<QListWidget *>(QString::fromUtf8("sessionListWidget"));
    QListWidget *pSnippetListWidget = mainWindow.findChild<QListWidget *>(QString::fromUtf8("snippetListWidget"));
    QLineEdit *pSearchLineEdit = mainWindow.findChild<QLineEdit *>(QString::fromUtf8("snippetSearchLineEdit"));
    QComboBox *pSearchHistoryComboBox = mainWindow.findChild<QComboBox *>(QString::fromUtf8("searchHistoryComboBox"));
    QComboBox *pSnippetTypeFilterComboBox = mainWindow.findChild<QComboBox *>(QString::fromUtf8("snippetTypeFilterComboBox"));
    QLabel *pAiStatusLabel = mainWindow.findChild<QLabel *>(QString::fromUtf8("aiStatusLabel"));
    QCheckBox *pShowArchivedCheckBox = mainWindow.findChild<QCheckBox *>(QString::fromUtf8("showArchivedCheckBox"));
    if (nullptr == pSessionListWidget || nullptr == pSnippetListWidget || nullptr == pSearchLineEdit || nullptr == pSearchHistoryComboBox || nullptr == pSnippetTypeFilterComboBox || nullptr == pAiStatusLabel || nullptr == pShowArchivedCheckBox)
    {
        PrintError(QString::fromUtf8("Main window verification widgets are missing."));
        return 610;
    }

    QAction *pNewSessionAction = mainWindow.findChild<QAction *>(QString::fromUtf8("newSessionAction"));
    QAction *pNewTextSnippetAction = mainWindow.findChild<QAction *>(QString::fromUtf8("newTextSnippetAction"));
    QAction *pCaptureScreenAction = mainWindow.findChild<QAction *>(QString::fromUtf8("captureScreenAction"));
    QAction *pCaptureRegionAction = mainWindow.findChild<QAction *>(QString::fromUtf8("captureRegionAction"));
    QAction *pImportImageAction = mainWindow.findChild<QAction *>(QString::fromUtf8("importImageAction"));
    QAction *pDuplicateSnippetAction = mainWindow.findChild<QAction *>(QString::fromUtf8("duplicateSnippetAction"));
    QAction *pMoveSnippetAction = mainWindow.findChild<QAction *>(QString::fromUtf8("moveSnippetAction"));
    QAction *pEditSessionAction = mainWindow.findChild<QAction *>(QString::fromUtf8("editSessionAction"));
    QAction *pFinishSessionAction = mainWindow.findChild<QAction *>(QString::fromUtf8("finishSessionAction"));
    QAction *pReopenSessionAction = mainWindow.findChild<QAction *>(QString::fromUtf8("reopenSessionAction"));
    QAction *pDeleteSessionAction = mainWindow.findChild<QAction *>(QString::fromUtf8("deleteSessionAction"));
    QAction *pEditSnippetAction = mainWindow.findChild<QAction *>(QString::fromUtf8("editSnippetAction"));
    QAction *pManageTagsAction = mainWindow.findChild<QAction *>(QString::fromUtf8("manageTagsAction"));
    QAction *pTagLibraryAction = mainWindow.findChild<QAction *>(QString::fromUtf8("tagLibraryAction"));
    QAction *pDeleteSnippetAction = mainWindow.findChild<QAction *>(QString::fromUtf8("deleteSnippetAction"));
    QAction *pToggleArchiveSnippetAction = mainWindow.findChild<QAction *>(QString::fromUtf8("toggleArchiveSnippetAction"));
    QAction *pSummarizeSnippetAction = mainWindow.findChild<QAction *>(QString::fromUtf8("summarizeSnippetAction"));
    QAction *pRetrySnippetSummaryAction = mainWindow.findChild<QAction *>(QString::fromUtf8("retrySnippetSummaryAction"));
    QAction *pSummarizeSessionAction = mainWindow.findChild<QAction *>(QString::fromUtf8("summarizeSessionAction"));
    QAction *pRetrySessionSummaryAction = mainWindow.findChild<QAction *>(QString::fromUtf8("retrySessionSummaryAction"));
    QAction *pAiSettingsAction = mainWindow.findChild<QAction *>(QString::fromUtf8("aiSettingsAction"));
    QAction *pExportMarkdownAction = mainWindow.findChild<QAction *>(QString::fromUtf8("exportMarkdownAction"));
    QAction *pRefreshAction = mainWindow.findChild<QAction *>(QString::fromUtf8("refreshAction"));
    QAction *pEditCurrentAction = mainWindow.findChild<QAction *>(QString::fromUtf8("editCurrentAction"));
    QAction *pDeleteCurrentAction = mainWindow.findChild<QAction *>(QString::fromUtf8("deleteCurrentAction"));
    QAction *pFocusSearchAction = mainWindow.findChild<QAction *>(QString::fromUtf8("focusSearchAction"));
    if (nullptr == pNewSessionAction
        || nullptr == pNewTextSnippetAction
        || nullptr == pCaptureScreenAction
        || nullptr == pCaptureRegionAction
        || nullptr == pImportImageAction
        || nullptr == pDuplicateSnippetAction
        || nullptr == pMoveSnippetAction
        || nullptr == pEditSessionAction
        || nullptr == pFinishSessionAction
        || nullptr == pReopenSessionAction
        || nullptr == pDeleteSessionAction
        || nullptr == pEditSnippetAction
        || nullptr == pManageTagsAction
        || nullptr == pTagLibraryAction
        || nullptr == pDeleteSnippetAction
        || nullptr == pToggleArchiveSnippetAction
        || nullptr == pSummarizeSnippetAction
        || nullptr == pRetrySnippetSummaryAction
        || nullptr == pSummarizeSessionAction
        || nullptr == pRetrySessionSummaryAction
        || nullptr == pAiSettingsAction
        || nullptr == pExportMarkdownAction
        || nullptr == pRefreshAction
        || nullptr == pEditCurrentAction
        || nullptr == pDeleteCurrentAction
        || nullptr == pFocusSearchAction)
    {
        PrintError(QString::fromUtf8("Main window verification actions are missing."));
        return 611;
    }

    if (!ActionHasShortcut(pNewSessionAction, QKeySequence(QString::fromUtf8("Ctrl+Shift+N")))
        || !ActionHasShortcut(pNewTextSnippetAction, QKeySequence::New)
        || !ActionHasShortcut(pCaptureScreenAction, QKeySequence(QString::fromUtf8("Ctrl+Shift+S")))
        || !ActionHasShortcut(pCaptureRegionAction, QKeySequence(QString::fromUtf8("Ctrl+Shift+R")))
        || !ActionHasShortcut(pImportImageAction, QKeySequence(QString::fromUtf8("Ctrl+Shift+I")))
        || !ActionHasShortcut(pDuplicateSnippetAction, QKeySequence(QString::fromUtf8("Ctrl+D")))
        || !ActionHasShortcut(pMoveSnippetAction, QKeySequence(QString::fromUtf8("Ctrl+Alt+V")))
        || !ActionHasShortcut(pManageTagsAction, QKeySequence(QString::fromUtf8("Ctrl+T")))
        || !ActionHasShortcut(pTagLibraryAction, QKeySequence(QString::fromUtf8("Ctrl+Shift+T")))
        || !ActionHasShortcut(pSummarizeSnippetAction, QKeySequence(QString::fromUtf8("Ctrl+Shift+M")))
        || !ActionHasShortcut(pRetrySnippetSummaryAction, QKeySequence(QString::fromUtf8("Ctrl+Shift+Y")))
        || !ActionHasShortcut(pSummarizeSessionAction, QKeySequence(QString::fromUtf8("Ctrl+Alt+M")))
        || !ActionHasShortcut(pRetrySessionSummaryAction, QKeySequence(QString::fromUtf8("Ctrl+Alt+Y")))
        || !ActionHasShortcut(pAiSettingsAction, QKeySequence(QString::fromUtf8("Ctrl+,")))
        || !ActionHasShortcut(pExportMarkdownAction, QKeySequence(QString::fromUtf8("Ctrl+Shift+E")))
        || !ActionHasShortcut(pRefreshAction, QKeySequence::Refresh)
        || !ActionHasShortcut(pEditCurrentAction, QKeySequence(Qt::Key_F2))
        || !ActionHasShortcut(pDeleteCurrentAction, QKeySequence::Delete)
        || !ActionHasShortcut(pFocusSearchAction, QKeySequence::Find))
    {
        PrintError(QString::fromUtf8("Shortcut mapping verification failed."));
        return 612;
    }

    if (!SelectListItemById(pSessionListWidget, activeSession.id()))
    {
        PrintError(QString::fromUtf8("Failed to select active session."));
        return 613;
    }

    if (pSnippetListWidget->count() != 1)
    {
        PrintError(QString::fromUtf8("Archived snippets should be hidden by default."));
        return 614;
    }

    if (!SelectListItemById(pSnippetListWidget, textSnippet.id()))
    {
        PrintError(QString::fromUtf8("Failed to select visible text snippet."));
        return 615;
    }

    if (!pEditSessionAction->isEnabled()
        || !pFinishSessionAction->isEnabled()
        || pReopenSessionAction->isEnabled()
        || !pDeleteSessionAction->isEnabled()
        || !pEditSnippetAction->isEnabled()
        || !pDuplicateSnippetAction->isEnabled()
        || !pMoveSnippetAction->isEnabled()
        || !pManageTagsAction->isEnabled()
        || !pTagLibraryAction->isEnabled()
        || !pDeleteSnippetAction->isEnabled()
        || !pToggleArchiveSnippetAction->isEnabled()
        || !pSummarizeSnippetAction->isEnabled()
        || pRetrySnippetSummaryAction->isEnabled()
        || !pSummarizeSessionAction->isEnabled()
        || pRetrySessionSummaryAction->isEnabled()
        || !pExportMarkdownAction->isEnabled()
        || !pEditCurrentAction->isEnabled()
        || !pDeleteCurrentAction->isEnabled())
    {
        PrintError(QString::fromUtf8("Active session or snippet action-state verification failed."));
        return 616;
    }

    pShowArchivedCheckBox->setChecked(true);
    QApplication::processEvents();
    if (pSnippetListWidget->count() != 2)
    {
        PrintError(QString::fromUtf8("Show Archived should reveal archived snippets."));
        return 617;
    }

    if (pSearchHistoryComboBox->count() < 1 || pSnippetTypeFilterComboBox->count() != 3)
    {
        PrintError(QString::fromUtf8("Search history or snippet type filter anchors are invalid."));
        return 6171;
    }

    pSnippetTypeFilterComboBox->setCurrentIndex(2);
    QApplication::processEvents();
    if (pSnippetListWidget->count() != 1)
    {
        PrintError(QString::fromUtf8("Image type filter should only show image snippets."));
        return 6172;
    }

    pSnippetTypeFilterComboBox->setCurrentIndex(1);
    QApplication::processEvents();
    if (pSnippetListWidget->count() != 1)
    {
        PrintError(QString::fromUtf8("Text type filter should only show text snippets."));
        return 6173;
    }

    pSnippetTypeFilterComboBox->setCurrentIndex(0);
    QApplication::processEvents();

    if (!SelectListItemById(pSnippetListWidget, archivedSnippet.id()))
    {
        PrintError(QString::fromUtf8("Failed to select archived snippet."));
        return 618;
    }

    if (pToggleArchiveSnippetAction->text() != QString::fromUtf8("Restore Snippet"))
    {
        PrintError(QString::fromUtf8("Archived snippet action text verification failed."));
        return 619;
    }

    QVector<qint64> vecMultiSelectSnippetIds;
    vecMultiSelectSnippetIds.append(textSnippet.id());
    vecMultiSelectSnippetIds.append(archivedSnippet.id());
    if (!SelectListItemsByIds(pSnippetListWidget, vecMultiSelectSnippetIds))
    {
        PrintError(QString::fromUtf8("Failed to multi-select snippets."));
        return 620;
    }

    if (pEditSnippetAction->isEnabled()
        || !pManageTagsAction->isEnabled()
        || pSummarizeSnippetAction->isEnabled()
        || !pDuplicateSnippetAction->isEnabled()
        || !pMoveSnippetAction->isEnabled()
        || !pDeleteSnippetAction->isEnabled()
        || !pToggleArchiveSnippetAction->isEnabled()
        || pEditCurrentAction->isEnabled()
        || !pDeleteCurrentAction->isEnabled()
        || !pMoveSnippetAction->text().contains(QString::fromUtf8("2"))
        || !pDeleteSnippetAction->text().contains(QString::fromUtf8("2"))
        || !pManageTagsAction->text().contains(QString::fromUtf8("2")))
    {
        PrintError(QString::fromUtf8("Multi-selection action-state verification failed. editSnippet=%1 manageTags=%2 summarizeSnippet=%3 duplicate=%4 move=%5 deleteSnippet=%6 toggleArchive=%7 editCurrent=%8 deleteCurrent=%9 moveText=%10 deleteText=%11 manageText=%12").arg(pEditSnippetAction->isEnabled() ? QString::fromUtf8("true") : QString::fromUtf8("false"), pManageTagsAction->isEnabled() ? QString::fromUtf8("true") : QString::fromUtf8("false"), pSummarizeSnippetAction->isEnabled() ? QString::fromUtf8("true") : QString::fromUtf8("false"), pDuplicateSnippetAction->isEnabled() ? QString::fromUtf8("true") : QString::fromUtf8("false"), pMoveSnippetAction->isEnabled() ? QString::fromUtf8("true") : QString::fromUtf8("false"), pDeleteSnippetAction->isEnabled() ? QString::fromUtf8("true") : QString::fromUtf8("false"), pToggleArchiveSnippetAction->isEnabled() ? QString::fromUtf8("true") : QString::fromUtf8("false"), pEditCurrentAction->isEnabled() ? QString::fromUtf8("true") : QString::fromUtf8("false"), pDeleteCurrentAction->isEnabled() ? QString::fromUtf8("true") : QString::fromUtf8("false"), pMoveSnippetAction->text(), pDeleteSnippetAction->text(), pManageTagsAction->text()));
        return 621;
    }


    const QString strEmptyDatabasePath = QDir(strTempDirectory).filePath(QString::fromUtf8("qtclip_mainwindow_empty_verify.sqlite"));
    QFile::remove(strEmptyDatabasePath);

    QCDatabaseManager emptyDatabaseManager;
    if (!emptyDatabaseManager.open(strEmptyDatabasePath) || !emptyDatabaseManager.initialize())
    {
        PrintError(emptyDatabaseManager.lastError());
        return 622;
    }

    QCSessionRepositorySqlite emptySessionRepository(&emptyDatabaseManager);
    QCSnippetRepositorySqlite emptySnippetRepository(&emptyDatabaseManager);
    QCAiRecordRepositorySqlite emptyAiRecordRepository(&emptyDatabaseManager);
    QCSettingsRepositorySqlite emptySettingsRepository(&emptyDatabaseManager);
    QCTagRepositorySqlite emptyTagRepository(&emptyDatabaseManager);
    QCSessionService emptySessionService(&emptySessionRepository);
    QCSnippetService emptySnippetService(&emptySnippetRepository);
    QCAiService emptyAiService(&emptyAiRecordRepository);
    QCSettingsService emptySettingsService(&emptySettingsRepository);
    QCTagService emptyTagService(&emptyTagRepository);
    QCAiProcessService emptyAiProcessService(&emptySessionService, &emptySnippetService, &emptyAiService, &emptySettingsService);
    QCExportDataService emptyExportDataService(&emptySessionService, &emptySnippetService, &emptyAiService);
    QCMdExportRenderer emptyMdExportRenderer;
    QCMdExportService emptyMdExportService(&emptyExportDataService, &emptyMdExportRenderer);
    QCScreenCaptureService emptyScreenCaptureService(&emptySettingsService);

    QCMainWindow emptyMainWindow(&emptySessionService,
                                 &emptySnippetService,
                                 &emptyTagService,
                                 &emptyAiService,
                                 &emptyAiProcessService,
                                 &emptySettingsService,
                                 &emptyMdExportService,
                                 &emptyScreenCaptureService);
    emptyMainWindow.show();
    QApplication::processEvents();

    QAction *pEmptyDuplicateSnippetAction = emptyMainWindow.findChild<QAction *>(QString::fromUtf8("duplicateSnippetAction"));
    QAction *pEmptyMoveSnippetAction = emptyMainWindow.findChild<QAction *>(QString::fromUtf8("moveSnippetAction"));
    QAction *pEmptyEditSessionAction = emptyMainWindow.findChild<QAction *>(QString::fromUtf8("editSessionAction"));
    QAction *pEmptyFinishSessionAction = emptyMainWindow.findChild<QAction *>(QString::fromUtf8("finishSessionAction"));
    QAction *pEmptyReopenSessionAction = emptyMainWindow.findChild<QAction *>(QString::fromUtf8("reopenSessionAction"));
    QAction *pEmptyDeleteSessionAction = emptyMainWindow.findChild<QAction *>(QString::fromUtf8("deleteSessionAction"));
    QAction *pEmptyEditSnippetAction = emptyMainWindow.findChild<QAction *>(QString::fromUtf8("editSnippetAction"));
    QAction *pEmptyTagLibraryAction = emptyMainWindow.findChild<QAction *>(QString::fromUtf8("tagLibraryAction"));
    QAction *pEmptyDeleteSnippetAction = emptyMainWindow.findChild<QAction *>(QString::fromUtf8("deleteSnippetAction"));
    QAction *pEmptySummarizeSnippetAction = emptyMainWindow.findChild<QAction *>(QString::fromUtf8("summarizeSnippetAction"));
    QAction *pEmptyRetrySnippetSummaryAction = emptyMainWindow.findChild<QAction *>(QString::fromUtf8("retrySnippetSummaryAction"));
    QAction *pEmptySummarizeSessionAction = emptyMainWindow.findChild<QAction *>(QString::fromUtf8("summarizeSessionAction"));
    QAction *pEmptyRetrySessionSummaryAction = emptyMainWindow.findChild<QAction *>(QString::fromUtf8("retrySessionSummaryAction"));
    QAction *pEmptyExportMarkdownAction = emptyMainWindow.findChild<QAction *>(QString::fromUtf8("exportMarkdownAction"));
    QAction *pEmptyEditCurrentAction = emptyMainWindow.findChild<QAction *>(QString::fromUtf8("editCurrentAction"));
    QAction *pEmptyDeleteCurrentAction = emptyMainWindow.findChild<QAction *>(QString::fromUtf8("deleteCurrentAction"));
    QLineEdit *pEmptySearchLineEdit = emptyMainWindow.findChild<QLineEdit *>(QString::fromUtf8("snippetSearchLineEdit"));
    QAction *pEmptyFocusSearchAction = emptyMainWindow.findChild<QAction *>(QString::fromUtf8("focusSearchAction"));
    if (nullptr == pEmptyDuplicateSnippetAction
        || nullptr == pEmptyMoveSnippetAction
        || nullptr == pEmptyEditSessionAction
        || nullptr == pEmptyFinishSessionAction
        || nullptr == pEmptyReopenSessionAction
        || nullptr == pEmptyDeleteSessionAction
        || nullptr == pEmptyEditSnippetAction
        || nullptr == pEmptyTagLibraryAction
        || nullptr == pEmptyDeleteSnippetAction
        || nullptr == pEmptySummarizeSnippetAction
        || nullptr == pEmptyRetrySnippetSummaryAction
        || nullptr == pEmptySummarizeSessionAction
        || nullptr == pEmptyRetrySessionSummaryAction
        || nullptr == pEmptyExportMarkdownAction
        || nullptr == pEmptyEditCurrentAction
        || nullptr == pEmptyDeleteCurrentAction
        || nullptr == pEmptySearchLineEdit
        || nullptr == pEmptyFocusSearchAction)
    {
        PrintError(QString::fromUtf8("Empty main window verification anchors are missing."));
        return 627;
    }

    if (pEmptyDuplicateSnippetAction->isEnabled()
        || pEmptyMoveSnippetAction->isEnabled()
        || pEmptyEditSessionAction->isEnabled()
        || pEmptyFinishSessionAction->isEnabled()
        || pEmptyReopenSessionAction->isEnabled()
        || pEmptyDeleteSessionAction->isEnabled()
        || pEmptyEditSnippetAction->isEnabled()
        || !pEmptyTagLibraryAction->isEnabled()
        || pEmptyDeleteSnippetAction->isEnabled()
        || pEmptySummarizeSnippetAction->isEnabled()
        || pEmptyRetrySnippetSummaryAction->isEnabled()
        || pEmptySummarizeSessionAction->isEnabled()
        || pEmptyRetrySessionSummaryAction->isEnabled()
        || pEmptyExportMarkdownAction->isEnabled()
        || pEmptyEditCurrentAction->isEnabled()
        || pEmptyDeleteCurrentAction->isEnabled())
    {
        PrintError(QString::fromUtf8("No-selection action-state verification failed. editSession=%1 finish=%2 reopen=%3 deleteSession=%4 editSnippet=%5 tagLibrary=%6 deleteSnippet=%7 summarizeSnippet=%8 summarizeSession=%9 export=%10 editCurrent=%11 deleteCurrent=%12").arg(pEmptyEditSessionAction->isEnabled() ? QString::fromUtf8("true") : QString::fromUtf8("false"), pEmptyFinishSessionAction->isEnabled() ? QString::fromUtf8("true") : QString::fromUtf8("false"), pEmptyReopenSessionAction->isEnabled() ? QString::fromUtf8("true") : QString::fromUtf8("false"), pEmptyDeleteSessionAction->isEnabled() ? QString::fromUtf8("true") : QString::fromUtf8("false"), pEmptyEditSnippetAction->isEnabled() ? QString::fromUtf8("true") : QString::fromUtf8("false"), pEmptyTagLibraryAction->isEnabled() ? QString::fromUtf8("true") : QString::fromUtf8("false"), pEmptyDeleteSnippetAction->isEnabled() ? QString::fromUtf8("true") : QString::fromUtf8("false"), pEmptySummarizeSnippetAction->isEnabled() ? QString::fromUtf8("true") : QString::fromUtf8("false"), pEmptySummarizeSessionAction->isEnabled() ? QString::fromUtf8("true") : QString::fromUtf8("false"), pEmptyExportMarkdownAction->isEnabled() ? QString::fromUtf8("true") : QString::fromUtf8("false"), pEmptyEditCurrentAction->isEnabled() ? QString::fromUtf8("true") : QString::fromUtf8("false"), pEmptyDeleteCurrentAction->isEnabled() ? QString::fromUtf8("true") : QString::fromUtf8("false")));
        return 628;
    }

    if (!SelectListItemById(pSessionListWidget, finishedSession.id()))
    {
        PrintError(QString::fromUtf8("Failed to select finished session."));
        return 626;
    }

    if (pFinishSessionAction->isEnabled()
        || !pReopenSessionAction->isEnabled()
        || !pEditSessionAction->isEnabled()
        || !pDeleteSessionAction->isEnabled()
        || pDuplicateSnippetAction->isEnabled()
        || pMoveSnippetAction->isEnabled()
        || !pSummarizeSessionAction->isEnabled()
        || !pExportMarkdownAction->isEnabled()
        || !pEditCurrentAction->isEnabled()
        || !pDeleteCurrentAction->isEnabled())
    {
        PrintError(QString::fromUtf8("Finished session action-state verification failed."));
        return 625;
    }

    pFocusSearchAction->trigger();
    QApplication::processEvents();
    if (!pFocusSearchAction->isEnabled() || nullptr == pSearchLineEdit)
    {
        PrintError(QString::fromUtf8("Focus search action verification failed."));
        return 629;
    }

    PrintInfo(QString::fromUtf8("Main window state verification passed."));
    PrintInfo(QString::fromUtf8("Visible snippets with archived hidden: 1"));
    PrintInfo(QString::fromUtf8("Visible snippets with archived shown: 2"));
    return 0;
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

    QStringList vecSearchHistory;
    vecSearchHistory << QString::fromUtf8("repository") << QString::fromUtf8("capture region") << QString::fromUtf8("export markdown");
    if (!settingsService.setSnippetSearchHistory(vecSearchHistory))
    {
        PrintError(settingsService.lastError());
        return 471;
    }

    QStringList vecLoadedSearchHistory;
    if (!settingsService.getSnippetSearchHistory(&vecLoadedSearchHistory))
    {
        PrintError(settingsService.lastError());
        return 472;
    }

    if (vecLoadedSearchHistory.size() != 3 || vecLoadedSearchHistory.first() != QString::fromUtf8("repository"))
    {
        PrintError(QString::fromUtf8("Search history round-trip verification failed."));
        return 473;
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

    QCStudySession activeSessionLookup;
    if (!sessionService.getActiveSession(&activeSessionLookup) || activeSessionLookup.id() != session.id())
    {
        PrintError(sessionService.lastError().isEmpty() ? QString::fromUtf8("Active session lookup verification failed.") : sessionService.lastError());
        return 5;
    }

    QCSnippet textSnippet;
    textSnippet.setSessionId(session.id());
    textSnippet.setCaptureType(QCCaptureType::ManualCaptureType);
    textSnippet.setTitle(QString());
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

    textSnippet.setTitle(QString::fromUtf8("Edited Text Snippet"));
    textSnippet.setNote(QString::fromUtf8("Updated note."));
    textSnippet.setContentText(QString::fromUtf8("Updated content keeps Repositories visible for text snippet editing verification."));
    textSnippet.setIsFavorite(true);
    if (!snippetService.updateSnippet(&textSnippet))
    {
        PrintError(snippetService.lastError());
        return 9;
    }

    QCSnippet updatedTextSnippet;
    if (!snippetService.getSnippetById(textSnippet.id(), &updatedTextSnippet)
        || updatedTextSnippet.title() != QString::fromUtf8("Edited Text Snippet")
        || updatedTextSnippet.note() != QString::fromUtf8("Updated note.")
        || updatedTextSnippet.contentText() != QString::fromUtf8("Updated content keeps Repositories visible for text snippet editing verification."))
    {
        PrintError(snippetService.lastError().isEmpty() ? QString::fromUtf8("Text snippet update verification failed.") : snippetService.lastError());
        return 10;
    }

    textSnippet = updatedTextSnippet;

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

    QVector<QCTag> vecSnippetTags = tagService.listTagsBySnippet(textSnippet.id());
    if (!tagService.lastError().isEmpty() || vecSnippetTags.isEmpty())
    {
        PrintError(tagService.lastError().isEmpty() ? QString::fromUtf8("Tag binding verification failed.") : tagService.lastError());
        return 8;
    }

    QCTag duplicateTag;
    duplicateTag.setName(QString::fromUtf8("Architecture"));
    if (tagService.createTag(&duplicateTag) || !tagService.lastError().contains(QString::fromUtf8("already exists"), Qt::CaseInsensitive))
    {
        PrintError(QString::fromUtf8("Duplicate tag protection verification failed."));
        return 55;
    }

    tag.setName(QString::fromUtf8("system-design"));
    if (!tagService.updateTag(&tag))
    {
        PrintError(tagService.lastError());
        return 56;
    }

    QCTag renamedTag;
    if (!tagService.getTagById(tag.id(), &renamedTag) || renamedTag.name().compare(QString::fromUtf8("system-design"), Qt::CaseInsensitive) != 0)
    {
        PrintError(tagService.lastError().isEmpty() ? QString::fromUtf8("Tag rename verification failed.") : tagService.lastError());
        return 57;
    }

    tag = renamedTag;
    vecSnippetTags = tagService.listTagsBySnippet(textSnippet.id());
    if (!tagService.lastError().isEmpty() || vecSnippetTags.isEmpty() || vecSnippetTags.at(0).name().compare(QString::fromUtf8("system-design"), Qt::CaseInsensitive) != 0)
    {
        PrintError(tagService.lastError().isEmpty() ? QString::fromUtf8("Renamed tag binding verification failed.") : tagService.lastError());
        return 58;
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
    imageSnippet.setTitle(QString());
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

    imageSnippet.setTitle(QString::fromUtf8("Edited Image Snippet"));
    imageSnippet.setNote(QString::fromUtf8("Updated image note."));
    if (!snippetService.updateSnippet(&imageSnippet))
    {
        PrintError(snippetService.lastError());
        return 12;
    }

    QCSnippet updatedImageSnippet;
    if (!snippetService.getSnippetById(imageSnippet.id(), &updatedImageSnippet)
        || updatedImageSnippet.title() != QString::fromUtf8("Edited Image Snippet")
        || updatedImageSnippet.note() != QString::fromUtf8("Updated image note."))
    {
        PrintError(snippetService.lastError().isEmpty() ? QString::fromUtf8("Image snippet update verification failed.") : snippetService.lastError());
        return 13;
    }

    imageSnippet = updatedImageSnippet;

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

    if (!snippetService.setArchived(textSnippet.id(), true))
    {
        PrintError(snippetService.lastError());
        return 25;
    }

    QCSnippet archivedSnippet;
    if (!snippetService.getSnippetById(textSnippet.id(), &archivedSnippet) || !archivedSnippet.isArchived())
    {
        PrintError(snippetService.lastError().isEmpty() ? QString::fromUtf8("Snippet archive verification failed.") : snippetService.lastError());
        return 22;
    }

    if (!snippetService.setArchived(textSnippet.id(), false))
    {
        PrintError(snippetService.lastError());
        return 23;
    }

    if (!snippetService.getSnippetById(textSnippet.id(), &archivedSnippet) || archivedSnippet.isArchived())
    {
        PrintError(snippetService.lastError().isEmpty() ? QString::fromUtf8("Snippet restore verification failed.") : snippetService.lastError());
        return 24;
    }

    if (!snippetService.setReviewState(textSnippet.id(), true))
    {
        PrintError(snippetService.lastError());
        return 21;
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

    const QVector<QCSnippet> vecSessionSnippetsForExport = snippetService.listSnippetsBySession(session.id());
    if (!snippetService.lastError().isEmpty() || vecSessionSnippetsForExport.isEmpty())
    {
        PrintError(snippetService.lastError().isEmpty() ? QString::fromUtf8("Export snippet listing verification failed.") : snippetService.lastError());
        return 18;
    }

    int nExpectedImageSnippetCount = 0;
    int nExpectedArchivedSnippetCount = 0;
    int nExpectedFavoriteSnippetCount = 0;
    int nExpectedSummarizedSnippetCount = 0;
    for (int i = 0; i < vecSessionSnippetsForExport.size(); ++i)
    {
        const QCSnippet& exportSnippet = vecSessionSnippetsForExport.at(i);
        if (exportSnippet.type() == QCSnippetType::ImageSnippetType)
            ++nExpectedImageSnippetCount;
        if (exportSnippet.isArchived())
            ++nExpectedArchivedSnippetCount;
        if (exportSnippet.isFavorite())
            ++nExpectedFavoriteSnippetCount;
        if (!exportSnippet.summary().trimmed().isEmpty())
            ++nExpectedSummarizedSnippetCount;
    }

    bool bExpectedHasSessionSummary = false;
    for (int i = 0; i < vecSessionAiRecords.size(); ++i)
    {
        if (vecSessionAiRecords.at(i).taskType() == QCAiTaskType::SessionSummaryTask
            && !vecSessionAiRecords.at(i).responseText().trimmed().isEmpty())
        {
            bExpectedHasSessionSummary = true;
            break;
        }
    }

    QCMdExportPreview exportPreview;
    if (!mdExportService.buildExportPreview(session.id(), &exportPreview))
    {
        PrintError(mdExportService.lastError());
        return 19;
    }

    if (exportPreview.m_strSessionTitle != session.title()
        || exportPreview.m_strCourseName != session.courseName()
        || exportPreview.m_nSnippetCount != vecSessionSnippetsForExport.size()
        || exportPreview.m_nImageSnippetCount != nExpectedImageSnippetCount
        || exportPreview.m_nArchivedSnippetCount != nExpectedArchivedSnippetCount
        || exportPreview.m_nFavoriteSnippetCount != nExpectedFavoriteSnippetCount
        || exportPreview.m_nSummarizedSnippetCount != nExpectedSummarizedSnippetCount
        || exportPreview.m_bHasSessionSummary != bExpectedHasSessionSummary)
    {
        PrintError(QString::fromUtf8("Markdown export preview verification failed."));
        return 20;
    }

    if (!mdExportService.exportSessionToFile(session.id(), strMarkdownPath))
    {
        PrintError(mdExportService.lastError());
        return 21;
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
        return 22;
    }

    const QByteArray byteArrayMarkdown = markdownFile.readAll();
    markdownFile.close();
    if (byteArrayMarkdown.isEmpty())
    {
        PrintError(QString::fromUtf8("Markdown export file is empty."));
        return 23;
    }

    const QString strMarkdownText = QString::fromUtf8(byteArrayMarkdown);
    if (!strMarkdownText.contains(QString::fromUtf8("## Session Details"))
        || !strMarkdownText.contains(QString::fromUtf8("## Overview"))
        || !strMarkdownText.contains(QString::fromUtf8("## Snippet Timeline"))
        || !strMarkdownText.contains(QString::fromUtf8("#### Summary"))
        || !strMarkdownText.contains(QString::fromUtf8("#### Attachment")))
    {
        PrintError(QString::fromUtf8("Markdown export structure verification failed."));
        return 24;
    }

    QCStudySession deleteSession;
    deleteSession.setTitle(QString::fromUtf8("Delete Verification Session"));
    deleteSession.setDescription(QString::fromUtf8("Used to verify deleteSession and deleteSnippet flows."));
    if (!sessionService.createSession(&deleteSession))
    {
        PrintError(sessionService.lastError());
        return 61;
    }

    QCSnippet deleteSnippet;
    deleteSnippet.setSessionId(deleteSession.id());
    deleteSnippet.setType(QCSnippetType::TextSnippetType);
    deleteSnippet.setCaptureType(QCCaptureType::ManualCaptureType);
    deleteSnippet.setTitle(QString::fromUtf8("Delete Verification Snippet"));
    deleteSnippet.setContentText(QString::fromUtf8("Temporary snippet for delete verification."));
    deleteSnippet.setNote(QString::fromUtf8("Delete after create."));
    deleteSnippet.setNoteKind(QCNoteKind::ReviewNoteKind);
    deleteSnippet.setNoteLevel(QCNoteLevel::ImportantNoteLevel);
    deleteSnippet.setSource(QString::fromUtf8("lecture"));
    deleteSnippet.setLanguage(QString::fromUtf8("en"));
    if (!snippetService.createTextSnippet(&deleteSnippet))
    {
        PrintError(snippetService.lastError());
        return 62;
    }

    if (!snippetService.deleteSnippet(deleteSnippet.id()))
    {
        PrintError(snippetService.lastError());
        return 63;
    }

    QCSnippet deletedSnippetLookup;
    if (snippetService.getSnippetById(deleteSnippet.id(), &deletedSnippetLookup))
    {
        PrintError(QString::fromUtf8("Deleted snippet lookup should have failed."));
        return 64;
    }

    if (!snippetService.lastError().contains(QString::fromUtf8("not found"), Qt::CaseInsensitive))
    {
        PrintError(QString::fromUtf8("Deleted snippet error verification failed."));
        return 65;
    }

    if (!sessionService.deleteSession(deleteSession.id()))
    {
        PrintError(sessionService.lastError());
        return 66;
    }

    QCStudySession deletedSessionLookup;
    if (sessionService.getSessionById(deleteSession.id(), &deletedSessionLookup))
    {
        PrintError(QString::fromUtf8("Deleted session lookup should have failed."));
        return 67;
    }

    if (!sessionService.lastError().contains(QString::fromUtf8("not found"), Qt::CaseInsensitive))
    {
        PrintError(QString::fromUtf8("Deleted session error verification failed."));
        return 68;
    }

    QCStudySession editableSession;
    editableSession.setTitle(QString::fromUtf8("Editable Session"));
    editableSession.setCourseName(QString::fromUtf8("Math"));
    editableSession.setDescription(QString::fromUtf8("Original description."));
    if (!sessionService.createSession(&editableSession))
    {
        PrintError(sessionService.lastError());
        return 69;
    }

    editableSession.setTitle(QString::fromUtf8("Edited Session"));
    editableSession.setCourseName(QString::fromUtf8("Advanced Math"));
    editableSession.setDescription(QString::fromUtf8("Updated description."));
    if (!sessionService.updateSession(&editableSession))
    {
        PrintError(sessionService.lastError());
        return 70;
    }

    QCStudySession updatedSessionLookup;
    if (!sessionService.getSessionById(editableSession.id(), &updatedSessionLookup)
        || updatedSessionLookup.title() != QString::fromUtf8("Edited Session")
        || updatedSessionLookup.courseName() != QString::fromUtf8("Advanced Math")
        || updatedSessionLookup.description() != QString::fromUtf8("Updated description."))
    {
        PrintError(sessionService.lastError().isEmpty() ? QString::fromUtf8("Session update verification failed.") : sessionService.lastError());
        return 70;
    }

    QCStudySession finishSession;
    finishSession.setTitle(QString::fromUtf8("Finish Verification Session"));
    finishSession.setDescription(QString::fromUtf8("Used to verify finishSession persistence."));
    if (!sessionService.createSession(&finishSession))
    {
        PrintError(sessionService.lastError());
        return 69;
    }

    const QDateTime dateTimeFinishedAt = QDateTime::currentDateTimeUtc();
    if (!sessionService.finishSession(finishSession.id(), dateTimeFinishedAt))
    {
        PrintError(sessionService.lastError());
        return 71;
    }

    QCStudySession finishedSessionLookup;
    if (!sessionService.getSessionById(finishSession.id(), &finishedSessionLookup))
    {
        PrintError(sessionService.lastError());
        return 72;
    }

    if (finishedSessionLookup.status() != QCSessionStatus::FinishedSessionStatus || !finishedSessionLookup.endedAt().isValid())
    {
        PrintError(QString::fromUtf8("Finish session verification failed."));
        return 73;
    }

    finishedSessionLookup.setStatus(QCSessionStatus::ActiveSessionStatus);
    finishedSessionLookup.setEndedAt(QDateTime());
    if (!sessionService.updateSession(&finishedSessionLookup))
    {
        PrintError(sessionService.lastError());
        return 74;
    }

    QCStudySession reopenedSessionLookup;
    if (!sessionService.getSessionById(finishSession.id(), &reopenedSessionLookup)
        || reopenedSessionLookup.status() != QCSessionStatus::ActiveSessionStatus
        || reopenedSessionLookup.endedAt().isValid())
    {
        PrintError(sessionService.lastError().isEmpty() ? QString::fromUtf8("Reopen session verification failed.") : sessionService.lastError());
        return 75;
    }

    if (!sessionService.deleteSession(editableSession.id()))
    {
        PrintError(sessionService.lastError());
        return 79;
    }

    if (!sessionService.deleteSession(finishSession.id()))
    {
        PrintError(sessionService.lastError());
        return 77;
    }

    if (!tagService.deleteTag(tag.id()))
    {
        PrintError(tagService.lastError());
        return 78;
    }

    const QVector<QCTag> vecTagsAfterDelete = tagService.listTagsBySnippet(textSnippet.id());
    if (!tagService.lastError().isEmpty() || !vecTagsAfterDelete.isEmpty())
    {
        PrintError(tagService.lastError().isEmpty() ? QString::fromUtf8("Tag delete cascade verification failed.") : tagService.lastError());
        return 79;
    }

    PrintInfo(QString::fromUtf8("Session created: %1").arg(session.id()));
    PrintInfo(QString::fromUtf8("Tag bound to snippet: %1").arg(QString::fromUtf8("system-design")));
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
    PrintInfo(QString::fromUtf8("Markdown preview snippets: %1").arg(exportPreview.m_nSnippetCount));
    PrintInfo(QString::fromUtf8("Markdown preview summarized snippets: %1").arg(exportPreview.m_nSummarizedSnippetCount));
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

    if (vecArguments.contains(QString::fromUtf8("--verify-management")))
        return RunManagementVerificationWorkflow();

    if (vecArguments.contains(QString::fromUtf8("--verify-mainwindow-state")))
        return RunMainWindowStateVerificationWorkflow();

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

    const int nSummarizeSessionFlagIndex = vecArguments.indexOf(QString::fromUtf8("--summarize-session"));
    if (nSummarizeSessionFlagIndex >= 0)
    {
        if (nSummarizeSessionFlagIndex + 1 >= vecArguments.size())
        {
            PrintError(QString::fromUtf8("Missing session id after --summarize-session."));
            return 17;
        }

        bool bOk = false;
        const qint64 nSessionId = vecArguments.at(nSummarizeSessionFlagIndex + 1).toLongLong(&bOk);
        if (!bOk || nSessionId <= 0)
        {
            PrintError(QString::fromUtf8("Session id is invalid."));
            return 18;
        }

        if (!aiProcessService.summarizeSession(nSessionId))
        {
            PrintError(aiProcessService.lastError().trimmed().isEmpty()
                ? QString::fromUtf8("Summarize Session failed.")
                : aiProcessService.lastError());
            return 19;
        }

        const QVector<QCAiRecord> vecAiRecords = aiService.listAiRecordsBySession(nSessionId);
        if (!aiService.lastError().trimmed().isEmpty())
        {
            PrintError(aiService.lastError());
            return 20;
        }

        QString strSessionSummary;
        for (int i = 0; i < vecAiRecords.size(); ++i)
        {
            const QCAiRecord& aiRecord = vecAiRecords.at(i);
            if (aiRecord.taskType() == QCAiTaskType::SessionSummaryTask && !aiRecord.responseText().trimmed().isEmpty())
            {
                strSessionSummary = aiRecord.responseText().trimmed();
                break;
            }
        }

        PrintInfo(QString::fromUtf8("Session %1 summarized.").arg(nSessionId));
        PrintInfo(QString::fromUtf8("Summary Preview: %1").arg(BuildPreviewText(strSessionSummary)));
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







