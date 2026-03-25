#ifndef QTCLIP_QCMAINWINDOW_H_
#define QTCLIP_QCMAINWINDOW_H_

// File: qcmainwindow.h
// Author: ZZMI1
// Created: 2026-03-23
// Description: Declares the minimal Qt Widgets main window used by the QtClip demo application.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include <QFutureWatcher>
#include <QHash>
#include <QList>
#include <QMainWindow>

#include "../../domain/entities/qcairecord.h"
#include "../../domain/entities/qctag.h"
#include "../../services/qcaiprocessservice.h"
#include "../../services/qcscreencaptureservice.h"

class QAction;
class QCloseEvent;
class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QTreeWidget;
class QTreeWidgetItem;
class QPlainTextEdit;
class QListWidgetItem;
class QPushButton;
class QMimeData;
class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;
class QEvent;
class QCSnippetTagDialog;
class QCStudySession;
class QCSnippet;
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

protected:
    void dragEnterEvent(QDragEnterEvent *pEvent) override;
    void dragMoveEvent(QDragMoveEvent *pEvent) override;
    void dropEvent(QDropEvent *pEvent) override;
    void closeEvent(QCloseEvent *pEvent) override;
    bool eventFilter(QObject *pObject, QEvent *pEvent) override;

private:
    QCMainWindow(const QCMainWindow& other);
    QCMainWindow& operator=(const QCMainWindow& other);

    void loadAppLanguage();
    bool isChineseUi() const;
    QString uiText(const QString& strChinese, const QString& strEnglish) const;
    void applyLocalizedTexts();
    void setupUi();
    void setupActions();
    void loadSessions();
    void rebuildNavigationTree();
    void onNavigationItemActivated(QTreeWidgetItem *pItem, int nColumn);
    void loadTagFilterOptions();
    void loadSnippets(qint64 nSessionId);
    void applySnippetFilters();
    void showSessionSummary(qint64 nSessionId);
    void showSnippetDetails(qint64 nSnippetId);
    void clearSnippetDetails();
    void updateDetailImage(const QString& strImagePath);
    void refreshImagePreviewScale();
    void updateActionState();
    void flushSnippetDraftToStorage(bool bShowError);
    void updateViewSummary(int nVisibleSnippetCount, int nTotalSnippetCount);
    void updateAiStatusDisplay();
    void updateSelectionContextDisplay();
    void focusSummaryEditor(QPlainTextEdit *pTextEdit);
    QString currentSnippetSummaryText() const;
    QString currentSessionSummaryText() const;
    void updateMenuTexts();
    QString currentSearchText() const;
    QString currentSnippetTypeFilter() const;
    bool matchesSnippetTypeFilter(const QCSnippet& snippet) const;
    void loadSearchHistoryOptions();
    void appendSearchHistory(const QString& strSearchText);
    QString buildHighlightedHtml(const QString& strText, const QString& strSearchText) const;
    QString buildSelectionContextText() const;
    void applyPlainTextHighlight(QPlainTextEdit *pTextEdit, const QString& strSearchText) const;
    QVector<qint64> selectedSnippetIds() const;
    int selectedSnippetCount() const;
    bool hasSingleSelectedSnippet() const;
    bool currentSnippetForAction(QCSnippet *pSnippet) const;
    void handleSnippetSummaryFinished();
    void handleSessionSummaryFinished();
    QString findSummaryFromAiRecords(const QVector<QCAiRecord>& vecAiRecords) const;
    QString findSessionSummaryFromAiRecords(const QVector<QCAiRecord>& vecAiRecords) const;
    bool currentSession(QCStudySession *pSession) const;
    bool currentSnippet(QCSnippet *pSnippet) const;
    qint64 currentSessionId() const;
    qint64 currentSnippetId() const;
    qint64 currentTagFilterId() const;
    bool selectSnippetById(qint64 nSnippetId);
    bool selectSnippetsByIds(const QVector<qint64>& vecSnippetIds);
    bool captureScreenToFile(QCScreenCaptureResult *pCaptureResult);
    bool captureRegionToFile(QCScreenCaptureResult *pCaptureResult, bool *pbCancelled, QString *pstrFailureMessage);
    bool isAutoSummarizeImageSnippetEnabled() const;
    bool startSnippetSummary(qint64 nSnippetId, bool bAutomatic);
    void handleSavedImageSnippet(qint64 nSessionId, qint64 nSnippetId, const QString& strSavedMessage);
    bool createImageSnippetFromFilePath(qint64 nSessionId, const QString& strFilePath, const QString& strTitle, const QString& strNote, qint64 *pnSnippetId);
    QStringList extractLocalImageFilePaths(const QMimeData *pMimeData) const;
    bool importImageFilesToCurrentSession(const QStringList& vecFilePaths,
                                          const QString& strSourceLabel,
                                          bool bTriggerWorkspaceSummary,
                                          qint64 *pnLastSnippetId);
    void refreshSnippetTagsDisplay(qint64 nSnippetId);
    void updateSnippetStateControls(const QCSnippet& snippet);
    void restoreDefaultFilters();
    QHash<qint64, int> buildTagUsageCounts(const QVector<QCTag>& vecTags) const;
    QHash<qint64, Qt::CheckState> buildTagSelectionStatesForSnippets(const QVector<qint64>& vecSnippetIds, const QVector<QCTag>& vecTags) const;
    bool applyTagDialogChanges(const QCSnippetTagDialog& dialog, qint64 *pnCreatedTagId, QVector<qint64> *pvecDeletedTagIds, const QString& strContextTitle);
    bool applyTagSelectionStatesToSnippets(const QVector<qint64>& vecSnippetIds, const QHash<qint64, Qt::CheckState>& hashTagStates);

    void onSessionSelectionChanged();
    void onSnippetSelectionChanged();
    void onSnippetFilterChanged();
    void onSearchHistoryChanged();
    void onClearSearch();
    void onClearSearchHistory();
    void onResetFilters();
    void onFavoriteToggled(bool bChecked);
    void onReviewToggled(bool bChecked);
    void onCreateSession();
    void onEditSession();
    void onEditSnippet();
    void onEditCurrentItem();
    void onCreateTextSnippet();
    void onFinishSession();
    void onReopenSession();
    void onCaptureScreen();
    void onCaptureRegion();
    void onImportImageSnippet();
    void onPasteImageFromClipboard();
    void onRunWorkspaceSummary();
    void onDuplicateSnippet();
    void onMoveSnippet();
    void onManageTags();
    void onClearSnippetTags();
    void onOpenTagLibrary();
    void onDeleteSnippet();
    void onToggleArchiveSnippet();
    void onDeleteSession();
    void onDeleteCurrentItem();
    void onSummarizeSnippet();
    void onRetrySnippetSummary();
    void onViewSnippetSummary();
    void onCopySnippetSummary();
    void onSummarizeSession();
    void onRetrySessionSummary();
    void onViewSessionSummary();
    void onCopySessionSummary();
    void onAiSettings();
    void onExportMarkdown();
    void onFocusSearch();
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
    QAction *m_pEditSessionAction;
    QAction *m_pEditSnippetAction;
    QAction *m_pNewTextSnippetAction;
    QAction *m_pFinishSessionAction;
    QAction *m_pReopenSessionAction;
    QAction *m_pCaptureScreenAction;
    QAction *m_pCaptureRegionAction;
    QAction *m_pImportImageAction;
    QAction *m_pDuplicateSnippetAction;
    QAction *m_pMoveSnippetAction;
    QAction *m_pManageTagsAction;
    QAction *m_pClearSnippetTagsAction;
    QAction *m_pTagLibraryAction;
    QAction *m_pDeleteSnippetAction;
    QAction *m_pToggleArchiveSnippetAction;
    QAction *m_pDeleteSessionAction;
    QAction *m_pSummarizeSnippetAction;
    QAction *m_pRetrySnippetSummaryAction;
    QAction *m_pViewSnippetSummaryAction;
    QAction *m_pCopySnippetSummaryAction;
    QAction *m_pSummarizeSessionAction;
    QAction *m_pRetrySessionSummaryAction;
    QAction *m_pViewSessionSummaryAction;
    QAction *m_pCopySessionSummaryAction;
    QAction *m_pAiSettingsAction;
    QAction *m_pExportMarkdownAction;
    QAction *m_pRefreshAction;
    QAction *m_pEditCurrentAction;
    QAction *m_pDeleteCurrentAction;
    QAction *m_pFocusSearchAction;
    QAction *m_pPasteImageAction;

    QListWidget *m_pSessionListWidget;
    QListWidget *m_pSnippetListWidget;
    QLineEdit *m_pNavigationSearchLineEdit;
    QTreeWidget *m_pNavigationTreeWidget;
    QLineEdit *m_pSnippetSearchLineEdit;
    QComboBox *m_pSearchHistoryComboBox;
    QPushButton *m_pClearSearchButton;
    QPushButton *m_pClearSearchHistoryButton;
    QPushButton *m_pResetFiltersButton;
    QPushButton *m_pSidebarNewSessionButton;
    QPushButton *m_pSidebarSettingsButton;
    QPushButton *m_pPasteImageButton;
    QPushButton *m_pRunSummaryButton;
    QPushButton *m_pTopExportButton;
    QCheckBox *m_pQuickFavoriteCheckBox;
    QCheckBox *m_pQuickReviewCheckBox;
    QCheckBox *m_pFavoriteOnlyCheckBox;
    QCheckBox *m_pReviewOnlyCheckBox;
    QComboBox *m_pSnippetTypeFilterComboBox;
    QCheckBox *m_pShowArchivedCheckBox;
    QComboBox *m_pTagFilterComboBox;
    QLabel *m_pSessionPanelTitleLabel;
    QLabel *m_pSnippetPanelTitleLabel;
    QLabel *m_pDetailPanelTitleLabel;
    QLabel *m_pSelectionContextLabel;
    QLabel *m_pViewSummaryLabel;
    QLabel *m_pAiStatusLabel;
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
    QPixmap m_originalPreviewPixmap;
    QString m_strCurrentImagePath;

    bool m_bSnippetSummaryRunning;
    bool m_bSessionSummaryRunning;
    bool m_bAutomaticSnippetSummary;
    bool m_bUpdatingSnippetStateControls;
    bool m_bSyncingNavigationSelection;
    bool m_bPopulatingSnippetEditors;
    qint64 m_nDisplayedSnippetId;
    bool m_bHasRetryableSnippetSummary;
    bool m_bHasRetryableSessionSummary;
    qint64 m_nRetrySnippetId;
    qint64 m_nRetrySessionId;
    QString m_strAiStatusMessage;
    QString m_strAppLanguage;
    QFutureWatcher<QCAiTaskExecutionResult> *m_pSnippetSummaryWatcher;
    QFutureWatcher<QCAiTaskExecutionResult> *m_pSessionSummaryWatcher;
};

#endif // QTCLIP_QCMAINWINDOW_H_









