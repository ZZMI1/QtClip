#ifndef QTCLIP_QCMAINWINDOW_H_
#define QTCLIP_QCMAINWINDOW_H_

// File: qcmainwindow.h
// Author: ZZMI1
// Created: 2026-03-23
// Description: Declares the minimal Qt Widgets main window used by the QtClip demo application.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include <QFutureWatcher>
#include <QMainWindow>

#include "../../domain/entities/qcairecord.h"
#include "../../services/qcaiprocessservice.h"
#include "../../services/qcscreencaptureservice.h"

class QAction;
class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPlainTextEdit;
class QPushButton;
class QCAiService;
class QCMdExportService;
class QCSessionService;
class QCSettingsService;
class QCSnippetService;
class QCTagService;

class QCMainWindow : public QMainWindow
{
public:
    QCMainWindow(QCSessionService *pSessionService,
                 QCSnippetService *pSnippetService,
                 QCTagService *pTagService,
                 QCAiService *pAiService,
                 QCAiProcessService *pAiProcessService,
                 QCSettingsService *pSettingsService,
                 QCMdExportService *pMdExportService,
                 QCScreenCaptureService *pScreenCaptureService,
                 QWidget *pParent = nullptr);
    virtual ~QCMainWindow() override;

private:
    QCMainWindow(const QCMainWindow& other);
    QCMainWindow& operator=(const QCMainWindow& other);

    void setupUi();
    void setupActions();
    void loadSessions();
    void loadTagFilterOptions();
    void loadSnippets(qint64 nSessionId);
    void applySnippetFilters();
    void showSessionSummary(qint64 nSessionId);
    void showSnippetDetails(qint64 nSnippetId);
    void clearSnippetDetails();
    void updateDetailImage(const QString& strImagePath);
    void updateAiActionState();
    void handleSnippetSummaryFinished();
    void handleSessionSummaryFinished();
    QString findSummaryFromAiRecords(const QVector<QCAiRecord>& vecAiRecords) const;
    QString findSessionSummaryFromAiRecords(const QVector<QCAiRecord>& vecAiRecords) const;
    qint64 currentSessionId() const;
    qint64 currentSnippetId() const;
    qint64 currentTagFilterId() const;
    bool selectSnippetById(qint64 nSnippetId);
    bool captureScreenToFile(QCScreenCaptureResult *pCaptureResult);
    bool captureRegionToFile(QCScreenCaptureResult *pCaptureResult, bool *pbCancelled, QString *pstrFailureMessage);
    bool isAutoSummarizeImageSnippetEnabled() const;
    bool startSnippetSummary(qint64 nSnippetId, bool bAutomatic);
    void handleSavedImageSnippet(qint64 nSessionId, qint64 nSnippetId, const QString& strSavedMessage);
    void refreshSnippetTagsDisplay(qint64 nSnippetId);
    void updateSnippetStateControls(const QCSnippet& snippet);
    void restoreDefaultFilters();

    void onSessionSelectionChanged();
    void onSnippetSelectionChanged();
    void onSnippetFilterChanged();
    void onClearSearch();
    void onResetFilters();
    void onFavoriteToggled(bool bChecked);
    void onReviewToggled(bool bChecked);
    void onCreateSession();
    void onCreateTextSnippet();
    void onCaptureScreen();
    void onCaptureRegion();
    void onImportImageSnippet();
    void onManageTags();
    void onSummarizeSnippet();
    void onSummarizeSession();
    void onAiSettings();
    void onExportMarkdown();
    void onRefresh();

private:
    QCSessionService *m_pSessionService;
    QCSnippetService *m_pSnippetService;
    QCTagService *m_pTagService;
    QCAiService *m_pAiService;
    QCAiProcessService *m_pAiProcessService;
    QCSettingsService *m_pSettingsService;
    QCMdExportService *m_pMdExportService;
    QCScreenCaptureService *m_pScreenCaptureService;

    QAction *m_pNewSessionAction;
    QAction *m_pNewTextSnippetAction;
    QAction *m_pCaptureScreenAction;
    QAction *m_pCaptureRegionAction;
    QAction *m_pImportImageAction;
    QAction *m_pManageTagsAction;
    QAction *m_pSummarizeSnippetAction;
    QAction *m_pSummarizeSessionAction;
    QAction *m_pAiSettingsAction;
    QAction *m_pExportMarkdownAction;
    QAction *m_pRefreshAction;

    QListWidget *m_pSessionListWidget;
    QListWidget *m_pSnippetListWidget;
    QLineEdit *m_pSnippetSearchLineEdit;
    QPushButton *m_pClearSearchButton;
    QPushButton *m_pResetFiltersButton;
    QCheckBox *m_pQuickFavoriteCheckBox;
    QCheckBox *m_pQuickReviewCheckBox;
    QCheckBox *m_pFavoriteOnlyCheckBox;
    QCheckBox *m_pReviewOnlyCheckBox;
    QComboBox *m_pTagFilterComboBox;
    QPlainTextEdit *m_pSessionSummaryTextEdit;
    QLabel *m_pSnippetTitleValueLabel;
    QLabel *m_pSnippetTagsValueLabel;
    QCheckBox *m_pSnippetFavoriteCheckBox;
    QCheckBox *m_pSnippetReviewCheckBox;
    QPlainTextEdit *m_pSnippetNoteTextEdit;
    QPlainTextEdit *m_pSnippetContentTextEdit;
    QPlainTextEdit *m_pSnippetSummaryTextEdit;
    QLabel *m_pImagePathValueLabel;
    QLabel *m_pImagePreviewLabel;

    bool m_bSnippetSummaryRunning;
    bool m_bSessionSummaryRunning;
    bool m_bAutomaticSnippetSummary;
    bool m_bUpdatingSnippetStateControls;
    QFutureWatcher<QCAiTaskExecutionResult> *m_pSnippetSummaryWatcher;
    QFutureWatcher<QCAiTaskExecutionResult> *m_pSessionSummaryWatcher;
};

#endif // QTCLIP_QCMAINWINDOW_H_


