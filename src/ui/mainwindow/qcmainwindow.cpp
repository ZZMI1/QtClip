
// File: qcmainwindow.cpp
// Author: ZZMI1
// Created: 2026-03-23
// Description: Implements the minimal Qt Widgets main window used by the QtClip demo application.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include "qcmainwindow.h"
#include "qcnavigationtreedelegate.h"
#include "qclegacyfiltersupport.h"

#include <QtConcurrent/QtConcurrent>

#include <QAction>
#include <QClipboard>
#include <QAbstractItemView>
#include <QApplication>
#include <QCheckBox>
#include <QCoreApplication>
#include <QCloseEvent>
#include <QComboBox>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QStyle>
#include <QEvent>
#include <QEventLoop>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHeaderView>
#include <QFrame>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QImage>
#include <QLabel>
#include <QKeySequence>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMimeData>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QPushButton>
#include <QSet>
#include <QSplitter>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QStandardPaths>
#include <QUrl>
#include <QTimer>
#include <QTextCursor>
#include <QTextDocument>
#include <QToolBar>
#include <QToolButton>
#include <QTreeWidgetItem>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QWidget>

#include "../dialogs/qcaisettingsdialog.h"
#include "../dialogs/qccreateimagesnippetdialog.h"
#include "../dialogs/qccreatesessiondialog.h"
#include "../dialogs/qccreatetextsnippetdialog.h"
#include "../dialogs/qcquickcapturedialog.h"
#include "../dialogs/qcsnippettagdialog.h"
#include "../widgets/qcscreenshotoverlay.h"
#include "../common/qcuilocalization.h"
#include "../../services/qcaiservice.h"
#include "../../services/qcmdexportservice.h"
#include "../../services/qcsessionservice.h"
#include "../../services/qcsettingsservice.h"
#include "../../services/qcsnippetservice.h"
#include "../../services/qctagservice.h"

namespace
{
QString SessionStatusText(QCSessionStatus sessionStatus)
{
    return sessionStatus == QCSessionStatus::FinishedSessionStatus
        ? QCUiText(QString::fromUtf8("已完成"), QString::fromUtf8("Finished"))
        : QCUiText(QString::fromUtf8("进行中"), QString::fromUtf8("Active"));
}

QString SessionItemText(const QCStudySession& session)
{
    const QString strStatus = SessionStatusText(session.status());
    const QString strTitle = QString::fromUtf8("[%1] %2").arg(strStatus, session.title().trimmed().isEmpty()
        ? QCUiText(QString::fromUtf8("未命名 Session"), QString::fromUtf8("Untitled Session"))
        : session.title().trimmed());

    if (session.courseName().trimmed().isEmpty())
        return strTitle;

    return QString::fromUtf8("%1\n%2").arg(strTitle, session.courseName());
}

bool IsSnippetMarkedForReview(const QCSnippet& snippet)
{
    return snippet.noteLevel() == QCNoteLevel::ReviewNoteLevel;
}

QString SnippetItemText(const QCSnippet& snippet)
{
    const QString strTime = snippet.capturedAt().isValid()
        ? snippet.capturedAt().toLocalTime().toString(QString::fromUtf8("MM-dd hh:mm"))
        : QString::fromUtf8("--:--");
    const QString strTitle = snippet.title().trimmed().isEmpty()
        ? QCUiText(QString::fromUtf8("Untitled Snippet"), QString::fromUtf8("Untitled Snippet"))
        : snippet.title().trimmed();

    return QString::fromUtf8("[%1] %2").arg(strTime, strTitle);
}


QString SanitizeSummaryForUi(const QString& strSummary)
{
    QString strValue = strSummary.trimmed();
    if (strValue.startsWith(QString::fromUtf8("Mock summary:"), Qt::CaseInsensitive))
        strValue = strValue.mid(QString::fromUtf8("Mock summary:").size()).trimmed();
    return strValue;
}

bool IsPromptLikeSummary(const QString& strSummary)
{
    const QString strValue = strSummary.trimmed().toLower();
    if (strValue.isEmpty())
        return true;

    return strValue.startsWith(QString::fromUtf8("summarize "))
        || strValue.startsWith(QString::fromUtf8("please summarize"))
        || strValue.contains(QString::fromUtf8("return only the summary text"))
        || strValue.contains(QString::fromUtf8("output requirement"))
        || strValue.contains(QString::fromUtf8("snippet type:"))
        || strValue.contains(QString::fromUtf8("session title:"))
        || strValue.contains(QString::fromUtf8("source:"));
}

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
}

QCMainWindow::QCMainWindow(QCSessionService *pSessionService,
                           QCSnippetService *pSnippetService,
                           QCTagService *pTagService,
                           QCAiService *pAiService,
                           QCAiProcessService *pAiProcessService,
                           QCSettingsService *pSettingsService,
                           QCMdExportService *pMdExportService,
                           QCScreenCaptureService *pScreenCaptureService,
                           QWidget *pParent)
    : QMainWindow(pParent)
    , m_pSessionService(pSessionService)
    , m_pSnippetService(pSnippetService)
    , m_pTagService(pTagService)
    , m_pAiService(pAiService)
    , m_pAiProcessService(pAiProcessService)
    , m_pSettingsService(pSettingsService)
    , m_pMdExportService(pMdExportService)
    , m_pScreenCaptureService(pScreenCaptureService)
    , m_pNewSessionAction(nullptr)
    , m_pEditSessionAction(nullptr)
    , m_pEditSnippetAction(nullptr)
    , m_pNewTextSnippetAction(nullptr)
    , m_pFinishSessionAction(nullptr)
    , m_pReopenSessionAction(nullptr)
    , m_pCaptureScreenAction(nullptr)
    , m_pCaptureRegionAction(nullptr)
    , m_pImportImageAction(nullptr)
    , m_pDuplicateSnippetAction(nullptr)
    , m_pMoveSnippetAction(nullptr)
    , m_pManageTagsAction(nullptr)
    , m_pClearSnippetTagsAction(nullptr)
    , m_pTagLibraryAction(nullptr)
    , m_pSummarizeSnippetAction(nullptr)
    , m_pRetrySnippetSummaryAction(nullptr)
    , m_pViewSnippetSummaryAction(nullptr)
    , m_pCopySnippetSummaryAction(nullptr)
    , m_pSummarizeSessionAction(nullptr)
    , m_pRetrySessionSummaryAction(nullptr)
    , m_pViewSessionSummaryAction(nullptr)
    , m_pCopySessionSummaryAction(nullptr)
    , m_pAiSettingsAction(nullptr)
    , m_pExportMarkdownAction(nullptr)
    , m_pRefreshAction(nullptr)
    , m_pEditCurrentAction(nullptr)
    , m_pDeleteCurrentAction(nullptr)
    , m_pFocusSearchAction(nullptr)
    , m_pPasteImageAction(nullptr)
    , m_pDeleteSnippetAction(nullptr)
    , m_pToggleArchiveSnippetAction(nullptr)
    , m_pDeleteSessionAction(nullptr)
    , m_pSessionListWidget(nullptr)
    , m_pSnippetListWidget(nullptr)
    , m_pNavigationSearchLineEdit(nullptr)
    , m_pNavigationTreeWidget(nullptr)
    , m_pSnippetSearchLineEdit(nullptr)
    , m_pSearchHistoryComboBox(nullptr)
    , m_pClearSearchButton(nullptr)
    , m_pClearSearchHistoryButton(nullptr)
    , m_pResetFiltersButton(nullptr)
    , m_pSidebarNewSessionButton(nullptr)
    , m_pSidebarSettingsButton(nullptr)
    , m_pPasteImageButton(nullptr)
    , m_pRunSummaryButton(nullptr)
    , m_pTopExportButton(nullptr)
    , m_pQuickFavoriteCheckBox(nullptr)
    , m_pQuickReviewCheckBox(nullptr)
    , m_pFavoriteOnlyCheckBox(nullptr)
    , m_pReviewOnlyCheckBox(nullptr)
    , m_pSnippetTypeFilterComboBox(nullptr)
    , m_pShowArchivedCheckBox(nullptr)
    , m_pTagFilterComboBox(nullptr)
    , m_pSessionPanelTitleLabel(nullptr)
    , m_pSnippetPanelTitleLabel(nullptr)
    , m_pDetailPanelTitleLabel(nullptr)
    , m_pSelectionContextLabel(nullptr)
    , m_pViewSummaryLabel(nullptr)
    , m_pAiStatusLabel(nullptr)
    , m_pSessionSummaryTextEdit(nullptr)
    , m_pSnippetTitleValueLabel(nullptr)
    , m_pSnippetTagsValueLabel(nullptr)
    , m_pSnippetFavoriteCheckBox(nullptr)
    , m_pSnippetReviewCheckBox(nullptr)
    , m_pSnippetNoteTextEdit(nullptr)
    , m_pSnippetContentTextEdit(nullptr)
    , m_pSnippetSummaryTextEdit(nullptr)
    , m_pImagePathValueLabel(nullptr)
    , m_pImagePreviewLabel(nullptr)
    , m_bSnippetSummaryRunning(false)
    , m_bSessionSummaryRunning(false)
    , m_bAutomaticSnippetSummary(false)
    , m_bUpdatingSnippetStateControls(false)
    , m_bSyncingNavigationSelection(false)
    , m_bPopulatingSnippetEditors(false)
    , m_nDisplayedSnippetId(0)
    , m_bHasRetryableSnippetSummary(false)
    , m_bHasRetryableSessionSummary(false)
    , m_nRetrySnippetId(0)
    , m_nRetrySessionId(0)
    , m_strAiStatusMessage(QString())
    , m_strAppLanguage(QString::fromUtf8("zh-CN"))
    , m_pSnippetSummaryWatcher(new QFutureWatcher<QCAiTaskExecutionResult>(this))
    , m_pSessionSummaryWatcher(new QFutureWatcher<QCAiTaskExecutionResult>(this))
{
    loadAppLanguage();
    m_strAiStatusMessage = uiText(QString::fromUtf8("AI 就绪"), QString::fromUtf8("AI Ready"));
    qApp->setProperty("qtclip.uiLanguage", m_strAppLanguage);
    setupUi();
    setAcceptDrops(true);
    setupActions();

    connect(m_pSnippetSummaryWatcher, &QFutureWatcher<QCAiTaskExecutionResult>::finished, this, [this]() {
        handleSnippetSummaryFinished();
    });
    connect(m_pSessionSummaryWatcher, &QFutureWatcher<QCAiTaskExecutionResult>::finished, this, [this]() {
        handleSessionSummaryFinished();
    });

    loadSessions();
}

QCMainWindow::~QCMainWindow()
{
}


void QCMainWindow::dragEnterEvent(QDragEnterEvent *pEvent)
{
    if (nullptr == pEvent)
        return;

    const QStringList vecImagePaths = extractLocalImageFilePaths(pEvent->mimeData());
    if (vecImagePaths.isEmpty())
    {
        pEvent->ignore();
        return;
    }

    pEvent->acceptProposedAction();
}

void QCMainWindow::dragMoveEvent(QDragMoveEvent *pEvent)
{
    if (nullptr == pEvent)
        return;

    const QStringList vecImagePaths = extractLocalImageFilePaths(pEvent->mimeData());
    if (vecImagePaths.isEmpty())
    {
        pEvent->ignore();
        return;
    }

    pEvent->acceptProposedAction();
}

void QCMainWindow::dropEvent(QDropEvent *pEvent)
{
    if (nullptr == pEvent)
        return;

    const QStringList vecImagePaths = extractLocalImageFilePaths(pEvent->mimeData());
    if (vecImagePaths.isEmpty())
    {
        pEvent->ignore();
        return;
    }

    qint64 nLastSnippetId = 0;
    if (importImageFilesToCurrentSession(vecImagePaths,
                                         true,
                                         &nLastSnippetId))
    {
        pEvent->acceptProposedAction();
        return;
    }

    pEvent->ignore();
}
void QCMainWindow::closeEvent(QCloseEvent *pEvent)
{
    flushSnippetDraftToStorage(false);
    QMainWindow::closeEvent(pEvent);
}

bool QCMainWindow::eventFilter(QObject *pObject, QEvent *pEvent)
{
    if (pObject == m_pImagePreviewLabel && nullptr != pEvent && pEvent->type() == QEvent::Resize)
        refreshImagePreviewScale();

    return QMainWindow::eventFilter(pObject, pEvent);
}

void QCMainWindow::loadAppLanguage()
{
    m_strAppLanguage = QString::fromUtf8("zh-CN");
    if (nullptr != m_pSettingsService)
        m_pSettingsService->getAppLanguage(&m_strAppLanguage);
    m_strAppLanguage = QCNormalizeUiLanguage(m_strAppLanguage);
}

bool QCMainWindow::isChineseUi() const
{
    return QCIsChineseUiLanguage(m_strAppLanguage);
}

QString QCMainWindow::uiText(const QString& strChinese, const QString& strEnglish) const
{
    return QCIsChineseUiLanguage(m_strAppLanguage) ? QCUiText(strChinese, strEnglish) : strEnglish;
}

void QCMainWindow::applyLocalizedTexts()
{
    updateMenuTexts();
    setWindowTitle(uiText(QString::fromUtf8("QtClip - Learning Material Organizer"), QString::fromUtf8("QtClip - Learning Material Organizer")));
    if (nullptr != m_pSessionPanelTitleLabel)
        m_pSessionPanelTitleLabel->setText(uiText(QString::fromUtf8("Session 列表"), QString::fromUtf8("Sessions")));
    if (nullptr != m_pSnippetPanelTitleLabel)
        m_pSnippetPanelTitleLabel->setText(uiText(QString::fromUtf8("Snippet 列表"), QString::fromUtf8("Snippets")));
    if (nullptr != m_pDetailPanelTitleLabel)
        m_pDetailPanelTitleLabel->setText(uiText(QString::fromUtf8("详情"), QString::fromUtf8("Details")));
    if (nullptr != m_pNavigationSearchLineEdit)
        m_pNavigationSearchLineEdit->setPlaceholderText(uiText(QString::fromUtf8("Search course or screenshot title"), QString::fromUtf8("Search course or screenshot title")));
    if (nullptr != m_pSnippetSearchLineEdit)
    {
        m_pSnippetSearchLineEdit->setPlaceholderText(uiText(QString::fromUtf8("搜索标题、内容、总结、备注"), QString::fromUtf8("Search title, content, summary, note")));
        m_pSnippetSearchLineEdit->setClearButtonEnabled(true);
    }
    if (nullptr != m_pClearSearchButton)
        m_pClearSearchButton->setText(uiText(QString::fromUtf8("清空"), QString::fromUtf8("Clear")));
    if (nullptr != m_pResetFiltersButton)
        m_pResetFiltersButton->setText(uiText(QString::fromUtf8("重置"), QString::fromUtf8("Reset")));
    if (nullptr != m_pSidebarNewSessionButton)
        m_pSidebarNewSessionButton->setText(uiText(QString::fromUtf8("新建"), QString::fromUtf8("New")));
    if (nullptr != m_pSidebarSettingsButton)
        m_pSidebarSettingsButton->setText(uiText(QString::fromUtf8("设置"), QString::fromUtf8("Settings")));
    if (nullptr != m_pPasteImageButton)
        m_pPasteImageButton->setText(uiText(QString::fromUtf8("粘贴图片"), QString::fromUtf8("Paste Image")));
    if (nullptr != m_pRunSummaryButton)
        m_pRunSummaryButton->setText(QString::fromUtf8("▶ ") + uiText(QString::fromUtf8("运行总结"), QString::fromUtf8("Run Summary")));
    if (nullptr != m_pTopExportButton)
        m_pTopExportButton->setText(uiText(QString::fromUtf8("导出"), QString::fromUtf8("Export")));
    if (nullptr != m_pClearSearchHistoryButton)
        m_pClearSearchHistoryButton->setText(uiText(QString::fromUtf8("历史"), QString::fromUtf8("History")));
    if (nullptr != m_pSearchHistoryComboBox && m_pSearchHistoryComboBox->count() > 0)
        m_pSearchHistoryComboBox->setItemText(0, uiText(QString::fromUtf8("最近搜索"), QString::fromUtf8("Recent Searches")));
    if (nullptr != m_pSnippetTypeFilterComboBox && m_pSnippetTypeFilterComboBox->count() >= 3)
    {
        m_pSnippetTypeFilterComboBox->setItemText(0, uiText(QString::fromUtf8("所有类型"), QString::fromUtf8("All Types")));
        m_pSnippetTypeFilterComboBox->setItemText(1, uiText(QString::fromUtf8("文本"), QString::fromUtf8("Text")));
        m_pSnippetTypeFilterComboBox->setItemText(2, uiText(QString::fromUtf8("图片"), QString::fromUtf8("Image")));
    }
    if (nullptr != m_pQuickFavoriteCheckBox)
        m_pQuickFavoriteCheckBox->setText(uiText(QString::fromUtf8("设为收藏"), QString::fromUtf8("Selected Favorite")));
    if (nullptr != m_pQuickReviewCheckBox)
        m_pQuickReviewCheckBox->setText(uiText(QString::fromUtf8("设为复习"), QString::fromUtf8("Selected Review")));
    if (nullptr != m_pFavoriteOnlyCheckBox)
        m_pFavoriteOnlyCheckBox->setText(uiText(QString::fromUtf8("仅收藏"), QString::fromUtf8("Favorites Only")));
    if (nullptr != m_pReviewOnlyCheckBox)
        m_pReviewOnlyCheckBox->setText(uiText(QString::fromUtf8("仅复习"), QString::fromUtf8("Review Only")));
    if (nullptr != m_pShowArchivedCheckBox)
        m_pShowArchivedCheckBox->setText(uiText(QString::fromUtf8("显示已归档"), QString::fromUtf8("Show Archived")));
    if (nullptr != m_pSnippetFavoriteCheckBox)
        m_pSnippetFavoriteCheckBox->setText(uiText(QString::fromUtf8("收藏"), QString::fromUtf8("Favorite")));
    if (nullptr != m_pSnippetReviewCheckBox)
        m_pSnippetReviewCheckBox->setText(uiText(QString::fromUtf8("复习"), QString::fromUtf8("Review")));
    if (nullptr != m_pImagePreviewLabel && m_pImagePreviewLabel->pixmap() == nullptr)
        m_pImagePreviewLabel->setText(uiText(QString::fromUtf8("暂无图片预览"), QString::fromUtf8("No image preview available.")));

    if (nullptr != m_pNewSessionAction) m_pNewSessionAction->setText(uiText(QString::fromUtf8("新建 Session"), QString::fromUtf8("New Session")));
    if (nullptr != m_pEditSessionAction) m_pEditSessionAction->setText(uiText(QString::fromUtf8("编辑 Session"), QString::fromUtf8("Edit Session")));
    if (nullptr != m_pNewTextSnippetAction) m_pNewTextSnippetAction->setText(uiText(QString::fromUtf8("新建文本"), QString::fromUtf8("New Text")));
    if (nullptr != m_pFinishSessionAction) m_pFinishSessionAction->setText(uiText(QString::fromUtf8("完成 Session"), QString::fromUtf8("Finish Session")));
    if (nullptr != m_pReopenSessionAction) m_pReopenSessionAction->setText(uiText(QString::fromUtf8("重新打开 Session"), QString::fromUtf8("Reopen Session")));
    if (nullptr != m_pEditSnippetAction) m_pEditSnippetAction->setText(uiText(QString::fromUtf8("编辑 Snippet"), QString::fromUtf8("Edit Snippet")));
    if (nullptr != m_pCaptureScreenAction) m_pCaptureScreenAction->setText(uiText(QString::fromUtf8("整屏截图"), QString::fromUtf8("Capture Screen")));
    if (nullptr != m_pCaptureRegionAction) m_pCaptureRegionAction->setText(uiText(QString::fromUtf8("区域截图"), QString::fromUtf8("Capture Region")));
    if (nullptr != m_pImportImageAction) m_pImportImageAction->setText(uiText(QString::fromUtf8("导入图片"), QString::fromUtf8("Import Image")));
    if (nullptr != m_pPasteImageAction) m_pPasteImageAction->setText(uiText(QString::fromUtf8("粘贴图片"), QString::fromUtf8("Paste Image")));
    if (nullptr != m_pTagLibraryAction) m_pTagLibraryAction->setText(uiText(QString::fromUtf8("标签库"), QString::fromUtf8("Tag Library")));
    if (nullptr != m_pSummarizeSnippetAction) m_pSummarizeSnippetAction->setText(uiText(QString::fromUtf8("总结 Snippet"), QString::fromUtf8("Summarize Snippet")));
    if (nullptr != m_pRetrySnippetSummaryAction) m_pRetrySnippetSummaryAction->setText(uiText(QString::fromUtf8("重试 Snippet AI"), QString::fromUtf8("Retry Snippet AI")));
    if (nullptr != m_pViewSnippetSummaryAction) m_pViewSnippetSummaryAction->setText(uiText(QString::fromUtf8("查看 Snippet 总结"), QString::fromUtf8("View Snippet Summary")));
    if (nullptr != m_pCopySnippetSummaryAction) m_pCopySnippetSummaryAction->setText(uiText(QString::fromUtf8("复制 Snippet 总结"), QString::fromUtf8("Copy Snippet Summary")));
    if (nullptr != m_pSummarizeSessionAction) m_pSummarizeSessionAction->setText(uiText(QString::fromUtf8("总结 Session"), QString::fromUtf8("Summarize Session")));
    if (nullptr != m_pRetrySessionSummaryAction) m_pRetrySessionSummaryAction->setText(uiText(QString::fromUtf8("重试 Session AI"), QString::fromUtf8("Retry Session AI")));
    if (nullptr != m_pViewSessionSummaryAction) m_pViewSessionSummaryAction->setText(uiText(QString::fromUtf8("查看 Session 总结"), QString::fromUtf8("View Session Summary")));
    if (nullptr != m_pCopySessionSummaryAction) m_pCopySessionSummaryAction->setText(uiText(QString::fromUtf8("复制 Session 总结"), QString::fromUtf8("Copy Session Summary")));
    if (nullptr != m_pAiSettingsAction) m_pAiSettingsAction->setText(uiText(QString::fromUtf8("设置"), QString::fromUtf8("Settings")));
    if (nullptr != m_pExportMarkdownAction) m_pExportMarkdownAction->setText(uiText(QString::fromUtf8("导出 Markdown"), QString::fromUtf8("Export Markdown")));
    if (nullptr != m_pRefreshAction) m_pRefreshAction->setText(uiText(QString::fromUtf8("刷新"), QString::fromUtf8("Refresh")));
    if (nullptr != m_pFocusSearchAction) m_pFocusSearchAction->setText(uiText(QString::fromUtf8("聚焦搜索"), QString::fromUtf8("Focus Search")));

    if (nullptr != m_pSessionSummaryTextEdit && m_pSessionSummaryTextEdit->toPlainText().trimmed().isEmpty())
        m_pSessionSummaryTextEdit->setPlainText(uiText(QString::fromUtf8("暂无 Session 总结"), QString::fromUtf8("No session summary available.")));
}


void QCMainWindow::updateMenuTexts()
{
    if (nullptr == menuBar())
        return;

    const QList<QAction *> vecMenuActions = menuBar()->actions();
    if (vecMenuActions.size() > 0 && nullptr != vecMenuActions.at(0))
        vecMenuActions.at(0)->setText(uiText(QString::fromUtf8("课程"), QString::fromUtf8("Course")));
    if (vecMenuActions.size() > 1 && nullptr != vecMenuActions.at(1))
        vecMenuActions.at(1)->setText(uiText(QString::fromUtf8("采集"), QString::fromUtf8("Capture")));
    if (vecMenuActions.size() > 2 && nullptr != vecMenuActions.at(2))
        vecMenuActions.at(2)->setText(QString::fromUtf8("AI"));
    if (vecMenuActions.size() > 3 && nullptr != vecMenuActions.at(3))
        vecMenuActions.at(3)->setText(uiText(QString::fromUtf8("工具"), QString::fromUtf8("Tools")));
}

void QCMainWindow::setupUi()
{
    setWindowTitle(uiText(QString::fromUtf8("QtClip - Learning Material Organizer"), QString::fromUtf8("QtClip - Learning Material Organizer")));
    resize(1360, 820);

    m_pSessionListWidget = new QListWidget(this);
    m_pSnippetListWidget = new QListWidget(this);
    m_pSessionListWidget->setVisible(false);
    m_pSnippetListWidget->setVisible(false);

    m_pNavigationSearchLineEdit = new QLineEdit(this);
    m_pNavigationTreeWidget = new QTreeWidget(this);

    m_pSnippetSearchLineEdit = new QLineEdit(this);
    m_pSearchHistoryComboBox = new QComboBox(this);
    m_pClearSearchButton = new QPushButton(QCoreApplication::translate("QCMainWindow", "Clear"), this);
    m_pClearSearchHistoryButton = new QPushButton(QCoreApplication::translate("QCMainWindow", "History"), this);
    m_pResetFiltersButton = new QPushButton(QCoreApplication::translate("QCMainWindow", "Reset"), this);

    m_pSidebarNewSessionButton = new QPushButton(QCoreApplication::translate("QCMainWindow", "New Course"), this);
    m_pSidebarSettingsButton = new QPushButton(QCoreApplication::translate("QCMainWindow", "Settings"), this);
    m_pPasteImageButton = new QPushButton(QCoreApplication::translate("QCMainWindow", "Paste Image"), this);
    m_pRunSummaryButton = new QPushButton(QString::fromUtf8("? ") + QCoreApplication::translate("QCMainWindow", "Run Summary"), this);
    m_pTopExportButton = new QPushButton(QCoreApplication::translate("QCMainWindow", "Export Markdown"), this);

    m_pQuickFavoriteCheckBox = nullptr;
    m_pQuickReviewCheckBox = nullptr;
    m_pFavoriteOnlyCheckBox = nullptr;
    m_pReviewOnlyCheckBox = nullptr;
    m_pSnippetFavoriteCheckBox = nullptr;
    m_pSnippetReviewCheckBox = nullptr;

    m_pSnippetTypeFilterComboBox = new QComboBox(this);
    m_pShowArchivedCheckBox = new QCheckBox(QCoreApplication::translate("QCMainWindow", "Show Archived"), this);
    m_pTagFilterComboBox = new QComboBox(this);
    QCLegacyFilterSupport::initializeAndHideLegacyFilterControls(m_pSnippetSearchLineEdit,
                                                                 m_pSearchHistoryComboBox,
                                                                 m_pClearSearchButton,
                                                                 m_pClearSearchHistoryButton,
                                                                 m_pResetFiltersButton,
                                                                 m_pSnippetTypeFilterComboBox,
                                                                 m_pShowArchivedCheckBox,
                                                                 m_pTagFilterComboBox);

    m_pSelectionContextLabel = new QLabel(this);
    m_pSelectionContextLabel->setWordWrap(true);
    m_pViewSummaryLabel = new QLabel(this);
    m_pViewSummaryLabel->setWordWrap(true);
    m_pAiStatusLabel = new QLabel(this);
    m_pAiStatusLabel->setWordWrap(true);

    m_pSessionSummaryTextEdit = new QPlainTextEdit(this);
    m_pSessionSummaryTextEdit->setReadOnly(true);
    m_pSessionSummaryTextEdit->setVisible(false);

    m_pSnippetSummaryTextEdit = new QPlainTextEdit(this);
    m_pSnippetSummaryTextEdit->setReadOnly(false);

    m_pSnippetTitleValueLabel = new QLabel(this);
    m_pSnippetTitleValueLabel->setWordWrap(true);
    m_pSnippetTitleValueLabel->setVisible(false);
    m_pSnippetTagsValueLabel = new QLabel(this);
    m_pSnippetTagsValueLabel->setWordWrap(true);
    m_pSnippetTagsValueLabel->setVisible(false);
    m_pSnippetNoteTextEdit = new QPlainTextEdit(this);
    m_pSnippetNoteTextEdit->setVisible(false);
    m_pSnippetContentTextEdit = new QPlainTextEdit(this);
    m_pSnippetContentTextEdit->setVisible(false);

    m_pImagePathValueLabel = new QLabel(this);
    m_pImagePathValueLabel->setWordWrap(true);
    m_pImagePathValueLabel->setVisible(false);
    m_pImagePreviewLabel = new QLabel(this);
    m_pImagePreviewLabel->setAlignment(Qt::AlignCenter);
    m_pImagePreviewLabel->setMinimumHeight(320);
    m_pImagePreviewLabel->setFrameShape(QFrame::StyledPanel);
    m_pImagePreviewLabel->installEventFilter(this);

    m_pSessionPanelTitleLabel = new QLabel(this);
    m_pSnippetPanelTitleLabel = nullptr;
    m_pDetailPanelTitleLabel = new QLabel(this);

    m_pNavigationSearchLineEdit->setPlaceholderText(QCoreApplication::translate("QCMainWindow", "Search course or screenshot title"));
    m_pNavigationSearchLineEdit->setClearButtonEnabled(true);

    m_pNavigationTreeWidget->setHeaderHidden(true);
    m_pNavigationTreeWidget->setRootIsDecorated(true);
    m_pNavigationTreeWidget->setUniformRowHeights(true);
    m_pNavigationTreeWidget->setIndentation(16);
    m_pNavigationTreeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_pNavigationTreeWidget->setContextMenuPolicy(Qt::ActionsContextMenu);
    m_pNavigationTreeWidget->header()->setStretchLastSection(true);
    m_pNavigationTreeWidget->setItemDelegate(new QCNavigationTreeDelegate(m_pNavigationTreeWidget));

    QWidget *pLeftPanelWidget = new QWidget(this);
    QVBoxLayout *pLeftPanelLayout = new QVBoxLayout(pLeftPanelWidget);
    pLeftPanelLayout->setSpacing(8);
    pLeftPanelLayout->setContentsMargins(10, 10, 10, 10);
    pLeftPanelLayout->addWidget(m_pSessionPanelTitleLabel);
    pLeftPanelLayout->addWidget(m_pSidebarNewSessionButton);
    pLeftPanelLayout->addWidget(m_pNavigationSearchLineEdit);
    pLeftPanelLayout->addWidget(m_pNavigationTreeWidget, 1);
    pLeftPanelLayout->addWidget(m_pSidebarSettingsButton);

    QWidget *pWorkspaceWidget = new QWidget(this);
    QVBoxLayout *pWorkspaceLayout = new QVBoxLayout(pWorkspaceWidget);
    pWorkspaceLayout->setSpacing(10);
    pWorkspaceLayout->setContentsMargins(12, 12, 12, 12);

    QWidget *pTopActionWidget = new QWidget(pWorkspaceWidget);
    QHBoxLayout *pTopActionLayout = new QHBoxLayout(pTopActionWidget);
    pTopActionLayout->setContentsMargins(0, 0, 0, 0);
    pTopActionLayout->setSpacing(8);
    pTopActionLayout->addWidget(m_pPasteImageButton);
    pTopActionLayout->addWidget(m_pRunSummaryButton);
    pTopActionLayout->addWidget(m_pTopExportButton);
    pTopActionLayout->addStretch(1);
    pWorkspaceLayout->addWidget(pTopActionWidget);

    m_pSelectionContextLabel->setVisible(false);
    m_pViewSummaryLabel->setVisible(false);
    m_pAiStatusLabel->setVisible(false);

    pWorkspaceLayout->addWidget(new QLabel(QCoreApplication::translate("QCMainWindow", "Preview"), pWorkspaceWidget));
    pWorkspaceLayout->addWidget(m_pImagePreviewLabel, 5);

    QWidget *pAnswerHeaderWidget = new QWidget(pWorkspaceWidget);
    QHBoxLayout *pAnswerHeaderLayout = new QHBoxLayout(pAnswerHeaderWidget);
    pAnswerHeaderLayout->setContentsMargins(0, 0, 0, 0);
    pAnswerHeaderLayout->setSpacing(8);
    pAnswerHeaderLayout->addWidget(new QLabel(QString::fromWCharArray(L"AI\u603b\u7ed3"), pAnswerHeaderWidget));
    QPushButton *pRetryInPlaceButton = new QPushButton(QCoreApplication::translate("QCMainWindow", "Retry"), pAnswerHeaderWidget);
    pAnswerHeaderLayout->addStretch(1);
    pAnswerHeaderLayout->addWidget(pRetryInPlaceButton);
    pWorkspaceLayout->addWidget(pAnswerHeaderWidget);
    m_pSnippetSummaryTextEdit->setPlaceholderText(QCoreApplication::translate("QCMainWindow", "Write and refine AI summary here"));
    pWorkspaceLayout->addWidget(m_pSnippetSummaryTextEdit, 4);

    m_pDetailPanelTitleLabel->setVisible(false);

    QSplitter *pMainSplitter = new QSplitter(this);
    pMainSplitter->setHandleWidth(1);
    pMainSplitter->addWidget(pLeftPanelWidget);
    pMainSplitter->addWidget(pWorkspaceWidget);
    pMainSplitter->setStretchFactor(0, 0);
    pMainSplitter->setStretchFactor(1, 1);
    pMainSplitter->setSizes(QList<int>() << 270 << 1090);
    setCentralWidget(pMainSplitter);

    pLeftPanelWidget->setObjectName(QString::fromUtf8("sessionPanelWidget"));
    pWorkspaceWidget->setObjectName(QString::fromUtf8("workspaceWidget"));
    m_pSessionListWidget->setObjectName(QString::fromUtf8("sessionListWidget"));
    m_pSnippetListWidget->setObjectName(QString::fromUtf8("snippetListWidget"));
    m_pNavigationSearchLineEdit->setObjectName(QString::fromUtf8("navigationSearchLineEdit"));
    m_pNavigationTreeWidget->setObjectName(QString::fromUtf8("navigationTreeWidget"));

    connect(m_pNavigationTreeWidget, &QTreeWidget::itemActivated, this, [this](QTreeWidgetItem *pItem, int nColumn) { onNavigationItemActivated(pItem, nColumn); });
    connect(m_pNavigationTreeWidget, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem *pItem, int nColumn) { onNavigationItemActivated(pItem, nColumn); });
    connect(m_pNavigationSearchLineEdit, &QLineEdit::textChanged, this, [this](const QString& strText) {
        if (nullptr != m_pSnippetSearchLineEdit)
            m_pSnippetSearchLineEdit->setText(strText);
        rebuildNavigationTree();
    });

    connect(m_pSessionListWidget, &QListWidget::itemSelectionChanged, this, [this]() { onSessionSelectionChanged(); });
    connect(m_pSnippetListWidget, &QListWidget::itemSelectionChanged, this, [this]() { onSnippetSelectionChanged(); });
    connect(m_pSidebarNewSessionButton, &QPushButton::clicked, this, [this]() { onCreateSession(); });
    connect(m_pSidebarSettingsButton, &QPushButton::clicked, this, [this]() { onAiSettings(); });
    connect(m_pPasteImageButton, &QPushButton::clicked, this, [this]() { onPasteImageFromClipboard(); });
    connect(m_pRunSummaryButton, &QPushButton::clicked, this, [this]() { onRunWorkspaceSummary(); });
    connect(m_pTopExportButton, &QPushButton::clicked, this, [this]() { onExportMarkdown(); });
    connect(pRetryInPlaceButton, &QPushButton::clicked, this, [this]() {
        if (hasSingleSelectedSnippet())
            onRetrySnippetSummary();
        else
            onRetrySessionSummary();
    });

    connect(qApp, &QApplication::focusChanged, this, [this](QWidget *pOld, QWidget *) {
        if (pOld == m_pSnippetSummaryTextEdit || pOld == m_pSnippetNoteTextEdit || pOld == m_pSnippetContentTextEdit)
            flushSnippetDraftToStorage(false);
    });

    statusBar()->showMessage(uiText(QString::fromUtf8("Ready: 1-New Course 2-Paste Image 3-Run Summary 4-Export"), QString::fromUtf8("Ready: 1-New Course 2-Paste Image 3-Run Summary 4-Export")));
    clearSnippetDetails();
    m_pSnippetSummaryTextEdit->setPlainText(QCoreApplication::translate("QCMainWindow", "No summary available."));
    loadTagFilterOptions();
    loadSearchHistoryOptions();
    applyLocalizedTexts();
    updateSelectionContextDisplay();
    updateAiStatusDisplay();
}
void QCMainWindow::setupActions()
{
    QToolBar *pToolBar = addToolBar(QString::fromUtf8("Main"));
    pToolBar->setObjectName(QString::fromUtf8("mainToolBar"));
    pToolBar->setMovable(false);
    pToolBar->setToolButtonStyle(Qt::ToolButtonTextOnly);
    pToolBar->setVisible(false);

    m_pNewSessionAction = pToolBar->addAction(QString::fromUtf8("New Session"));
    m_pEditSessionAction = pToolBar->addAction(QString::fromUtf8("Edit Session"));
    m_pNewTextSnippetAction = pToolBar->addAction(QString::fromUtf8("New Text"));
    m_pFinishSessionAction = pToolBar->addAction(QString::fromUtf8("Finish Session"));
    m_pReopenSessionAction = pToolBar->addAction(QString::fromUtf8("Reopen Session"));
    m_pEditSnippetAction = pToolBar->addAction(QString::fromUtf8("Edit Snippet"));
    m_pCaptureScreenAction = pToolBar->addAction(QString::fromUtf8("Capture Screen"));
    m_pCaptureRegionAction = pToolBar->addAction(QString::fromUtf8("Capture Region"));
    m_pImportImageAction = pToolBar->addAction(QString::fromUtf8("Import Image"));
    m_pDuplicateSnippetAction = pToolBar->addAction(uiText(QString::fromUtf8("复制 Snippet"), QString::fromUtf8("Duplicate Snippet")));
    m_pMoveSnippetAction = pToolBar->addAction(uiText(QString::fromUtf8("移动 Snippet"), QString::fromUtf8("Move Snippet")));
    m_pManageTagsAction = pToolBar->addAction(uiText(QString::fromUtf8("Snippet 标签"), QString::fromUtf8("Snippet Tags")));
    m_pClearSnippetTagsAction = pToolBar->addAction(uiText(QString::fromUtf8("清空 Snippet 标签"), QString::fromUtf8("Clear Snippet Tags")));
    m_pTagLibraryAction = pToolBar->addAction(uiText(QString::fromUtf8("标签库"), QString::fromUtf8("Tag Library")));
    m_pDeleteSnippetAction = pToolBar->addAction(uiText(QString::fromUtf8("删除 Snippet"), QString::fromUtf8("Delete Snippet")));
    m_pToggleArchiveSnippetAction = pToolBar->addAction(uiText(QString::fromUtf8("归档 / 恢复"), QString::fromUtf8("Archive / Restore")));
    m_pDeleteSessionAction = pToolBar->addAction(uiText(QString::fromUtf8("删除 Session"), QString::fromUtf8("Delete Session")));
    m_pSummarizeSnippetAction = pToolBar->addAction(uiText(QString::fromUtf8("总结 Snippet"), QString::fromUtf8("Summarize Snippet")));
    m_pRetrySnippetSummaryAction = pToolBar->addAction(uiText(QString::fromUtf8("重试 Snippet AI"), QString::fromUtf8("Retry Snippet AI")));
    m_pViewSnippetSummaryAction = pToolBar->addAction(uiText(QString::fromUtf8("查看 Snippet 总结"), QString::fromUtf8("View Snippet Summary")));
    m_pCopySnippetSummaryAction = pToolBar->addAction(uiText(QString::fromUtf8("复制 Snippet 总结"), QString::fromUtf8("Copy Snippet Summary")));
    m_pSummarizeSessionAction = pToolBar->addAction(uiText(QString::fromUtf8("总结 Session"), QString::fromUtf8("Summarize Session")));
    m_pRetrySessionSummaryAction = pToolBar->addAction(uiText(QString::fromUtf8("重试 Session AI"), QString::fromUtf8("Retry Session AI")));
    m_pViewSessionSummaryAction = pToolBar->addAction(uiText(QString::fromUtf8("查看 Session 总结"), QString::fromUtf8("View Session Summary")));
    m_pCopySessionSummaryAction = pToolBar->addAction(uiText(QString::fromUtf8("复制 Session 总结"), QString::fromUtf8("Copy Session Summary")));
    m_pAiSettingsAction = pToolBar->addAction(uiText(QString::fromUtf8("设置"), QString::fromUtf8("Settings")));
    m_pExportMarkdownAction = pToolBar->addAction(uiText(QString::fromUtf8("导出 Markdown"), QString::fromUtf8("Export Markdown")));
    m_pRefreshAction = pToolBar->addAction(uiText(QString::fromUtf8("刷新"), QString::fromUtf8("Refresh")));

    m_pEditCurrentAction = new QAction(uiText(QString::fromUtf8("编辑当前项"), QString::fromUtf8("Edit Current")), this);
    m_pDeleteCurrentAction = new QAction(uiText(QString::fromUtf8("删除当前项"), QString::fromUtf8("Delete Current")), this);
    m_pFocusSearchAction = new QAction(uiText(QString::fromUtf8("聚焦搜索"), QString::fromUtf8("Focus Search")), this);
    m_pPasteImageAction = new QAction(uiText(QString::fromUtf8("粘贴图片"), QString::fromUtf8("Paste Image")), this);

    const Qt::ShortcutContext shortcutContext = Qt::WidgetWithChildrenShortcut;
    m_pNewSessionAction->setShortcut(QKeySequence(QString::fromUtf8("Ctrl+Shift+N")));
    m_pNewTextSnippetAction->setShortcuts(QList<QKeySequence>()
        << QKeySequence::New
        << QKeySequence(QString::fromUtf8("Ctrl+Alt+N")));
    m_pCaptureScreenAction->setShortcut(QKeySequence(QString::fromUtf8("Ctrl+Shift+S")));
    m_pCaptureRegionAction->setShortcut(QKeySequence(QString::fromUtf8("Ctrl+Shift+R")));
    m_pImportImageAction->setShortcut(QKeySequence(QString::fromUtf8("Ctrl+Shift+I")));
    m_pDuplicateSnippetAction->setShortcut(QKeySequence(QString::fromUtf8("Ctrl+D")));
    m_pMoveSnippetAction->setShortcut(QKeySequence(QString::fromUtf8("Ctrl+Alt+V")));
    m_pManageTagsAction->setShortcut(QKeySequence());
    m_pTagLibraryAction->setShortcut(QKeySequence());
    m_pSummarizeSnippetAction->setShortcut(QKeySequence(QString::fromUtf8("Ctrl+Shift+M")));
    m_pRetrySnippetSummaryAction->setShortcut(QKeySequence(QString::fromUtf8("Ctrl+Shift+Y")));
    m_pViewSnippetSummaryAction->setShortcut(QKeySequence(QString::fromUtf8("Ctrl+Shift+L")));
    m_pCopySnippetSummaryAction->setShortcut(QKeySequence(QString::fromUtf8("Ctrl+Shift+C")));
    m_pSummarizeSessionAction->setShortcut(QKeySequence(QString::fromUtf8("Ctrl+Alt+M")));
    m_pRetrySessionSummaryAction->setShortcut(QKeySequence(QString::fromUtf8("Ctrl+Alt+Y")));
    m_pViewSessionSummaryAction->setShortcut(QKeySequence(QString::fromUtf8("Ctrl+Alt+L")));
    m_pCopySessionSummaryAction->setShortcut(QKeySequence(QString::fromUtf8("Ctrl+Alt+C")));
    m_pAiSettingsAction->setShortcut(QKeySequence(QString::fromUtf8("Ctrl+,")));
    m_pExportMarkdownAction->setShortcut(QKeySequence(QString::fromUtf8("Ctrl+Shift+E")));
    m_pRefreshAction->setShortcuts(QList<QKeySequence>() << QKeySequence::Refresh << QKeySequence(QString::fromUtf8("Ctrl+R")));
    m_pEditCurrentAction->setShortcut(QKeySequence(Qt::Key_F2));
    m_pDeleteCurrentAction->setShortcut(QKeySequence::Delete);
    m_pFocusSearchAction->setShortcuts(QList<QKeySequence>() << QKeySequence::Find << QKeySequence(QString::fromUtf8("/")));
    m_pPasteImageAction->setShortcut(QKeySequence::Paste);

    const QList<QAction *> vecActions = QList<QAction *>()
        << m_pNewSessionAction
        << m_pEditSessionAction
        << m_pNewTextSnippetAction
        << m_pFinishSessionAction
        << m_pReopenSessionAction
        << m_pEditSnippetAction
        << m_pCaptureScreenAction
        << m_pCaptureRegionAction
        << m_pImportImageAction
        << m_pDuplicateSnippetAction
        << m_pMoveSnippetAction
        << m_pManageTagsAction
        << m_pClearSnippetTagsAction
        << m_pTagLibraryAction
        << m_pDeleteSnippetAction
        << m_pToggleArchiveSnippetAction
        << m_pDeleteSessionAction
        << m_pSummarizeSnippetAction
        << m_pRetrySnippetSummaryAction
        << m_pViewSnippetSummaryAction
        << m_pCopySnippetSummaryAction
        << m_pSummarizeSessionAction
        << m_pRetrySessionSummaryAction
        << m_pViewSessionSummaryAction
        << m_pCopySessionSummaryAction
        << m_pAiSettingsAction
        << m_pExportMarkdownAction
        << m_pRefreshAction
        << m_pEditCurrentAction
        << m_pDeleteCurrentAction
        << m_pFocusSearchAction
        << m_pPasteImageAction;
    for (int i = 0; i < vecActions.size(); ++i)
    {
        QAction *pAction = vecActions.at(i);
        if (nullptr == pAction)
            continue;

        pAction->setShortcutContext(shortcutContext);
        if (pAction->parent() == this)
            addAction(pAction);
    }

    connect(m_pNewSessionAction, &QAction::triggered, this, [this]() { onCreateSession(); });
    connect(m_pEditSessionAction, &QAction::triggered, this, [this]() { onEditSession(); });
    connect(m_pNewTextSnippetAction, &QAction::triggered, this, [this]() { onCreateTextSnippet(); });
    connect(m_pFinishSessionAction, &QAction::triggered, this, [this]() { onFinishSession(); });
    connect(m_pReopenSessionAction, &QAction::triggered, this, [this]() { onReopenSession(); });
    connect(m_pEditSnippetAction, &QAction::triggered, this, [this]() { onEditSnippet(); });
    connect(m_pCaptureScreenAction, &QAction::triggered, this, [this]() { onCaptureScreen(); });
    connect(m_pCaptureRegionAction, &QAction::triggered, this, [this]() { onCaptureRegion(); });
    connect(m_pImportImageAction, &QAction::triggered, this, [this]() { onImportImageSnippet(); });
    connect(m_pDuplicateSnippetAction, &QAction::triggered, this, [this]() { onDuplicateSnippet(); });
    connect(m_pMoveSnippetAction, &QAction::triggered, this, [this]() { onMoveSnippet(); });
    connect(m_pManageTagsAction, &QAction::triggered, this, [this]() { onManageTags(); });
    connect(m_pClearSnippetTagsAction, &QAction::triggered, this, [this]() { onClearSnippetTags(); });
    connect(m_pTagLibraryAction, &QAction::triggered, this, [this]() { onOpenTagLibrary(); });
    connect(m_pDeleteSnippetAction, &QAction::triggered, this, [this]() { onDeleteSnippet(); });
    connect(m_pToggleArchiveSnippetAction, &QAction::triggered, this, [this]() { onToggleArchiveSnippet(); });
    connect(m_pDeleteSessionAction, &QAction::triggered, this, [this]() { onDeleteSession(); });
    connect(m_pSummarizeSnippetAction, &QAction::triggered, this, [this]() { onSummarizeSnippet(); });
    connect(m_pRetrySnippetSummaryAction, &QAction::triggered, this, [this]() { onRetrySnippetSummary(); });
    connect(m_pViewSnippetSummaryAction, &QAction::triggered, this, [this]() { onViewSnippetSummary(); });
    connect(m_pCopySnippetSummaryAction, &QAction::triggered, this, [this]() { onCopySnippetSummary(); });
    connect(m_pSummarizeSessionAction, &QAction::triggered, this, [this]() { onSummarizeSession(); });
    connect(m_pRetrySessionSummaryAction, &QAction::triggered, this, [this]() { onRetrySessionSummary(); });
    connect(m_pViewSessionSummaryAction, &QAction::triggered, this, [this]() { onViewSessionSummary(); });
    connect(m_pCopySessionSummaryAction, &QAction::triggered, this, [this]() { onCopySessionSummary(); });
    connect(m_pAiSettingsAction, &QAction::triggered, this, [this]() { onAiSettings(); });
    connect(m_pExportMarkdownAction, &QAction::triggered, this, [this]() { onExportMarkdown(); });
    connect(m_pRefreshAction, &QAction::triggered, this, [this]() { onRefresh(); });
    connect(m_pEditCurrentAction, &QAction::triggered, this, [this]() { onEditCurrentItem(); });
    connect(m_pDeleteCurrentAction, &QAction::triggered, this, [this]() { onDeleteCurrentItem(); });
    connect(m_pFocusSearchAction, &QAction::triggered, this, [this]() { onFocusSearch(); });
    connect(m_pPasteImageAction, &QAction::triggered, this, [this]() { onPasteImageFromClipboard(); });

    m_pNewSessionAction->setObjectName(QString::fromUtf8("newSessionAction"));
    m_pEditSessionAction->setObjectName(QString::fromUtf8("editSessionAction"));
    m_pNewTextSnippetAction->setObjectName(QString::fromUtf8("newTextSnippetAction"));
    m_pFinishSessionAction->setObjectName(QString::fromUtf8("finishSessionAction"));
    m_pReopenSessionAction->setObjectName(QString::fromUtf8("reopenSessionAction"));
    m_pEditSnippetAction->setObjectName(QString::fromUtf8("editSnippetAction"));
    m_pCaptureScreenAction->setObjectName(QString::fromUtf8("captureScreenAction"));
    m_pCaptureRegionAction->setObjectName(QString::fromUtf8("captureRegionAction"));
    m_pImportImageAction->setObjectName(QString::fromUtf8("importImageAction"));
    m_pDuplicateSnippetAction->setObjectName(QString::fromUtf8("duplicateSnippetAction"));
    m_pMoveSnippetAction->setObjectName(QString::fromUtf8("moveSnippetAction"));
    m_pDeleteSessionAction->setObjectName(QString::fromUtf8("deleteSessionAction"));
    m_pToggleArchiveSnippetAction->setObjectName(QString::fromUtf8("toggleArchiveSnippetAction"));
    m_pDeleteSnippetAction->setObjectName(QString::fromUtf8("deleteSnippetAction"));
    m_pManageTagsAction->setObjectName(QString::fromUtf8("manageTagsAction"));
    m_pClearSnippetTagsAction->setObjectName(QString::fromUtf8("clearSnippetTagsAction"));
    m_pTagLibraryAction->setObjectName(QString::fromUtf8("tagLibraryAction"));
    m_pSummarizeSnippetAction->setObjectName(QString::fromUtf8("summarizeSnippetAction"));
    m_pRetrySnippetSummaryAction->setObjectName(QString::fromUtf8("retrySnippetSummaryAction"));
    m_pViewSnippetSummaryAction->setObjectName(QString::fromUtf8("viewSnippetSummaryAction"));
    m_pCopySnippetSummaryAction->setObjectName(QString::fromUtf8("copySnippetSummaryAction"));
    m_pSummarizeSessionAction->setObjectName(QString::fromUtf8("summarizeSessionAction"));
    m_pRetrySessionSummaryAction->setObjectName(QString::fromUtf8("retrySessionSummaryAction"));
    m_pViewSessionSummaryAction->setObjectName(QString::fromUtf8("viewSessionSummaryAction"));
    m_pCopySessionSummaryAction->setObjectName(QString::fromUtf8("copySessionSummaryAction"));
    m_pAiSettingsAction->setObjectName(QString::fromUtf8("aiSettingsAction"));
    m_pExportMarkdownAction->setObjectName(QString::fromUtf8("exportMarkdownAction"));
    m_pRefreshAction->setObjectName(QString::fromUtf8("refreshAction"));
    m_pEditCurrentAction->setObjectName(QString::fromUtf8("editCurrentAction"));
    m_pDeleteCurrentAction->setObjectName(QString::fromUtf8("deleteCurrentAction"));
    m_pFocusSearchAction->setObjectName(QString::fromUtf8("focusSearchAction"));
    m_pPasteImageAction->setObjectName(QString::fromUtf8("pasteImageAction"));

    QMenu *pSessionMenu = menuBar()->addMenu(uiText(QString::fromUtf8("Course"), QString::fromUtf8("Course")));
    pSessionMenu->addAction(m_pNewSessionAction);
    pSessionMenu->addAction(m_pEditSessionAction);
    pSessionMenu->addAction(m_pFinishSessionAction);
    pSessionMenu->addAction(m_pReopenSessionAction);
    pSessionMenu->addAction(m_pDeleteSessionAction);

    QMenu *pCaptureMenu = menuBar()->addMenu(uiText(QString::fromUtf8("Capture"), QString::fromUtf8("Capture")));
    pCaptureMenu->addAction(m_pPasteImageAction);
    pCaptureMenu->addAction(m_pImportImageAction);
    pCaptureMenu->addAction(m_pCaptureScreenAction);
    pCaptureMenu->addAction(m_pCaptureRegionAction);

    QMenu *pAiMenu = menuBar()->addMenu(QString::fromUtf8("AI"));
    pAiMenu->addAction(m_pSummarizeSnippetAction);
    pAiMenu->addAction(m_pRetrySnippetSummaryAction);
    pAiMenu->addAction(m_pCopySnippetSummaryAction);
    pAiMenu->addSeparator();
    pAiMenu->addAction(m_pAiSettingsAction);

    QMenu *pToolsMenu = menuBar()->addMenu(uiText(QString::fromUtf8("Tools"), QString::fromUtf8("Tools")));
    pToolsMenu->addAction(m_pExportMarkdownAction);
    pToolsMenu->addAction(m_pRefreshAction);
    pToolsMenu->addAction(m_pFocusSearchAction);

    if (nullptr != m_pSessionListWidget)
    {
        m_pSessionListWidget->addAction(m_pEditSessionAction);
        m_pSessionListWidget->addAction(m_pFinishSessionAction);
        m_pSessionListWidget->addAction(m_pReopenSessionAction);
        m_pSessionListWidget->addAction(m_pSummarizeSessionAction);
        m_pSessionListWidget->addAction(m_pRetrySessionSummaryAction);
        m_pSessionListWidget->addAction(m_pViewSessionSummaryAction);
        m_pSessionListWidget->addAction(m_pCopySessionSummaryAction);
        m_pSessionListWidget->addAction(m_pExportMarkdownAction);
        m_pSessionListWidget->addAction(m_pDeleteSessionAction);
    }

    if (nullptr != m_pSnippetListWidget)
    {
        m_pSnippetListWidget->addAction(m_pEditSnippetAction);
        m_pSnippetListWidget->addAction(m_pDuplicateSnippetAction);
        m_pSnippetListWidget->addAction(m_pMoveSnippetAction);
        m_pSnippetListWidget->addAction(m_pManageTagsAction);
        m_pSnippetListWidget->addAction(m_pClearSnippetTagsAction);
        m_pSnippetListWidget->addAction(m_pSummarizeSnippetAction);
        m_pSnippetListWidget->addAction(m_pRetrySnippetSummaryAction);
        m_pSnippetListWidget->addAction(m_pViewSnippetSummaryAction);
        m_pSnippetListWidget->addAction(m_pCopySnippetSummaryAction);
        m_pSnippetListWidget->addAction(m_pToggleArchiveSnippetAction);
        m_pSnippetListWidget->addAction(m_pDeleteSnippetAction);
    }

    updateActionState();
}
void QCMainWindow::loadSessions()
{
    flushSnippetDraftToStorage(false);
    const qint64 nPreviousSessionId = currentSessionId();
    QSignalBlocker sessionBlocker(m_pSessionListWidget);
    m_pSessionListWidget->clear();
    loadTagFilterOptions();

    const QVector<QCStudySession> vecSessions = m_pSessionService->listSessions();
    if (!m_pSessionService->lastError().isEmpty())
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("加载 Session"), QString::fromUtf8("Load Sessions")), m_pSessionService->lastError());
        return;
    }

    for (int i = 0; i < vecSessions.size(); ++i)
    {
        const QCStudySession& session = vecSessions.at(i);
        const QString strIndexedSessionText = QString::fromUtf8("%1. %2").arg(i + 1).arg(SessionItemText(session));
        QListWidgetItem *pItem = new QListWidgetItem(strIndexedSessionText, m_pSessionListWidget);
        pItem->setData(Qt::UserRole, session.id());
        pItem->setToolTip(session.description());
    }

    if (m_pSessionListWidget->count() > 0)
    {
        bool bRestored = false;
        if (nPreviousSessionId > 0)
        {
            for (int i = 0; i < m_pSessionListWidget->count(); ++i)
            {
                QListWidgetItem *pItem = m_pSessionListWidget->item(i);
                if (pItem->data(Qt::UserRole).toLongLong() == nPreviousSessionId)
                {
                    m_pSessionListWidget->setCurrentItem(pItem);
                    bRestored = true;
                    break;
                }
            }
        }

        if (!bRestored)
            m_pSessionListWidget->setCurrentRow(0);
    }

    sessionBlocker.unblock();
    if (m_pSessionListWidget->currentItem() != nullptr)
    {
        onSessionSelectionChanged();
        return;
    }

    QSignalBlocker snippetBlocker(m_pSnippetListWidget);
    m_pSnippetListWidget->clear();
    clearSnippetDetails();
    m_pSessionSummaryTextEdit->setPlainText(uiText(QString::fromUtf8("暂无 Session 总结"), QString::fromUtf8("No session summary available.")));
    updateViewSummary(0, 0);
    updateActionState();
}

void QCMainWindow::rebuildNavigationTree()
{
    if (nullptr == m_pNavigationTreeWidget || nullptr == m_pSessionService || nullptr == m_pSnippetService)
        return;

    const qint64 nCurrentSessionId = currentSessionId();
    const qint64 nCurrentSnippetId = currentSnippetId();
    const QString strSearchText = currentSearchText().trimmed();

    const QVector<QCStudySession> vecSessions = m_pSessionService->listSessions();
    if (!m_pSessionService->lastError().isEmpty())
        return;

    m_bSyncingNavigationSelection = true;
    QSignalBlocker blocker(m_pNavigationTreeWidget);
    m_pNavigationTreeWidget->clear();

    const int nTypeRole = Qt::UserRole + 1;
    const int nSessionRole = Qt::UserRole + 2;
    const int nSnippetRole = Qt::UserRole + 3;

    for (int i = 0; i < vecSessions.size(); ++i)
    {
        const QCStudySession& session = vecSessions.at(i);
        const QString strSessionTitle = session.title().trimmed().isEmpty()
            ? uiText(QString::fromUtf8("Untitled Course"), QString::fromUtf8("Untitled Course"))
            : session.title().trimmed();
        const QString strCourseName = session.courseName().trimmed();
        const QString strSessionNeedle = (strSessionTitle + QString::fromUtf8(" ") + strCourseName).trimmed();
        const bool bSessionMatched = strSearchText.isEmpty() || strSessionNeedle.contains(strSearchText, Qt::CaseInsensitive);

        const QVector<QCSnippet> vecSnippets = m_pSnippetService->listSnippetsBySession(session.id());
        if (!m_pSnippetService->lastError().isEmpty())
            continue;

        QVector<QCSnippet> vecVisibleSnippets;
        for (int j = 0; j < vecSnippets.size(); ++j)
        {
            const QCSnippet& snippet = vecSnippets.at(j);
            const QString strSnippetTitle = snippet.title().trimmed().isEmpty()
                ? uiText(QString::fromUtf8("Untitled Screenshot"), QString::fromUtf8("Untitled Screenshot"))
                : snippet.title().trimmed();
            if (strSearchText.isEmpty() || bSessionMatched || strSnippetTitle.contains(strSearchText, Qt::CaseInsensitive))
                vecVisibleSnippets.append(snippet);
        }

        if (!bSessionMatched && vecVisibleSnippets.isEmpty())
            continue;

        QTreeWidgetItem *pSessionItem = new QTreeWidgetItem(m_pNavigationTreeWidget);
        pSessionItem->setText(0, QString::fromUtf8("%1").arg(strSessionTitle));
        pSessionItem->setData(0, nTypeRole, QString::fromUtf8("session"));
        pSessionItem->setData(0, nSessionRole, session.id());
        pSessionItem->setData(0, nSnippetRole, 0);
        pSessionItem->setToolTip(0, strCourseName.isEmpty() ? strSessionTitle : QString::fromUtf8("%1\n%2").arg(strSessionTitle, strCourseName));

        for (int j = 0; j < vecVisibleSnippets.size(); ++j)
        {
            const QCSnippet& snippet = vecVisibleSnippets.at(j);
            const QString strSnippetTitle = snippet.title().trimmed().isEmpty()
                ? uiText(QString::fromUtf8("Untitled Screenshot"), QString::fromUtf8("Untitled Screenshot"))
                : snippet.title().trimmed();
            const QString strTime = snippet.capturedAt().isValid()
                ? snippet.capturedAt().toLocalTime().toString(QString::fromUtf8("MM-dd hh:mm"))
                : QString::fromUtf8("--:--");

            QTreeWidgetItem *pSnippetItem = new QTreeWidgetItem(pSessionItem);
            pSnippetItem->setText(0, QString::fromUtf8("[%1] %2").arg(strTime, strSnippetTitle));
            pSnippetItem->setData(0, nTypeRole, QString::fromUtf8("snippet"));
            pSnippetItem->setData(0, nSessionRole, session.id());
            pSnippetItem->setData(0, nSnippetRole, snippet.id());
            pSnippetItem->setToolTip(0, strSnippetTitle);
        }

        pSessionItem->setExpanded(true);
    }

    QTreeWidgetItem *pTargetItem = nullptr;
    if (nCurrentSnippetId > 0)
    {
        for (int i = 0; i < m_pNavigationTreeWidget->topLevelItemCount() && nullptr == pTargetItem; ++i)
        {
            QTreeWidgetItem *pSessionItem = m_pNavigationTreeWidget->topLevelItem(i);
            if (nullptr == pSessionItem)
                continue;

            for (int j = 0; j < pSessionItem->childCount(); ++j)
            {
                QTreeWidgetItem *pSnippetItem = pSessionItem->child(j);
                if (nullptr != pSnippetItem && pSnippetItem->data(0, nSnippetRole).toLongLong() == nCurrentSnippetId)
                {
                    pTargetItem = pSnippetItem;
                    break;
                }
            }
        }
    }

    if (nullptr == pTargetItem && nCurrentSessionId > 0)
    {
        for (int i = 0; i < m_pNavigationTreeWidget->topLevelItemCount(); ++i)
        {
            QTreeWidgetItem *pSessionItem = m_pNavigationTreeWidget->topLevelItem(i);
            if (nullptr != pSessionItem && pSessionItem->data(0, nSessionRole).toLongLong() == nCurrentSessionId)
            {
                pTargetItem = pSessionItem;
                break;
            }
        }
    }

    if (nullptr != pTargetItem)
    {
        m_pNavigationTreeWidget->setCurrentItem(pTargetItem);
        m_pNavigationTreeWidget->scrollToItem(pTargetItem);
    }

    m_bSyncingNavigationSelection = false;
}

void QCMainWindow::onNavigationItemActivated(QTreeWidgetItem *pItem, int nColumn)
{
    Q_UNUSED(nColumn);
    if (nullptr == pItem || m_bSyncingNavigationSelection || nullptr == m_pSessionListWidget)
        return;

    const QString strType = pItem->data(0, Qt::UserRole + 1).toString();
    const qint64 nSessionId = pItem->data(0, Qt::UserRole + 2).toLongLong();
    const qint64 nSnippetId = pItem->data(0, Qt::UserRole + 3).toLongLong();

    if (nSessionId <= 0)
        return;

    QListWidgetItem *pSessionTarget = nullptr;
    for (int i = 0; i < m_pSessionListWidget->count(); ++i)
    {
        QListWidgetItem *pSessionItem = m_pSessionListWidget->item(i);
        if (nullptr != pSessionItem && pSessionItem->data(Qt::UserRole).toLongLong() == nSessionId)
        {
            pSessionTarget = pSessionItem;
            break;
        }
    }

    if (nullptr == pSessionTarget)
        return;

    if (m_pSessionListWidget->currentItem() != pSessionTarget)
        m_pSessionListWidget->setCurrentItem(pSessionTarget);

    if (strType == QString::fromUtf8("session"))
        return;

    if (nSnippetId > 0)
    {
        if (!selectSnippetById(nSnippetId))
            loadSnippets(nSessionId);

        if (selectSnippetById(nSnippetId))
            onSnippetSelectionChanged();
    }
}

void QCMainWindow::loadTagFilterOptions()
{
    if (nullptr == m_pTagFilterComboBox)
        return;

    const qint64 nCurrentTagId = currentTagFilterId();
    m_pTagFilterComboBox->blockSignals(true);
    m_pTagFilterComboBox->clear();
    m_pTagFilterComboBox->addItem(uiText(QString::fromUtf8("所有标签"), QString::fromUtf8("All Tags")), 0);

    if (nullptr != m_pTagService)
    {
        const QVector<QCTag> vecTags = m_pTagService->listTags();
        if (m_pTagService->lastError().isEmpty())
        {
            for (int i = 0; i < vecTags.size(); ++i)
                m_pTagFilterComboBox->addItem(vecTags.at(i).name(), vecTags.at(i).id());
        }
    }

    int nRestoreIndex = 0;
    for (int i = 0; i < m_pTagFilterComboBox->count(); ++i)
    {
        if (m_pTagFilterComboBox->itemData(i).toLongLong() == nCurrentTagId)
        {
            nRestoreIndex = i;
            break;
        }
    }

    m_pTagFilterComboBox->setCurrentIndex(nRestoreIndex);
    m_pTagFilterComboBox->blockSignals(false);
}

void QCMainWindow::loadSnippets(qint64 nSessionId)
{
    Q_UNUSED(nSessionId);
    applySnippetFilters();
}

void QCMainWindow::applySnippetFilters()
{
    flushSnippetDraftToStorage(false);
    const qint64 nSessionId = currentSessionId();
    const QVector<qint64> vecPreviousSnippetIds = selectedSnippetIds();

    QSignalBlocker snippetBlocker(m_pSnippetListWidget);
    m_pSnippetListWidget->clear();
    clearSnippetDetails();

    if (nSessionId <= 0)
    {
        updateViewSummary(0, 0);
        rebuildNavigationTree();
        updateActionState();
        return;
    }

    const QVector<QCSnippet> vecAllSnippets = m_pSnippetService->listSnippetsBySession(nSessionId);
    if (!m_pSnippetService->lastError().isEmpty())
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("加载 Snippet"), QString::fromUtf8("Load Snippets")), m_pSnippetService->lastError());
        return;
    }

    const QString strSearchText = currentSearchText();
    const QVector<QCSnippet> vecSnippets = m_pSnippetService->querySnippets(nSessionId,
                                                                            strSearchText,
                                                                            (nullptr != m_pFavoriteOnlyCheckBox) ? m_pFavoriteOnlyCheckBox->isChecked() : false,
                                                                            (nullptr != m_pReviewOnlyCheckBox) ? m_pReviewOnlyCheckBox->isChecked() : false,
                                                                            currentTagFilterId());
    if (!m_pSnippetService->lastError().isEmpty())
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("加载 Snippet"), QString::fromUtf8("Load Snippets")), m_pSnippetService->lastError());
        return;
    }

    const bool bShowArchived = (nullptr != m_pShowArchivedCheckBox) ? m_pShowArchivedCheckBox->isChecked() : false;
    for (int i = 0; i < vecSnippets.size(); ++i)
    {
        const QCSnippet& snippet = vecSnippets.at(i);
        if (snippet.isArchived() && !bShowArchived)
            continue;
        if (!matchesSnippetTypeFilter(snippet))
            continue;

        QListWidgetItem *pItem = new QListWidgetItem(SnippetItemText(snippet), m_pSnippetListWidget);
        pItem->setData(Qt::UserRole, snippet.id());
    }

    updateViewSummary(m_pSnippetListWidget->count(), vecAllSnippets.size());

    if (m_pSnippetListWidget->count() > 0)
    {
        if (!selectSnippetsByIds(vecPreviousSnippetIds))
            m_pSnippetListWidget->setCurrentRow(0);
    }

    rebuildNavigationTree();
    snippetBlocker.unblock();
    if (m_pSnippetListWidget->currentItem() != nullptr)
    {
        onSnippetSelectionChanged();
        return;
    }

    clearSnippetDetails();
    updateActionState();
}

void QCMainWindow::showSessionSummary(qint64 nSessionId)
{
    m_pSessionSummaryTextEdit->setPlainText(uiText(QString::fromUtf8("暂无 Session 总结"), QString::fromUtf8("No session summary available.")));

    if (nSessionId <= 0)
    {
        applyPlainTextHighlight(m_pSessionSummaryTextEdit, currentSearchText());
        return;
    }

    const QVector<QCAiRecord> vecAiRecords = m_pAiService->listAiRecordsBySession(nSessionId);
    if (!m_pAiService->lastError().isEmpty())
    {
        applyPlainTextHighlight(m_pSessionSummaryTextEdit, currentSearchText());
        return;
    }

    QString strSessionSummary = SanitizeSummaryForUi(findSessionSummaryFromAiRecords(vecAiRecords));
    if (IsPromptLikeSummary(strSessionSummary))
        strSessionSummary.clear();
    if (!strSessionSummary.trimmed().isEmpty())
        m_pSessionSummaryTextEdit->setPlainText(strSessionSummary);

    m_pSnippetSummaryTextEdit->setPlainText(m_pSessionSummaryTextEdit->toPlainText());
    applyPlainTextHighlight(m_pSessionSummaryTextEdit, currentSearchText());
    applyPlainTextHighlight(m_pSnippetSummaryTextEdit, currentSearchText());
}
void QCMainWindow::showSnippetDetails(qint64 nSnippetId)
{
    clearSnippetDetails();

    if (nSnippetId <= 0)
        return;

    QCSnippet snippet;
    if (!m_pSnippetService->getSnippetById(nSnippetId, &snippet))
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("加载 Snippet"), QString::fromUtf8("Load Snippet")), m_pSnippetService->lastError());
        return;
    }

    const QString strSearchText = currentSearchText();
    m_pSnippetTitleValueLabel->setTextFormat(Qt::RichText);
    m_pSnippetTitleValueLabel->setText(buildHighlightedHtml(snippet.title().trimmed().isEmpty()
        ? QString::fromUtf8("Untitled Snippet")
        : snippet.title(),
        strSearchText));
    m_bPopulatingSnippetEditors = true;
    m_pSnippetNoteTextEdit->setPlainText(snippet.note());
    m_pSnippetContentTextEdit->setPlainText(snippet.contentText());
    m_bPopulatingSnippetEditors = false;
    m_nDisplayedSnippetId = nSnippetId;

    refreshSnippetTagsDisplay(nSnippetId);

    QString strSummary = SanitizeSummaryForUi(snippet.summary());
    const QVector<QCAiRecord> vecAiRecords = m_pAiService->listAiRecordsBySnippet(nSnippetId);
    if (m_pAiService->lastError().isEmpty() && strSummary.trimmed().isEmpty())
        strSummary = SanitizeSummaryForUi(findSummaryFromAiRecords(vecAiRecords));
    if (IsPromptLikeSummary(strSummary))
        strSummary = QString::fromUtf8("Detected an old prompt-style result. Click Run Summary to regenerate.");
    m_pSnippetSummaryTextEdit->setPlainText(strSummary);
    applyPlainTextHighlight(m_pSnippetNoteTextEdit, strSearchText);
    applyPlainTextHighlight(m_pSnippetContentTextEdit, strSearchText);
    applyPlainTextHighlight(m_pSnippetSummaryTextEdit, strSearchText);
    updateSnippetStateControls(snippet);

    if (snippet.type() == QCSnippetType::ImageSnippetType)
    {
        QCAttachment primaryAttachment;
        if (m_pSnippetService->getPrimaryAttachmentBySnippetId(nSnippetId, &primaryAttachment))
        {
            updateDetailImage(primaryAttachment.filePath());
        }
        else
        {
            m_pImagePathValueLabel->setText(uiText(QString::fromUtf8("图片附件不可用"), QString::fromUtf8("Image attachment unavailable.")));
            m_pImagePreviewLabel->setText(uiText(QString::fromUtf8("暂无图片预览"), QString::fromUtf8("No image preview available.")));
        }
    }
    else
    {
        m_pImagePathValueLabel->setText(uiText(QString::fromUtf8("无图片附件"), QString::fromUtf8("No image attachment.")));
        m_pImagePreviewLabel->setText(uiText(QString::fromUtf8("暂无图片预览"), QString::fromUtf8("No image preview available.")));
    }

    updateActionState();
}

void QCMainWindow::clearSnippetDetails()
{
    m_pSnippetTitleValueLabel->setTextFormat(Qt::PlainText);
    m_pSnippetTitleValueLabel->clear();
    m_pSnippetTagsValueLabel->setText(uiText(QString::fromUtf8("无标签"), QString::fromUtf8("No tags.")));
    m_pSnippetNoteTextEdit->clear();
    m_pSnippetContentTextEdit->clear();
    m_bPopulatingSnippetEditors = true;
    m_pSnippetSummaryTextEdit->clear();
    applyPlainTextHighlight(m_pSnippetNoteTextEdit, QString());
    applyPlainTextHighlight(m_pSnippetContentTextEdit, QString());
    applyPlainTextHighlight(m_pSnippetSummaryTextEdit, QString());
    m_bPopulatingSnippetEditors = false;
    m_nDisplayedSnippetId = 0;
    m_pImagePathValueLabel->clear();
    m_pImagePreviewLabel->clear();
    m_pImagePreviewLabel->setPixmap(QPixmap());
    m_pImagePreviewLabel->setText(uiText(QString::fromUtf8("暂无图片预览"), QString::fromUtf8("No image preview available.")));

    m_bUpdatingSnippetStateControls = true;
    if (nullptr != m_pQuickFavoriteCheckBox)
    {
        m_pQuickFavoriteCheckBox->setChecked(false);
        m_pQuickFavoriteCheckBox->setEnabled(false);
    }
    if (nullptr != m_pQuickReviewCheckBox)
    {
        m_pQuickReviewCheckBox->setChecked(false);
        m_pQuickReviewCheckBox->setEnabled(false);
    }
    if (nullptr != m_pSnippetFavoriteCheckBox)
    {
        m_pSnippetFavoriteCheckBox->setChecked(false);
        m_pSnippetFavoriteCheckBox->setEnabled(false);
    }
    if (nullptr != m_pSnippetReviewCheckBox)
    {
        m_pSnippetReviewCheckBox->setChecked(false);
        m_pSnippetReviewCheckBox->setEnabled(false);
    }
    m_bUpdatingSnippetStateControls = false;
    updateActionState();
}

void QCMainWindow::updateDetailImage(const QString& strImagePath)
{
    m_strCurrentImagePath = strImagePath;
    m_pImagePathValueLabel->setText(strImagePath);

    QPixmap pixmapImage(strImagePath);
    if (pixmapImage.isNull())
    {
        m_originalPreviewPixmap = QPixmap();
        m_pImagePreviewLabel->setPixmap(QPixmap());
        m_pImagePreviewLabel->setText(uiText(QString::fromUtf8("Image preview unavailable."), QString::fromUtf8("Image preview unavailable.")));
        return;
    }

    m_originalPreviewPixmap = pixmapImage;
    refreshImagePreviewScale();
}

void QCMainWindow::refreshImagePreviewScale()
{
    if (nullptr == m_pImagePreviewLabel)
        return;

    if (m_originalPreviewPixmap.isNull())
    {
        m_pImagePreviewLabel->setPixmap(QPixmap());
        m_pImagePreviewLabel->setText(QCoreApplication::translate("QCMainWindow", "No image preview available."));
        return;
    }

    QSize previewSize = m_pImagePreviewLabel->contentsRect().size();
    if (previewSize.width() < 200 || previewSize.height() < 160)
        return;

    m_pImagePreviewLabel->setText(QString());
    m_pImagePreviewLabel->setPixmap(m_originalPreviewPixmap.scaled(previewSize,
                                                                   Qt::KeepAspectRatio,
                                                                   Qt::SmoothTransformation));
}

void QCMainWindow::focusSummaryEditor(QPlainTextEdit *pTextEdit)
{
    if (nullptr == pTextEdit)
        return;

    pTextEdit->setFocus();
    QTextCursor cursor = pTextEdit->textCursor();
    cursor.movePosition(QTextCursor::Start);
    pTextEdit->setTextCursor(cursor);
    pTextEdit->ensureCursorVisible();
}

QString QCMainWindow::currentSnippetSummaryText() const
{
    return nullptr == m_pSnippetSummaryTextEdit ? QString() : m_pSnippetSummaryTextEdit->toPlainText().trimmed();
}

QString QCMainWindow::currentSessionSummaryText() const
{
    return nullptr == m_pSessionSummaryTextEdit ? QString() : m_pSessionSummaryTextEdit->toPlainText().trimmed();
}

void QCMainWindow::updateAiStatusDisplay()
{
    if (nullptr == m_pAiStatusLabel)
        return;

    QString strStatusMessage = m_strAiStatusMessage.trimmed();
    if (strStatusMessage.isEmpty())
        strStatusMessage = uiText(QString::fromUtf8("AI 就绪。你可以总结、查看或复制当前结果。"), QString::fromUtf8("AI idle. You can summarize, view, or copy the current result."));

    if (m_bSnippetSummaryRunning)
        strStatusMessage = uiText(QString::fromUtf8("Snippet AI 正在运行。任务完成后当前总结将刷新；如果失败，重试选项将保持可用。"), QString::fromUtf8("Snippet AI is running. The current summary will refresh when the task finishes; retry stays available if it fails."));
    else if (m_bSessionSummaryRunning)
        strStatusMessage = uiText(QString::fromUtf8("Session AI 正在运行。Session 总结将在任务完成后刷新；如果失败，重试选项将保持可用。"), QString::fromUtf8("Session AI is running. The session summary will refresh when the task finishes; retry stays available if it fails."));

    m_pAiStatusLabel->setText(strStatusMessage);
}

void QCMainWindow::updateSelectionContextDisplay()
{
    if (nullptr == m_pSelectionContextLabel)
        return;

    m_pSelectionContextLabel->setText(buildSelectionContextText());
}

QString QCMainWindow::buildSelectionContextText() const
{
    QCStudySession session;
    const bool bHasSession = currentSession(&session);
    const QVector<qint64> vecSnippetIds = selectedSnippetIds();

    if (!bHasSession)
        return uiText(QString::fromUtf8("Step 1/4: Select or create a course on the left."), QString::fromUtf8("Step 1/4: Select or create a course on the left."));

    if (vecSnippetIds.isEmpty())
        return uiText(QString::fromUtf8("Step 2/4: Paste or import screenshots."), QString::fromUtf8("Step 2/4: Paste or import screenshots."));

    if (currentSnippetSummaryText().trimmed().isEmpty())
        return uiText(QString::fromUtf8("Step 3/4: Run AI summary, then edit AI Summary below."), QString::fromUtf8("Step 3/4: Run AI summary, then edit AI Summary below."));

    return uiText(QString::fromUtf8("Step 4/4: Export Markdown."), QString::fromUtf8("Step 4/4: Export Markdown."));
}

void QCMainWindow::flushSnippetDraftToStorage(bool bShowError)
{
    if (m_bPopulatingSnippetEditors)
        return;

    if (m_nDisplayedSnippetId <= 0 || nullptr == m_pSnippetService)
        return;

    QCSnippet snippet;
    if (!m_pSnippetService->getSnippetById(m_nDisplayedSnippetId, &snippet))
        return;

    const QString strSummary = (nullptr != m_pSnippetSummaryTextEdit) ? m_pSnippetSummaryTextEdit->toPlainText() : QString();
    if (snippet.summary() == strSummary)
        return;

    snippet.setSummary(strSummary);
    if (!m_pSnippetService->updateSnippet(&snippet))
    {
        if (bShowError)
            QMessageBox::warning(this,
                                 uiText(QString::fromUtf8("Save Summary"), QString::fromUtf8("Save Summary")),
                                 m_pSnippetService->lastError());
        return;
    }

    statusBar()->showMessage(uiText(QString::fromUtf8("Summary auto-saved."), QString::fromUtf8("Summary auto-saved.")), 1500);
}

void QCMainWindow::updateActionState()
{
    const bool bAnyAiTaskRunning = m_bSnippetSummaryRunning || m_bSessionSummaryRunning;

    QCStudySession session;
    const bool bHasSession = currentSession(&session);
    const QVector<qint64> vecSelectedSnippetIds = selectedSnippetIds();
    const int nSelectedSnippetCount = vecSelectedSnippetIds.size();
    const bool bHasSingleSnippet = (nSelectedSnippetCount == 1);
    QCSnippet snippet;
    const bool bHasSnippet = currentSnippetForAction(&snippet);
    const bool bSessionActive = bHasSession && session.status() == QCSessionStatus::ActiveSessionStatus;
    const bool bSessionFinished = bHasSession && session.status() == QCSessionStatus::FinishedSessionStatus;

    int nArchivedSnippetCount = 0;
    for (int i = 0; i < vecSelectedSnippetIds.size(); ++i)
    {
        QCSnippet selectedSnippet;
        if (m_pSnippetService->getSnippetById(vecSelectedSnippetIds.at(i), &selectedSnippet) && selectedSnippet.isArchived())
            ++nArchivedSnippetCount;
    }
    const bool bAllSelectedArchived = nSelectedSnippetCount > 0 && nArchivedSnippetCount == nSelectedSnippetCount;
    const bool bCanEditSnippet = bHasSingleSnippet && !bAnyAiTaskRunning;
    const bool bCanManageTags = nSelectedSnippetCount > 0 && !bAnyAiTaskRunning;
    const bool bCanSummarizeSnippet = bHasSingleSnippet && !bAnyAiTaskRunning;
    const bool bCanOrganizeSnippets = nSelectedSnippetCount > 0 && !bAnyAiTaskRunning;
    const bool bCanEditSession = bHasSession;
    const bool bCanDeleteSession = bHasSession && !bAnyAiTaskRunning;

    if (nullptr != m_pEditSessionAction)
        m_pEditSessionAction->setEnabled(bCanEditSession);
    if (nullptr != m_pFinishSessionAction)
        m_pFinishSessionAction->setEnabled(bSessionActive && !bAnyAiTaskRunning);
    if (nullptr != m_pReopenSessionAction)
        m_pReopenSessionAction->setEnabled(bSessionFinished && !bAnyAiTaskRunning);
    if (nullptr != m_pDeleteSessionAction)
        m_pDeleteSessionAction->setEnabled(bCanDeleteSession);
    if (nullptr != m_pEditSnippetAction)
        m_pEditSnippetAction->setEnabled(bCanEditSnippet);
    if (nullptr != m_pDuplicateSnippetAction)
    {
        m_pDuplicateSnippetAction->setEnabled(bCanOrganizeSnippets);
        m_pDuplicateSnippetAction->setText(nSelectedSnippetCount > 1
            ? uiText(QString::fromUtf8("复制 %1 条 Snippet"), QString::fromUtf8("Duplicate %1 Snippets")).arg(nSelectedSnippetCount)
            : uiText(QString::fromUtf8("复制 Snippet"), QString::fromUtf8("Duplicate Snippet")));
    }
    if (nullptr != m_pMoveSnippetAction)
    {
        m_pMoveSnippetAction->setEnabled(bCanOrganizeSnippets);
        m_pMoveSnippetAction->setText(nSelectedSnippetCount > 1
            ? uiText(QString::fromUtf8("移动 %1 条 Snippet"), QString::fromUtf8("Move %1 Snippets")).arg(nSelectedSnippetCount)
            : uiText(QString::fromUtf8("移动 Snippet"), QString::fromUtf8("Move Snippet")));
    }
    if (nullptr != m_pToggleArchiveSnippetAction)
    {
        m_pToggleArchiveSnippetAction->setEnabled(bCanOrganizeSnippets);
        if (nSelectedSnippetCount > 1)
        {
            m_pToggleArchiveSnippetAction->setText(bAllSelectedArchived
                ? uiText(QString::fromUtf8("恢复 %1 条 Snippet"), QString::fromUtf8("Restore %1 Snippets")).arg(nSelectedSnippetCount)
                : uiText(QString::fromUtf8("归档 %1 条 Snippet"), QString::fromUtf8("Archive %1 Snippets")).arg(nSelectedSnippetCount));
        }
        else
        {
            m_pToggleArchiveSnippetAction->setText((bHasSnippet && snippet.isArchived())
                ? uiText(QString::fromUtf8("恢复 Snippet"), QString::fromUtf8("Restore Snippet"))
                : uiText(QString::fromUtf8("归档 Snippet"), QString::fromUtf8("Archive Snippet")));
        }
    }
    if (nullptr != m_pDeleteSnippetAction)
    {
        m_pDeleteSnippetAction->setEnabled(bCanOrganizeSnippets);
        m_pDeleteSnippetAction->setText(nSelectedSnippetCount > 1
            ? uiText(QString::fromUtf8("删除 %1 条 Snippet"), QString::fromUtf8("Delete %1 Snippets")).arg(nSelectedSnippetCount)
            : uiText(QString::fromUtf8("删除 Snippet"), QString::fromUtf8("Delete Snippet")));
    }
    if (nullptr != m_pManageTagsAction)
    {
        m_pManageTagsAction->setEnabled(bCanManageTags);
        m_pManageTagsAction->setText(nSelectedSnippetCount > 1
            ? uiText(QString::fromUtf8("为 %1 条 Snippet 设置标签"), QString::fromUtf8("Tag %1 Snippets")).arg(nSelectedSnippetCount)
            : uiText(QString::fromUtf8("Snippet 标签"), QString::fromUtf8("Snippet Tags")));
    }
    if (nullptr != m_pTagLibraryAction)
        m_pTagLibraryAction->setEnabled(nullptr != m_pTagService && !bAnyAiTaskRunning);
    if (nullptr != m_pClearSnippetTagsAction)
        m_pClearSnippetTagsAction->setEnabled(bCanManageTags);
    const bool bHasSnippetSummaryText = !currentSnippetSummaryText().trimmed().isEmpty();
    const bool bHasSessionSummaryText = !currentSessionSummaryText().trimmed().isEmpty();
    if (nullptr != m_pSummarizeSnippetAction)
        m_pSummarizeSnippetAction->setEnabled(bCanSummarizeSnippet);
    if (nullptr != m_pRetrySnippetSummaryAction)
        m_pRetrySnippetSummaryAction->setEnabled(m_bHasRetryableSnippetSummary && !bAnyAiTaskRunning);
    if (nullptr != m_pViewSnippetSummaryAction)
        m_pViewSnippetSummaryAction->setEnabled(bHasSingleSnippet && bHasSnippetSummaryText);
    if (nullptr != m_pCopySnippetSummaryAction)
        m_pCopySnippetSummaryAction->setEnabled(bHasSingleSnippet && bHasSnippetSummaryText);
    if (nullptr != m_pSummarizeSessionAction)
        m_pSummarizeSessionAction->setEnabled(bHasSession && !bAnyAiTaskRunning);
    if (nullptr != m_pRetrySessionSummaryAction)
        m_pRetrySessionSummaryAction->setEnabled(m_bHasRetryableSessionSummary && !bAnyAiTaskRunning);
    if (nullptr != m_pViewSessionSummaryAction)
        m_pViewSessionSummaryAction->setEnabled(bHasSession && bHasSessionSummaryText);
    if (nullptr != m_pCopySessionSummaryAction)
        m_pCopySessionSummaryAction->setEnabled(bHasSession && bHasSessionSummaryText);
    if (nullptr != m_pExportMarkdownAction)
        m_pExportMarkdownAction->setEnabled(bHasSession && !bAnyAiTaskRunning);
    if (nullptr != m_pTopExportButton)
        m_pTopExportButton->setEnabled(bHasSession && !bAnyAiTaskRunning);
    if (nullptr != m_pRunSummaryButton)
        m_pRunSummaryButton->setEnabled(bHasSession && !bAnyAiTaskRunning);
    if (nullptr != m_pPasteImageButton)
        m_pPasteImageButton->setEnabled(bHasSession && !bAnyAiTaskRunning);
    if (nullptr != m_pSidebarNewSessionButton)
        m_pSidebarNewSessionButton->setEnabled(!bAnyAiTaskRunning);
    if (nullptr != m_pSidebarSettingsButton)
        m_pSidebarSettingsButton->setEnabled(!bAnyAiTaskRunning);
    if (nullptr != m_pPasteImageAction)
        m_pPasteImageAction->setEnabled(bHasSession && !bAnyAiTaskRunning);
    if (nullptr != m_pEditCurrentAction)
    {
        m_pEditCurrentAction->setEnabled(bCanEditSnippet || (nSelectedSnippetCount == 0 && bCanEditSession));
        m_pEditCurrentAction->setText(bCanEditSnippet
            ? uiText(QString::fromUtf8("编辑选中 Snippet"), QString::fromUtf8("Edit Selected Snippet"))
            : ((nSelectedSnippetCount == 0 && bCanEditSession)
                ? uiText(QString::fromUtf8("编辑当前 Session"), QString::fromUtf8("Edit Current Session"))
                : uiText(QString::fromUtf8("编辑当前项"), QString::fromUtf8("Edit Current"))));
    }
    if (nullptr != m_pDeleteCurrentAction)
    {
        m_pDeleteCurrentAction->setEnabled(bCanOrganizeSnippets || (nSelectedSnippetCount == 0 && bCanDeleteSession));
        if (nSelectedSnippetCount > 1)
            m_pDeleteCurrentAction->setText(uiText(QString::fromUtf8("删除 %1 条 Snippet"), QString::fromUtf8("Delete %1 Snippets")).arg(nSelectedSnippetCount));
        else if (nSelectedSnippetCount == 1)
            m_pDeleteCurrentAction->setText(uiText(QString::fromUtf8("删除选中 Snippet"), QString::fromUtf8("Delete Selected Snippet")));
        else if (bCanDeleteSession)
            m_pDeleteCurrentAction->setText(uiText(QString::fromUtf8("删除当前 Session"), QString::fromUtf8("Delete Current Session")));
        else
            m_pDeleteCurrentAction->setText(uiText(QString::fromUtf8("删除当前项"), QString::fromUtf8("Delete Current")));
    }
    if (nullptr != m_pFocusSearchAction)
        m_pFocusSearchAction->setEnabled(nullptr != m_pSnippetSearchLineEdit);

    updateSelectionContextDisplay();
}
void QCMainWindow::updateViewSummary(int nVisibleSnippetCount, int nTotalSnippetCount)
{
    if (nullptr == m_pViewSummaryLabel)
        return;

    QCStudySession session;
    if (!currentSession(&session))
    {
        m_pViewSummaryLabel->setText(uiText(QString::fromUtf8("未选择 Session。"), QString::fromUtf8("No session selected.")));
        return;
    }

    QStringList vecTokens;
    vecTokens.append(SessionStatusText(session.status()));
    vecTokens.append(uiText(QString::fromUtf8("当前可见 %1/%2 条 snippet"), QString::fromUtf8("Visible %1/%2 snippets")).arg(nVisibleSnippetCount).arg(nTotalSnippetCount));
    if (nullptr != m_pFavoriteOnlyCheckBox && m_pFavoriteOnlyCheckBox->isChecked())
        vecTokens.append(uiText(QString::fromUtf8("收藏"), QString::fromUtf8("Favorites")));
    if (nullptr != m_pReviewOnlyCheckBox && m_pReviewOnlyCheckBox->isChecked())
        vecTokens.append(uiText(QString::fromUtf8("复习"), QString::fromUtf8("Review")));
    if (nullptr != m_pSnippetTypeFilterComboBox && currentSnippetTypeFilter() != QString::fromUtf8("all"))
        vecTokens.append(uiText(QString::fromUtf8("类型: %1"), QString::fromUtf8("Type: %1")).arg(m_pSnippetTypeFilterComboBox->currentText()));
    if (nullptr != m_pShowArchivedCheckBox && m_pShowArchivedCheckBox->isChecked())
        vecTokens.append(uiText(QString::fromUtf8("已归档可见"), QString::fromUtf8("Archived Visible")));
    if (currentTagFilterId() > 0 && nullptr != m_pTagFilterComboBox)
        vecTokens.append(uiText(QString::fromUtf8("标签: %1"), QString::fromUtf8("Tag: %1")).arg(m_pTagFilterComboBox->currentText()));
    if (!currentSearchText().isEmpty())
        vecTokens.append(uiText(QString::fromUtf8("搜索: %1"), QString::fromUtf8("Search: %1")).arg(currentSearchText()));
    if (nullptr != m_pSearchHistoryComboBox && m_pSearchHistoryComboBox->count() > 1)
        vecTokens.append(uiText(QString::fromUtf8("历史: %1"), QString::fromUtf8("History: %1")).arg(m_pSearchHistoryComboBox->count() - 1));
    if (selectedSnippetCount() > 0)
        vecTokens.append(uiText(QString::fromUtf8("选中: %1"), QString::fromUtf8("Selected: %1")).arg(selectedSnippetCount()));

    m_pViewSummaryLabel->setText(QString::fromUtf8("%1 | %2")
        .arg(session.title().trimmed().isEmpty() ? uiText(QString::fromUtf8("未命名 Session"), QString::fromUtf8("Untitled Session")) : session.title().trimmed(),
             vecTokens.join(QString::fromUtf8(" | "))));
}

QString QCMainWindow::currentSearchText() const
{
    if (nullptr != m_pNavigationSearchLineEdit)
        return m_pNavigationSearchLineEdit->text().trimmed();

    return nullptr == m_pSnippetSearchLineEdit ? QString() : m_pSnippetSearchLineEdit->text().trimmed();
}

QString QCMainWindow::currentSnippetTypeFilter() const
{
    return nullptr == m_pSnippetTypeFilterComboBox ? QString::fromUtf8("all") : m_pSnippetTypeFilterComboBox->currentData().toString();
}

bool QCMainWindow::matchesSnippetTypeFilter(const QCSnippet& snippet) const
{
    const QString strTypeFilter = currentSnippetTypeFilter();
    if (strTypeFilter == QString::fromUtf8("text"))
        return snippet.type() == QCSnippetType::TextSnippetType;
    if (strTypeFilter == QString::fromUtf8("image"))
        return snippet.type() == QCSnippetType::ImageSnippetType;

    return true;
}

void QCMainWindow::loadSearchHistoryOptions()
{
    if (nullptr == m_pSearchHistoryComboBox)
        return;

    QStringList vecSearchHistory;
    if (nullptr != m_pSettingsService)
        m_pSettingsService->getSnippetSearchHistory(&vecSearchHistory);

    QSignalBlocker blocker(m_pSearchHistoryComboBox);
    m_pSearchHistoryComboBox->clear();
    m_pSearchHistoryComboBox->addItem(uiText(QString::fromUtf8("最近搜索"), QString::fromUtf8("Recent Searches")), QString());
    for (int i = 0; i < vecSearchHistory.size(); ++i)
        m_pSearchHistoryComboBox->addItem(vecSearchHistory.at(i), vecSearchHistory.at(i));
    m_pSearchHistoryComboBox->setCurrentIndex(0);

    if (nullptr != m_pClearSearchHistoryButton)
        m_pClearSearchHistoryButton->setEnabled(!vecSearchHistory.isEmpty());
}

void QCMainWindow::appendSearchHistory(const QString& strSearchText)
{
    const QString strNormalizedSearchText = strSearchText.trimmed();
    if (strNormalizedSearchText.isEmpty() || nullptr == m_pSettingsService)
        return;

    QStringList vecSearchHistory;
    if (!m_pSettingsService->getSnippetSearchHistory(&vecSearchHistory))
        return;

    vecSearchHistory.removeAll(strNormalizedSearchText);
    vecSearchHistory.prepend(strNormalizedSearchText);
    while (vecSearchHistory.size() > 8)
        vecSearchHistory.removeLast();
    if (!m_pSettingsService->setSnippetSearchHistory(vecSearchHistory))
        return;

    loadSearchHistoryOptions();
}

QString QCMainWindow::buildHighlightedHtml(const QString& strText, const QString& strSearchText) const
{
    const QString strEscapedText = strText.toHtmlEscaped();
    const QString strNormalizedSearchText = strSearchText.trimmed();
    if (strEscapedText.isEmpty() || strNormalizedSearchText.isEmpty())
        return strEscapedText;

    QRegularExpression expression(QRegularExpression::escape(strNormalizedSearchText), QRegularExpression::CaseInsensitiveOption);
    QString strHighlightedText = strEscapedText;
    strHighlightedText.replace(expression, QString::fromUtf8("<span style=\"background-color:#fff3a1;\">\0</span>"));
    return strHighlightedText;
}

void QCMainWindow::applyPlainTextHighlight(QPlainTextEdit *pTextEdit, const QString& strSearchText) const
{
    if (nullptr == pTextEdit)
        return;

    QList<QTextEdit::ExtraSelection> vecSelections;
    const QString strNormalizedSearchText = strSearchText.trimmed();
    if (!strNormalizedSearchText.isEmpty())
    {
        QTextCursor cursor(pTextEdit->document());
        QTextCharFormat format;
        format.setBackground(QColor(255, 243, 161));
        format.setForeground(QColor(45, 45, 45));

        while (!(cursor = pTextEdit->document()->find(strNormalizedSearchText,
                                                      cursor,
                                                      QTextDocument::FindCaseSensitively)).isNull())
        {
            QTextEdit::ExtraSelection selection;
            selection.cursor = cursor;
            selection.format = format;
            vecSelections.append(selection);
        }

        if (vecSelections.isEmpty())
        {
            QTextCursor insensitiveCursor(pTextEdit->document());
            while (!(insensitiveCursor = pTextEdit->document()->find(strNormalizedSearchText,
                                                                     insensitiveCursor,
                                                                     QTextDocument::FindFlags())).isNull())
            {
                QTextEdit::ExtraSelection selection;
                selection.cursor = insensitiveCursor;
                selection.format = format;
                vecSelections.append(selection);
            }
        }
    }

    pTextEdit->setExtraSelections(vecSelections);
}

QHash<qint64, int> QCMainWindow::buildTagUsageCounts(const QVector<QCTag>& vecTags) const
{
    QHash<qint64, int> hashTagUsageCounts;
    if (nullptr == m_pTagService)
        return hashTagUsageCounts;

    for (int i = 0; i < vecTags.size(); ++i)
    {
        const QCTag& tag = vecTags.at(i);
        const int nUsageCount = m_pTagService->countSnippetsByTag(tag.id());
        if (nUsageCount < 0)
            return QHash<qint64, int>();

        hashTagUsageCounts.insert(tag.id(), nUsageCount);
    }

    return hashTagUsageCounts;
}

QHash<qint64, Qt::CheckState> QCMainWindow::buildTagSelectionStatesForSnippets(const QVector<qint64>& vecSnippetIds, const QVector<QCTag>& vecTags) const
{
    QHash<qint64, Qt::CheckState> hashTagStates;
    if (nullptr == m_pTagService || vecSnippetIds.isEmpty())
        return hashTagStates;

    QHash<qint64, int> hashTagSelectedCount;
    for (int i = 0; i < vecSnippetIds.size(); ++i)
    {
        const QVector<QCTag> vecSnippetTags = m_pTagService->listTagsBySnippet(vecSnippetIds.at(i));
        if (!m_pTagService->lastError().isEmpty())
            return QHash<qint64, Qt::CheckState>();

        for (int j = 0; j < vecSnippetTags.size(); ++j)
            hashTagSelectedCount.insert(vecSnippetTags.at(j).id(), hashTagSelectedCount.value(vecSnippetTags.at(j).id(), 0) + 1);
    }

    for (int i = 0; i < vecTags.size(); ++i)
    {
        const qint64 nTagId = vecTags.at(i).id();
        const int nSelectedCount = hashTagSelectedCount.value(nTagId, 0);
        if (nSelectedCount <= 0)
            hashTagStates.insert(nTagId, Qt::Unchecked);
        else if (nSelectedCount == vecSnippetIds.size())
            hashTagStates.insert(nTagId, Qt::Checked);
        else
            hashTagStates.insert(nTagId, Qt::PartiallyChecked);
    }

    return hashTagStates;
}

bool QCMainWindow::applyTagDialogChanges(const QCSnippetTagDialog& dialog,
                                         qint64 *pnCreatedTagId,
                                         QVector<qint64> *pvecDeletedTagIds,
                                         const QString& strContextTitle)
{
    if (nullptr != pnCreatedTagId)
        *pnCreatedTagId = 0;
    if (nullptr != pvecDeletedTagIds)
        pvecDeletedTagIds->clear();

    if (nullptr == m_pTagService)
        return false;

    const QHash<qint64, QString> hashRenamedTags = dialog.renamedTags();
    for (QHash<qint64, QString>::const_iterator it = hashRenamedTags.constBegin(); it != hashRenamedTags.constEnd(); ++it)
    {
        QCTag existingTag;
        if (!m_pTagService->getTagById(it.key(), &existingTag))
        {
            QMessageBox::warning(this, strContextTitle, m_pTagService->lastError());
            return false;
        }

        existingTag.setName(it.value());
        if (!m_pTagService->updateTag(&existingTag))
        {
            QMessageBox::warning(this, strContextTitle, m_pTagService->lastError());
            return false;
        }
    }

    const QVector<qint64> vecDeletedTagIds = dialog.deletedTagIds();
    for (int i = 0; i < vecDeletedTagIds.size(); ++i)
    {
        if (!m_pTagService->deleteTag(vecDeletedTagIds.at(i)))
        {
            QMessageBox::warning(this, strContextTitle, m_pTagService->lastError());
            return false;
        }
    }

    if (nullptr != pvecDeletedTagIds)
        *pvecDeletedTagIds = vecDeletedTagIds;

    const QString strNewTagName = dialog.newTagName().trimmed();
    if (strNewTagName.isEmpty())
        return true;

    const QVector<QCTag> vecUpdatedTags = m_pTagService->listTags();
    if (!m_pTagService->lastError().isEmpty())
    {
        QMessageBox::warning(this, strContextTitle, m_pTagService->lastError());
        return false;
    }

    for (int i = 0; i < vecUpdatedTags.size(); ++i)
    {
        if (vecUpdatedTags.at(i).name().compare(strNewTagName, Qt::CaseInsensitive) == 0)
        {
            if (nullptr != pnCreatedTagId)
                *pnCreatedTagId = vecUpdatedTags.at(i).id();
            return true;
        }
    }

    QCTag newTag;
    newTag.setName(strNewTagName);
    if (!m_pTagService->createTag(&newTag))
    {
        QMessageBox::warning(this, strContextTitle, m_pTagService->lastError());
        return false;
    }

    if (nullptr != pnCreatedTagId)
        *pnCreatedTagId = newTag.id();
    return true;
}

bool QCMainWindow::applyTagSelectionStatesToSnippets(const QVector<qint64>& vecSnippetIds,
                                                     const QHash<qint64, Qt::CheckState>& hashTagStates)
{
    if (nullptr == m_pTagService)
        return false;

    for (int i = 0; i < vecSnippetIds.size(); ++i)
    {
        const qint64 nSnippetId = vecSnippetIds.at(i);
        const QVector<QCTag> vecCurrentTags = m_pTagService->listTagsBySnippet(nSnippetId);
        if (!m_pTagService->lastError().isEmpty())
            return false;

        QSet<qint64> setFinalIds;
        for (int j = 0; j < vecCurrentTags.size(); ++j)
            setFinalIds.insert(vecCurrentTags.at(j).id());

        for (QHash<qint64, Qt::CheckState>::const_iterator it = hashTagStates.constBegin(); it != hashTagStates.constEnd(); ++it)
        {
            if (it.value() == Qt::Checked)
                setFinalIds.insert(it.key());
            else if (it.value() == Qt::Unchecked)
                setFinalIds.remove(it.key());
        }

        QVector<qint64> vecFinalIds;
        const QList<qint64> vecIdList = setFinalIds.values();
        for (int j = 0; j < vecIdList.size(); ++j)
            vecFinalIds.append(vecIdList.at(j));

        if (!m_pTagService->replaceSnippetTags(nSnippetId, vecFinalIds))
            return false;
    }

    return true;
}

bool QCMainWindow::currentSession(QCStudySession *pSession) const
{
    if (nullptr == pSession)
        return false;

    const qint64 nSessionId = currentSessionId();
    if (nSessionId <= 0)
        return false;

    if (nullptr == m_pSessionService)
        return false;

    return m_pSessionService->getSessionById(nSessionId, pSession);
}

QVector<qint64> QCMainWindow::selectedSnippetIds() const
{
    QVector<qint64> vecSnippetIds;
    if (nullptr == m_pSnippetListWidget)
        return vecSnippetIds;

    const QList<QListWidgetItem *> vecSelectedItems = m_pSnippetListWidget->selectedItems();
    for (int i = 0; i < vecSelectedItems.size(); ++i)
    {
        QListWidgetItem *pItem = vecSelectedItems.at(i);
        if (nullptr == pItem)
            continue;
        vecSnippetIds.append(pItem->data(Qt::UserRole).toLongLong());
    }

    return vecSnippetIds;
}

int QCMainWindow::selectedSnippetCount() const
{
    return selectedSnippetIds().size();
}

bool QCMainWindow::hasSingleSelectedSnippet() const
{
    return selectedSnippetCount() == 1;
}

bool QCMainWindow::currentSnippetForAction(QCSnippet *pSnippet) const
{
    if (nullptr == pSnippet || !hasSingleSelectedSnippet())
        return false;

    return currentSnippet(pSnippet);
}

bool QCMainWindow::currentSnippet(QCSnippet *pSnippet) const
{
    if (nullptr == pSnippet)
        return false;

    const QVector<qint64> vecSnippetIds = selectedSnippetIds();
    if (vecSnippetIds.size() != 1)
        return false;

    if (nullptr == m_pSnippetService)
        return false;

    return m_pSnippetService->getSnippetById(vecSnippetIds.first(), pSnippet);
}

void QCMainWindow::handleSnippetSummaryFinished()
{
    const bool bAutomaticSnippetSummary = m_bAutomaticSnippetSummary;
    m_bSnippetSummaryRunning = false;
    m_bAutomaticSnippetSummary = false;

    const QCAiTaskExecutionResult executionResult = m_pSnippetSummaryWatcher->result();
    if (!m_pAiProcessService->applyTaskResult(executionResult))
    {
        const QString strErrorMessage = m_pAiProcessService->lastError();
        m_bHasRetryableSnippetSummary = executionResult.m_context.m_nSnippetId > 0;
        m_nRetrySnippetId = executionResult.m_context.m_nSnippetId;
        m_strAiStatusMessage = QString::fromUtf8("Snippet AI 总结失败。请使用“重试 Snippet AI”再次尝试。\n%1").arg(strErrorMessage);
        updateAiStatusDisplay();
        updateActionState();
        if (bAutomaticSnippetSummary)
            QMessageBox::warning(this, QString::fromUtf8("自动总结图片 Snippet"), QString::fromUtf8("Snippet 已保存，但自动总结失败：\n%1").arg(strErrorMessage));
        else
            QMessageBox::warning(this, QString::fromUtf8("总结 Snippet"), strErrorMessage);

        statusBar()->showMessage(bAutomaticSnippetSummary
            ? QString::fromUtf8("图片 Snippet 已保存，但自动总结失败。")
            : QString::fromUtf8("Snippet 总结失败。"), 5000);
        return;
    }

    m_bHasRetryableSnippetSummary = false;
    m_nRetrySnippetId = executionResult.m_context.m_nSnippetId;
    const QString strSnippetPreview = executionResult.m_aiResponse.m_strText.trimmed().isEmpty() ? executionResult.m_aiResponse.m_strRawResponse.trimmed() : executionResult.m_aiResponse.m_strText.trimmed();
    m_strAiStatusMessage = uiText(QString::fromUtf8("Snippet AI 已完成。查看或复制当前总结： %1"), QString::fromUtf8("Snippet AI completed. View or copy the current summary: %1")).arg(strSnippetPreview.isEmpty() ? QString::number(executionResult.m_context.m_nSnippetId) : strSnippetPreview);
    updateAiStatusDisplay();
    updateActionState();
    loadSnippets(executionResult.m_context.m_nSessionId);
    if (!selectSnippetById(executionResult.m_context.m_nSnippetId))
    {
        QMessageBox::warning(this,
                             bAutomaticSnippetSummary ? QString::fromUtf8("自动总结图片 Snippet") : QString::fromUtf8("总结 Snippet"),
                             QString::fromUtf8("Snippet 总结完成，但自动选择失败。"));
        return;
    }

    showSnippetDetails(executionResult.m_context.m_nSnippetId);
    focusSummaryEditor(m_pSnippetSummaryTextEdit);
    statusBar()->showMessage(bAutomaticSnippetSummary
        ? QString::fromUtf8("图片 Snippet 已保存并完成总结。")
        : QString::fromUtf8("Snippet 总结已生成。"), 5000);
}

void QCMainWindow::handleSessionSummaryFinished()
{
    m_bSessionSummaryRunning = false;

    const QCAiTaskExecutionResult executionResult = m_pSessionSummaryWatcher->result();
    if (!m_pAiProcessService->applyTaskResult(executionResult))
    {
        m_bHasRetryableSessionSummary = executionResult.m_context.m_nSessionId > 0;
        m_nRetrySessionId = executionResult.m_context.m_nSessionId;
        m_strAiStatusMessage = QString::fromUtf8("Session AI 总结失败。请使用“重试 Session AI”再次尝试。\n%1").arg(m_pAiProcessService->lastError());
        updateAiStatusDisplay();
        updateActionState();
        QMessageBox::warning(this, uiText(QString::fromUtf8("总结 Session"), QString::fromUtf8("Summarize Session")), m_pAiProcessService->lastError());
        statusBar()->showMessage(QString::fromUtf8("Session 总结失败。"), 5000);
        return;
    }

    m_bHasRetryableSessionSummary = false;
    m_nRetrySessionId = executionResult.m_context.m_nSessionId;
    const QString strSessionPreview = executionResult.m_aiResponse.m_strText.trimmed().isEmpty() ? executionResult.m_aiResponse.m_strRawResponse.trimmed() : executionResult.m_aiResponse.m_strText.trimmed();
    m_strAiStatusMessage = uiText(QString::fromUtf8("Session AI 已完成（右侧 Session 摘要）。查看或复制当前总结： %1"), QString::fromUtf8("Session AI completed (right-side Session Summary). View or copy the current summary: %1")).arg(strSessionPreview.isEmpty() ? QString::number(executionResult.m_context.m_nSessionId) : strSessionPreview);
    updateAiStatusDisplay();
    updateActionState();
    showSessionSummary(executionResult.m_context.m_nSessionId);
    focusSummaryEditor(m_pSnippetSummaryTextEdit);
    statusBar()->showMessage(uiText(QString::fromUtf8("Session 总结已生成。"), QString::fromUtf8("Session summary generated.")), 5000);
}

QString QCMainWindow::findSummaryFromAiRecords(const QVector<QCAiRecord>& vecAiRecords) const
{
    for (int i = 0; i < vecAiRecords.size(); ++i)
    {
        const QCAiRecord& aiRecord = vecAiRecords.at(i);
        if (aiRecord.taskType() == QCAiTaskType::SnippetSummaryTask && !aiRecord.responseText().trimmed().isEmpty())
            return aiRecord.responseText();
    }

    return QString();
}

QString QCMainWindow::findSessionSummaryFromAiRecords(const QVector<QCAiRecord>& vecAiRecords) const
{
    for (int i = 0; i < vecAiRecords.size(); ++i)
    {
        const QCAiRecord& aiRecord = vecAiRecords.at(i);
        if (aiRecord.taskType() == QCAiTaskType::SessionSummaryTask && !aiRecord.responseText().trimmed().isEmpty())
            return aiRecord.responseText();
    }

    return QString();
}

qint64 QCMainWindow::currentSessionId() const
{
    const QListWidgetItem *pCurrentItem = m_pSessionListWidget->currentItem();
    if (nullptr == pCurrentItem)
        return 0;

    return pCurrentItem->data(Qt::UserRole).toLongLong();
}

qint64 QCMainWindow::currentSnippetId() const
{
    const QListWidgetItem *pCurrentItem = m_pSnippetListWidget->currentItem();
    if (nullptr == pCurrentItem)
        return 0;

    return pCurrentItem->data(Qt::UserRole).toLongLong();
}

qint64 QCMainWindow::currentTagFilterId() const
{
    if (nullptr == m_pTagFilterComboBox)
        return 0;

    return m_pTagFilterComboBox->currentData().toLongLong();
}

bool QCMainWindow::selectSnippetById(qint64 nSnippetId)
{
    QVector<qint64> vecSnippetIds;
    vecSnippetIds.append(nSnippetId);
    return selectSnippetsByIds(vecSnippetIds);
}

bool QCMainWindow::selectSnippetsByIds(const QVector<qint64>& vecSnippetIds)
{
    if (nullptr == m_pSnippetListWidget || vecSnippetIds.isEmpty())
        return false;

    bool bSelectedAny = false;
    QListWidgetItem *pFirstSelectedItem = nullptr;
    for (int i = 0; i < m_pSnippetListWidget->count(); ++i)
    {
        QListWidgetItem *pItem = m_pSnippetListWidget->item(i);
        if (nullptr == pItem)
            continue;

        const bool bShouldSelect = vecSnippetIds.contains(pItem->data(Qt::UserRole).toLongLong());
        pItem->setSelected(bShouldSelect);
        if (bShouldSelect && nullptr == pFirstSelectedItem)
            pFirstSelectedItem = pItem;
        bSelectedAny = bSelectedAny || bShouldSelect;
    }

    if (1 == vecSnippetIds.size() && nullptr != pFirstSelectedItem)
        m_pSnippetListWidget->setCurrentItem(pFirstSelectedItem);

    return bSelectedAny;
}
bool QCMainWindow::captureScreenToFile(QCScreenCaptureResult *pCaptureResult)
{
    if (nullptr == pCaptureResult)
        return false;

    hide();
    QApplication::processEvents();

    QEventLoop delayLoop;
    QTimer::singleShot(180, &delayLoop, SLOT(quit()));
    delayLoop.exec();

    const bool bCaptureSuccess = m_pScreenCaptureService->capturePrimaryScreen(pCaptureResult);

    showNormal();
    raise();
    activateWindow();
    QApplication::processEvents();

    return bCaptureSuccess;
}

bool QCMainWindow::captureRegionToFile(QCScreenCaptureResult *pCaptureResult, bool *pbCancelled, QString *pstrFailureMessage)
{
    if (nullptr == pCaptureResult || nullptr == pbCancelled)
        return false;

    *pbCancelled = false;
    if (nullptr != pstrFailureMessage)
        pstrFailureMessage->clear();

    hide();
    QApplication::processEvents();

    QEventLoop delayLoop;
    QTimer::singleShot(180, &delayLoop, SLOT(quit()));
    delayLoop.exec();

    QCScreenshotOverlay overlay;
    QRect rectSelectedRegion;
    const bool bSelected = overlay.selectRegion(&rectSelectedRegion);

    showNormal();
    raise();
    activateWindow();
    QApplication::processEvents();

    if (!bSelected)
    {
        *pbCancelled = overlay.wasCancelled();
        if (nullptr != pstrFailureMessage)
            *pstrFailureMessage = overlay.lastError();
        return false;
    }

    const bool bCaptureSuccess = m_pScreenCaptureService->capturePrimaryScreenRegion(rectSelectedRegion, pCaptureResult);
    if (!bCaptureSuccess && nullptr != pstrFailureMessage)
        *pstrFailureMessage = m_pScreenCaptureService->lastError();

    return bCaptureSuccess;
}

bool QCMainWindow::isAutoSummarizeImageSnippetEnabled() const
{
    if (nullptr == m_pSettingsService)
        return false;

    QCAiRuntimeSettings aiSettings;
    if (!m_pSettingsService->loadAiSettings(&aiSettings))
        return false;

    return aiSettings.m_bAutoSummarizeImageSnippet;
}

bool QCMainWindow::startSnippetSummary(qint64 nSnippetId, bool bAutomatic)
{
    if (m_bSnippetSummaryRunning || m_bSessionSummaryRunning)
        return false;

    if (nSnippetId <= 0)
        return false;

    if (nullptr == m_pAiProcessService)
        return false;

    QCAiTaskExecutionContext executionContext;
    if (!m_pAiProcessService->prepareSnippetSummary(nSnippetId, &executionContext))
        return false;

    m_bSnippetSummaryRunning = true;
    m_bAutomaticSnippetSummary = bAutomatic;
    m_nRetrySnippetId = nSnippetId;
    QCSnippet snippet;
    const QString strSnippetTitle = (m_pSnippetService != nullptr && m_pSnippetService->getSnippetById(nSnippetId, &snippet) && !snippet.title().trimmed().isEmpty())
        ? snippet.title().trimmed()
        : QString::number(nSnippetId);
    m_strAiStatusMessage = bAutomatic
        ? uiText(QString::fromUtf8("图片 Snippet AI 已在后台启动：%1"), QString::fromUtf8("Image snippet AI started in background: %1")).arg(strSnippetTitle)
        : uiText(QString::fromUtf8("Snippet AI 已在后台启动：%1"), QString::fromUtf8("Snippet AI started in background: %1")).arg(strSnippetTitle);
    updateAiStatusDisplay();
    updateActionState();
    statusBar()->showMessage(bAutomatic
        ? uiText(QString::fromUtf8("正在后台生成图片 Snippet 总结..."), QString::fromUtf8("Generating image snippet summary in background..."))
        : uiText(QString::fromUtf8("正在后台生成 Snippet 总结..."), QString::fromUtf8("Generating snippet summary in background...")));
    m_pSnippetSummaryWatcher->setFuture(QtConcurrent::run(RunAiTaskInBackground,
                                                          m_pAiProcessService,
                                                          executionContext));
    return true;
}

void QCMainWindow::handleSavedImageSnippet(qint64 nSessionId, qint64 nSnippetId, const QString& strSavedMessage)
{
    loadSnippets(nSessionId);
    if (!selectSnippetById(nSnippetId))
    {
        QMessageBox::warning(this, QString::fromUtf8("Image Snippet"), QString::fromUtf8("Image snippet was saved, but automatic selection failed."));
        return;
    }

    showSnippetDetails(nSnippetId);

    if (!isAutoSummarizeImageSnippetEnabled())
    {
        statusBar()->showMessage(strSavedMessage, 5000);
        return;
    }

    if (!startSnippetSummary(nSnippetId, true))
    {
        const QString strErrorMessage = (nullptr != m_pAiProcessService) ? m_pAiProcessService->lastError() : QString();
        if (!strErrorMessage.trimmed().isEmpty())
            QMessageBox::warning(this, QString::fromUtf8("Auto Summarize Image Snippet"), QString::fromUtf8("Snippet was saved, but auto summary could not start:\n%1").arg(strErrorMessage));

        statusBar()->showMessage(QString::fromUtf8("%1 Auto summary did not start.").arg(strSavedMessage), 5000);
    }
}

void QCMainWindow::refreshSnippetTagsDisplay(qint64 nSnippetId)
{
    m_pSnippetTagsValueLabel->setText(uiText(QString::fromUtf8("无标签"), QString::fromUtf8("No tags.")));

    if (nSnippetId <= 0 || nullptr == m_pTagService)
        return;

    const QVector<QCTag> vecTags = m_pTagService->listTagsBySnippet(nSnippetId);
    if (!m_pTagService->lastError().isEmpty())
        return;

    QStringList vecNames;
    for (int i = 0; i < vecTags.size(); ++i)
        vecNames.append(vecTags.at(i).name());

    if (!vecNames.isEmpty())
        m_pSnippetTagsValueLabel->setText(vecNames.join(QString::fromUtf8(", ")));
}


void QCMainWindow::updateSnippetStateControls(const QCSnippet& snippet)
{
    const bool bIsReview = IsSnippetMarkedForReview(snippet);

    m_bUpdatingSnippetStateControls = true;
    if (nullptr != m_pQuickFavoriteCheckBox)
    {
        m_pQuickFavoriteCheckBox->setEnabled(true);
        m_pQuickFavoriteCheckBox->setChecked(snippet.isFavorite());
    }
    if (nullptr != m_pQuickReviewCheckBox)
    {
        m_pQuickReviewCheckBox->setEnabled(true);
        m_pQuickReviewCheckBox->setChecked(bIsReview);
    }
    if (nullptr != m_pSnippetFavoriteCheckBox)
    {
        m_pSnippetFavoriteCheckBox->setEnabled(true);
        m_pSnippetFavoriteCheckBox->setChecked(snippet.isFavorite());
    }
    if (nullptr != m_pSnippetReviewCheckBox)
    {
        m_pSnippetReviewCheckBox->setEnabled(true);
        m_pSnippetReviewCheckBox->setChecked(bIsReview);
    }
    m_bUpdatingSnippetStateControls = false;
}
void QCMainWindow::onDuplicateSnippet()
{
    QVector<qint64> vecSnippetIds = selectedSnippetIds();
    if (vecSnippetIds.isEmpty())
    {
        QMessageBox::information(this, uiText(QString::fromUtf8("复制 Snippet"), QString::fromUtf8("Duplicate Snippet")), uiText(QString::fromUtf8("请先选择一个或多个 snippet。"), QString::fromUtf8("Select one or more snippets first.")));
        return;
    }

    const qint64 nTargetSessionId = currentSessionId();
    if (nTargetSessionId <= 0)
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("复制 Snippet"), QString::fromUtf8("Duplicate Snippet")), uiText(QString::fromUtf8("当前 Session 无效。"), QString::fromUtf8("Current session is invalid.")));
        return;
    }

    QSet<qint64> setUniqueIds;
    QVector<qint64> vecUniqueSnippetIds;
    for (int i = 0; i < vecSnippetIds.size(); ++i)
    {
        if (!setUniqueIds.contains(vecSnippetIds.at(i)))
        {
            setUniqueIds.insert(vecSnippetIds.at(i));
            vecUniqueSnippetIds.append(vecSnippetIds.at(i));
        }
    }

    QVector<qint64> vecNewSnippetIds;
    QStringList vecFailedIds;
    int nTagCopyWarningCount = 0;
    for (int i = 0; i < vecUniqueSnippetIds.size(); ++i)
    {
        qint64 nNewSnippetId = 0;
        if (!m_pSnippetService->duplicateSnippet(vecUniqueSnippetIds.at(i), nTargetSessionId, &nNewSnippetId))
        {
            vecFailedIds.append(QString::number(vecUniqueSnippetIds.at(i)));
            continue;
        }

        if (nullptr != m_pTagService)
        {
            const QVector<QCTag> vecSourceTags = m_pTagService->listTagsBySnippet(vecUniqueSnippetIds.at(i));
            if (m_pTagService->lastError().isEmpty())
            {
                QVector<qint64> vecTagIds;
                for (int j = 0; j < vecSourceTags.size(); ++j)
                    vecTagIds.append(vecSourceTags.at(j).id());

                if (!vecTagIds.isEmpty() && !m_pTagService->replaceSnippetTags(nNewSnippetId, vecTagIds))
                    ++nTagCopyWarningCount;
            }
            else
            {
                ++nTagCopyWarningCount;
            }
        }

        vecNewSnippetIds.append(nNewSnippetId);
    }

    if (vecNewSnippetIds.isEmpty())
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("复制 Snippet"), QString::fromUtf8("Duplicate Snippet")), uiText(QString::fromUtf8("没有 snippet 被复制。"), QString::fromUtf8("No snippets were duplicated.")));
        return;
    }

    loadSnippets(nTargetSessionId);
    selectSnippetsByIds(vecNewSnippetIds);
    onSnippetSelectionChanged();
    statusBar()->showMessage(uiText(QString::fromUtf8("已复制 %1 个 snippet。"), QString::fromUtf8("Duplicated %1 snippet(s).")).arg(vecNewSnippetIds.size()), 4000);

    if (!vecFailedIds.isEmpty() || nTagCopyWarningCount > 0)
    {
        QStringList vecWarnings;
        if (!vecFailedIds.isEmpty())
            vecWarnings.append(uiText(QString::fromUtf8("失败的 snippet ID：%1"), QString::fromUtf8("Failed snippet ids: %1")).arg(vecFailedIds.join(QString::fromUtf8(", "))));
        if (nTagCopyWarningCount > 0)
            vecWarnings.append(uiText(QString::fromUtf8("%1 个重复的 snippet 未完全复制标签。"), QString::fromUtf8("%1 duplicated snippet(s) did not copy tags completely.")).arg(nTagCopyWarningCount));
        QMessageBox::warning(this, uiText(QString::fromUtf8("复制 Snippet"), QString::fromUtf8("Duplicate Snippet")), vecWarnings.join(QString::fromUtf8("\n")));
    }
}

void QCMainWindow::onMoveSnippet()
{
    QVector<qint64> vecSnippetIds = selectedSnippetIds();
    if (vecSnippetIds.isEmpty())
    {
        QMessageBox::information(this, uiText(QString::fromUtf8("移动 Snippet"), QString::fromUtf8("Move Snippet")), uiText(QString::fromUtf8("请先选择一个或多个 snippet。"), QString::fromUtf8("Select one or more snippets first.")));
        return;
    }

    if (nullptr == m_pSnippetService || nullptr == m_pSessionService)
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("移动 Snippet"), QString::fromUtf8("Move Snippet")), uiText(QString::fromUtf8("所需服务不可用。"), QString::fromUtf8("Required services are unavailable.")));
        return;
    }

    QSet<qint64> setUniqueIds;
    QVector<qint64> vecUniqueSnippetIds;
    for (int i = 0; i < vecSnippetIds.size(); ++i)
    {
        if (!setUniqueIds.contains(vecSnippetIds.at(i)))
        {
            setUniqueIds.insert(vecSnippetIds.at(i));
            vecUniqueSnippetIds.append(vecSnippetIds.at(i));
        }
    }

    const qint64 nCurrentSessionId = currentSessionId();
    const QVector<QCStudySession> vecSessions = m_pSessionService->listSessions();
    if (!m_pSessionService->lastError().isEmpty())
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("移动 Snippet"), QString::fromUtf8("Move Snippet")), m_pSessionService->lastError());
        return;
    }

    QStringList vecSessionLabels;
    QVector<qint64> vecSessionIds;
    for (int i = 0; i < vecSessions.size(); ++i)
    {
        const QCStudySession& session = vecSessions.at(i);
        if (session.id() == nCurrentSessionId)
            continue;

        const QString strTitle = session.title().trimmed().isEmpty()
            ? uiText(QString::fromUtf8("无标题 Session"), QString::fromUtf8("Untitled Session"))
            : session.title().trimmed();
        const QString strStatus = session.status() == QCSessionStatus::FinishedSessionStatus
            ? uiText(QString::fromUtf8("已完成"), QString::fromUtf8("Finished"))
            : uiText(QString::fromUtf8("活动中"), QString::fromUtf8("Active"));
        vecSessionLabels.append(QString::fromUtf8("%1 [%2]").arg(strTitle, strStatus));
        vecSessionIds.append(session.id());
    }

    if (vecSessionIds.isEmpty())
    {
        QMessageBox::information(this, uiText(QString::fromUtf8("移动 Snippet"), QString::fromUtf8("Move Snippet")), uiText(QString::fromUtf8("没有其他可用的 Session。"), QString::fromUtf8("No other session is available.")));
        return;
    }

    bool bAccepted = false;
    const QString strSelectedLabel = QInputDialog::getItem(this,
                                                           uiText(QString::fromUtf8("移动 Snippet"), QString::fromUtf8("Move Snippet")),
                                                           vecUniqueSnippetIds.size() > 1 ? uiText(QString::fromUtf8("将选中的 snippet 移动至 Session"), QString::fromUtf8("Move selected snippets to session")) : uiText(QString::fromUtf8("移动至 Session"), QString::fromUtf8("Move to session")),
                                                           vecSessionLabels,
                                                           0,
                                                           false,
                                                           &bAccepted);
    if (!bAccepted || strSelectedLabel.trimmed().isEmpty())
        return;

    const int nSelectedIndex = vecSessionLabels.indexOf(strSelectedLabel);
    if (nSelectedIndex < 0 || nSelectedIndex >= vecSessionIds.size())
        return;

    const qint64 nTargetSessionId = vecSessionIds.at(nSelectedIndex);

    int nAlreadyInTargetCount = 0;
    for (int i = 0; i < vecUniqueSnippetIds.size(); ++i)
    {
        QCSnippet snippet;
        if (m_pSnippetService->getSnippetById(vecUniqueSnippetIds.at(i), &snippet) && snippet.sessionId() == nTargetSessionId)
            ++nAlreadyInTargetCount;
    }

    if (!m_pSnippetService->moveSnippetsToSession(vecUniqueSnippetIds, nTargetSessionId))
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("移动 Snippet"), QString::fromUtf8("Move Snippet")), m_pSnippetService->lastError());
        return;
    }

    loadSessions();
    for (int i = 0; i < m_pSessionListWidget->count(); ++i)
    {
        QListWidgetItem *pItem = m_pSessionListWidget->item(i);
        if (nullptr != pItem && pItem->data(Qt::UserRole).toLongLong() == nTargetSessionId)
        {
            m_pSessionListWidget->setCurrentItem(pItem);
            break;
        }
    }
    loadSnippets(nTargetSessionId);
    selectSnippetsByIds(vecUniqueSnippetIds);
    onSnippetSelectionChanged();

    statusBar()->showMessage(uiText(QString::fromUtf8("已将 %1 个 snippet 移动到另一个 Session。"), QString::fromUtf8("Moved %1 snippet(s) to another session.")).arg(vecUniqueSnippetIds.size() - nAlreadyInTargetCount), 4000);
    if (nAlreadyInTargetCount > 0)
    {
        QMessageBox::information(this, uiText(QString::fromUtf8("移动 Snippet"), QString::fromUtf8("Move Snippet")), uiText(QString::fromUtf8("%1 个 snippet 已在目标 Session 中，已跳过。"), QString::fromUtf8("%1 snippet(s) were already in the target session and were skipped.")).arg(nAlreadyInTargetCount));
    }
}

void QCMainWindow::onClearSnippetTags()
{
    const QVector<qint64> vecSnippetIds = selectedSnippetIds();
    if (vecSnippetIds.isEmpty())
    {
        QMessageBox::information(this, uiText(QString::fromUtf8("清空 Snippet 标签"), QString::fromUtf8("Clear Snippet Tags")), uiText(QString::fromUtf8("请先选择一个或多个 snippet。"), QString::fromUtf8("Select one or more snippets first.")));
        return;
    }

    if (nullptr == m_pTagService)
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("清空 Snippet 标签"), QString::fromUtf8("Clear Snippet Tags")), uiText(QString::fromUtf8("标签服务不可用。"), QString::fromUtf8("Tag service is unavailable.")));
        return;
    }

    const QMessageBox::StandardButton button = QMessageBox::question(this,
                                                                     uiText(QString::fromUtf8("清空 Snippet 标签"), QString::fromUtf8("Clear Snippet Tags")),
                                                                     uiText(QString::fromUtf8("确定要清空选中的 %1 条 snippet 的所有标签吗？"), QString::fromUtf8("Clear all tags from %1 selected snippet(s)?")).arg(vecSnippetIds.size()),
                                                                     QMessageBox::Yes | QMessageBox::No,
                                                                     QMessageBox::No);
    if (button != QMessageBox::Yes)
        return;

    int nUpdatedCount = 0;
    QStringList vecFailedIds;
    for (int i = 0; i < vecSnippetIds.size(); ++i)
    {
        if (m_pTagService->replaceSnippetTags(vecSnippetIds.at(i), QVector<qint64>()))
            ++nUpdatedCount;
        else
            vecFailedIds.append(QString::number(vecSnippetIds.at(i)));
    }

    loadTagFilterOptions();
    loadSnippets(currentSessionId());
    selectSnippetsByIds(vecSnippetIds);
    onSnippetSelectionChanged();

    if (!vecFailedIds.isEmpty())
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("清空 Snippet 标签"), QString::fromUtf8("Clear Snippet Tags")), uiText(QString::fromUtf8("已清空 %1 个 snippet 的标签，但以下 snippet ID 失败：%2"), QString::fromUtf8("Cleared tags for %1 snippet(s), but these snippet ids failed: %2")).arg(nUpdatedCount).arg(vecFailedIds.join(QString::fromUtf8(", "))));
    }

    statusBar()->showMessage(uiText(QString::fromUtf8("已清空 %1 个 snippet 的标签。"), QString::fromUtf8("Cleared tags for %1 snippet(s).")).arg(nUpdatedCount), 4000);
}

void QCMainWindow::onManageTags()
{
    const QVector<qint64> vecSnippetIds = selectedSnippetIds();
    if (vecSnippetIds.isEmpty())
    {
        QMessageBox::information(this, uiText(QString::fromUtf8("Snippet 标签"), QString::fromUtf8("Snippet Tags")), QString::fromUtf8("Select one or more snippets first."));
        return;
    }

    if (nullptr == m_pTagService)
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("Snippet 标签"), QString::fromUtf8("Snippet Tags")), QString::fromUtf8("Tag service is unavailable."));
        return;
    }

    const QVector<QCTag> vecAvailableTags = m_pTagService->listTags();
    if (!m_pTagService->lastError().isEmpty())
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("Snippet 标签"), QString::fromUtf8("Snippet Tags")), m_pTagService->lastError());
        return;
    }

    const QHash<qint64, int> hashUsageCounts = buildTagUsageCounts(vecAvailableTags);
    if (!m_pTagService->lastError().isEmpty())
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("Snippet 标签"), QString::fromUtf8("Snippet Tags")), m_pTagService->lastError());
        return;
    }

    const QHash<qint64, Qt::CheckState> hashTagStates = buildTagSelectionStatesForSnippets(vecSnippetIds, vecAvailableTags);
    if (!m_pTagService->lastError().isEmpty())
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("Snippet 标签"), QString::fromUtf8("Snippet Tags")), m_pTagService->lastError());
        return;
    }

    QCSnippetTagDialog dialog(this);
    dialog.setWindowTitle(uiText(QString::fromUtf8("Snippet 标签"), QString::fromUtf8("Snippet Tags")));
    dialog.setBindingEnabled(true);
    dialog.setContextText(vecSnippetIds.size() > 1
        ? QString::fromUtf8("选中的标签将应用于所有选定的 snippet。未选中的标签将从所有选定的 snippet 中移除。混合标签保持不变，直到您点击它们。")
        : QString::fromUtf8("选中的标签将绑定到选定的 snippet。您还可以在此处创建、重命名或删除可重用标签。"));
    dialog.setAvailableTags(vecAvailableTags);
    dialog.setTagUsageCounts(hashUsageCounts);
    dialog.setTagSelectionStates(hashTagStates);
    if (QDialog::Accepted != dialog.exec())
        return;

    qint64 nCreatedTagId = 0;
    QVector<qint64> vecDeletedTagIds;
    if (!applyTagDialogChanges(dialog, &nCreatedTagId, &vecDeletedTagIds, uiText(QString::fromUtf8("Snippet 标签"), QString::fromUtf8("Snippet Tags"))))
        return;

    QHash<qint64, Qt::CheckState> hashFinalStates = dialog.tagSelectionStates();
    for (int i = 0; i < vecDeletedTagIds.size(); ++i)
        hashFinalStates.remove(vecDeletedTagIds.at(i));
    if (nCreatedTagId > 0)
        hashFinalStates.insert(nCreatedTagId, Qt::Checked);

    if (!applyTagSelectionStatesToSnippets(vecSnippetIds, hashFinalStates))
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("Snippet 标签"), QString::fromUtf8("Snippet Tags")), m_pTagService->lastError());
        return;
    }

    const qint64 nSessionId = currentSessionId();
    loadTagFilterOptions();
    loadSnippets(nSessionId);
    selectSnippetsByIds(vecSnippetIds);
    onSnippetSelectionChanged();
    statusBar()->showMessage(uiText(QString::fromUtf8("已为 %1 条 snippet 更新标签。"), QString::fromUtf8("Tags updated for %1 snippet(s).")).arg(vecSnippetIds.size()), 4000);
}

void QCMainWindow::onOpenTagLibrary()
{
    if (nullptr == m_pTagService)
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("标签库"), QString::fromUtf8("Tag Library")), uiText(QString::fromUtf8("标签服务不可用。"), QString::fromUtf8("Tag service is unavailable.")));
        return;
    }

    const QVector<QCTag> vecAvailableTags = m_pTagService->listTags();
    if (!m_pTagService->lastError().isEmpty())
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("标签库"), QString::fromUtf8("Tag Library")), m_pTagService->lastError());
        return;
    }

    const QHash<qint64, int> hashUsageCounts = buildTagUsageCounts(vecAvailableTags);
    if (!m_pTagService->lastError().isEmpty())
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("标签库"), QString::fromUtf8("Tag Library")), m_pTagService->lastError());
        return;
    }

    QCSnippetTagDialog dialog(this);
    dialog.setWindowTitle(uiText(QString::fromUtf8("标签库"), QString::fromUtf8("Tag Library")));
    dialog.setBindingEnabled(false);
    dialog.setContextText(uiText(QString::fromUtf8("在此管理标签。使用统计显示当前引用各标签的 snippet 数量。"),
                                 QString::fromUtf8("Manage reusable tags here. Usage counts show how many snippets currently reference each tag.")));
    dialog.setAvailableTags(vecAvailableTags);
    dialog.setTagUsageCounts(hashUsageCounts);
    if (QDialog::Accepted != dialog.exec())
        return;

    qint64 nCreatedTagId = 0;
    QVector<qint64> vecDeletedTagIds;
    if (!applyTagDialogChanges(dialog, &nCreatedTagId, &vecDeletedTagIds, uiText(QString::fromUtf8("标签库"), QString::fromUtf8("Tag Library"))))
        return;

    loadTagFilterOptions();
    applySnippetFilters();
    onSnippetSelectionChanged();
    statusBar()->showMessage(uiText(QString::fromUtf8("标签库已更新。"), QString::fromUtf8("Tag library updated.")), 4000);
}

void QCMainWindow::onDeleteSnippet()
{
    const QVector<qint64> vecSnippetIds = selectedSnippetIds();
    if (vecSnippetIds.isEmpty())
    {
        QMessageBox::information(this, uiText(QString::fromUtf8("删除 Snippet"), QString::fromUtf8("Delete Snippet")), uiText(QString::fromUtf8("请先选择一个或多个 snippet。"), QString::fromUtf8("Select one or more snippets first.")));
        return;
    }

    QString strMessage = vecSnippetIds.size() > 1
        ? uiText(QString::fromUtf8("确认删除选中的 %1 条 snippet？\n此操作不可撤销。"), QString::fromUtf8("Delete %1 selected snippets\nThis action cannot be undone.")).arg(vecSnippetIds.size())
        : uiText(QString::fromUtf8("确认删除选中的 snippet？\n此操作不可撤销。"), QString::fromUtf8("Delete selected snippet\nThis action cannot be undone."));
    const QMessageBox::StandardButton button = QMessageBox::question(this,
                                                                     uiText(QString::fromUtf8("删除 Snippet"), QString::fromUtf8("Delete Snippet")),
                                                                     strMessage,
                                                                     QMessageBox::Yes | QMessageBox::No,
                                                                     QMessageBox::No);
    if (QMessageBox::Yes != button)
        return;

    const qint64 nSessionId = currentSessionId();
    for (int i = 0; i < vecSnippetIds.size(); ++i)
    {
        if (!m_pSnippetService->deleteSnippet(vecSnippetIds.at(i)))
        {
            QMessageBox::warning(this, uiText(QString::fromUtf8("删除 Snippet"), QString::fromUtf8("Delete Snippet")), m_pSnippetService->lastError());
            return;
        }
    }

    loadSnippets(nSessionId);
    statusBar()->showMessage(uiText(QString::fromUtf8("已删除 %1 条 snippet。"), QString::fromUtf8("Deleted %1 snippet(s).")).arg(vecSnippetIds.size()), 4000);
}

void QCMainWindow::onToggleArchiveSnippet()
{
    const QVector<qint64> vecSnippetIds = selectedSnippetIds();
    if (vecSnippetIds.isEmpty())
    {
        QMessageBox::information(this, uiText(QString::fromUtf8("归档 Snippet"), QString::fromUtf8("Archive Snippet")), uiText(QString::fromUtf8("请先选择一个或多个 snippet。"), QString::fromUtf8("Select one or more snippets first.")));
        return;
    }

    int nArchivedCount = 0;
    for (int i = 0; i < vecSnippetIds.size(); ++i)
    {
        QCSnippet snippet;
        if (!m_pSnippetService->getSnippetById(vecSnippetIds.at(i), &snippet))
        {
            QMessageBox::warning(this, uiText(QString::fromUtf8("归档 Snippet"), QString::fromUtf8("Archive Snippet")), m_pSnippetService->lastError());
            return;
        }
        if (snippet.isArchived())
            ++nArchivedCount;
    }

    const bool bRestore = nArchivedCount == vecSnippetIds.size();
    for (int i = 0; i < vecSnippetIds.size(); ++i)
    {
        if (!m_pSnippetService->setArchived(vecSnippetIds.at(i), !bRestore))
        {
            QMessageBox::warning(this, uiText(QString::fromUtf8("归档 Snippet"), QString::fromUtf8("Archive Snippet")), m_pSnippetService->lastError());
            return;
        }
    }

    loadSnippets(currentSessionId());
    if (bRestore)
        selectSnippetsByIds(vecSnippetIds);
    statusBar()->showMessage(bRestore
        ? uiText(QString::fromUtf8("已恢复 %1 个 snippet。"), QString::fromUtf8("Restored %1 snippet(s).")).arg(vecSnippetIds.size())
        : uiText(QString::fromUtf8("已归档 %1 个 snippet。"), QString::fromUtf8("Archived %1 snippet(s).")).arg(vecSnippetIds.size()), 4000);
}

void QCMainWindow::onDeleteSession()
{
    const qint64 nSessionId = currentSessionId();
    if (nSessionId <= 0)
    {
        QMessageBox::information(this, uiText(QString::fromUtf8("删除 Session"), QString::fromUtf8("Delete Session")), uiText(QString::fromUtf8("请先选择一个 Session。"), QString::fromUtf8("Select a session first.")));
        return;
    }

    QCStudySession session;
    if (!m_pSessionService->getSessionById(nSessionId, &session))
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("删除 Session"), QString::fromUtf8("Delete Session")), m_pSessionService->lastError());
        return;
    }

    const QString strTitle = session.title().trimmed().isEmpty() ? uiText(QString::fromUtf8("无标题 Session"), QString::fromUtf8("Untitled Session")) : session.title().trimmed();
    const QMessageBox::StandardButton button = QMessageBox::question(
        this,
        uiText(QString::fromUtf8("删除 Session"), QString::fromUtf8("Delete Session")),
        uiText(QString::fromUtf8("确认删除 Session: %1？\n其中的 snippet 也将被移除，此操作不可撤销。"), QString::fromUtf8("Delete session: %1\nIts snippets will also be removed and this action cannot be undone.")).arg(strTitle),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (QMessageBox::Yes != button)
        return;

    if (!m_pSessionService->deleteSession(nSessionId))
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("删除 Session"), QString::fromUtf8("Delete Session")), m_pSessionService->lastError());
        return;
    }

    loadSessions();
    statusBar()->showMessage(uiText(QString::fromUtf8("Session 已删除。"), QString::fromUtf8("Session deleted.")), 4000);
}

void QCMainWindow::onSessionSelectionChanged()
{
    flushSnippetDraftToStorage(false);
    const qint64 nSessionId = currentSessionId();
    showSessionSummary(nSessionId);
    loadSnippets(nSessionId);
}

void QCMainWindow::onSnippetSelectionChanged()
{
    flushSnippetDraftToStorage(false);
    const QVector<qint64> vecSnippetIds = selectedSnippetIds();
    if (vecSnippetIds.size() == 1)
    {
        showSnippetDetails(vecSnippetIds.first());
    }
    else
    {
        clearSnippetDetails();
        if (vecSnippetIds.size() > 1)
        {
            m_pSnippetTitleValueLabel->setText(QString::fromUtf8("%1 snippets selected.").arg(vecSnippetIds.size()));
            m_pSnippetTagsValueLabel->setText(uiText(QString::fromUtf8("Batch mode: tag, clear tags, move, duplicate, archive/restore, and delete."), QString::fromUtf8("Batch mode: tag, clear tags, move, duplicate, archive/restore, and delete.")));
            m_pSnippetNoteTextEdit->setPlainText(uiText(QString::fromUtf8("Batch mode keeps the current selection for organize actions."), QString::fromUtf8("Batch mode keeps the current selection for organize actions.")));
            m_pSnippetContentTextEdit->setPlainText(uiText(QString::fromUtf8("Edit and summarize remain single-screenshot actions. Session actions stay in navigation and menus."), QString::fromUtf8("Edit and summarize remain single-screenshot actions. Session actions stay in navigation and menus.")));
        }
    }

    if (!m_bSyncingNavigationSelection && nullptr != m_pNavigationTreeWidget)
    {
        const qint64 nSnippetId = vecSnippetIds.size() == 1 ? vecSnippetIds.first() : 0;
        const qint64 nSessionId = currentSessionId();

        m_bSyncingNavigationSelection = true;
        QSignalBlocker blocker(m_pNavigationTreeWidget);
        QTreeWidgetItem *pTargetItem = nullptr;
        for (int i = 0; i < m_pNavigationTreeWidget->topLevelItemCount() && nullptr == pTargetItem; ++i)
        {
            QTreeWidgetItem *pSessionItem = m_pNavigationTreeWidget->topLevelItem(i);
            if (nullptr == pSessionItem)
                continue;

            if (nSnippetId > 0)
            {
                for (int j = 0; j < pSessionItem->childCount(); ++j)
                {
                    QTreeWidgetItem *pSnippetItem = pSessionItem->child(j);
                    if (nullptr != pSnippetItem && pSnippetItem->data(0, Qt::UserRole + 3).toLongLong() == nSnippetId)
                    {
                        pTargetItem = pSnippetItem;
                        break;
                    }
                }
            }
            else if (pSessionItem->data(0, Qt::UserRole + 2).toLongLong() == nSessionId)
            {
                pTargetItem = pSessionItem;
                break;
            }
        }

        if (nullptr != pTargetItem)
            m_pNavigationTreeWidget->setCurrentItem(pTargetItem);

        m_bSyncingNavigationSelection = false;
    }

    updateActionState();
}

void QCMainWindow::onSnippetFilterChanged()
{
    appendSearchHistory(currentSearchText());
    applySnippetFilters();
}

void QCMainWindow::onSearchHistoryChanged()
{
    if (nullptr == m_pSearchHistoryComboBox || nullptr == m_pSnippetSearchLineEdit)
        return;

    const QString strSearchText = m_pSearchHistoryComboBox->currentData().toString().trimmed();
    if (strSearchText.isEmpty())
        return;

    m_pSnippetSearchLineEdit->setText(strSearchText);
    applySnippetFilters();
}

void QCMainWindow::onClearSearch()
{
    if (nullptr == m_pSnippetSearchLineEdit)
        return;

    m_pSnippetSearchLineEdit->clear();
    if (nullptr != m_pSearchHistoryComboBox)
        m_pSearchHistoryComboBox->setCurrentIndex(0);
    applySnippetFilters();
    statusBar()->showMessage(uiText(QString::fromUtf8("已清空搜索"), QString::fromUtf8("Search cleared.")), 3000);
}

void QCMainWindow::onClearSearchHistory()
{
    if (nullptr == m_pSettingsService)
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("搜索历史"), QString::fromUtf8("Search History")), uiText(QString::fromUtf8("设置服务不可用。"), QString::fromUtf8("Settings service is unavailable.")));
        return;
    }

    if (!m_pSettingsService->setSnippetSearchHistory(QStringList()))
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("搜索历史"), QString::fromUtf8("Search History")), m_pSettingsService->lastError());
        return;
    }

    loadSearchHistoryOptions();
    statusBar()->showMessage(uiText(QString::fromUtf8("搜索历史已清空。"), QString::fromUtf8("Search history cleared.")), 3000);
}

void QCMainWindow::restoreDefaultFilters()
{
    if (nullptr != m_pNavigationSearchLineEdit)
        m_pNavigationSearchLineEdit->setPlaceholderText(uiText(QString::fromUtf8("Search course or screenshot title"), QString::fromUtf8("Search course or screenshot title")));
    if (nullptr != m_pSnippetSearchLineEdit)
        m_pSnippetSearchLineEdit->clear();
    if (nullptr != m_pSearchHistoryComboBox)
        m_pSearchHistoryComboBox->setCurrentIndex(0);
    if (nullptr != m_pFavoriteOnlyCheckBox)
        m_pFavoriteOnlyCheckBox->setChecked(false);
    if (nullptr != m_pReviewOnlyCheckBox)
        m_pReviewOnlyCheckBox->setChecked(false);
    if (nullptr != m_pSnippetTypeFilterComboBox)
        m_pSnippetTypeFilterComboBox->setCurrentIndex(0);
    if (nullptr != m_pShowArchivedCheckBox)
        m_pShowArchivedCheckBox->setChecked(false);
    if (nullptr != m_pTagFilterComboBox)
        m_pTagFilterComboBox->setCurrentIndex(0);
}

void QCMainWindow::onResetFilters()
{
    restoreDefaultFilters();
    applySnippetFilters();
    statusBar()->showMessage(uiText(QString::fromUtf8("筛选已重置。"), QString::fromUtf8("Filters reset.")), 3000);
}

void QCMainWindow::onFavoriteToggled(bool bChecked)
{
    if (m_bUpdatingSnippetStateControls)
        return;

    const qint64 nSnippetId = currentSnippetId();
    if (nSnippetId <= 0)
        return;

    if (!m_pSnippetService->setFavorite(nSnippetId, bChecked))
    {
        QMessageBox::warning(this, QString::fromUtf8("Snippet State"), m_pSnippetService->lastError());
        showSnippetDetails(nSnippetId);
        return;
    }

    loadSnippets(currentSessionId());
    if (selectSnippetById(nSnippetId))
        showSnippetDetails(nSnippetId);
    else
        clearSnippetDetails();

    statusBar()->showMessage(bChecked ? QString::fromUtf8("Snippet marked as favorite.")
                                      : QString::fromUtf8("Snippet favorite removed."), 3000);
}

void QCMainWindow::onReviewToggled(bool bChecked)
{
    if (m_bUpdatingSnippetStateControls)
        return;

    const qint64 nSnippetId = currentSnippetId();
    if (nSnippetId <= 0)
        return;

    if (!m_pSnippetService->setReviewState(nSnippetId, bChecked))
    {
        QMessageBox::warning(this, QString::fromUtf8("Snippet State"), m_pSnippetService->lastError());
        showSnippetDetails(nSnippetId);
        return;
    }

    loadSnippets(currentSessionId());
    if (selectSnippetById(nSnippetId))
        showSnippetDetails(nSnippetId);
    else
        clearSnippetDetails();

    statusBar()->showMessage(bChecked ? QString::fromUtf8("Snippet marked for review.")
                                      : QString::fromUtf8("Snippet review state cleared."), 3000);
}
void QCMainWindow::onCreateSession()
{
    QCStudySession activeSession;
    if (m_pSessionService->getActiveSession(&activeSession))
    {
        const QString strActiveTitle = activeSession.title().trimmed().isEmpty()
            ? QString::fromUtf8("Untitled Session")
            : activeSession.title().trimmed();
        const QMessageBox::StandardButton button = QMessageBox::question(
            this,
            QString::fromUtf8("New Session"),
            QString::fromUtf8("Finish active session: %1\nQtClip works better when only one session stays active at a time.").arg(strActiveTitle),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::Yes);
        if (QMessageBox::Yes != button)
            return;

        if (!m_pSessionService->finishSession(activeSession.id(), QDateTime::currentDateTimeUtc()))
        {
            QMessageBox::warning(this, QString::fromUtf8("New Session"), m_pSessionService->lastError());
            return;
        }
    }
    else if (!m_pSessionService->lastError().contains(QString::fromUtf8("No active study session"), Qt::CaseInsensitive))
    {
        QMessageBox::warning(this, QString::fromUtf8("New Session"), m_pSessionService->lastError());
        return;
    }

    QCCreateSessionDialog dialog(this);
    if (QDialog::Accepted != dialog.exec())
        return;

    QCStudySession session;
    session.setTitle(dialog.title());
    session.setCourseName(dialog.courseName());
    session.setDescription(dialog.description());
    if (!m_pSessionService->createSession(&session))
    {
        QMessageBox::warning(this, QString::fromUtf8("New Session"), m_pSessionService->lastError());
        return;
    }

    loadSessions();
    for (int i = 0; i < m_pSessionListWidget->count(); ++i)
    {
        QListWidgetItem *pItem = m_pSessionListWidget->item(i);
        if (pItem->data(Qt::UserRole).toLongLong() == session.id())
        {
            m_pSessionListWidget->setCurrentItem(pItem);
            statusBar()->showMessage(uiText(QString::fromUtf8("Session 已创建。"), QString::fromUtf8("Session created.")), 4000);
            break;
        }
    }
}

void QCMainWindow::onEditSession()
{
    const qint64 nSessionId = currentSessionId();
    if (nSessionId <= 0)
    {
        QMessageBox::information(this, uiText(QString::fromUtf8("编辑 Session"), QString::fromUtf8("Edit Session")), uiText(QString::fromUtf8("请先选择一个 Session。"), QString::fromUtf8("Select a session first.")));
        return;
    }

    QCStudySession session;
    if (!m_pSessionService->getSessionById(nSessionId, &session))
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("编辑 Session"), QString::fromUtf8("Edit Session")), m_pSessionService->lastError());
        return;
    }

    QCCreateSessionDialog dialog(this);
    dialog.setWindowTitle(uiText(QString::fromUtf8("编辑 Session"), QString::fromUtf8("Edit Session")));
    dialog.setTitle(session.title());
    dialog.setCourseName(session.courseName());
    dialog.setDescription(session.description());
    if (QDialog::Accepted != dialog.exec())
        return;

    session.setTitle(dialog.title());
    session.setCourseName(dialog.courseName());
    session.setDescription(dialog.description());
    if (!m_pSessionService->updateSession(&session))
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("编辑 Session"), QString::fromUtf8("Edit Session")), m_pSessionService->lastError());
        return;
    }

    loadSessions();
    statusBar()->showMessage(uiText(QString::fromUtf8("Session 已更新。"), QString::fromUtf8("Session updated.")), 4000);
}

void QCMainWindow::onFinishSession()
{
    const qint64 nSessionId = currentSessionId();
    if (nSessionId <= 0)
    {
        QMessageBox::information(this, uiText(QString::fromUtf8("完成 Session"), QString::fromUtf8("Finish Session")), uiText(QString::fromUtf8("请先选择一个 Session。"), QString::fromUtf8("Select a session first.")));
        return;
    }

    QCStudySession session;
    if (!m_pSessionService->getSessionById(nSessionId, &session))
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("完成 Session"), QString::fromUtf8("Finish Session")), m_pSessionService->lastError());
        return;
    }

    if (session.status() == QCSessionStatus::FinishedSessionStatus)
    {
        QMessageBox::information(this, uiText(QString::fromUtf8("完成 Session"), QString::fromUtf8("Finish Session")), uiText(QString::fromUtf8("该 Session 已完成。"), QString::fromUtf8("The selected session is already finished.")));
        return;
    }

    const QString strTitle = session.title().trimmed().isEmpty() ? QString::fromUtf8("Untitled Session") : session.title().trimmed();
    const QMessageBox::StandardButton button = QMessageBox::question(
        this,
        QString::fromUtf8("Finish Session"),
        QString::fromUtf8("Finish session: %1\nYou can still browse its snippets afterwards.").arg(strTitle),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (QMessageBox::Yes != button)
        return;

    if (!m_pSessionService->finishSession(nSessionId, QDateTime::currentDateTimeUtc()))
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("完成 Session"), QString::fromUtf8("Finish Session")), m_pSessionService->lastError());
        return;
    }

    loadSessions();
    statusBar()->showMessage(uiText(QString::fromUtf8("Session 已完成。"), QString::fromUtf8("Session finished.")), 4000);
}

void QCMainWindow::onReopenSession()
{
    const qint64 nSessionId = currentSessionId();
    if (nSessionId <= 0)
    {
        QMessageBox::information(this, uiText(QString::fromUtf8("重新打开 Session"), QString::fromUtf8("Reopen Session")), uiText(QString::fromUtf8("请先选择一个 Session。"), QString::fromUtf8("Select a session first.")));
        return;
    }

    QCStudySession session;
    if (!m_pSessionService->getSessionById(nSessionId, &session))
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("重新打开 Session"), QString::fromUtf8("Reopen Session")), m_pSessionService->lastError());
        return;
    }

    if (session.status() == QCSessionStatus::ActiveSessionStatus)
    {
        QMessageBox::information(this, uiText(QString::fromUtf8("重新打开 Session"), QString::fromUtf8("Reopen Session")), uiText(QString::fromUtf8("该 Session 已经是进行中状态。"), QString::fromUtf8("The selected session is already active.")));
        return;
    }

    session.setStatus(QCSessionStatus::ActiveSessionStatus);
    session.setEndedAt(QDateTime());
    if (!m_pSessionService->updateSession(&session))
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("重新打开 Session"), QString::fromUtf8("Reopen Session")), m_pSessionService->lastError());
        return;
    }

    loadSessions();
    statusBar()->showMessage(uiText(QString::fromUtf8("Session 已重新打开。"), QString::fromUtf8("Session reopened.")), 4000);
}

void QCMainWindow::onEditCurrentItem()
{
    if (nullptr != m_pEditSnippetAction && m_pEditSnippetAction->isEnabled() && hasSingleSelectedSnippet() && currentSnippetId() > 0)
    {
        onEditSnippet();
        return;
    }

    if (nullptr != m_pEditSessionAction && m_pEditSessionAction->isEnabled() && currentSessionId() > 0)
        onEditSession();
}

void QCMainWindow::onCreateTextSnippet()
{
    const qint64 nSessionId = currentSessionId();
    if (nSessionId <= 0)
    {
        QMessageBox::information(this, uiText(QString::fromUtf8("新建文本 Snippet"), QString::fromUtf8("New Text Snippet")), uiText(QString::fromUtf8("在创建文本 snippet 之前，请先选择一个 Session。"), QString::fromUtf8("Select a session first before creating a text snippet.")));
        return;
    }

    QCCreateTextSnippetDialog dialog(this);
    if (QDialog::Accepted != dialog.exec())
        return;

    QCSnippet snippet;
    snippet.setSessionId(nSessionId);
    snippet.setCaptureType(QCCaptureType::ManualCaptureType);
    snippet.setTitle(dialog.title());
    snippet.setNote(QString());
    snippet.setContentText(dialog.content());
    snippet.setNoteKind(QCNoteKind::ConceptNoteKind);
    snippet.setNoteLevel(QCNoteLevel::ImportantNoteLevel);
    snippet.setSource(QString::fromUtf8("ui"));
    snippet.setLanguage(QString::fromUtf8("en"));

    if (!m_pSnippetService->createTextSnippet(&snippet))
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("新建文本 Snippet"), QString::fromUtf8("New Text Snippet")), m_pSnippetService->lastError());
        return;
    }

    loadSnippets(nSessionId);
    if (!selectSnippetById(snippet.id()))
        QMessageBox::warning(this, uiText(QString::fromUtf8("新建文本 Snippet"), QString::fromUtf8("New Text Snippet")), uiText(QString::fromUtf8("Snippet 已保存，但自动选择失败。"), QString::fromUtf8("Snippet was saved, but automatic selection failed.")));
    else
        statusBar()->showMessage(uiText(QString::fromUtf8("文本 Snippet 已创建。"), QString::fromUtf8("Text snippet created.")), 4000);
}

void QCMainWindow::onEditSnippet()
{
    if (!hasSingleSelectedSnippet())
    {
        QMessageBox::information(this, uiText(QString::fromUtf8("编辑 Snippet"), QString::fromUtf8("Edit Snippet")), uiText(QString::fromUtf8("请先准确选择一个 snippet。"), QString::fromUtf8("Select exactly one snippet first.")));
        return;
    }

    QCSnippet snippet;
    if (!currentSnippetForAction(&snippet))
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("编辑 Snippet"), QString::fromUtf8("Edit Snippet")), m_pSnippetService->lastError());
        return;
    }

    if (snippet.type() == QCSnippetType::TextSnippetType)
    {
        QCCreateTextSnippetDialog dialog(this);
        dialog.setWindowTitle(uiText(QString::fromUtf8("编辑文本 Snippet"), QString::fromUtf8("Edit Text Snippet")));
        dialog.setTitle(snippet.title());
        dialog.setNote(snippet.note());
        dialog.setContent(snippet.contentText());
        if (QDialog::Accepted != dialog.exec())
            return;

        snippet.setTitle(dialog.title());
        snippet.setNote(QString());
        snippet.setContentText(dialog.content());
    }
    else if (snippet.type() == QCSnippetType::ImageSnippetType)
    {
        QCAttachment primaryAttachment;
        if (!m_pSnippetService->getPrimaryAttachmentBySnippetId(snippet.id(), &primaryAttachment))
        {
            QMessageBox::warning(this, uiText(QString::fromUtf8("编辑 Snippet"), QString::fromUtf8("Edit Snippet")), m_pSnippetService->lastError());
            return;
        }

        QCQuickCaptureDialog dialog(primaryAttachment.filePath(), this);
        dialog.setWindowTitle(uiText(QString::fromUtf8("编辑图片 Snippet"), QString::fromUtf8("Edit Image Snippet")));
        dialog.setTitle(snippet.title());
        dialog.setNote(snippet.note());
        if (QDialog::Accepted != dialog.exec())
            return;

        snippet.setTitle(dialog.title());
        snippet.setNote(QString());
    }
    else
    {
        QMessageBox::information(this, uiText(QString::fromUtf8("编辑 Snippet"), QString::fromUtf8("Edit Snippet")), uiText(QString::fromUtf8("目前仅支持编辑文本和图片 Snippet。"), QString::fromUtf8("Editing is currently available for text and image snippets.")));
        return;
    }

    if (!m_pSnippetService->updateSnippet(&snippet))
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("编辑 Snippet"), QString::fromUtf8("Edit Snippet")), m_pSnippetService->lastError());
        return;
    }

    loadSnippets(snippet.sessionId());
    if (selectSnippetById(snippet.id()))
        showSnippetDetails(snippet.id());

    statusBar()->showMessage(uiText(QString::fromUtf8("Snippet 已更新。"), QString::fromUtf8("Snippet updated.")), 4000);
}

void QCMainWindow::onCaptureScreen()
{
    const qint64 nSessionId = currentSessionId();
    if (nSessionId <= 0)
    {
        QMessageBox::information(this, uiText(QString::fromUtf8("全屏截图"), QString::fromUtf8("Capture Screen")), uiText(QString::fromUtf8("请先选择一个 Session，然后捕获屏幕。"), QString::fromUtf8("Select a session first before capturing the screen.")));
        return;
    }

    if (nullptr == m_pScreenCaptureService)
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("全屏截图"), QString::fromUtf8("Capture Screen")), uiText(QString::fromUtf8("屏幕截图服务不可用。"), QString::fromUtf8("Screen capture service is unavailable.")));
        return;
    }

    statusBar()->showMessage(uiText(QString::fromUtf8("正在捕获主屏幕..."), QString::fromUtf8("Capturing primary screen...")));
    QCScreenCaptureResult captureResult;
    if (!captureScreenToFile(&captureResult))
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("全屏截图"), QString::fromUtf8("Capture Screen")), m_pScreenCaptureService->lastError());
        statusBar()->showMessage(uiText(QString::fromUtf8("屏幕截图失败。"), QString::fromUtf8("Screen capture failed.")), 4000);
        return;
    }

    QCQuickCaptureDialog dialog(captureResult.m_strFilePath, this);
    if (QDialog::Accepted != dialog.exec())
    {
        statusBar()->showMessage(uiText(QString::fromUtf8("屏幕截图已取消。"), QString::fromUtf8("Screen capture cancelled.")), 3000);
        return;
    }

    QCSnippet snippet;
    snippet.setSessionId(nSessionId);
    snippet.setType(QCSnippetType::ImageSnippetType);
    snippet.setCaptureType(QCCaptureType::ScreenCaptureType);
    snippet.setTitle(dialog.title());
    snippet.setNote(QString());
    snippet.setNoteKind(QCNoteKind::ReviewNoteKind);
    snippet.setNoteLevel(QCNoteLevel::ImportantNoteLevel);
    snippet.setSource(QString::fromUtf8("screen"));
    snippet.setCapturedAt(captureResult.m_dateTimeCreatedAt);
    snippet.setCreatedAt(captureResult.m_dateTimeCreatedAt);
    snippet.setUpdatedAt(captureResult.m_dateTimeCreatedAt);

    QCAttachment primaryAttachment;
    primaryAttachment.setFilePath(dialog.imagePath());
    primaryAttachment.setFileName(QFileInfo(dialog.imagePath()).fileName());
    primaryAttachment.setMimeType(QString::fromUtf8("image/png"));
    primaryAttachment.setCreatedAt(captureResult.m_dateTimeCreatedAt);

    if (!m_pSnippetService->createImageSnippetWithPrimaryAttachment(&snippet, &primaryAttachment))
    {
        QMessageBox::warning(this, QString::fromUtf8("Capture Screen"), m_pSnippetService->lastError());
        statusBar()->showMessage(uiText(QString::fromUtf8("捕获的图像无法保存。"), QString::fromUtf8("Captured image could not be saved.")), 5000);
        return;
    }

    handleSavedImageSnippet(nSessionId, snippet.id(), QString::fromUtf8("Screenshot saved and selected."));
}
void QCMainWindow::onCaptureRegion()
{
    const qint64 nSessionId = currentSessionId();
    if (nSessionId <= 0)
    {
        QMessageBox::information(this, uiText(QString::fromUtf8("区域截图"), QString::fromUtf8("Capture Region")), uiText(QString::fromUtf8("请先选择一个 Session，然后捕获区域。"), QString::fromUtf8("Select a session first before capturing a region.")));
        return;
    }

    if (nullptr == m_pScreenCaptureService)
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("区域截图"), QString::fromUtf8("Capture Region")), uiText(QString::fromUtf8("屏幕截图服务不可用。"), QString::fromUtf8("Screen capture service is unavailable.")));
        return;
    }

    statusBar()->showMessage(uiText(QString::fromUtf8("请在主屏幕上选择捕获区域..."), QString::fromUtf8("Select a capture region on the primary screen...")));
    QCScreenCaptureResult captureResult;
    bool bCancelled = false;
    QString strCaptureFailureMessage;
    if (!captureRegionToFile(&captureResult, &bCancelled, &strCaptureFailureMessage))
    {
        if (bCancelled)
        {
            statusBar()->showMessage(uiText(QString::fromUtf8("区域截图已取消。"), QString::fromUtf8("Region capture cancelled.")), 3000);
            return;
        }

        const QString strMessage = strCaptureFailureMessage.trimmed().isEmpty()
            ? uiText(QString::fromUtf8("区域截图失败。"), QString::fromUtf8("Region capture failed."))
            : strCaptureFailureMessage;
        QMessageBox::warning(this, uiText(QString::fromUtf8("区域截图"), QString::fromUtf8("Capture Region")), strMessage);
        statusBar()->showMessage(strMessage, 4000);
        return;
    }

    statusBar()->showMessage(uiText(QString::fromUtf8("区域已捕获：%1 x %2"), QString::fromUtf8("Region captured: %1 x %2")).arg(captureResult.m_nWidth).arg(captureResult.m_nHeight), 3000);

    QCQuickCaptureDialog dialog(captureResult.m_strFilePath, this);
    if (QDialog::Accepted != dialog.exec())
    {
        statusBar()->showMessage(QString::fromUtf8("Region capture cancelled."), 3000);
        return;
    }

    QCSnippet snippet;
    snippet.setSessionId(nSessionId);
    snippet.setType(QCSnippetType::ImageSnippetType);
    snippet.setCaptureType(QCCaptureType::ScreenCaptureType);
    snippet.setTitle(dialog.title());
    snippet.setNote(QString());
    snippet.setNoteKind(QCNoteKind::ReviewNoteKind);
    snippet.setNoteLevel(QCNoteLevel::ImportantNoteLevel);
    snippet.setSource(QString::fromUtf8("region"));
    snippet.setCapturedAt(captureResult.m_dateTimeCreatedAt);
    snippet.setCreatedAt(captureResult.m_dateTimeCreatedAt);
    snippet.setUpdatedAt(captureResult.m_dateTimeCreatedAt);

    QCAttachment primaryAttachment;
    primaryAttachment.setFilePath(dialog.imagePath());
    primaryAttachment.setFileName(QFileInfo(dialog.imagePath()).fileName());
    primaryAttachment.setMimeType(QString::fromUtf8("image/png"));
    primaryAttachment.setCreatedAt(captureResult.m_dateTimeCreatedAt);

    if (!m_pSnippetService->createImageSnippetWithPrimaryAttachment(&snippet, &primaryAttachment))
    {
        QMessageBox::warning(this, QString::fromUtf8("Capture Region"), m_pSnippetService->lastError());
        statusBar()->showMessage(uiText(QString::fromUtf8("区域图像无法保存。"), QString::fromUtf8("Region image could not be saved.")), 5000);
        return;
    }

    handleSavedImageSnippet(nSessionId, snippet.id(), QString::fromUtf8("Region capture saved and selected."));
}

void QCMainWindow::onImportImageSnippet()
{
    const qint64 nSessionId = currentSessionId();
    if (nSessionId <= 0)
    {
        QMessageBox::information(this, uiText(QString::fromUtf8("导入图片"), QString::fromUtf8("Import Image")), uiText(QString::fromUtf8("在导入图片之前，请先选择一个 Session。"), QString::fromUtf8("Select a session first before importing an image.")));
        return;
    }

    QString strInitialImageDirectory;
    bool bDefaultCopyImportedImage = false;
    if (nullptr != m_pSettingsService)
    {
        m_pSettingsService->getScreenshotSaveDirectory(&strInitialImageDirectory);
        m_pSettingsService->getDefaultCopyImportedImageToCaptureDirectory(&bDefaultCopyImportedImage);
    }

    QCCreateImageSnippetDialog dialog(strInitialImageDirectory,
                                      bDefaultCopyImportedImage,
                                      strInitialImageDirectory,
                                      m_pScreenCaptureService,
                                      this);
    if (QDialog::Accepted != dialog.exec())
        return;

    QString strAttachmentFilePath = dialog.filePath();
    if (dialog.shouldCopyImportedImageToDefaultCaptureDirectory())
    {
        if (nullptr == m_pScreenCaptureService)
        {
            QMessageBox::warning(this, uiText(QString::fromUtf8("导入图片"), QString::fromUtf8("Import Image")), uiText(QString::fromUtf8("屏幕截图服务不可用。"), QString::fromUtf8("Screen capture service is unavailable.")));
            return;
        }

        QString strCopiedFilePath;
        if (!m_pScreenCaptureService->copyImportedImageToCaptureDirectory(dialog.filePath(), &strCopiedFilePath))
        {
            QMessageBox::warning(this, uiText(QString::fromUtf8("导入图片"), QString::fromUtf8("Import Image")), m_pScreenCaptureService->lastError());
            return;
        }

        strAttachmentFilePath = strCopiedFilePath;
    }

    QCSnippet snippet;
    snippet.setSessionId(nSessionId);
    snippet.setType(QCSnippetType::ImageSnippetType);
    snippet.setCaptureType(QCCaptureType::ImportCaptureType);
    snippet.setTitle(dialog.title());
    snippet.setNote(QString());
    snippet.setNoteKind(QCNoteKind::ReviewNoteKind);
    snippet.setNoteLevel(QCNoteLevel::ReviewNoteLevel);
    snippet.setSource(QString::fromUtf8("file"));

    QCAttachment primaryAttachment;
    primaryAttachment.setFilePath(strAttachmentFilePath);
    primaryAttachment.setFileName(QFileInfo(strAttachmentFilePath).fileName());
    primaryAttachment.setMimeType(QString::fromUtf8("image/png"));

    if (!m_pSnippetService->createImageSnippetWithPrimaryAttachment(&snippet, &primaryAttachment))
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("导入图片"), QString::fromUtf8("Import Image")), m_pSnippetService->lastError());
        return;
    }

    handleSavedImageSnippet(nSessionId, snippet.id(), QString::fromUtf8("Imported image saved."));
    if (dialog.shouldCopyImportedImageToDefaultCaptureDirectory())
    {
        statusBar()->showMessage(uiText(QString::fromUtf8("导入的图片已通过拷贝文件保存：%1"), QString::fromUtf8("Imported image saved using copied file: %1")).arg(strAttachmentFilePath), 6000);
    }
}

bool QCMainWindow::createImageSnippetFromFilePath(qint64 nSessionId,
                                                  const QString& strFilePath,
                                                  const QString& strTitle,
                                                  qint64 *pnSnippetId)
{
    if (nullptr != pnSnippetId)
        *pnSnippetId = 0;

    if (nSessionId <= 0)
    {
        QMessageBox::information(this, uiText(QString::fromUtf8("导入图片"), QString::fromUtf8("Import Image")), uiText(QString::fromUtf8("请先选择一个 Session。"), QString::fromUtf8("Select a session first.")));
        return false;
    }

    const QString strNormalizedFilePath = strFilePath.trimmed();
    if (strNormalizedFilePath.isEmpty())
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("导入图片"), QString::fromUtf8("Import Image")), uiText(QString::fromUtf8("图片路径为空。"), QString::fromUtf8("Image path is empty.")));
        return false;
    }

    QCSnippet snippet;
    snippet.setSessionId(nSessionId);
    snippet.setType(QCSnippetType::ImageSnippetType);
    snippet.setCaptureType(QCCaptureType::ImportCaptureType);
    snippet.setTitle(strTitle.trimmed());
    snippet.setNote(QString());
    snippet.setNoteKind(QCNoteKind::ReviewNoteKind);
    snippet.setNoteLevel(QCNoteLevel::ReviewNoteLevel);
    snippet.setSource(QString::fromUtf8("clipboard"));

    QCAttachment primaryAttachment;
    primaryAttachment.setFilePath(strNormalizedFilePath);
    primaryAttachment.setFileName(QFileInfo(strNormalizedFilePath).fileName());
    primaryAttachment.setMimeType(QString::fromUtf8("image/png"));

    if (!m_pSnippetService->createImageSnippetWithPrimaryAttachment(&snippet, &primaryAttachment))
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("导入图片"), QString::fromUtf8("Import Image")), m_pSnippetService->lastError());
        return false;
    }

    if (nullptr != pnSnippetId)
        *pnSnippetId = snippet.id();

    return true;
}


QStringList QCMainWindow::extractLocalImageFilePaths(const QMimeData *pMimeData) const
{
    QStringList vecImagePaths;
    if (nullptr == pMimeData)
        return vecImagePaths;

    const QList<QUrl> vecUrls = pMimeData->urls();
    for (int i = 0; i < vecUrls.size(); ++i)
    {
        const QUrl& url = vecUrls.at(i);
        if (!url.isLocalFile())
            continue;

        const QString strLocalPath = url.toLocalFile().trimmed();
        if (strLocalPath.isEmpty())
            continue;

        const QString strSuffix = QFileInfo(strLocalPath).suffix().toLower();
        if (strSuffix == QString::fromUtf8("png")
            || strSuffix == QString::fromUtf8("jpg")
            || strSuffix == QString::fromUtf8("jpeg")
            || strSuffix == QString::fromUtf8("bmp")
            || strSuffix == QString::fromUtf8("webp")
            || strSuffix == QString::fromUtf8("gif"))
        {
            if (!vecImagePaths.contains(strLocalPath, Qt::CaseInsensitive))
                vecImagePaths.append(strLocalPath);
        }
    }

    return vecImagePaths;
}

bool QCMainWindow::importImageFilesToCurrentSession(const QStringList& vecFilePaths,
                                                    bool bTriggerWorkspaceSummary,
                                                    qint64 *pnLastSnippetId)
{
    if (nullptr != pnLastSnippetId)
        *pnLastSnippetId = 0;

    const qint64 nSessionId = currentSessionId();
    if (nSessionId <= 0)
    {
        QMessageBox::information(this,
                                 uiText(QString::fromUtf8("导入图片"), QString::fromUtf8("Import Image")),
                                 uiText(QString::fromUtf8("请先在左侧选择 Session。"), QString::fromUtf8("Select a session first from the left panel.")));
        return false;
    }

    if (vecFilePaths.isEmpty())
    {
        QMessageBox::information(this,
                                 uiText(QString::fromUtf8("导入图片"), QString::fromUtf8("Import Image")),
                                 uiText(QString::fromUtf8("没有可导入的图片。"), QString::fromUtf8("No image files to import.")));
        return false;
    }

    int nSucceededCount = 0;
    qint64 nLastSnippetId = 0;
    for (int i = 0; i < vecFilePaths.size(); ++i)
    {
        const QString strImagePath = vecFilePaths.at(i).trimmed();
        if (strImagePath.isEmpty())
            continue;

        const QFileInfo fileInfo(strImagePath);
        const QString strTitle = fileInfo.completeBaseName().trimmed().isEmpty()
            ? uiText(QString::fromUtf8("图片 %1"), QString::fromUtf8("Image %1")).arg(i + 1)
            : fileInfo.completeBaseName().trimmed();

        qint64 nSnippetId = 0;
        if (!createImageSnippetFromFilePath(nSessionId, strImagePath, strTitle, &nSnippetId))
            continue;

        ++nSucceededCount;
        nLastSnippetId = nSnippetId;
    }

    if (nSucceededCount <= 0)
        return false;

    if (nullptr != pnLastSnippetId)
        *pnLastSnippetId = nLastSnippetId;

    loadSnippets(nSessionId);
    if (nLastSnippetId > 0)
        selectSnippetById(nLastSnippetId);

    const QString strMessage = uiText(QString::fromUtf8("已导入 %1 张图片。"), QString::fromUtf8("Imported %1 image(s)."))
        .arg(nSucceededCount);
    statusBar()->showMessage(strMessage, 5000);

    if (bTriggerWorkspaceSummary)
        onRunWorkspaceSummary();

    return true;
}
void QCMainWindow::onPasteImageFromClipboard()
{
    const QClipboard *pClipboard = QGuiApplication::clipboard();
    const QMimeData *pMimeData = (nullptr != pClipboard) ? pClipboard->mimeData() : nullptr;

    QStringList vecImagePaths = extractLocalImageFilePaths(pMimeData);

    if ((nullptr != pMimeData) && pMimeData->hasImage())
    {
        const QImage image = qvariant_cast<QImage>(pMimeData->imageData());
        if (!image.isNull())
        {
            QString strSaveDirectory;
            if (nullptr != m_pSettingsService)
                m_pSettingsService->getScreenshotSaveDirectory(&strSaveDirectory);
            if (strSaveDirectory.trimmed().isEmpty())
                strSaveDirectory = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
            if (strSaveDirectory.trimmed().isEmpty())
                strSaveDirectory = QDir::tempPath();

            QDir saveDirectory(strSaveDirectory);
            if (saveDirectory.exists() || saveDirectory.mkpath(QString::fromUtf8(".")))
            {
                const QString strFileName = QString::fromUtf8("paste_%1.png").arg(QDateTime::currentDateTime().toString(QString::fromUtf8("yyyyMMdd_hhmmss_zzz")));
                const QString strImagePath = saveDirectory.filePath(strFileName);
                if (image.save(strImagePath, "PNG") && !vecImagePaths.contains(strImagePath, Qt::CaseInsensitive))
                    vecImagePaths.append(strImagePath);
            }
        }
    }

    qint64 nLastSnippetId = 0;
    importImageFilesToCurrentSession(vecImagePaths,
                                     true,
                                     &nLastSnippetId);
}
void QCMainWindow::onRunWorkspaceSummary()
{
    if (m_bSessionSummaryRunning || m_bSnippetSummaryRunning)
    {
        statusBar()->showMessage(uiText(QString::fromUtf8("AI task is running. Please wait."), QString::fromUtf8("AI task is running. Please wait.")), 3000);
        return;
    }

    if (hasSingleSelectedSnippet())
    {
        onSummarizeSnippet();
        return;
    }

    onSummarizeSession();
}
void QCMainWindow::onDeleteCurrentItem()
{
    if (nullptr != m_pDeleteSnippetAction && m_pDeleteSnippetAction->isEnabled() && !selectedSnippetIds().isEmpty() && currentSnippetId() > 0)
    {
        onDeleteSnippet();
        return;
    }

    if (nullptr != m_pDeleteSessionAction && m_pDeleteSessionAction->isEnabled() && currentSessionId() > 0)
        onDeleteSession();
}

void QCMainWindow::onSummarizeSnippet()
{
    if (!hasSingleSelectedSnippet())
    {
        QMessageBox::information(this, uiText(QString::fromUtf8("总结 Snippet"), QString::fromUtf8("Summarize Snippet")), uiText(QString::fromUtf8("请先准确选择一个 snippet。"), QString::fromUtf8("Select exactly one snippet first.")));
        return;
    }

    const qint64 nSnippetId = selectedSnippetIds().first();
    if (nullptr == m_pAiProcessService)
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("总结 Snippet"), QString::fromUtf8("Summarize Snippet")), uiText(QString::fromUtf8("AI 处理服务不可用。"), QString::fromUtf8("AI process service is unavailable.")));
        return;
    }

    if (!startSnippetSummary(nSnippetId, false))
    {
        const QString strErrorMessage = m_pAiProcessService->lastError();
        if (!strErrorMessage.trimmed().isEmpty())
            QMessageBox::warning(this, QString::fromUtf8("Summarize Snippet"), strErrorMessage);
    }
}

void QCMainWindow::onSummarizeSession()
{
    if (m_bSessionSummaryRunning || m_bSnippetSummaryRunning)
        return;

    const qint64 nSessionId = currentSessionId();
    if (nSessionId <= 0)
    {
        QMessageBox::information(this, uiText(QString::fromUtf8("Session 总结"), QString::fromUtf8("Summarize Session")), uiText(QString::fromUtf8("请先选择一个 Session。"), QString::fromUtf8("Select a session first.")));
        return;
    }

    if (nullptr == m_pAiProcessService)
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("Session 总结"), QString::fromUtf8("Summarize Session")), uiText(QString::fromUtf8("AI 处理服务不可用。"), QString::fromUtf8("AI process service is unavailable.")));
        return;
    }

    QCAiTaskExecutionContext executionContext;
    if (!m_pAiProcessService->prepareSessionSummary(nSessionId, &executionContext))
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("总结 Session"), QString::fromUtf8("Summarize Session")), m_pAiProcessService->lastError());
        return;
    }

    m_bSessionSummaryRunning = true;
    m_nRetrySessionId = nSessionId;
    m_strAiStatusMessage = uiText(QString::fromUtf8("Session AI 已在后台启动。如果运行失败，您可以重试。"), QString::fromUtf8("Session AI started in background. Retry stays available if this run fails."));
    updateAiStatusDisplay();
    updateActionState();
    statusBar()->showMessage(uiText(QString::fromUtf8("正在后台生成 Session 总结..."), QString::fromUtf8("Generating session summary in background...")));
    m_pSessionSummaryWatcher->setFuture(QtConcurrent::run(RunAiTaskInBackground,
                                                          m_pAiProcessService,
                                                          executionContext));
}

void QCMainWindow::onRetrySnippetSummary()
{
    if (!m_bHasRetryableSnippetSummary || m_nRetrySnippetId <= 0)
        return;

    if (!startSnippetSummary(m_nRetrySnippetId, false))
    {
        const QString strErrorMessage = (nullptr != m_pAiProcessService) ? m_pAiProcessService->lastError() : QString();
        if (!strErrorMessage.trimmed().isEmpty())
            QMessageBox::warning(this, uiText(QString::fromUtf8("重试 Snippet AI"), QString::fromUtf8("Retry Snippet AI")), strErrorMessage);
    }
}

void QCMainWindow::onRetrySessionSummary()
{
    if (!m_bHasRetryableSessionSummary || m_nRetrySessionId <= 0)
        return;

    if (m_bSessionSummaryRunning || m_bSnippetSummaryRunning || nullptr == m_pAiProcessService)
        return;

    QCAiTaskExecutionContext executionContext;
    if (!m_pAiProcessService->prepareSessionSummary(m_nRetrySessionId, &executionContext))
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("重试 Session AI"), QString::fromUtf8("Retry Session AI")), m_pAiProcessService->lastError());
        return;
    }

    m_bSessionSummaryRunning = true;
    m_strAiStatusMessage = uiText(QString::fromUtf8("Session AI 重试已在后台启动。"), QString::fromUtf8("Session AI retry started in background."));
    updateAiStatusDisplay();
    updateActionState();
    statusBar()->showMessage(uiText(QString::fromUtf8("正在后台重试 Session 总结..."), QString::fromUtf8("Retrying session summary in background...")));
    m_pSessionSummaryWatcher->setFuture(QtConcurrent::run(RunAiTaskInBackground,
                                                          m_pAiProcessService,
                                                          executionContext));
}

void QCMainWindow::onViewSnippetSummary()
{
    if (currentSnippetSummaryText().isEmpty())
        return;

    focusSummaryEditor(m_pSnippetSummaryTextEdit);
    statusBar()->showMessage(uiText(QString::fromUtf8("已跳转至 Snippet 总结。"), QString::fromUtf8("Focused snippet summary.")), 2500);
}

void QCMainWindow::onCopySnippetSummary()
{
    const QString strSummary = currentSnippetSummaryText();
    if (strSummary.isEmpty())
        return;

    QGuiApplication::clipboard()->setText(strSummary);
    statusBar()->showMessage(uiText(QString::fromUtf8("Snippet 总结已复制。"), QString::fromUtf8("Snippet summary copied.")), 2500);
}

void QCMainWindow::onViewSessionSummary()
{
    if (currentSessionSummaryText().isEmpty())
        return;

    focusSummaryEditor(m_pSnippetSummaryTextEdit);
    statusBar()->showMessage(uiText(QString::fromUtf8("已跳转至 Session 总结。"), QString::fromUtf8("Focused session summary.")), 2500);
}

void QCMainWindow::onCopySessionSummary()
{
    const QString strSummary = currentSessionSummaryText();
    if (strSummary.isEmpty())
        return;

    QGuiApplication::clipboard()->setText(strSummary);
    statusBar()->showMessage(uiText(QString::fromUtf8("Session 总结已复制。"), QString::fromUtf8("Session summary copied.")), 2500);
}

void QCMainWindow::onAiSettings()
{
    if (nullptr == m_pSettingsService)
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("设置"), QString::fromUtf8("Settings")), uiText(QString::fromUtf8("设置服务不可用。"), QString::fromUtf8("Settings service is unavailable.")));
        return;
    }

    const QVector<QCAiRuntimeSettings> vecDefaultAiSettingsProfiles = m_pSettingsService->defaultAiSettingsProfiles();
    QVector<QCAiRuntimeSettings> vecAiSettingsProfiles;
    if (!m_pSettingsService->loadAiSettingsProfiles(&vecAiSettingsProfiles))
    {
        QMessageBox::warning(this, QString::fromUtf8("Settings"), m_pSettingsService->lastError());
        return;
    }

    int nActiveAiProfileIndex = m_pSettingsService->defaultAiProfileIndex();
    if (!m_pSettingsService->getActiveAiProfileIndex(&nActiveAiProfileIndex))
    {
        QMessageBox::warning(this, QString::fromUtf8("Settings"), m_pSettingsService->lastError());
        return;
    }

    QString strAppLanguage;
    if (!m_pSettingsService->getAppLanguage(&strAppLanguage))
    {
        QMessageBox::warning(this, QString::fromUtf8("Settings"), m_pSettingsService->lastError());
        return;
    }

    QString strScreenshotDirectory;
    if (!m_pSettingsService->getScreenshotSaveDirectory(&strScreenshotDirectory))
    {
        QMessageBox::warning(this, QString::fromUtf8("Settings"), m_pSettingsService->lastError());
        return;
    }

    QString strExportDirectory;
    if (!m_pSettingsService->getExportDirectory(&strExportDirectory))
    {
        QMessageBox::warning(this, QString::fromUtf8("Settings"), m_pSettingsService->lastError());
        return;
    }

    bool bDefaultCopyImportedImage = false;
    if (!m_pSettingsService->getDefaultCopyImportedImageToCaptureDirectory(&bDefaultCopyImportedImage))
    {
        QMessageBox::warning(this, QString::fromUtf8("Settings"), m_pSettingsService->lastError());
        return;
    }

    QCAiSettingsDialog dialog(m_pAiProcessService, this);
    dialog.setDefaultState(vecDefaultAiSettingsProfiles,
                           m_pSettingsService->defaultAiProfileIndex(),
                           m_pSettingsService->defaultAppLanguage(),
                           m_pSettingsService->defaultScreenshotSaveDirectory(),
                           m_pSettingsService->defaultExportDirectory(),
                           m_pSettingsService->defaultCopyImportedImageToCaptureDirectory());
    dialog.setDialogState(vecAiSettingsProfiles,
                          nActiveAiProfileIndex,
                          strAppLanguage,
                          strScreenshotDirectory,
                          strExportDirectory,
                          bDefaultCopyImportedImage);
    if (QDialog::Accepted != dialog.exec())
        return;

    if (!m_pSettingsService->saveAiSettingsProfiles(dialog.aiSettingsProfiles()))
    {
        QMessageBox::warning(this, QString::fromUtf8("Settings"), m_pSettingsService->lastError());
        return;
    }

    if (!m_pSettingsService->setActiveAiProfileIndex(dialog.activeAiProfileIndex()))
    {
        QMessageBox::warning(this, QString::fromUtf8("Settings"), m_pSettingsService->lastError());
        return;
    }

    if (!m_pSettingsService->setAppLanguage(dialog.appLanguage()))
    {
        QMessageBox::warning(this, QString::fromUtf8("Settings"), m_pSettingsService->lastError());
        return;
    }

    m_strAppLanguage = QCNormalizeUiLanguage(dialog.appLanguage());
    qApp->setProperty("qtclip.uiLanguage", m_strAppLanguage);
    applyLocalizedTexts();
    loadSessions();
    updateSelectionContextDisplay();
    updateActionState();

    if (!m_pSettingsService->setScreenshotSaveDirectory(dialog.screenshotSaveDirectory()))
    {
        QMessageBox::warning(this, QString::fromUtf8("Settings"), m_pSettingsService->lastError());
        return;
    }

    if (!m_pSettingsService->setExportDirectory(dialog.exportDirectory()))
    {
        QMessageBox::warning(this, QString::fromUtf8("Settings"), m_pSettingsService->lastError());
        return;
    }

    if (!m_pSettingsService->setDefaultCopyImportedImageToCaptureDirectory(dialog.defaultCopyImportedImageToCaptureDirectory()))
    {
        QMessageBox::warning(this, QString::fromUtf8("Settings"), m_pSettingsService->lastError());
        return;
    }

    statusBar()->showMessage(uiText(QString::fromUtf8("设置已保存。"), QString::fromUtf8("Settings saved.")), 4000);
}

void QCMainWindow::onExportMarkdown()
{
    flushSnippetDraftToStorage(false);
    const qint64 nSessionId = currentSessionId();
    if (nSessionId <= 0)
    {
        QMessageBox::information(this,
                                 uiText(QString::fromUtf8("Export Markdown"), QString::fromUtf8("Export Markdown")),
                                 uiText(QString::fromUtf8("Select a course first."), QString::fromUtf8("Select a course first.")));
        return;
    }

    if (nullptr == m_pMdExportService)
    {
        QMessageBox::warning(this,
                             uiText(QString::fromUtf8("Export Markdown"), QString::fromUtf8("Export Markdown")),
                             uiText(QString::fromUtf8("Export service is unavailable."), QString::fromUtf8("Export service is unavailable.")));
        return;
    }

    QCMdExportPreview exportPreview;
    if (!m_pMdExportService->buildExportPreview(nSessionId, &exportPreview))
    {
        QMessageBox::warning(this,
                             uiText(QString::fromUtf8("Export Markdown"), QString::fromUtf8("Export Markdown")),
                             uiText(QString::fromUtf8("Unable to prepare export:\n%1"), QString::fromUtf8("Unable to prepare export:\n%1")).arg(m_pMdExportService->lastError()));
        return;
    }

    QString strExportDirectory;
    if (nullptr != m_pSettingsService)
        m_pSettingsService->getExportDirectory(&strExportDirectory);

    QString strBaseName = exportPreview.m_strSessionTitle.trimmed();
    if (strBaseName.isEmpty())
        strBaseName = QString::fromUtf8("qtclip_export");
    strBaseName.replace(QRegularExpression(QString::fromUtf8("[^A-Za-z0-9_-]+")), QString::fromUtf8("_"));
    strBaseName.replace(QRegularExpression(QString::fromUtf8("_+")), QString::fromUtf8("_"));
    if (strBaseName.isEmpty())
        strBaseName = QString::fromUtf8("qtclip_export");

    QString strOutputFilePath = strExportDirectory.trimmed().isEmpty()
        ? QString()
        : QDir(strExportDirectory).filePath(strBaseName + QString::fromUtf8(".md"));

    if (strOutputFilePath.trimmed().isEmpty())
    {
        strOutputFilePath = QFileDialog::getSaveFileName(this,
                                                         uiText(QString::fromUtf8("Export Markdown"), QString::fromUtf8("Export Markdown")),
                                                         strBaseName + QString::fromUtf8(".md"),
                                                         QString::fromUtf8("Markdown (*.md)"));
        if (strOutputFilePath.isEmpty())
            return;
    }

    if (!m_pMdExportService->exportSessionToFile(nSessionId, strOutputFilePath))
    {
        QMessageBox::warning(this,
                             uiText(QString::fromUtf8("Export Markdown"), QString::fromUtf8("Export Markdown")),
                             uiText(QString::fromUtf8("Export failed:\n%1"), QString::fromUtf8("Export failed:\n%1")).arg(m_pMdExportService->lastError()));
        return;
    }

    statusBar()->showMessage(uiText(QString::fromUtf8("Exported: %1"), QString::fromUtf8("Exported: %1")).arg(strOutputFilePath), 5000);
}

void QCMainWindow::onFocusSearch()
{
    QLineEdit *pSearchLineEdit = (nullptr != m_pNavigationSearchLineEdit) ? m_pNavigationSearchLineEdit : m_pSnippetSearchLineEdit;
    if (nullptr == pSearchLineEdit)
        return;

    pSearchLineEdit->setFocus(Qt::ShortcutFocusReason);
    pSearchLineEdit->selectAll();
}

void QCMainWindow::onRefresh()
{
    loadSessions();
    statusBar()->showMessage(uiText(QString::fromUtf8("已刷新。"), QString::fromUtf8("Refreshed.")), 3000);
}





























































































