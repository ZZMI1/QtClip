// File: main.cpp
// Author: ZZMI1
// Created: 2026-03-23
// Description: Provides the minimal Qt Widgets application entry used by the QtClip demo application.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QtConcurrent/QtConcurrent>
#include <QDir>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QInputDialog>
#include <QGuiApplication>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSharedPointer>
#include <QScreen>
#include <QSet>
#include <QStandardPaths>
#include <QTextStream>
#include <functional>

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
#include "../ui/dialogs/qcaisettingsdialog.h"
#include "../ui/dialogs/qccreateimagesnippetdialog.h"
#include "../ui/dialogs/qccreatesessiondialog.h"
#include "../ui/dialogs/qccreatetextsnippetdialog.h"
#include "../ui/dialogs/qcquickcapturedialog.h"
#include "../ui/dialogs/qcsnippettagdialog.h"
#include "../ui/mainwindow/qcmainwindow.h"
#include "../ui/common/qcuitheme.h"
#include "../ui/common/qcuilocalization.h"

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

    QDir().mkpath(strDataDirectory);

    return QDir(strDataDirectory).filePath(QString::fromUtf8("qtclip.sqlite"));
}

QString BuildScopedTempPath(const QString& strPrefix, const QString& strFileName)
{
    QString strTempDirectory = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (strTempDirectory.isEmpty())
        strTempDirectory = QDir::tempPath();

    const QString strScopeSuffix = QString::fromUtf8("%1_%2")
        .arg(QCoreApplication::applicationPid())
        .arg(QDateTime::currentMSecsSinceEpoch());
    return QDir(strTempDirectory).filePath(QString::fromUtf8("qtclip_%1_%2_%3").arg(strPrefix, strScopeSuffix, strFileName));
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

void SelectMultipleSnippetItemsById(QListWidget *pListWidget, const QVector<qint64>& vecIds)
{
    if (nullptr == pListWidget)
        return;

    QSet<qint64> setIds;
    for (int i = 0; i < vecIds.size(); ++i)
    {
        if (vecIds.at(i) > 0)
            setIds.insert(vecIds.at(i));
    }

    pListWidget->clearSelection();
    QListWidgetItem *pCurrentItem = nullptr;
    for (int i = 0; i < pListWidget->count(); ++i)
    {
        QListWidgetItem *pItem = pListWidget->item(i);
        if (nullptr == pItem)
            continue;

        if (setIds.contains(pItem->data(Qt::UserRole).toLongLong()))
        {
            pItem->setSelected(true);
            if (nullptr == pCurrentItem)
                pCurrentItem = pItem;
        }
    }

    if (nullptr != pCurrentItem)
    {
        pListWidget->setCurrentItem(pCurrentItem);
        QApplication::processEvents();
    }
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

int RunSmokeWorkflow();

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
    QLabel *pSelectionContextLabel = mainWindow.findChild<QLabel *>(QString::fromUtf8("selectionContextLabel"));
    QLabel *pAiStatusLabel = mainWindow.findChild<QLabel *>(QString::fromUtf8("aiStatusLabel"));
    QCheckBox *pShowArchivedCheckBox = mainWindow.findChild<QCheckBox *>(QString::fromUtf8("showArchivedCheckBox"));
    if (nullptr == pSessionListWidget || nullptr == pSnippetListWidget || nullptr == pSearchLineEdit || nullptr == pSearchHistoryComboBox || nullptr == pSnippetTypeFilterComboBox || nullptr == pSelectionContextLabel || nullptr == pAiStatusLabel || nullptr == pShowArchivedCheckBox)
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

    if (pSessionListWidget->contextMenuPolicy() != Qt::ActionsContextMenu || pSnippetListWidget->contextMenuPolicy() != Qt::ActionsContextMenu)
    {
        PrintError(QString::fromUtf8("Context menu policy verification failed."));
        return 6121;
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

    if (!pSelectionContextLabel->text().contains(QString::fromUtf8("session"), Qt::CaseInsensitive)
        && !pSelectionContextLabel->text().contains(QString::fromUtf8("??"), Qt::CaseInsensitive))
    {
        PrintError(QString::fromUtf8("Session selection context label verification failed."));
        return 6141;
    }

    if (!SelectListItemById(pSnippetListWidget, textSnippet.id()))
    {
        PrintError(QString::fromUtf8("Failed to select visible text snippet."));
        return 615;
    }

    if (!pSelectionContextLabel->text().contains(QString::fromUtf8("1 selected snippet"), Qt::CaseInsensitive)
        && !pSelectionContextLabel->text().contains(QString::fromUtf8("1"), Qt::CaseInsensitive))
    {
        PrintError(QString::fromUtf8("Single snippet context label verification failed."));
        return 6151;
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

    if (pToggleArchiveSnippetAction->text() != QString::fromUtf8("Restore Snippet")
        && pToggleArchiveSnippetAction->text() != QCUiText(QString::fromUtf8("恢复 Snippet"), QString::fromUtf8("Restore Snippet")))
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

    if (!pSelectionContextLabel->text().contains(QString::fromUtf8("2 selected snippets"), Qt::CaseInsensitive)
        && !pSelectionContextLabel->text().contains(QString::fromUtf8("2"), Qt::CaseInsensitive))
    {
        PrintError(QString::fromUtf8("Multi-selection context label verification failed."));
        return 6201;
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

    QLabel *pEmptySelectionContextLabel = emptyMainWindow.findChild<QLabel *>(QString::fromUtf8("selectionContextLabel"));
    if (nullptr == pEmptySelectionContextLabel || (!pEmptySelectionContextLabel->text().contains(QCUiText(QString::fromUtf8("未选择会话"), QString::fromUtf8("no session selected")), Qt::CaseInsensitive)))
    {
        PrintError(QString::fromUtf8("Empty selection context label verification failed."));
        return 6271;
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

    if (!pSelectionContextLabel->text().contains(QString::fromUtf8("session"), Qt::CaseInsensitive)
        && !pSelectionContextLabel->text().contains(QString::fromUtf8("??"), Qt::CaseInsensitive))
    {
        PrintError(QString::fromUtf8("Finished session context label verification failed."));
        return 6241;
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

int RunBoundaryVerificationWorkflow()
{
    QString strTempDirectory = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (strTempDirectory.isEmpty())
        strTempDirectory = QDir::tempPath();

    const QString strDatabasePath = QDir(strTempDirectory).filePath(QString::fromUtf8("qtclip_boundary_verify.sqlite"));
    QFile::remove(strDatabasePath);

    QCDatabaseManager databaseManager;
    if (!databaseManager.open(strDatabasePath) || !databaseManager.initialize())
    {
        PrintError(databaseManager.lastError());
        return 701;
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
    Q_UNUSED(aiService);
    Q_UNUSED(settingsService);

    QCStudySession sourceSession;
    sourceSession.setTitle(QString::fromUtf8("Boundary Source"));
    if (!sessionService.createSession(&sourceSession))
    {
        PrintError(sessionService.lastError());
        return 702;
    }

    QCStudySession targetSession;
    targetSession.setTitle(QString::fromUtf8("Boundary Target"));
    if (!sessionService.createSession(&targetSession))
    {
        PrintError(sessionService.lastError());
        return 703;
    }

    QCSnippet textSnippet;
    textSnippet.setSessionId(sourceSession.id());
    textSnippet.setCaptureType(QCCaptureType::ManualCaptureType);
    textSnippet.setTitle(QString::fromUtf8("Boundary Snippet"));
    textSnippet.setContentText(QString::fromUtf8("Boundary content."));
    textSnippet.setNoteKind(QCNoteKind::ConceptNoteKind);
    textSnippet.setNoteLevel(QCNoteLevel::ImportantNoteLevel);
    if (!snippetService.createTextSnippet(&textSnippet))
    {
        PrintError(snippetService.lastError());
        return 704;
    }

    qint64 nNewSnippetId = 0;
    if (snippetService.duplicateSnippet(0, sourceSession.id(), &nNewSnippetId)
        || !snippetService.lastError().contains(QString::fromUtf8("invalid"), Qt::CaseInsensitive))
    {
        PrintError(QString::fromUtf8("Duplicate boundary verification failed."));
        return 705;
    }

    if (snippetService.moveSnippetsToSession(QVector<qint64>(), targetSession.id())
        || !snippetService.lastError().contains(QString::fromUtf8("empty"), Qt::CaseInsensitive))
    {
        PrintError(QString::fromUtf8("Empty move boundary verification failed."));
        return 706;
    }

    QVector<qint64> vecSnippetIds;
    vecSnippetIds.append(textSnippet.id());
    if (!snippetService.moveSnippetsToSession(vecSnippetIds, targetSession.id()))
    {
        PrintError(snippetService.lastError());
        return 707;
    }

    if (snippetService.moveSnippetsToSession(vecSnippetIds, targetSession.id())
        || !snippetService.lastError().contains(QString::fromUtf8("already belong"), Qt::CaseInsensitive))
    {
        PrintError(QString::fromUtf8("Same-target move boundary verification failed."));
        return 708;
    }

    if (tagService.replaceSnippetTags(textSnippet.id(), QVector<qint64>() << 999999))
    {
        PrintError(QString::fromUtf8("Invalid tag boundary verification failed."));
        return 709;
    }

    if (!tagService.lastError().contains(QString::fromUtf8("tag"), Qt::CaseInsensitive))
    {
        PrintError(QString::fromUtf8("Invalid tag boundary error message verification failed."));
        return 710;
    }

    PrintInfo(QString::fromUtf8("Boundary verification passed."));
    return 0;
}

int RunSelectionFlowVerificationWorkflow()
{
    QString strTempDirectory = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (strTempDirectory.isEmpty())
        strTempDirectory = QDir::tempPath();

    const QString strDatabasePath = BuildScopedTempPath(QString::fromUtf8("selection_flow"), QString::fromUtf8("verify.sqlite"));
    QFile::remove(strDatabasePath);

    QCDatabaseManager databaseManager;
    if (!databaseManager.open(strDatabasePath) || !databaseManager.initialize())
    {
        PrintError(databaseManager.lastError());
        return 703;
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

    QCStudySession sourceSession;
    sourceSession.setTitle(QString::fromUtf8("Selection Source"));
    if (!sessionService.createSession(&sourceSession))
    {
        PrintError(sessionService.lastError());
        return 704;
    }

    QCStudySession targetSession;
    targetSession.setTitle(QString::fromUtf8("Selection Target"));
    if (!sessionService.createSession(&targetSession))
    {
        PrintError(sessionService.lastError());
        return 705;
    }

    QVector<qint64> vecSourceSnippetIds;
    for (int i = 0; i < 3; ++i)
    {
        QCSnippet snippet;
        snippet.setSessionId(sourceSession.id());
        snippet.setCaptureType(QCCaptureType::ManualCaptureType);
        snippet.setTitle(QString::fromUtf8("Selection Snippet %1").arg(i + 1));
        snippet.setContentText(QString::fromUtf8("Selection flow content %1").arg(i + 1));
        snippet.setNoteKind(QCNoteKind::ConceptNoteKind);
        snippet.setNoteLevel(QCNoteLevel::ImportantNoteLevel);
        if (!snippetService.createTextSnippet(&snippet))
        {
            PrintError(snippetService.lastError());
            return 706;
        }
        vecSourceSnippetIds.append(snippet.id());
    }

    QCMainWindow mainWindow(&sessionService, &snippetService, &tagService, &aiService, &aiProcessService, &settingsService, &mdExportService, &screenCaptureService);
    mainWindow.show();
    QApplication::processEvents();

    QListWidget *pSessionListWidget = mainWindow.findChild<QListWidget *>(QString::fromUtf8("sessionListWidget"));
    QListWidget *pSnippetListWidget = mainWindow.findChild<QListWidget *>(QString::fromUtf8("snippetListWidget"));
    QAction *pMoveSnippetAction = mainWindow.findChild<QAction *>(QString::fromUtf8("moveSnippetAction"));
    QAction *pDuplicateSnippetAction = mainWindow.findChild<QAction *>(QString::fromUtf8("duplicateSnippetAction"));
    QAction *pEditSnippetAction = mainWindow.findChild<QAction *>(QString::fromUtf8("editSnippetAction"));
    QAction *pDeleteCurrentAction = mainWindow.findChild<QAction *>(QString::fromUtf8("deleteCurrentAction"));
    QLabel *pSelectionContextLabel = mainWindow.findChild<QLabel *>(QString::fromUtf8("selectionContextLabel"));
    if (nullptr == pSessionListWidget || nullptr == pSnippetListWidget || nullptr == pMoveSnippetAction || nullptr == pDuplicateSnippetAction || nullptr == pEditSnippetAction || nullptr == pDeleteCurrentAction || nullptr == pSelectionContextLabel)
    {
        PrintError(QString::fromUtf8("Selection flow anchors are missing."));
        return 707;
    }

    if (!SelectListItemById(pSessionListWidget, sourceSession.id()))
    {
        PrintError(QString::fromUtf8("Failed to select source session."));
        return 708;
    }

    if (!SelectListItemsByIds(pSnippetListWidget, vecSourceSnippetIds.mid(0, 2)))
    {
        PrintError(QString::fromUtf8("Failed to multi-select source snippets."));
        return 709;
    }
    QApplication::processEvents();
    if (pSnippetListWidget->selectedItems().size() != 2
        || !pMoveSnippetAction->isEnabled()
        || !pDuplicateSnippetAction->isEnabled()
        || pEditSnippetAction->isEnabled()
        || !pDeleteCurrentAction->isEnabled())
    {
        PrintError(QString::fromUtf8("Selection flow action state verification failed."));
        return 710;
    }

    if (!pSelectionContextLabel->text().contains(QString::fromUtf8("2")))
    {
        PrintError(QString::fromUtf8("Selection context label did not reflect multi-selection."));
        return 713;
    }

    if (!snippetService.moveSnippetsToSession(vecSourceSnippetIds.mid(0, 2), targetSession.id()))
    {
        PrintError(snippetService.lastError());
        return 711;
    }

    QVector<QCSnippet> vecTargetSnippets = snippetService.listSnippetsBySession(targetSession.id());
    if (vecTargetSnippets.size() < 2)
    {
        PrintError(QString::fromUtf8("Moved snippets were not found in target session."));
        return 712;
    }

    PrintInfo(QString::fromUtf8("Selection flow verification passed."));
    return 0;
}

int RunBatchDuplicateVerificationWorkflow()
{
    const QString strDatabasePath = BuildScopedTempPath(QString::fromUtf8("batch_duplicate"), QString::fromUtf8("verify.sqlite"));
    QFile::remove(strDatabasePath);

    QCDatabaseManager databaseManager;
    if (!databaseManager.open(strDatabasePath) || !databaseManager.initialize())
    {
        PrintError(databaseManager.lastError());
        return 771;
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

    QCStudySession session;
    session.setTitle(QString::fromUtf8("Duplicate Session"));
    if (!sessionService.createSession(&session))
    {
        PrintError(sessionService.lastError());
        return 772;
    }

    QVector<qint64> vecSourceSnippetIds;
    for (int i = 0; i < 2; ++i)
    {
        QCSnippet snippet;
        snippet.setSessionId(session.id());
        snippet.setCaptureType(QCCaptureType::ManualCaptureType);
        snippet.setTitle(QString::fromUtf8("Duplicate Source %1").arg(i + 1));
        snippet.setContentText(QString::fromUtf8("Duplicate verification content %1").arg(i + 1));
        snippet.setNoteKind(QCNoteKind::ConceptNoteKind);
        snippet.setNoteLevel(QCNoteLevel::ImportantNoteLevel);
        if (!snippetService.createTextSnippet(&snippet))
        {
            PrintError(snippetService.lastError());
            return 773;
        }
        vecSourceSnippetIds.append(snippet.id());
    }

    QCMainWindow mainWindow(&sessionService, &snippetService, &tagService, &aiService, &aiProcessService, &settingsService, &mdExportService, &screenCaptureService);
    mainWindow.show();
    QApplication::processEvents();

    QListWidget *pSessionListWidget = mainWindow.findChild<QListWidget *>(QString::fromUtf8("sessionListWidget"));
    QListWidget *pSnippetListWidget = mainWindow.findChild<QListWidget *>(QString::fromUtf8("snippetListWidget"));
    QAction *pDuplicateAction = mainWindow.findChild<QAction *>(QString::fromUtf8("duplicateSnippetAction"));
    QLabel *pContextLabel = mainWindow.findChild<QLabel *>(QString::fromUtf8("selectionContextLabel"));
    if (nullptr == pSessionListWidget || nullptr == pSnippetListWidget || nullptr == pDuplicateAction || nullptr == pContextLabel)
    {
        PrintError(QString::fromUtf8("Batch duplicate anchors are missing."));
        return 774;
    }

    if (!SelectListItemById(pSessionListWidget, session.id()) || !SelectListItemsByIds(pSnippetListWidget, vecSourceSnippetIds))
    {
        PrintError(QString::fromUtf8("Failed to prepare duplicate verification selection."));
        return 775;
    }

    const int nBeforeCount = snippetService.listSnippetsBySession(session.id()).size();
    pDuplicateAction->trigger();
    QApplication::processEvents();

    const QVector<QCSnippet> vecAfterSnippets = snippetService.listSnippetsBySession(session.id());
    if (vecAfterSnippets.size() != nBeforeCount + vecSourceSnippetIds.size())
    {
        PrintError(QString::fromUtf8("Batch duplicate count verification failed."));
        return 776;
    }

    if (pSnippetListWidget->selectedItems().size() != vecSourceSnippetIds.size()
        || !pContextLabel->text().contains(QString::number(vecSourceSnippetIds.size())))
    {
        PrintError(QString::fromUtf8("Batch duplicate selection verification failed."));
        return 777;
    }

    PrintInfo(QString::fromUtf8("Batch duplicate verification passed."));
    return 0;
}

int RunBatchMoveVerificationWorkflow()
{
    const QString strDatabasePath = BuildScopedTempPath(QString::fromUtf8("batch_move"), QString::fromUtf8("verify.sqlite"));
    QFile::remove(strDatabasePath);

    QCDatabaseManager databaseManager;
    if (!databaseManager.open(strDatabasePath) || !databaseManager.initialize())
    {
        PrintError(databaseManager.lastError());
        return 778;
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

    QCStudySession sourceSession;
    sourceSession.setTitle(QString::fromUtf8("Move Source"));
    if (!sessionService.createSession(&sourceSession))
    {
        PrintError(sessionService.lastError());
        return 779;
    }

    QCStudySession targetSession;
    targetSession.setTitle(QString::fromUtf8("Move Target"));
    if (!sessionService.createSession(&targetSession))
    {
        PrintError(sessionService.lastError());
        return 780;
    }

    QVector<qint64> vecSourceSnippetIds;
    for (int i = 0; i < 2; ++i)
    {
        QCSnippet snippet;
        snippet.setSessionId(sourceSession.id());
        snippet.setCaptureType(QCCaptureType::ManualCaptureType);
        snippet.setTitle(QString::fromUtf8("Move Source %1").arg(i + 1));
        snippet.setContentText(QString::fromUtf8("Move verification content %1").arg(i + 1));
        snippet.setNoteKind(QCNoteKind::ConceptNoteKind);
        snippet.setNoteLevel(QCNoteLevel::ImportantNoteLevel);
        if (!snippetService.createTextSnippet(&snippet))
        {
            PrintError(snippetService.lastError());
            return 781;
        }
        vecSourceSnippetIds.append(snippet.id());
    }

    QCMainWindow mainWindow(&sessionService, &snippetService, &tagService, &aiService, &aiProcessService, &settingsService, &mdExportService, &screenCaptureService);
    mainWindow.show();
    QApplication::processEvents();

    QListWidget *pSessionListWidget = mainWindow.findChild<QListWidget *>(QString::fromUtf8("sessionListWidget"));
    QListWidget *pSnippetListWidget = mainWindow.findChild<QListWidget *>(QString::fromUtf8("snippetListWidget"));
    QAction *pMoveAction = mainWindow.findChild<QAction *>(QString::fromUtf8("moveSnippetAction"));
    if (nullptr == pSessionListWidget || nullptr == pSnippetListWidget || nullptr == pMoveAction)
    {
        PrintError(QString::fromUtf8("Batch move anchors are missing."));
        return 782;
    }

    if (!SelectListItemById(pSessionListWidget, sourceSession.id()) || !SelectListItemsByIds(pSnippetListWidget, vecSourceSnippetIds))
    {
        PrintError(QString::fromUtf8("Failed to prepare move verification selection."));
        return 783;
    }

    QTimer::singleShot(0, [&]() {
        const QList<QWidget *> vecTopLevelWidgets = QApplication::topLevelWidgets();
        for (int i = 0; i < vecTopLevelWidgets.size(); ++i)
        {
            QInputDialog *pDialog = qobject_cast<QInputDialog *>(vecTopLevelWidgets.at(i));
            if (nullptr == pDialog)
                continue;

            const QStringList vecItems = pDialog->comboBoxItems();
            for (int j = 0; j < vecItems.size(); ++j)
            {
                if (vecItems.at(j).contains(QString::fromUtf8("Move Target")))
                {
                    pDialog->setTextValue(vecItems.at(j));
                    pDialog->accept();
                    return;
                }
            }
        }
    });

    pMoveAction->trigger();
    QApplication::processEvents(QEventLoop::AllEvents, 100);

    const QVector<QCSnippet> vecTargetSnippets = snippetService.listSnippetsBySession(targetSession.id());
    if (vecTargetSnippets.size() != vecSourceSnippetIds.size())
    {
        PrintError(QString::fromUtf8("Batch move target count verification failed."));
        return 784;
    }

    if (!SelectListItemById(pSessionListWidget, targetSession.id()) || pSnippetListWidget->selectedItems().size() != vecSourceSnippetIds.size())
    {
        PrintError(QString::fromUtf8("Batch move selection retention verification failed."));
        return 785;
    }

    PrintInfo(QString::fromUtf8("Batch move verification passed."));
    return 0;
}

int RunSearchHistoryVerificationWorkflow()
{
    const QString strDatabasePath = BuildScopedTempPath(QString::fromUtf8("search_history"), QString::fromUtf8("verify.sqlite"));
    QFile::remove(strDatabasePath);

    QCDatabaseManager databaseManager;
    if (!databaseManager.open(strDatabasePath) || !databaseManager.initialize())
    {
        PrintError(databaseManager.lastError());
        return 786;
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

    if (!settingsService.setSnippetSearchHistory(QStringList() << QString::fromUtf8("alpha") << QString::fromUtf8("beta")))
    {
        PrintError(settingsService.lastError());
        return 787;
    }

    QCMainWindow mainWindow(&sessionService, &snippetService, &tagService, &aiService, &aiProcessService, &settingsService, &mdExportService, &screenCaptureService);
    mainWindow.show();
    QApplication::processEvents();

    QComboBox *pHistoryCombo = mainWindow.findChild<QComboBox *>(QString::fromUtf8("searchHistoryComboBox"));
    QPushButton *pClearHistoryButton = mainWindow.findChild<QPushButton *>(QString::fromUtf8("clearSearchHistoryButton"));
    if (nullptr == pHistoryCombo || nullptr == pClearHistoryButton)
    {
        PrintError(QString::fromUtf8("Search history anchors are missing."));
        return 788;
    }

    if (!pClearHistoryButton->isEnabled() || pHistoryCombo->count() < 2)
    {
        PrintError(QString::fromUtf8("Search history preload verification failed."));
        return 789;
    }

    pClearHistoryButton->click();
    QApplication::processEvents();

    QStringList vecHistory;
    if (!settingsService.getSnippetSearchHistory(&vecHistory))
    {
        PrintError(settingsService.lastError());
        return 790;
    }

    if (!vecHistory.isEmpty() || pClearHistoryButton->isEnabled())
    {
        PrintError(QString::fromUtf8("Search history clear verification failed."));
        return 791;
    }

    PrintInfo(QString::fromUtf8("Search history verification passed."));
    return 0;
}

int RunTagClearActionVerificationWorkflow()
{
    const QString strDatabasePath = BuildScopedTempPath(QString::fromUtf8("tag_clear_action"), QString::fromUtf8("verify.sqlite"));
    QFile::remove(strDatabasePath);

    QCDatabaseManager databaseManager;
    if (!databaseManager.open(strDatabasePath) || !databaseManager.initialize())
    {
        PrintError(databaseManager.lastError());
        return 792;
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

    QCStudySession session;
    session.setTitle(QString::fromUtf8("Tag Clear Action Session"));
    if (!sessionService.createSession(&session))
    {
        PrintError(sessionService.lastError());
        return 793;
    }

    QCSnippet snippetA;
    snippetA.setSessionId(session.id());
    snippetA.setCaptureType(QCCaptureType::ManualCaptureType);
    snippetA.setTitle(QString::fromUtf8("Tag Action A"));
    snippetA.setContentText(QString::fromUtf8("content A"));
    snippetA.setNoteKind(QCNoteKind::ConceptNoteKind);
    snippetA.setNoteLevel(QCNoteLevel::ImportantNoteLevel);
    if (!snippetService.createTextSnippet(&snippetA))
    {
        PrintError(snippetService.lastError());
        return 794;
    }

    QCSnippet snippetB = snippetA;
    snippetB.setId(0);
    snippetB.setTitle(QString::fromUtf8("Tag Action B"));
    if (!snippetService.createTextSnippet(&snippetB))
    {
        PrintError(snippetService.lastError());
        return 795;
    }

    QCTag tag;
    tag.setName(QString::fromUtf8("to-clear"));
    if (!tagService.createTag(&tag))
    {
        PrintError(tagService.lastError());
        return 796;
    }

    if (!tagService.replaceSnippetTags(snippetA.id(), QVector<qint64>() << tag.id())
        || !tagService.replaceSnippetTags(snippetB.id(), QVector<qint64>() << tag.id()))
    {
        PrintError(tagService.lastError());
        return 797;
    }

    QCMainWindow mainWindow(&sessionService, &snippetService, &tagService, &aiService, &aiProcessService, &settingsService, &mdExportService, &screenCaptureService);
    mainWindow.show();
    QApplication::processEvents();

    QListWidget *pSessionListWidget = mainWindow.findChild<QListWidget *>(QString::fromUtf8("sessionListWidget"));
    QListWidget *pSnippetListWidget = mainWindow.findChild<QListWidget *>(QString::fromUtf8("snippetListWidget"));
    QAction *pClearTagsAction = mainWindow.findChild<QAction *>(QString::fromUtf8("clearSnippetTagsAction"));
    if (nullptr == pSessionListWidget || nullptr == pSnippetListWidget || nullptr == pClearTagsAction)
    {
        PrintError(QString::fromUtf8("Tag clear action anchors are missing."));
        return 798;
    }

    if (!SelectListItemById(pSessionListWidget, session.id()) || !SelectListItemsByIds(pSnippetListWidget, QVector<qint64>() << snippetA.id() << snippetB.id()))
    {
        PrintError(QString::fromUtf8("Failed to prepare tag clear selection."));
        return 799;
    }

    QSharedPointer<int> pAttempts(new int(0));
    QSharedPointer<std::function<void()>> pAutoAccept(new std::function<void()>());
    *pAutoAccept = [pAttempts, pAutoAccept]() {
        ++(*pAttempts);
        QMessageBox *pDialog = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
        if (nullptr != pDialog)
        {
            if (QAbstractButton *pYesButton = pDialog->button(QMessageBox::Yes))
                pYesButton->click();
            else
                pDialog->accept();
            return;
        }

        if (*pAttempts < 80)
            QTimer::singleShot(10, [pAutoAccept]() { (*pAutoAccept)(); });
    };

    QTimer::singleShot(0, [pAutoAccept]() { (*pAutoAccept)(); });
    pClearTagsAction->trigger();
    QApplication::processEvents(QEventLoop::AllEvents, 200);

    if (!tagService.listTagsBySnippet(snippetA.id()).isEmpty() || !tagService.listTagsBySnippet(snippetB.id()).isEmpty())
    {
        PrintError(QString::fromUtf8("Tag clear action verification failed."));
        return 800;
    }

    PrintInfo(QString::fromUtf8("Tag clear action verification passed."));
    return 0;
}

int RunTagBatchVerificationWorkflow()
{
    QString strTempDirectory = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (strTempDirectory.isEmpty())
        strTempDirectory = QDir::tempPath();

    const QString strDatabasePath = BuildScopedTempPath(QString::fromUtf8("tag_batch"), QString::fromUtf8("verify.sqlite"));
    QFile::remove(strDatabasePath);

    QCDatabaseManager databaseManager;
    if (!databaseManager.open(strDatabasePath) || !databaseManager.initialize())
    {
        PrintError(databaseManager.lastError());
        return 720;
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

    QCStudySession session;
    session.setTitle(QString::fromUtf8("Tag Batch Session"));
    if (!sessionService.createSession(&session))
    {
        PrintError(sessionService.lastError());
        return 721;
    }

    QCSnippet snippetA;
    snippetA.setSessionId(session.id());
    snippetA.setCaptureType(QCCaptureType::ManualCaptureType);
    snippetA.setTitle(QString::fromUtf8("Tag Snippet A"));
    snippetA.setContentText(QString::fromUtf8("Tag batch content A"));
    snippetA.setNoteKind(QCNoteKind::ConceptNoteKind);
    snippetA.setNoteLevel(QCNoteLevel::ImportantNoteLevel);
    if (!snippetService.createTextSnippet(&snippetA))
    {
        PrintError(snippetService.lastError());
        return 722;
    }

    QCSnippet snippetB = snippetA;
    snippetB.setId(0);
    snippetB.setTitle(QString::fromUtf8("Tag Snippet B"));
    snippetB.setContentText(QString::fromUtf8("Tag batch content B"));
    if (!snippetService.createTextSnippet(&snippetB))
    {
        PrintError(snippetService.lastError());
        return 723;
    }

    QCTag tagA; tagA.setName(QString::fromUtf8("alpha"));
    QCTag tagB; tagB.setName(QString::fromUtf8("beta"));
    if (!tagService.createTag(&tagA) || !tagService.createTag(&tagB))
    {
        PrintError(tagService.lastError());
        return 724;
    }

    if (!tagService.replaceSnippetTags(snippetA.id(), QVector<qint64>() << tagA.id() << tagB.id())
        || !tagService.replaceSnippetTags(snippetB.id(), QVector<qint64>() << tagA.id()))
    {
        PrintError(tagService.lastError());
        return 725;
    }

    QCMainWindow mainWindow(&sessionService, &snippetService, &tagService, &aiService, &aiProcessService, &settingsService, &mdExportService, &screenCaptureService);
    mainWindow.show();
    QApplication::processEvents();

    QListWidget *pSessionListWidget = mainWindow.findChild<QListWidget *>(QString::fromUtf8("sessionListWidget"));
    QListWidget *pSnippetListWidget = mainWindow.findChild<QListWidget *>(QString::fromUtf8("snippetListWidget"));
    QAction *pManageTagsAction = mainWindow.findChild<QAction *>(QString::fromUtf8("manageTagsAction"));
    QAction *pClearTagsAction = mainWindow.findChild<QAction *>(QString::fromUtf8("clearSnippetTagsAction"));
    QAction *pTagLibraryAction = mainWindow.findChild<QAction *>(QString::fromUtf8("tagLibraryAction"));
    if (nullptr == pSessionListWidget || nullptr == pSnippetListWidget || nullptr == pManageTagsAction || nullptr == pClearTagsAction || nullptr == pTagLibraryAction)
    {
        PrintError(QString::fromUtf8("Tag batch anchors are missing."));
        return 726;
    }

    if (!SelectListItemById(pSessionListWidget, session.id()))
    {
        PrintError(QString::fromUtf8("Failed to select tag batch session."));
        return 727;
    }

    SelectMultipleSnippetItemsById(pSnippetListWidget, QVector<qint64>() << snippetA.id() << snippetB.id());
    QApplication::processEvents();
    if (!pManageTagsAction->isEnabled() || !pClearTagsAction->isEnabled() || !pTagLibraryAction->isEnabled())
    {
        PrintError(QString::fromUtf8("Tag batch action state verification failed."));
        return 728;
    }

    if (!tagService.replaceSnippetTags(snippetA.id(), QVector<qint64>()) || !tagService.replaceSnippetTags(snippetB.id(), QVector<qint64>()))
    {
        PrintError(tagService.lastError());
        return 729;
    }

    if (!tagService.listTagsBySnippet(snippetA.id()).isEmpty() || !tagService.listTagsBySnippet(snippetB.id()).isEmpty())
    {
        PrintError(QString::fromUtf8("Tag clear verification failed."));
        return 730;
    }

    PrintInfo(QString::fromUtf8("Tag batch verification passed."));
    return 0;
}

int RunSearchFilterVerificationWorkflow()
{
    QString strTempDirectory = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (strTempDirectory.isEmpty())
        strTempDirectory = QDir::tempPath();

    const QString strDatabasePath = QDir(strTempDirectory).filePath(QString::fromUtf8("qtclip_search_filter_verify.sqlite"));
    QFile::remove(strDatabasePath);

    QCDatabaseManager databaseManager;
    if (!databaseManager.open(strDatabasePath) || !databaseManager.initialize())
    {
        PrintError(databaseManager.lastError());
        return 731;
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

    QCStudySession session;
    session.setTitle(QString::fromUtf8("Search Filter Session"));
    if (!sessionService.createSession(&session))
    {
        PrintError(sessionService.lastError());
        return 732;
    }

    QCSnippet textSnippet;
    textSnippet.setSessionId(session.id());
    textSnippet.setCaptureType(QCCaptureType::ManualCaptureType);
    textSnippet.setTitle(QString::fromUtf8("Alpha Topic"));
    textSnippet.setNote(QString::fromUtf8("Alpha note"));
    textSnippet.setContentText(QString::fromUtf8("Searchable alpha content"));
    textSnippet.setSummary(QString::fromUtf8("Alpha summary"));
    textSnippet.setNoteKind(QCNoteKind::ConceptNoteKind);
    textSnippet.setNoteLevel(QCNoteLevel::ImportantNoteLevel);
    if (!snippetService.createTextSnippet(&textSnippet))
    {
        PrintError(snippetService.lastError());
        return 733;
    }

    QCSnippet imageSnippet;
    imageSnippet.setSessionId(session.id());
    imageSnippet.setType(QCSnippetType::ImageSnippetType);
    imageSnippet.setCaptureType(QCCaptureType::ManualCaptureType);
    imageSnippet.setTitle(QString::fromUtf8("Beta Image"));
    imageSnippet.setNote(QString::fromUtf8("Beta note"));
    imageSnippet.setSummary(QString::fromUtf8("Beta summary"));
    imageSnippet.setNoteKind(QCNoteKind::ReviewNoteKind);
    imageSnippet.setNoteLevel(QCNoteLevel::ImportantNoteLevel);
    QCAttachment attachment;
    attachment.setFilePath(QDir(strTempDirectory).filePath(QString::fromUtf8("qtclip_search_filter_verify.png")));
    CreateVerificationImageFile(attachment.filePath());
    attachment.setFileName(QFileInfo(attachment.filePath()).fileName());
    attachment.setMimeType(QString::fromUtf8("image/png"));
    attachment.setCreatedAt(QDateTime::currentDateTimeUtc());
    if (!snippetService.createImageSnippetWithPrimaryAttachment(&imageSnippet, &attachment))
    {
        PrintError(snippetService.lastError());
        return 734;
    }

    QCMainWindow mainWindow(&sessionService, &snippetService, &tagService, &aiService, &aiProcessService, &settingsService, &mdExportService, &screenCaptureService);
    mainWindow.show();
    QApplication::processEvents();

    QListWidget *pSessionListWidget = mainWindow.findChild<QListWidget *>(QString::fromUtf8("sessionListWidget"));
    QListWidget *pSnippetListWidget = mainWindow.findChild<QListWidget *>(QString::fromUtf8("snippetListWidget"));
    QLineEdit *pSearchLineEdit = mainWindow.findChild<QLineEdit *>(QString::fromUtf8("snippetSearchLineEdit"));
    QComboBox *pSearchHistoryComboBox = mainWindow.findChild<QComboBox *>(QString::fromUtf8("searchHistoryComboBox"));
    QPushButton *pClearHistoryButton = mainWindow.findChild<QPushButton *>(QString::fromUtf8("clearSearchHistoryButton"));
    QComboBox *pTypeFilterComboBox = mainWindow.findChild<QComboBox *>(QString::fromUtf8("snippetTypeFilterComboBox"));
    QLabel *pViewSummaryLabel = mainWindow.findChild<QLabel *>(QString::fromUtf8("viewSummaryLabel"));
    if (nullptr == pSessionListWidget || nullptr == pSnippetListWidget || nullptr == pSearchLineEdit || nullptr == pSearchHistoryComboBox || nullptr == pClearHistoryButton || nullptr == pTypeFilterComboBox || nullptr == pViewSummaryLabel)
    {
        PrintError(QString::fromUtf8("Search filter anchors are missing."));
        return 735;
    }

    if (!SelectListItemById(pSessionListWidget, session.id()))
    {
        PrintError(QString::fromUtf8("Failed to select search filter session."));
        return 736;
    }

    if (!settingsService.setSnippetSearchHistory(QStringList() << QString::fromUtf8("Alpha") << QString::fromUtf8("Beta")))
    {
        PrintError(settingsService.lastError());
        return 737;
    }

    mainWindow.close();
    QCMainWindow refreshedWindow(&sessionService, &snippetService, &tagService, &aiService, &aiProcessService, &settingsService, &mdExportService, &screenCaptureService);
    refreshedWindow.show();
    QApplication::processEvents();

    pSessionListWidget = refreshedWindow.findChild<QListWidget *>(QString::fromUtf8("sessionListWidget"));
    pSnippetListWidget = refreshedWindow.findChild<QListWidget *>(QString::fromUtf8("snippetListWidget"));
    pSearchLineEdit = refreshedWindow.findChild<QLineEdit *>(QString::fromUtf8("snippetSearchLineEdit"));
    pSearchHistoryComboBox = refreshedWindow.findChild<QComboBox *>(QString::fromUtf8("searchHistoryComboBox"));
    pClearHistoryButton = refreshedWindow.findChild<QPushButton *>(QString::fromUtf8("clearSearchHistoryButton"));
    pTypeFilterComboBox = refreshedWindow.findChild<QComboBox *>(QString::fromUtf8("snippetTypeFilterComboBox"));
    pViewSummaryLabel = refreshedWindow.findChild<QLabel *>(QString::fromUtf8("viewSummaryLabel"));
    if (nullptr == pSessionListWidget || nullptr == pSnippetListWidget || nullptr == pSearchLineEdit || nullptr == pSearchHistoryComboBox || nullptr == pClearHistoryButton || nullptr == pTypeFilterComboBox || nullptr == pViewSummaryLabel)
    {
        PrintError(QString::fromUtf8("Refreshed search filter anchors are missing."));
        return 738;
    }

    if (!SelectListItemById(pSessionListWidget, session.id()))
    {
        PrintError(QString::fromUtf8("Failed to reselect search filter session."));
        return 742;
    }

    if (pSearchHistoryComboBox->count() < 3 || !pClearHistoryButton->isEnabled())
    {
        PrintError(QString::fromUtf8("Search history population verification failed."));
        return 740;
    }

    pSearchHistoryComboBox->setCurrentIndex(1);
    QApplication::processEvents();
    if (pSnippetListWidget->count() != 1 || !pSearchLineEdit->text().contains(QString::fromUtf8("Alpha")) || !pViewSummaryLabel->text().contains(QString::fromUtf8("Alpha")))
    {
        PrintError(QString::fromUtf8("Search filter text verification failed."));
        return 741;
    }

    pTypeFilterComboBox->setCurrentIndex(2);
    QApplication::processEvents();
    if (pSnippetListWidget->count() != 0)
    {
        PrintError(QString::fromUtf8("Type filter verification failed."));
        return 739;
    }

    settingsService.setSnippetSearchHistory(QStringList());
    PrintInfo(QString::fromUtf8("Search filter verification passed."));
    return 0;
}

int RunAiRetryVerificationWorkflow()
{
    QString strTempDirectory = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (strTempDirectory.isEmpty())
        strTempDirectory = QDir::tempPath();

    const QString strDatabasePath = QDir(strTempDirectory).filePath(QString::fromUtf8("qtclip_ai_retry_verify.sqlite"));
    QFile::remove(strDatabasePath);

    QCDatabaseManager databaseManager;
    if (!databaseManager.open(strDatabasePath) || !databaseManager.initialize())
    {
        PrintError(databaseManager.lastError());
        return 711;
    }

    QCSessionRepositorySqlite sessionRepository(&databaseManager);
    QCSnippetRepositorySqlite snippetRepository(&databaseManager);
    QCAiRecordRepositorySqlite aiRecordRepository(&databaseManager);
    QCSettingsRepositorySqlite settingsRepository(&databaseManager);
    QCSessionService sessionService(&sessionRepository);
    QCSnippetService snippetService(&snippetRepository);
    QCAiService aiService(&aiRecordRepository);
    QCSettingsService settingsService(&settingsRepository);
    QCAiProcessService aiProcessService(&sessionService, &snippetService, &aiService, &settingsService);

    QCAiRuntimeSettings invalidSettings;
    invalidSettings.m_bUseMockProvider = false;
    invalidSettings.m_strBaseUrl = QString::fromUtf8("https://invalid.local/v1");
    invalidSettings.m_strApiKey.clear();
    invalidSettings.m_strModel = QString::fromUtf8("invalid-model");
    QCAiConnectionTestResult connectionTestResult;
    if (aiProcessService.testConnection(invalidSettings, &connectionTestResult))
    {
        PrintError(QString::fromUtf8("AI retry verification expected a failed connection test."));
        return 712;
    }

    if (!connectionTestResult.m_strMessage.contains(QString::fromUtf8("Mode:"))
        || !connectionTestResult.m_strMessage.contains(QString::fromUtf8("Endpoint:"))
        || !connectionTestResult.m_strMessage.contains(QString::fromUtf8("Message:")))
    {
        PrintError(QString::fromUtf8("AI diagnostic message verification failed."));
        return 713;
    }

    QCAiRuntimeSettings mockSettings;
    mockSettings.m_bUseMockProvider = true;
    mockSettings.m_strModel = QString::fromUtf8("mock-summary-v1");
    if (!settingsService.saveAiSettings(mockSettings))
    {
        PrintError(settingsService.lastError());
        return 714;
    }

    QCStudySession session;
    session.setTitle(QString::fromUtf8("AI Retry Verify Session"));
    if (!sessionService.createSession(&session))
    {
        PrintError(sessionService.lastError());
        return 715;
    }

    QCSnippet snippet;
    snippet.setSessionId(session.id());
    snippet.setCaptureType(QCCaptureType::ManualCaptureType);
    snippet.setTitle(QString::fromUtf8("Retry Snippet"));
    snippet.setContentText(QString::fromUtf8("Verify AI summarize stability."));
    snippet.setNoteKind(QCNoteKind::ConceptNoteKind);
    snippet.setNoteLevel(QCNoteLevel::ImportantNoteLevel);
    if (!snippetService.createTextSnippet(&snippet))
    {
        PrintError(snippetService.lastError());
        return 716;
    }

    if (!aiProcessService.summarizeSnippet(snippet.id()))
    {
        PrintError(aiProcessService.lastError());
        return 717;
    }

    QCSnippet summarizedSnippet;
    if (!snippetService.getSnippetById(snippet.id(), &summarizedSnippet) || summarizedSnippet.summary().trimmed().isEmpty())
    {
        PrintError(QString::fromUtf8("AI summarize success-path verification failed."));
        return 718;
    }

    PrintInfo(QString::fromUtf8("AI retry verification passed."));
    PrintInfo(QString::fromUtf8("Failure diagnostic preview: %1").arg(BuildPreviewText(connectionTestResult.m_strMessage)));
    return 0;
}

int RunExportVerificationWorkflow()
{
    QString strDatabasePath = BuildScopedTempPath(QString::fromUtf8("export_verify"), QString::fromUtf8("verify.sqlite"));
    QString strImagePath = BuildScopedTempPath(QString::fromUtf8("export_verify"), QString::fromUtf8("verify.png"));
    QString strOutputPath = BuildScopedTempPath(QString::fromUtf8("export_verify"), QString::fromUtf8("verify.md"));
    QFile::remove(strDatabasePath);
    QFile::remove(strImagePath);
    QFile::remove(strOutputPath);

    QCDatabaseManager databaseManager;
    if (!databaseManager.open(strDatabasePath) || !databaseManager.initialize())
    {
        PrintError(databaseManager.lastError());
        return 719;
    }

    if (CreateVerificationImageFile(strImagePath).isEmpty())
    {
        PrintError(QString::fromUtf8("Failed to create export verification image file."));
        return 7191;
    }

    QCSessionRepositorySqlite sessionRepository(&databaseManager);
    QCSnippetRepositorySqlite snippetRepository(&databaseManager);
    QCAiRecordRepositorySqlite aiRecordRepository(&databaseManager);
    QCSessionService sessionService(&sessionRepository);
    QCSnippetService snippetService(&snippetRepository);
    QCAiService aiService(&aiRecordRepository);
    QCExportDataService exportDataService(&sessionService, &snippetService, &aiService);
    QCMdExportRenderer mdExportRenderer;
    QCMdExportService mdExportService(&exportDataService, &mdExportRenderer);

    QCStudySession session;
    session.setTitle(QString::fromUtf8("Export Verify Session"));
    session.setCourseName(QString::fromUtf8("Verification"));
    session.setDescription(QString::fromUtf8("Export verification context."));
    if (!sessionService.createSession(&session))
    {
        PrintError(sessionService.lastError());
        return 7192;
    }

    QCSnippet textSnippet;
    textSnippet.setSessionId(session.id());
    textSnippet.setCaptureType(QCCaptureType::ManualCaptureType);
    textSnippet.setTitle(QString::fromUtf8("Export Verify Text"));
    textSnippet.setNote(QString::fromUtf8("Export note"));
    textSnippet.setContentText(QString::fromUtf8("Export content body."));
    textSnippet.setSummary(QString::fromUtf8("Export summary."));
    textSnippet.setNoteKind(QCNoteKind::ConceptNoteKind);
    textSnippet.setNoteLevel(QCNoteLevel::ImportantNoteLevel);
    if (!snippetService.createTextSnippet(&textSnippet))
    {
        PrintError(snippetService.lastError());
        return 7193;
    }

    QCSnippet imageSnippet;
    imageSnippet.setSessionId(session.id());
    imageSnippet.setType(QCSnippetType::ImageSnippetType);
    imageSnippet.setCaptureType(QCCaptureType::ImportCaptureType);
    imageSnippet.setTitle(QString::fromUtf8("Export Verify Image"));
    imageSnippet.setNote(QString::fromUtf8("Export image note"));
    imageSnippet.setNoteKind(QCNoteKind::ReviewNoteKind);
    imageSnippet.setNoteLevel(QCNoteLevel::ImportantNoteLevel);
    QCAttachment attachment;
    attachment.setFilePath(strImagePath);
    attachment.setFileName(QFileInfo(strImagePath).fileName());
    attachment.setMimeType(QString::fromUtf8("image/png"));
    attachment.setCreatedAt(QDateTime::currentDateTimeUtc());
    if (!snippetService.createImageSnippetWithPrimaryAttachment(&imageSnippet, &attachment))
    {
        PrintError(snippetService.lastError());
        return 7194;
    }

    QCAiRecord sessionRecord;
    sessionRecord.setSessionId(session.id());
    sessionRecord.setTaskType(QCAiTaskType::SessionSummaryTask);
    sessionRecord.setProviderName(QString::fromUtf8("mock"));
    sessionRecord.setModelName(QString::fromUtf8("mock-summary-v1"));
    sessionRecord.setPromptText(QString::fromUtf8("Summarize the current study session for export verification."));
    sessionRecord.setResponseText(QString::fromUtf8("Session export summary."));
    sessionRecord.setStatus(QString::fromUtf8("completed"));
    sessionRecord.setCreatedAt(QDateTime::currentDateTimeUtc());
    if (!aiService.createAiRecord(&sessionRecord))
    {
        PrintError(aiService.lastError());
        return 7195;
    }

    QCMdExportPreview exportPreview;
    if (!mdExportService.buildExportPreview(session.id(), &exportPreview))
    {
        PrintError(mdExportService.lastError());
        return 7196;
    }

    if (exportPreview.m_nSnippetCount != 2 || exportPreview.m_nImageSnippetCount != 1 || !exportPreview.m_bHasSessionSummary)
    {
        PrintError(QString::fromUtf8("Export preview verification failed."));
        return 7197;
    }

    if (!mdExportService.exportSessionToFile(session.id(), strOutputPath))
    {
        PrintError(mdExportService.lastError());
        return 7198;
    }

    QFile outputFile(strOutputPath);
    if (!outputFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        PrintError(QString::fromUtf8("Export output file could not be opened."));
        return 7199;
    }

    const QString strMarkdown = QString::fromUtf8(outputFile.readAll());
    if (!strMarkdown.contains(QString::fromUtf8("## Session Details"))
        || !strMarkdown.contains(QString::fromUtf8("## Overview"))
        || !strMarkdown.contains(QString::fromUtf8("## Snippet Timeline"))
        || !strMarkdown.contains(QString::fromUtf8("Export Verify Text"))
        || !strMarkdown.contains(QString::fromUtf8("Export Verify Image")))
    {
        PrintError(QString::fromUtf8("Export markdown structure verification failed."));
        return 7200;
    }

    PrintInfo(QString::fromUtf8("Export verification passed."));
    PrintInfo(QString::fromUtf8("Export output: %1").arg(strOutputPath));
    return 0;
}

int RunLiveAiVerificationWorkflow()
{
    const QString strDatabasePath = ResolveAppDataPath();

    QCDatabaseManager databaseManager;
    if (!databaseManager.open(strDatabasePath) || !databaseManager.initialize())
    {
        PrintError(databaseManager.lastError());
        return 767;
    }

    QCSessionRepositorySqlite sessionRepository(&databaseManager);
    QCSnippetRepositorySqlite snippetRepository(&databaseManager);
    QCAiRecordRepositorySqlite aiRecordRepository(&databaseManager);
    QCSettingsRepositorySqlite settingsRepository(&databaseManager);
    QCSessionService sessionService(&sessionRepository);
    QCSnippetService snippetService(&snippetRepository);
    QCAiService aiService(&aiRecordRepository);
    QCSettingsService settingsService(&settingsRepository);
    QCAiProcessService aiProcessService(&sessionService, &snippetService, &aiService, &settingsService);

    QCAiRuntimeSettings aiSettings;
    if (!settingsService.loadAiSettings(&aiSettings))
    {
        PrintError(settingsService.lastError());
        return 768;
    }

    if (aiSettings.m_bUseMockProvider)
    {
        PrintInfo(QString::fromUtf8("Live AI verification skipped: current active profile uses the mock provider."));
        return 0;
    }

    if (aiSettings.m_strBaseUrl.trimmed().isEmpty() || aiSettings.m_strApiKey.trimmed().isEmpty() || aiSettings.m_strModel.trimmed().isEmpty())
    {
        PrintInfo(QString::fromUtf8("Live AI verification skipped: active real provider configuration is incomplete."));
        return 0;
    }

    QCAiConnectionTestResult firstResult;
    if (!aiProcessService.testConnection(aiSettings, &firstResult) || !firstResult.m_bSuccess)
    {
        PrintError(firstResult.m_strMessage.trimmed().isEmpty()
            ? QString::fromUtf8("Live AI verification failed on attempt 1.")
            : firstResult.m_strMessage);
        return 769;
    }

    QCAiConnectionTestResult secondResult;
    if (!aiProcessService.testConnection(aiSettings, &secondResult) || !secondResult.m_bSuccess)
    {
        PrintError(secondResult.m_strMessage.trimmed().isEmpty()
            ? QString::fromUtf8("Live AI verification failed on attempt 2.")
            : secondResult.m_strMessage);
        return 770;
    }

    PrintInfo(QString::fromUtf8("Live AI verification passed."));
    PrintInfo(firstResult.m_strMessage);
    PrintInfo(secondResult.m_strMessage);
    return 0;
}

int RunLanguageVerificationWorkflow()
{
    QString strTempDirectory = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (strTempDirectory.isEmpty())
        strTempDirectory = QDir::tempPath();

    const QString strDatabasePath = QDir(strTempDirectory).filePath(QString::fromUtf8("qtclip_language_verify.sqlite"));
    QFile::remove(strDatabasePath);

    QCDatabaseManager databaseManager;
    if (!databaseManager.open(strDatabasePath) || !databaseManager.initialize())
    {
        PrintError(databaseManager.lastError());
        return 719;
    }

    QCSessionRepositorySqlite sessionRepository(&databaseManager);
    QCSnippetRepositorySqlite snippetRepository(&databaseManager);
    QCAiRecordRepositorySqlite aiRecordRepository(&databaseManager);
    QCSettingsRepositorySqlite settingsRepository(&databaseManager);
    QCSessionService sessionService(&sessionRepository);
    QCSnippetService snippetService(&snippetRepository);
    QCAiService aiService(&aiRecordRepository);
    QCSettingsService settingsService(&settingsRepository);
    QCAiProcessService aiProcessService(&sessionService, &snippetService, &aiService, &settingsService);

    if (settingsService.defaultAppLanguage() != QString::fromUtf8("zh-CN"))
    {
        PrintError(QString::fromUtf8("Default app language verification failed."));
        return 720;
    }

    QString strLoadedLanguage;
    if (!settingsService.getAppLanguage(&strLoadedLanguage) || strLoadedLanguage != QString::fromUtf8("zh-CN"))
    {
        PrintError(settingsService.lastError().isEmpty() ? QString::fromUtf8("Initial app language load verification failed.") : settingsService.lastError());
        return 721;
    }

    QCAiRuntimeSettings aiSettings;
    aiSettings.m_bUseMockProvider = true;
    aiSettings.m_strModel = QString::fromUtf8("mock-summary-v1");
    if (!settingsService.saveAiSettings(aiSettings))
    {
        PrintError(settingsService.lastError());
        return 722;
    }

    QCStudySession session;
    session.setTitle(QString::fromUtf8("Language Verify Session"));
    if (!sessionService.createSession(&session))
    {
        PrintError(sessionService.lastError());
        return 723;
    }

    QCSnippet snippet;
    snippet.setSessionId(session.id());
    snippet.setCaptureType(QCCaptureType::ManualCaptureType);
    snippet.setTitle(QString::fromUtf8("Language Verify Snippet"));
    snippet.setContentText(QString::fromUtf8("Language verification content."));
    snippet.setNoteKind(QCNoteKind::ConceptNoteKind);
    snippet.setNoteLevel(QCNoteLevel::ImportantNoteLevel);
    if (!snippetService.createTextSnippet(&snippet))
    {
        PrintError(snippetService.lastError());
        return 724;
    }

    QCAiTaskExecutionContext executionContext;
    if (!aiProcessService.prepareSnippetSummary(snippet.id(), &executionContext)
        || !executionContext.m_aiRequest.m_strSystemPrompt.contains(QString::fromUtf8("??")))
    {
        PrintError(QString::fromUtf8("Chinese AI prompt verification failed."));
        return 725;
    }

    QCExportDataService exportDataService(&sessionService, &snippetService, &aiService);
    QCMdExportRenderer mdExportRenderer;
    QCMdExportService mdExportService(&exportDataService, &mdExportRenderer);
    QCScreenCaptureService screenCaptureService(&settingsService);
    QCTagRepositorySqlite tagRepository(&databaseManager);
    QCTagService tagService(&tagRepository);
    QCMainWindow chineseMainWindow(&sessionService,
                                   &snippetService,
                                   &tagService,
                                   &aiService,
                                   &aiProcessService,
                                   &settingsService,
                                   &mdExportService,
                                   &screenCaptureService);

    QAction *pChineseSettingsAction = chineseMainWindow.findChild<QAction *>(QString::fromUtf8("aiSettingsAction"));
    QLineEdit *pChineseSearchLineEdit = chineseMainWindow.findChild<QLineEdit *>(QString::fromUtf8("snippetSearchLineEdit"));
    QLabel *pChineseContextLabel = chineseMainWindow.findChild<QLabel *>(QString::fromUtf8("selectionContextLabel"));
    if (nullptr == pChineseSettingsAction
        || pChineseSettingsAction->text() != QString::fromUtf8("??")
        || nullptr == pChineseSearchLineEdit
        || !pChineseSearchLineEdit->placeholderText().contains(QString::fromUtf8("??"))
        || nullptr == pChineseContextLabel
        || !pChineseContextLabel->text().contains(QCUiText(QString::fromUtf8("未选择会话"), QString::fromUtf8("no session selected"))))
    {
        PrintError(QString::fromUtf8("Chinese UI text verification failed."));
        return 7251;
    }

    if (!settingsService.setAppLanguage(QString::fromUtf8("en-US")))
    {
        PrintError(settingsService.lastError());
        return 726;
    }

    if (!settingsService.getAppLanguage(&strLoadedLanguage) || strLoadedLanguage != QString::fromUtf8("en-US"))
    {
        PrintError(QString::fromUtf8("English app language round-trip verification failed."));
        return 727;
    }

    if (!aiProcessService.prepareSnippetSummary(snippet.id(), &executionContext)
        || !executionContext.m_aiRequest.m_strSystemPrompt.contains(QString::fromUtf8("plain English"), Qt::CaseInsensitive))
    {
        PrintError(QString::fromUtf8("English AI prompt verification failed."));
        return 728;
    }

    QCMainWindow englishMainWindow(&sessionService,
                                   &snippetService,
                                   &tagService,
                                   &aiService,
                                   &aiProcessService,
                                   &settingsService,
                                   &mdExportService,
                                   &screenCaptureService);
    QAction *pEnglishSettingsAction = englishMainWindow.findChild<QAction *>(QString::fromUtf8("aiSettingsAction"));
    QLineEdit *pEnglishSearchLineEdit = englishMainWindow.findChild<QLineEdit *>(QString::fromUtf8("snippetSearchLineEdit"));
    QLabel *pEnglishContextLabel = englishMainWindow.findChild<QLabel *>(QString::fromUtf8("selectionContextLabel"));
    if (nullptr == pEnglishSettingsAction
        || pEnglishSettingsAction->text() != QString::fromUtf8("Settings")
        || nullptr == pEnglishSearchLineEdit
        || !pEnglishSearchLineEdit->placeholderText().contains(QString::fromUtf8("Search"))
        || nullptr == pEnglishContextLabel
        || !pEnglishContextLabel->text().contains(QString::fromUtf8("Current context")))
    {
        PrintError(QString::fromUtf8("English UI text verification failed."));
        return 7281;
    }

    PrintInfo(QString::fromUtf8("Language verification passed."));
    PrintInfo(QString::fromUtf8("Current language: %1").arg(strLoadedLanguage));
    return 0;
}


int RunAiStatusVerificationWorkflow()
{
    QString strTempDirectory = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (strTempDirectory.isEmpty())
        strTempDirectory = QDir::tempPath();

    const QString strDatabasePath = BuildScopedTempPath(QString::fromUtf8("ai_status"), QString::fromUtf8("verify.sqlite"));
    QFile::remove(strDatabasePath);

    QCDatabaseManager databaseManager;
    if (!databaseManager.open(strDatabasePath) || !databaseManager.initialize())
    {
        PrintError(databaseManager.lastError());
        return 740;
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
    aiSettings.m_strModel = QString::fromUtf8("mock-summary-v1");
    if (!settingsService.saveAiSettings(aiSettings))
    {
        PrintError(settingsService.lastError());
        return 741;
    }

    QCStudySession session;
    session.setTitle(QString::fromUtf8("AI Status Session"));
    if (!sessionService.createSession(&session))
    {
        PrintError(sessionService.lastError());
        return 742;
    }

    QCSnippet snippet;
    snippet.setSessionId(session.id());
    snippet.setCaptureType(QCCaptureType::ManualCaptureType);
    snippet.setTitle(QString::fromUtf8("AI Status Snippet"));
    snippet.setContentText(QString::fromUtf8("Verify status label update."));
    snippet.setNoteKind(QCNoteKind::ConceptNoteKind);
    snippet.setNoteLevel(QCNoteLevel::ImportantNoteLevel);
    if (!snippetService.createTextSnippet(&snippet))
    {
        PrintError(snippetService.lastError());
        return 743;
    }

    QCMainWindow mainWindow(&sessionService, &snippetService, &tagService, &aiService, &aiProcessService, &settingsService, &mdExportService, &screenCaptureService);
    mainWindow.show();
    QApplication::processEvents();

    QListWidget *pSessionListWidget = mainWindow.findChild<QListWidget *>(QString::fromUtf8("sessionListWidget"));
    QListWidget *pSnippetListWidget = mainWindow.findChild<QListWidget *>(QString::fromUtf8("snippetListWidget"));
    QAction *pSummarizeSnippetAction = mainWindow.findChild<QAction *>(QString::fromUtf8("summarizeSnippetAction"));
    QLabel *pAiStatusLabel = mainWindow.findChild<QLabel *>(QString::fromUtf8("aiStatusLabel"));
    if (nullptr == pSessionListWidget || nullptr == pSnippetListWidget || nullptr == pSummarizeSnippetAction || nullptr == pAiStatusLabel)
    {
        PrintError(QString::fromUtf8("AI status anchors are missing."));
        return 744;
    }

    if (!SelectListItemById(pSessionListWidget, session.id()) || !SelectListItemById(pSnippetListWidget, snippet.id()))
    {
        PrintError(QString::fromUtf8("Failed to select AI status verification snippet."));
        return 745;
    }

    pSummarizeSnippetAction->trigger();
    QApplication::processEvents();
    const QString strRunningStatus = pAiStatusLabel->text();
    if (strRunningStatus.trimmed().isEmpty())
    {
        PrintError(QString::fromUtf8("AI running status did not update."));
        return 746;
    }

    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < 5000)
    {
        QApplication::processEvents(QEventLoop::AllEvents, 50);
        if (pAiStatusLabel->text().contains(QCUiText(QString::fromUtf8("已完成"), QString::fromUtf8("completed")), Qt::CaseInsensitive))
        {
            break;
        }
    }

    if (!(pAiStatusLabel->text().contains(QCUiText(QString::fromUtf8("已完成"), QString::fromUtf8("completed")), Qt::CaseInsensitive)))
    {
        PrintError(QString::fromUtf8("AI completion status verification failed."));
        return 747;
    }

    PrintInfo(QString::fromUtf8("AI status verification passed."));
    return 0;
}

int RunAiViewVerificationWorkflow()
{
    QString strTempDirectory = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (strTempDirectory.isEmpty())
        strTempDirectory = QDir::tempPath();

    const QString strDatabasePath = BuildScopedTempPath(QString::fromUtf8("ai_view"), QString::fromUtf8("verify.sqlite"));
    QFile::remove(strDatabasePath);

    QCDatabaseManager databaseManager;
    if (!databaseManager.open(strDatabasePath) || !databaseManager.initialize())
    {
        PrintError(databaseManager.lastError());
        return 748;
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
    aiSettings.m_strModel = QString::fromUtf8("mock-summary-v1");
    if (!settingsService.saveAiSettings(aiSettings))
    {
        PrintError(settingsService.lastError());
        return 749;
    }

    QCStudySession session;
    session.setTitle(QString::fromUtf8("AI View Session"));
    if (!sessionService.createSession(&session))
    {
        PrintError(sessionService.lastError());
        return 750;
    }

    QCSnippet snippet;
    snippet.setSessionId(session.id());
    snippet.setCaptureType(QCCaptureType::ManualCaptureType);
    snippet.setTitle(QString::fromUtf8("AI View Snippet"));
    snippet.setContentText(QString::fromUtf8("Verify view and copy summary workflow."));
    snippet.setNoteKind(QCNoteKind::ConceptNoteKind);
    snippet.setNoteLevel(QCNoteLevel::ImportantNoteLevel);
    if (!snippetService.createTextSnippet(&snippet))
    {
        PrintError(snippetService.lastError());
        return 751;
    }

    if (!aiProcessService.summarizeSnippet(snippet.id()) || !aiProcessService.summarizeSession(session.id()))
    {
        PrintError(aiProcessService.lastError());
        return 752;
    }

    QCMainWindow mainWindow(&sessionService, &snippetService, &tagService, &aiService, &aiProcessService, &settingsService, &mdExportService, &screenCaptureService);
    mainWindow.show();
    QApplication::processEvents();

    QListWidget *pSessionListWidget = mainWindow.findChild<QListWidget *>(QString::fromUtf8("sessionListWidget"));
    QListWidget *pSnippetListWidget = mainWindow.findChild<QListWidget *>(QString::fromUtf8("snippetListWidget"));
    QAction *pViewSnippetSummaryAction = mainWindow.findChild<QAction *>(QString::fromUtf8("viewSnippetSummaryAction"));
    QAction *pCopySnippetSummaryAction = mainWindow.findChild<QAction *>(QString::fromUtf8("copySnippetSummaryAction"));
    QAction *pViewSessionSummaryAction = mainWindow.findChild<QAction *>(QString::fromUtf8("viewSessionSummaryAction"));
    QAction *pCopySessionSummaryAction = mainWindow.findChild<QAction *>(QString::fromUtf8("copySessionSummaryAction"));
    QPlainTextEdit *pSnippetSummaryTextEdit = mainWindow.findChild<QPlainTextEdit *>(QString::fromUtf8("snippetSummaryTextEdit"));
    QPlainTextEdit *pSessionSummaryTextEdit = mainWindow.findChild<QPlainTextEdit *>(QString::fromUtf8("sessionSummaryTextEdit"));
    if (nullptr == pSessionListWidget || nullptr == pSnippetListWidget || nullptr == pViewSnippetSummaryAction || nullptr == pCopySnippetSummaryAction || nullptr == pViewSessionSummaryAction || nullptr == pCopySessionSummaryAction || nullptr == pSnippetSummaryTextEdit || nullptr == pSessionSummaryTextEdit)
    {
        PrintError(QString::fromUtf8("AI view anchors are missing."));
        return 753;
    }

    if (!SelectListItemById(pSessionListWidget, session.id()) || !SelectListItemById(pSnippetListWidget, snippet.id()))
    {
        PrintError(QString::fromUtf8("Failed to select AI view targets."));
        return 754;
    }

    if (!pViewSnippetSummaryAction->isEnabled() || !pCopySnippetSummaryAction->isEnabled() || !pViewSessionSummaryAction->isEnabled() || !pCopySessionSummaryAction->isEnabled())
    {
        PrintError(QString::fromUtf8("AI view/copy action enable verification failed."));
        return 755;
    }

    pCopySnippetSummaryAction->trigger();
    QApplication::processEvents();
    if (QGuiApplication::clipboard()->text().trimmed().isEmpty())
    {
        PrintError(QString::fromUtf8("Snippet summary copy verification failed."));
        return 756;
    }

    pViewSessionSummaryAction->trigger();
    QApplication::processEvents();
    if (!pSessionSummaryTextEdit->hasFocus() || pSnippetSummaryTextEdit->toPlainText().trimmed().isEmpty())
    {
        PrintError(QString::fromUtf8("AI summary focus verification failed."));
        return 757;
    }

    PrintInfo(QString::fromUtf8("AI view verification passed."));
    return 0;
}

int RunExportScopeVerificationWorkflow()
{
    QString strTempDirectory = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (strTempDirectory.isEmpty())
        strTempDirectory = QDir::tempPath();

    const QString strDatabasePath = BuildScopedTempPath(QString::fromUtf8("export_scope"), QString::fromUtf8("verify.sqlite"));
    const QString strImagePath = BuildScopedTempPath(QString::fromUtf8("export_scope"), QString::fromUtf8("verify.png"));
    const QString strSessionExportPath = BuildScopedTempPath(QString::fromUtf8("export_scope"), QString::fromUtf8("all.md"));
    const QString strSelectedExportPath = BuildScopedTempPath(QString::fromUtf8("export_scope"), QString::fromUtf8("selected.md"));
    QFile::remove(strDatabasePath);
    QFile::remove(strImagePath);
    QFile::remove(strSessionExportPath);
    QFile::remove(strSelectedExportPath);

    QCDatabaseManager databaseManager;
    if (!databaseManager.open(strDatabasePath) || !databaseManager.initialize())
    {
        PrintError(databaseManager.lastError());
        return 758;
    }

    CreateVerificationImageFile(strImagePath);

    QCSessionRepositorySqlite sessionRepository(&databaseManager);
    QCSnippetRepositorySqlite snippetRepository(&databaseManager);
    QCAiRecordRepositorySqlite aiRecordRepository(&databaseManager);
    QCSessionService sessionService(&sessionRepository);
    QCSnippetService snippetService(&snippetRepository);
    QCAiService aiService(&aiRecordRepository);
    QCExportDataService exportDataService(&sessionService, &snippetService, &aiService);
    QCMdExportRenderer mdExportRenderer;
    QCMdExportService mdExportService(&exportDataService, &mdExportRenderer);

    QCStudySession session;
    session.setTitle(QString::fromUtf8("Export Scope Session"));
    if (!sessionService.createSession(&session))
    {
        PrintError(sessionService.lastError());
        return 759;
    }

    QCSnippet textSnippet;
    textSnippet.setSessionId(session.id());
    textSnippet.setCaptureType(QCCaptureType::ManualCaptureType);
    textSnippet.setTitle(QString::fromUtf8("Scope Text"));
    textSnippet.setContentText(QString::fromUtf8("Visible in selected export."));
    textSnippet.setNoteKind(QCNoteKind::ConceptNoteKind);
    textSnippet.setNoteLevel(QCNoteLevel::ImportantNoteLevel);
    if (!snippetService.createTextSnippet(&textSnippet))
    {
        PrintError(snippetService.lastError());
        return 760;
    }

    QCSnippet imageSnippet;
    imageSnippet.setSessionId(session.id());
    imageSnippet.setType(QCSnippetType::ImageSnippetType);
    imageSnippet.setCaptureType(QCCaptureType::ImportCaptureType);
    imageSnippet.setTitle(QString::fromUtf8("Scope Image"));
    imageSnippet.setNote(QString::fromUtf8("Only in full export."));
    imageSnippet.setNoteKind(QCNoteKind::ReviewNoteKind);
    imageSnippet.setNoteLevel(QCNoteLevel::ImportantNoteLevel);
    QCAttachment attachment;
    attachment.setFilePath(strImagePath);
    attachment.setFileName(QFileInfo(strImagePath).fileName());
    attachment.setMimeType(QString::fromUtf8("image/png"));
    attachment.setCreatedAt(QDateTime::currentDateTimeUtc());
    if (!snippetService.createImageSnippetWithPrimaryAttachment(&imageSnippet, &attachment))
    {
        PrintError(snippetService.lastError());
        return 761;
    }

    QCMdExportPreview fullPreview;
    QCMdExportPreview selectedPreview;
    if (!mdExportService.buildExportPreview(session.id(), &fullPreview)
        || !mdExportService.buildExportPreviewForSnippets(session.id(), QVector<qint64>() << textSnippet.id(), &selectedPreview))
    {
        PrintError(mdExportService.lastError());
        return 762;
    }

    if (fullPreview.m_nSnippetCount != 2 || selectedPreview.m_nSnippetCount != 1)
    {
        PrintError(QString::fromUtf8("Export scope preview verification failed."));
        return 763;
    }

    if (!mdExportService.exportSessionToFile(session.id(), strSessionExportPath)
        || !mdExportService.exportSnippetsToFile(session.id(), QVector<qint64>() << textSnippet.id(), strSelectedExportPath))
    {
        PrintError(mdExportService.lastError());
        return 764;
    }

    const QString strFullMarkdown = QFile(strSessionExportPath).open(QIODevice::ReadOnly | QIODevice::Text)
        ? QString::fromUtf8(QFile(strSessionExportPath).readAll())
        : QString();
    QFile selectedFile(strSelectedExportPath);
    QString strSelectedMarkdown;
    if (selectedFile.open(QIODevice::ReadOnly | QIODevice::Text))
        strSelectedMarkdown = QString::fromUtf8(selectedFile.readAll());
    if (strSelectedMarkdown.isEmpty())
    {
        PrintError(QString::fromUtf8("Selected export file verification failed."));
        return 765;
    }

    if (!strSelectedMarkdown.contains(QString::fromUtf8("Scope Text")) || strSelectedMarkdown.contains(QString::fromUtf8("Scope Image")))
    {
        PrintError(QString::fromUtf8("Selected export scope content verification failed."));
        return 766;
    }

    PrintInfo(QString::fromUtf8("Export scope verification passed."));
    return 0;
}

int RunExportFailureVerificationWorkflow()
{
    QString strTempDirectory = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (strTempDirectory.isEmpty())
        strTempDirectory = QDir::tempPath();

    const QString strDatabasePath = BuildScopedTempPath(QString::fromUtf8("export_failure"), QString::fromUtf8("verify.sqlite"));
    QFile::remove(strDatabasePath);

    QCDatabaseManager databaseManager;
    if (!databaseManager.open(strDatabasePath) || !databaseManager.initialize())
    {
        PrintError(databaseManager.lastError());
        return 748;
    }

    QCSessionRepositorySqlite sessionRepository(&databaseManager);
    QCSnippetRepositorySqlite snippetRepository(&databaseManager);
    QCAiRecordRepositorySqlite aiRecordRepository(&databaseManager);
    QCSessionService sessionService(&sessionRepository);
    QCSnippetService snippetService(&snippetRepository);
    QCAiService aiService(&aiRecordRepository);
    QCExportDataService exportDataService(&sessionService, &snippetService, &aiService);
    QCMdExportRenderer mdExportRenderer;
    QCMdExportService mdExportService(&exportDataService, &mdExportRenderer);

    QCStudySession session;
    session.setTitle(QString::fromUtf8("Export Failure Session"));
    if (!sessionService.createSession(&session))
    {
        PrintError(sessionService.lastError());
        return 749;
    }

    const QString strInvalidOutputPath = QString::fromUtf8("Z:/qtclip/does/not/exist/export.md");
    if (mdExportService.exportSessionToFile(session.id(), strInvalidOutputPath))
    {
        PrintError(QString::fromUtf8("Export failure verification expected a failure."));
        return 750;
    }

    if (!mdExportService.lastError().contains(QString::fromUtf8("export.md"), Qt::CaseInsensitive)
        && !mdExportService.lastError().contains(QString::fromUtf8("qtclip"), Qt::CaseInsensitive))
    {
        PrintError(QString::fromUtf8("Export failure message did not include path context."));
        return 751;
    }

    PrintInfo(QString::fromUtf8("Export failure verification passed."));
    PrintInfo(mdExportService.lastError());
    return 0;
}

int RunLanguageUiVerificationWorkflow()
{
    QString strTempDirectory = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (strTempDirectory.isEmpty())
        strTempDirectory = QDir::tempPath();

    const QString strDatabasePath = BuildScopedTempPath(QString::fromUtf8("language_ui"), QString::fromUtf8("verify.sqlite"));
    QFile::remove(strDatabasePath);

    QCDatabaseManager databaseManager;
    if (!databaseManager.open(strDatabasePath) || !databaseManager.initialize())
    {
        PrintError(databaseManager.lastError());
        return 729;
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

    auto verifyWindow = [&](const QString& strLanguage,
                            const QString& strSettingsText,
                            const QString& strSearchMarker,
                            const QString& strContextMarker,
                            const QString& strDeleteSessionText,
                            const QString& strHistoryButtonText) -> bool {
        if (!settingsService.setAppLanguage(strLanguage))
            return false;

        QCMainWindow mainWindow(&sessionService,
                                &snippetService,
                                &tagService,
                                &aiService,
                                &aiProcessService,
                                &settingsService,
                                &mdExportService,
                                &screenCaptureService);

        QAction *pSettingsAction = mainWindow.findChild<QAction *>(QString::fromUtf8("aiSettingsAction"));
        QAction *pDeleteSessionAction = mainWindow.findChild<QAction *>(QString::fromUtf8("deleteSessionAction"));
        QLineEdit *pSearchLineEdit = mainWindow.findChild<QLineEdit *>(QString::fromUtf8("snippetSearchLineEdit"));
        QLabel *pContextLabel = mainWindow.findChild<QLabel *>(QString::fromUtf8("selectionContextLabel"));
        QComboBox *pHistoryComboBox = mainWindow.findChild<QComboBox *>(QString::fromUtf8("searchHistoryComboBox"));
        QPushButton *pClearSearchHistoryButton = mainWindow.findChild<QPushButton *>(QString::fromUtf8("clearSearchHistoryButton"));
        if (nullptr == pSettingsAction
            || nullptr == pDeleteSessionAction
            || nullptr == pSearchLineEdit
            || nullptr == pContextLabel
            || nullptr == pHistoryComboBox
            || nullptr == pClearSearchHistoryButton)
        {
            return false;
        }

        return pSettingsAction->text() == strSettingsText
            && pDeleteSessionAction->text() == strDeleteSessionText
            && pClearSearchHistoryButton->text() == strHistoryButtonText
            && pSearchLineEdit->placeholderText().contains(strSearchMarker)
            && pContextLabel->text().contains(strContextMarker)
            && !pHistoryComboBox->itemText(0).trimmed().isEmpty();
    };

    if (!verifyWindow(QString::fromUtf8("zh-CN"), QString::fromUtf8("设置"), QString::fromUtf8("搜索"), QString::fromUtf8("未选择会话"), QString::fromUtf8("删除会话"), QString::fromUtf8("清除历史")))
    {
        PrintError(QString::fromUtf8("Chinese UI verification failed."));
        return 730;
    }

    QCCreateSessionDialog zhSessionDialog;
    QCCreateTextSnippetDialog zhTextDialog;
    QCQuickCaptureDialog zhQuickDialog(QString::fromUtf8(""));
    QCSnippetTagDialog zhTagDialog;
    QCCreateImageSnippetDialog zhImageDialog(QString::fromUtf8(""), false, QString::fromUtf8("C:/tmp"), &screenCaptureService);
    if (!zhSessionDialog.windowTitle().contains(QString::fromUtf8("??"))
        || !zhTextDialog.windowTitle().contains(QString::fromUtf8("??"))
        || !zhQuickDialog.windowTitle().contains(QString::fromUtf8("??"))
        || !zhTagDialog.windowTitle().contains(QString::fromUtf8("??"))
        || !zhImageDialog.windowTitle().contains(QString::fromUtf8("??")))
    {
        PrintError(QString::fromUtf8("Chinese dialog localization verification failed."));
        return 7301;
    }

    if (!verifyWindow(QString::fromUtf8("en-US"), QString::fromUtf8("Settings"), QString::fromUtf8("Search"), QString::fromUtf8("Current context"), QString::fromUtf8("Delete Session"), QString::fromUtf8("History")))
    {
        PrintError(QString::fromUtf8("English UI verification failed."));
        return 731;
    }

    QCCreateSessionDialog enSessionDialog;
    QCCreateTextSnippetDialog enTextDialog;
    QCQuickCaptureDialog enQuickDialog(QString::fromUtf8(""));
    QCSnippetTagDialog enTagDialog;
    QCCreateImageSnippetDialog enImageDialog(QString::fromUtf8(""), false, QString::fromUtf8("C:/tmp"), &screenCaptureService);
    if (!enSessionDialog.windowTitle().contains(QString::fromUtf8("Session"))
        || !enTextDialog.windowTitle().contains(QString::fromUtf8("Text"))
        || !enQuickDialog.windowTitle().contains(QString::fromUtf8("Quick"))
        || !enTagDialog.windowTitle().contains(QString::fromUtf8("Tag"))
        || !enImageDialog.windowTitle().contains(QString::fromUtf8("Image")))
    {
        PrintError(QString::fromUtf8("English dialog localization verification failed."));
        return 7311;
    }

    QCAiSettingsDialog dialog(&aiProcessService);
    dialog.setDefaultState(settingsService.defaultAiSettingsProfiles(),
                           settingsService.defaultAiProfileIndex(),
                           QString::fromUtf8("zh-CN"),
                           settingsService.defaultScreenshotSaveDirectory(),
                           settingsService.defaultExportDirectory(),
                           settingsService.defaultCopyImportedImageToCaptureDirectory());
    dialog.setDialogState(settingsService.defaultAiSettingsProfiles(),
                          settingsService.defaultAiProfileIndex(),
                          QString::fromUtf8("zh-CN"),
                          settingsService.defaultScreenshotSaveDirectory(),
                          settingsService.defaultExportDirectory(),
                          settingsService.defaultCopyImportedImageToCaptureDirectory());
    if (!dialog.windowTitle().contains(QString::fromUtf8("??")))
    {
        PrintError(QString::fromUtf8("Settings dialog Chinese verification failed."));
        return 732;
    }

    PrintInfo(QString::fromUtf8("Language UI verification passed."));
    return 0;
}

int RunSmokeWorkflow()
{
    QString strTempDirectory = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (strTempDirectory.isEmpty())
        strTempDirectory = QDir::tempPath();

    const QString strDatabasePath = BuildScopedTempPath(QString::fromUtf8("smoke"), QString::fromUtf8("smoke.sqlite"));
    const QString strSmokeScreenshotDirectory = BuildScopedTempPath(QString::fromUtf8("smoke"), QString::fromUtf8("captures"));
    const QString strSmokeExportDirectory = BuildScopedTempPath(QString::fromUtf8("smoke"), QString::fromUtf8("exports"));
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
    application.setOrganizationName(QString::fromUtf8("QtClip"));
    application.setApplicationName(QString::fromUtf8("QtClip"));
    QCApplyAppTheme(&application);
    const QStringList vecArguments = application.arguments();

    if (vecArguments.contains(QString::fromUtf8("--smoke")))
        return RunSmokeWorkflow();

    if (vecArguments.contains(QString::fromUtf8("--verify-management")))
        return RunManagementVerificationWorkflow();

    if (vecArguments.contains(QString::fromUtf8("--verify-mainwindow-state")))
        return RunMainWindowStateVerificationWorkflow();

    if (vecArguments.contains(QString::fromUtf8("--verify-boundaries")))
        return RunBoundaryVerificationWorkflow();

    if (vecArguments.contains(QString::fromUtf8("--verify-selection-flow")))
        return RunSelectionFlowVerificationWorkflow();

    if (vecArguments.contains(QString::fromUtf8("--verify-batch-duplicate")))
        return RunBatchDuplicateVerificationWorkflow();

    if (vecArguments.contains(QString::fromUtf8("--verify-batch-move")))
        return RunBatchMoveVerificationWorkflow();

    if (vecArguments.contains(QString::fromUtf8("--verify-tag-batch")))
        return RunTagBatchVerificationWorkflow();

    if (vecArguments.contains(QString::fromUtf8("--verify-tag-clear-action")))
        return RunTagClearActionVerificationWorkflow();

    if (vecArguments.contains(QString::fromUtf8("--verify-search-filter")))
        return RunSearchFilterVerificationWorkflow();

    if (vecArguments.contains(QString::fromUtf8("--verify-search-history")))
        return RunSearchHistoryVerificationWorkflow();

    if (vecArguments.contains(QString::fromUtf8("--verify-ai-retry")))
        return RunAiRetryVerificationWorkflow();

    if (vecArguments.contains(QString::fromUtf8("--verify-ai-status")))
        return RunAiStatusVerificationWorkflow();

    if (vecArguments.contains(QString::fromUtf8("--verify-ai-view")))
        return RunAiViewVerificationWorkflow();

    if (vecArguments.contains(QString::fromUtf8("--verify-export")))
        return RunExportVerificationWorkflow();

    if (vecArguments.contains(QString::fromUtf8("--verify-export-scope")))
        return RunExportScopeVerificationWorkflow();

    if (vecArguments.contains(QString::fromUtf8("--verify-export-failure")))
        return RunExportFailureVerificationWorkflow();

    if (vecArguments.contains(QString::fromUtf8("--verify-live-ai")))
        return RunLiveAiVerificationWorkflow();

    if (vecArguments.contains(QString::fromUtf8("--verify-language")))
        return RunLanguageVerificationWorkflow();

    if (vecArguments.contains(QString::fromUtf8("--verify-language-ui")))
        return RunLanguageUiVerificationWorkflow();

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










