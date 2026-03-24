
// File: qcmainwindow.cpp
// Author: ZZMI1
// Created: 2026-03-23
// Description: Implements the minimal Qt Widgets main window used by the QtClip demo application.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include "qcmainwindow.h"

#include <QtConcurrent/QtConcurrent>

#include <QAction>
#include <QAbstractItemView>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QEventLoop>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QKeySequence>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
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
#include <QUrl>
#include <QTimer>
#include <QTextCursor>
#include <QTextDocument>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>

#include "../dialogs/qcaisettingsdialog.h"
#include "../dialogs/qccreateimagesnippetdialog.h"
#include "../dialogs/qccreatesessiondialog.h"
#include "../dialogs/qccreatetextsnippetdialog.h"
#include "../dialogs/qcquickcapturedialog.h"
#include "../dialogs/qcsnippettagdialog.h"
#include "../widgets/qcscreenshotoverlay.h"
#include "../../services/qcaiservice.h"
#include "../../services/qcmdexportservice.h"
#include "../../services/qcsessionservice.h"
#include "../../services/qcsettingsservice.h"
#include "../../services/qcsnippetservice.h"
#include "../../services/qctagservice.h"

namespace
{
QString SessionItemText(const QCStudySession& session)
{
    const QString strStatus = session.status() == QCSessionStatus::FinishedSessionStatus
        ? QString::fromUtf8("Finished")
        : QString::fromUtf8("Active");
    const QString strTitle = QString::fromUtf8("[%1] %2").arg(strStatus, session.title().trimmed().isEmpty()
        ? QString::fromUtf8("Untitled Session")
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
        ? QString::fromUtf8("Untitled Snippet")
        : snippet.title();

    QStringList vecStateTokens;
    if (snippet.isFavorite())
        vecStateTokens.append(QString::fromUtf8("Fav"));
    if (IsSnippetMarkedForReview(snippet))
        vecStateTokens.append(QString::fromUtf8("Review"));
    if (snippet.isArchived())
        vecStateTokens.append(QString::fromUtf8("Archived"));

    const QString strStatePrefix = vecStateTokens.isEmpty()
        ? QString()
        : QString::fromUtf8("[%1] ").arg(vecStateTokens.join(QString::fromUtf8("|")));
    return QString::fromUtf8("%1[%2] %3").arg(strStatePrefix, strTime, strTitle);
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
    , m_pTagLibraryAction(nullptr)
    , m_pSummarizeSnippetAction(nullptr)
    , m_pRetrySnippetSummaryAction(nullptr)
    , m_pSummarizeSessionAction(nullptr)
    , m_pRetrySessionSummaryAction(nullptr)
    , m_pAiSettingsAction(nullptr)
    , m_pExportMarkdownAction(nullptr)
    , m_pRefreshAction(nullptr)
    , m_pEditCurrentAction(nullptr)
    , m_pDeleteCurrentAction(nullptr)
    , m_pFocusSearchAction(nullptr)
    , m_pDeleteSnippetAction(nullptr)
    , m_pToggleArchiveSnippetAction(nullptr)
    , m_pDeleteSessionAction(nullptr)
    , m_pSessionListWidget(nullptr)
    , m_pSnippetListWidget(nullptr)
    , m_pSnippetSearchLineEdit(nullptr)
    , m_pSearchHistoryComboBox(nullptr)
    , m_pClearSearchButton(nullptr)
    , m_pResetFiltersButton(nullptr)
    , m_pQuickFavoriteCheckBox(nullptr)
    , m_pQuickReviewCheckBox(nullptr)
    , m_pFavoriteOnlyCheckBox(nullptr)
    , m_pReviewOnlyCheckBox(nullptr)
    , m_pSnippetTypeFilterComboBox(nullptr)
    , m_pShowArchivedCheckBox(nullptr)
    , m_pTagFilterComboBox(nullptr)
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
    , m_bHasRetryableSnippetSummary(false)
    , m_bHasRetryableSessionSummary(false)
    , m_nRetrySnippetId(0)
    , m_nRetrySessionId(0)
    , m_strAiStatusMessage(QString::fromUtf8("AI ???"))
    , m_strAppLanguage(QString::fromUtf8("zh-CN"))
    , m_pSnippetSummaryWatcher(new QFutureWatcher<QCAiTaskExecutionResult>(this))
    , m_pSessionSummaryWatcher(new QFutureWatcher<QCAiTaskExecutionResult>(this))
{
    loadAppLanguage();
    qApp->setProperty("qtclip.uiLanguage", m_strAppLanguage);
    setupUi();
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

void QCMainWindow::loadAppLanguage()
{
    m_strAppLanguage = QString::fromUtf8("zh-CN");
    if (nullptr != m_pSettingsService)
        m_pSettingsService->getAppLanguage(&m_strAppLanguage);
    if (m_strAppLanguage.trimmed().compare(QString::fromUtf8("en-US"), Qt::CaseInsensitive) == 0)
        m_strAppLanguage = QString::fromUtf8("en-US");
    else
        m_strAppLanguage = QString::fromUtf8("zh-CN");
}

bool QCMainWindow::isChineseUi() const
{
    return m_strAppLanguage.trimmed().compare(QString::fromUtf8("en-US"), Qt::CaseInsensitive) != 0;
}

QString QCMainWindow::uiText(const QString& strChinese, const QString& strEnglish) const
{
    return isChineseUi() ? strChinese : strEnglish;
}

void QCMainWindow::applyLocalizedTexts()
{
    updateMenuTexts();
    setWindowTitle(uiText(QString::fromUtf8("QtClip"), QString::fromUtf8("QtClip")));
    if (nullptr != m_pSnippetSearchLineEdit)
        m_pSnippetSearchLineEdit->setPlaceholderText(uiText(QString::fromUtf8("?????????????"), QString::fromUtf8("Search title, content, summary, note")));
    if (nullptr != m_pClearSearchButton)
        m_pClearSearchButton->setText(uiText(QString::fromUtf8("??"), QString::fromUtf8("Clear")));
    if (nullptr != m_pResetFiltersButton)
        m_pResetFiltersButton->setText(uiText(QString::fromUtf8("??"), QString::fromUtf8("Reset")));
    if (nullptr != m_pQuickFavoriteCheckBox)
        m_pQuickFavoriteCheckBox->setText(uiText(QString::fromUtf8("?????"), QString::fromUtf8("Selected Favorite")));
    if (nullptr != m_pQuickReviewCheckBox)
        m_pQuickReviewCheckBox->setText(uiText(QString::fromUtf8("?????"), QString::fromUtf8("Selected Review")));
    if (nullptr != m_pFavoriteOnlyCheckBox)
        m_pFavoriteOnlyCheckBox->setText(uiText(QString::fromUtf8("???"), QString::fromUtf8("Favorites Only")));
    if (nullptr != m_pReviewOnlyCheckBox)
        m_pReviewOnlyCheckBox->setText(uiText(QString::fromUtf8("???"), QString::fromUtf8("Review Only")));
    if (nullptr != m_pShowArchivedCheckBox)
        m_pShowArchivedCheckBox->setText(uiText(QString::fromUtf8("????"), QString::fromUtf8("Show Archived")));
    if (nullptr != m_pSnippetFavoriteCheckBox)
        m_pSnippetFavoriteCheckBox->setText(uiText(QString::fromUtf8("??"), QString::fromUtf8("Favorite")));
    if (nullptr != m_pSnippetReviewCheckBox)
        m_pSnippetReviewCheckBox->setText(uiText(QString::fromUtf8("??"), QString::fromUtf8("Review")));
    if (nullptr != m_pImagePreviewLabel && m_pImagePreviewLabel->pixmap() == nullptr)
        m_pImagePreviewLabel->setText(uiText(QString::fromUtf8("???????"), QString::fromUtf8("No image preview available.")));

    if (nullptr != m_pNewSessionAction) m_pNewSessionAction->setText(uiText(QString::fromUtf8("????"), QString::fromUtf8("New Session")));
    if (nullptr != m_pEditSessionAction) m_pEditSessionAction->setText(uiText(QString::fromUtf8("????"), QString::fromUtf8("Edit Session")));
    if (nullptr != m_pNewTextSnippetAction) m_pNewTextSnippetAction->setText(uiText(QString::fromUtf8("????"), QString::fromUtf8("New Text")));
    if (nullptr != m_pFinishSessionAction) m_pFinishSessionAction->setText(uiText(QString::fromUtf8("????"), QString::fromUtf8("Finish Session")));
    if (nullptr != m_pReopenSessionAction) m_pReopenSessionAction->setText(uiText(QString::fromUtf8("??????"), QString::fromUtf8("Reopen Session")));
    if (nullptr != m_pEditSnippetAction) m_pEditSnippetAction->setText(uiText(QString::fromUtf8("????"), QString::fromUtf8("Edit Snippet")));
    if (nullptr != m_pCaptureScreenAction) m_pCaptureScreenAction->setText(uiText(QString::fromUtf8("????"), QString::fromUtf8("Capture Screen")));
    if (nullptr != m_pCaptureRegionAction) m_pCaptureRegionAction->setText(uiText(QString::fromUtf8("????"), QString::fromUtf8("Capture Region")));
    if (nullptr != m_pImportImageAction) m_pImportImageAction->setText(uiText(QString::fromUtf8("????"), QString::fromUtf8("Import Image")));
    if (nullptr != m_pTagLibraryAction) m_pTagLibraryAction->setText(uiText(QString::fromUtf8("???"), QString::fromUtf8("Tag Library")));
    if (nullptr != m_pSummarizeSnippetAction) m_pSummarizeSnippetAction->setText(uiText(QString::fromUtf8("????"), QString::fromUtf8("Summarize Snippet")));
    if (nullptr != m_pRetrySnippetSummaryAction) m_pRetrySnippetSummaryAction->setText(uiText(QString::fromUtf8("???? AI"), QString::fromUtf8("Retry Snippet AI")));
    if (nullptr != m_pSummarizeSessionAction) m_pSummarizeSessionAction->setText(uiText(QString::fromUtf8("????"), QString::fromUtf8("Summarize Session")));
    if (nullptr != m_pRetrySessionSummaryAction) m_pRetrySessionSummaryAction->setText(uiText(QString::fromUtf8("???? AI"), QString::fromUtf8("Retry Session AI")));
    if (nullptr != m_pAiSettingsAction) m_pAiSettingsAction->setText(uiText(QString::fromUtf8("??"), QString::fromUtf8("Settings")));
    if (nullptr != m_pExportMarkdownAction) m_pExportMarkdownAction->setText(uiText(QString::fromUtf8("?? Markdown"), QString::fromUtf8("Export Markdown")));
    if (nullptr != m_pRefreshAction) m_pRefreshAction->setText(uiText(QString::fromUtf8("??"), QString::fromUtf8("Refresh")));
    if (nullptr != m_pFocusSearchAction) m_pFocusSearchAction->setText(uiText(QString::fromUtf8("????"), QString::fromUtf8("Focus Search")));

    if (nullptr != m_pSessionSummaryTextEdit && m_pSessionSummaryTextEdit->toPlainText().trimmed().isEmpty())
        m_pSessionSummaryTextEdit->setPlainText(uiText(QString::fromUtf8("???????"), QString::fromUtf8("No session summary available.")));
}


void QCMainWindow::updateMenuTexts()
{
    if (nullptr == menuBar())
        return;

    const QList<QAction *> vecMenuActions = menuBar()->actions();
    if (vecMenuActions.size() > 0 && nullptr != vecMenuActions.at(0))
        vecMenuActions.at(0)->setText(uiText(QString::fromUtf8("??"), QString::fromUtf8("Session")));
    if (vecMenuActions.size() > 1 && nullptr != vecMenuActions.at(1))
        vecMenuActions.at(1)->setText(uiText(QString::fromUtf8("??"), QString::fromUtf8("Snippet")));
    if (vecMenuActions.size() > 2 && nullptr != vecMenuActions.at(2))
        vecMenuActions.at(2)->setText(QString::fromUtf8("AI"));
    if (vecMenuActions.size() > 3 && nullptr != vecMenuActions.at(3))
        vecMenuActions.at(3)->setText(uiText(QString::fromUtf8("??"), QString::fromUtf8("Tools")));
}

void QCMainWindow::setupUi()
{
    setWindowTitle(uiText(QString::fromUtf8("QtClip"), QString::fromUtf8("QtClip")));
    resize(1280, 760);

    m_pSessionListWidget = new QListWidget(this);
    m_pSnippetListWidget = new QListWidget(this);
    m_pSnippetSearchLineEdit = new QLineEdit(this);
    m_pSearchHistoryComboBox = new QComboBox(this);
    m_pClearSearchButton = new QPushButton(QString::fromUtf8("Clear"), this);
    m_pResetFiltersButton = new QPushButton(QString::fromUtf8("Reset"), this);
    m_pQuickFavoriteCheckBox = new QCheckBox(QString::fromUtf8("Selected Favorite"), this);
    m_pQuickReviewCheckBox = new QCheckBox(QString::fromUtf8("Selected Review"), this);
    m_pFavoriteOnlyCheckBox = new QCheckBox(QString::fromUtf8("Favorites Only"), this);
    m_pReviewOnlyCheckBox = new QCheckBox(QString::fromUtf8("Review Only"), this);
    m_pSnippetTypeFilterComboBox = new QComboBox(this);
    m_pShowArchivedCheckBox = new QCheckBox(QString::fromUtf8("Show Archived"), this);
    m_pTagFilterComboBox = new QComboBox(this);
    m_pSelectionContextLabel = new QLabel(this);
    m_pViewSummaryLabel = new QLabel(this);
    m_pAiStatusLabel = new QLabel(this);
    m_pSessionSummaryTextEdit = new QPlainTextEdit(this);
    m_pSnippetTitleValueLabel = new QLabel(this);
    m_pSnippetTagsValueLabel = new QLabel(this);
    m_pSnippetFavoriteCheckBox = new QCheckBox(QString::fromUtf8("Favorite"), this);
    m_pSnippetReviewCheckBox = new QCheckBox(QString::fromUtf8("Review"), this);
    m_pSnippetNoteTextEdit = new QPlainTextEdit(this);
    m_pSnippetContentTextEdit = new QPlainTextEdit(this);
    m_pSnippetSummaryTextEdit = new QPlainTextEdit(this);
    m_pImagePathValueLabel = new QLabel(this);
    m_pImagePreviewLabel = new QLabel(this);

    m_pSessionListWidget->setAlternatingRowColors(true);
    m_pSnippetListWidget->setAlternatingRowColors(true);
    m_pSessionListWidget->setContextMenuPolicy(Qt::ActionsContextMenu);
    m_pSnippetListWidget->setContextMenuPolicy(Qt::ActionsContextMenu);
    m_pSnippetListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_pSnippetSearchLineEdit->setPlaceholderText(uiText(QString::fromUtf8("?????????????"), QString::fromUtf8("Search title, content, summary, note")));
    m_pSearchHistoryComboBox->setMinimumWidth(170);
    m_pSearchHistoryComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContentsOnFirstShow);
    m_pSearchHistoryComboBox->addItem(uiText(QString::fromUtf8("????"), QString::fromUtf8("Recent Searches")), QString());
    m_pSnippetTypeFilterComboBox->addItem(uiText(QString::fromUtf8("????"), QString::fromUtf8("All Types")), QString::fromUtf8("all"));
    m_pSnippetTypeFilterComboBox->addItem(uiText(QString::fromUtf8("??"), QString::fromUtf8("Text")), QString::fromUtf8("text"));
    m_pSnippetTypeFilterComboBox->addItem(uiText(QString::fromUtf8("??"), QString::fromUtf8("Image")), QString::fromUtf8("image"));
    m_pTagFilterComboBox->setMinimumWidth(140);
    m_pSelectionContextLabel->setWordWrap(true);
    m_pViewSummaryLabel->setWordWrap(true);
    m_pAiStatusLabel->setWordWrap(true);

    m_pSessionSummaryTextEdit->setReadOnly(true);
    m_pSnippetTitleValueLabel->setWordWrap(true);
    m_pSnippetTagsValueLabel->setWordWrap(true);
    m_pImagePathValueLabel->setWordWrap(true);
    m_pImagePreviewLabel->setMinimumSize(320, 220);
    m_pImagePreviewLabel->setAlignment(Qt::AlignCenter);
    m_pImagePreviewLabel->setFrameShape(QFrame::StyledPanel);
    m_pImagePreviewLabel->setText(uiText(QString::fromUtf8("???????"), QString::fromUtf8("No image preview available.")));

    m_pQuickFavoriteCheckBox->setEnabled(false);
    m_pQuickReviewCheckBox->setEnabled(false);
    m_pSnippetFavoriteCheckBox->setEnabled(false);
    m_pSnippetReviewCheckBox->setEnabled(false);
    m_pSnippetNoteTextEdit->setReadOnly(true);
    m_pSnippetContentTextEdit->setReadOnly(true);
    m_pSnippetSummaryTextEdit->setReadOnly(true);

    QWidget *pDetailWidget = new QWidget(this);
    QFormLayout *pDetailLayout = new QFormLayout();
    QWidget *pStateWidget = new QWidget(this);
    QHBoxLayout *pStateLayout = new QHBoxLayout();
    pStateLayout->addWidget(m_pSnippetFavoriteCheckBox);
    pStateLayout->addWidget(m_pSnippetReviewCheckBox);
    pStateLayout->addStretch(1);
    pStateLayout->setContentsMargins(0, 0, 0, 0);
    pStateWidget->setLayout(pStateLayout);
    pDetailLayout->addRow(uiText(QString::fromUtf8("????"), QString::fromUtf8("Session Summary")), m_pSessionSummaryTextEdit);
    pDetailLayout->addRow(uiText(QString::fromUtf8("??"), QString::fromUtf8("Title")), m_pSnippetTitleValueLabel);
    pDetailLayout->addRow(uiText(QString::fromUtf8("??"), QString::fromUtf8("Tags")), m_pSnippetTagsValueLabel);
    pDetailLayout->addRow(uiText(QString::fromUtf8("??"), QString::fromUtf8("State")), pStateWidget);
    pDetailLayout->addRow(uiText(QString::fromUtf8("??"), QString::fromUtf8("Note")), m_pSnippetNoteTextEdit);
    pDetailLayout->addRow(uiText(QString::fromUtf8("??"), QString::fromUtf8("Content")), m_pSnippetContentTextEdit);
    pDetailLayout->addRow(uiText(QString::fromUtf8("????"), QString::fromUtf8("Snippet Summary")), m_pSnippetSummaryTextEdit);
    pDetailLayout->addRow(uiText(QString::fromUtf8("????"), QString::fromUtf8("Image Path")), m_pImagePathValueLabel);
    pDetailLayout->addRow(uiText(QString::fromUtf8("??"), QString::fromUtf8("Preview")), m_pImagePreviewLabel);
    pDetailWidget->setLayout(pDetailLayout);

    QWidget *pSnippetPanelWidget = new QWidget(this);
    QVBoxLayout *pSnippetPanelLayout = new QVBoxLayout();
    QHBoxLayout *pFilterLayout = new QHBoxLayout();
    QHBoxLayout *pQuickStateLayout = new QHBoxLayout();
    pFilterLayout->addWidget(m_pSnippetSearchLineEdit, 1);
    pFilterLayout->addWidget(m_pSearchHistoryComboBox);
    pFilterLayout->addWidget(m_pClearSearchButton);
    pFilterLayout->addWidget(m_pResetFiltersButton);
    pFilterLayout->addWidget(m_pFavoriteOnlyCheckBox);
    pFilterLayout->addWidget(m_pReviewOnlyCheckBox);
    pFilterLayout->addWidget(new QLabel(uiText(QString::fromUtf8("??"), QString::fromUtf8("Type")), this));
    pFilterLayout->addWidget(m_pSnippetTypeFilterComboBox);
    pFilterLayout->addWidget(m_pShowArchivedCheckBox);
    pFilterLayout->addWidget(new QLabel(uiText(QString::fromUtf8("??"), QString::fromUtf8("Tag")), this));
    pFilterLayout->addWidget(m_pTagFilterComboBox);
    pQuickStateLayout->addWidget(new QLabel(uiText(QString::fromUtf8("???"), QString::fromUtf8("Selected")), this));
    pQuickStateLayout->addWidget(m_pQuickFavoriteCheckBox);
    pQuickStateLayout->addWidget(m_pQuickReviewCheckBox);
    pQuickStateLayout->addStretch(1);
    pSnippetPanelLayout->addLayout(pFilterLayout);
    pSnippetPanelLayout->addWidget(m_pSelectionContextLabel);
    pSnippetPanelLayout->addWidget(m_pViewSummaryLabel);
    pSnippetPanelLayout->addWidget(m_pAiStatusLabel);
    pSnippetPanelLayout->addLayout(pQuickStateLayout);
    pSnippetPanelLayout->addWidget(m_pSnippetListWidget, 1);
    pSnippetPanelLayout->setContentsMargins(0, 0, 0, 0);
    pSnippetPanelWidget->setLayout(pSnippetPanelLayout);

    QSplitter *pMainSplitter = new QSplitter(this);
    pMainSplitter->addWidget(m_pSessionListWidget);
    pMainSplitter->addWidget(pSnippetPanelWidget);
    pMainSplitter->addWidget(pDetailWidget);
    pMainSplitter->setStretchFactor(0, 0);
    pMainSplitter->setStretchFactor(1, 1);
    pMainSplitter->setStretchFactor(2, 2);
    pMainSplitter->setSizes(QList<int>() << 240 << 460 << 520);
    setCentralWidget(pMainSplitter);

    connect(m_pSessionListWidget, &QListWidget::itemSelectionChanged, this, [this]() { onSessionSelectionChanged(); });
    connect(m_pSnippetListWidget, &QListWidget::itemSelectionChanged, this, [this]() { onSnippetSelectionChanged(); });
    connect(m_pSnippetSearchLineEdit, &QLineEdit::returnPressed, this, [this]() { onSnippetFilterChanged(); });
    connect(m_pSearchHistoryComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) { onSearchHistoryChanged(); });
    connect(m_pClearSearchButton, &QPushButton::clicked, this, [this]() { onClearSearch(); });
    connect(m_pResetFiltersButton, &QPushButton::clicked, this, [this]() { onResetFilters(); });
    connect(m_pFavoriteOnlyCheckBox, &QCheckBox::toggled, this, [this](bool) { onSnippetFilterChanged(); });
    connect(m_pReviewOnlyCheckBox, &QCheckBox::toggled, this, [this](bool) { onSnippetFilterChanged(); });
    connect(m_pSnippetTypeFilterComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) { onSnippetFilterChanged(); });
    connect(m_pShowArchivedCheckBox, &QCheckBox::toggled, this, [this](bool) { onSnippetFilterChanged(); });
    connect(m_pTagFilterComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) { onSnippetFilterChanged(); });
    connect(m_pQuickFavoriteCheckBox, &QCheckBox::toggled, this, [this](bool bChecked) { onFavoriteToggled(bChecked); });
    connect(m_pQuickReviewCheckBox, &QCheckBox::toggled, this, [this](bool bChecked) { onReviewToggled(bChecked); });
    connect(m_pSnippetFavoriteCheckBox, &QCheckBox::toggled, this, [this](bool bChecked) { onFavoriteToggled(bChecked); });
    connect(m_pSnippetReviewCheckBox, &QCheckBox::toggled, this, [this](bool bChecked) { onReviewToggled(bChecked); });

    m_pSessionListWidget->setObjectName(QString::fromUtf8("sessionListWidget"));
    m_pSnippetSearchLineEdit->setObjectName(QString::fromUtf8("snippetSearchLineEdit"));
    m_pSearchHistoryComboBox->setObjectName(QString::fromUtf8("searchHistoryComboBox"));
    m_pClearSearchButton->setObjectName(QString::fromUtf8("clearSearchButton"));
    m_pResetFiltersButton->setObjectName(QString::fromUtf8("resetFiltersButton"));
    m_pQuickFavoriteCheckBox->setObjectName(QString::fromUtf8("quickFavoriteCheckBox"));
    m_pQuickReviewCheckBox->setObjectName(QString::fromUtf8("quickReviewCheckBox"));
    m_pFavoriteOnlyCheckBox->setObjectName(QString::fromUtf8("favoriteOnlyCheckBox"));
    m_pReviewOnlyCheckBox->setObjectName(QString::fromUtf8("reviewOnlyCheckBox"));
    m_pSnippetTypeFilterComboBox->setObjectName(QString::fromUtf8("snippetTypeFilterComboBox"));
    m_pTagFilterComboBox->setObjectName(QString::fromUtf8("tagFilterComboBox"));
    m_pSnippetFavoriteCheckBox->setObjectName(QString::fromUtf8("snippetFavoriteCheckBox"));
    m_pSnippetReviewCheckBox->setObjectName(QString::fromUtf8("snippetReviewCheckBox"));
    m_pSnippetListWidget->setObjectName(QString::fromUtf8("snippetListWidget"));
    m_pShowArchivedCheckBox->setObjectName(QString::fromUtf8("showArchivedCheckBox"));
    m_pSelectionContextLabel->setObjectName(QString::fromUtf8("selectionContextLabel"));
    m_pViewSummaryLabel->setObjectName(QString::fromUtf8("viewSummaryLabel"));
    m_pAiStatusLabel->setObjectName(QString::fromUtf8("aiStatusLabel"));

    statusBar()->showMessage(uiText(QString::fromUtf8("???"), QString::fromUtf8("Ready.")));
    clearSnippetDetails();
    m_pSessionSummaryTextEdit->setPlainText(uiText(QString::fromUtf8("???????"), QString::fromUtf8("No session summary available.")));
    loadTagFilterOptions();
    loadSearchHistoryOptions();
    applyLocalizedTexts();
    updateSelectionContextDisplay();
    updateAiStatusDisplay();
}
void QCMainWindow::setupActions()
{
    QToolBar *pToolBar = addToolBar(QString::fromUtf8("Main"));
    pToolBar->setMovable(false);

    m_pNewSessionAction = pToolBar->addAction(QString::fromUtf8("New Session"));
    m_pEditSessionAction = pToolBar->addAction(QString::fromUtf8("Edit Session"));
    m_pNewTextSnippetAction = pToolBar->addAction(QString::fromUtf8("New Text"));
    m_pFinishSessionAction = pToolBar->addAction(QString::fromUtf8("Finish Session"));
    m_pReopenSessionAction = pToolBar->addAction(QString::fromUtf8("Reopen Session"));
    m_pEditSnippetAction = pToolBar->addAction(QString::fromUtf8("Edit Snippet"));
    m_pCaptureScreenAction = pToolBar->addAction(QString::fromUtf8("Capture Screen"));
    m_pCaptureRegionAction = pToolBar->addAction(QString::fromUtf8("Capture Region"));
    m_pImportImageAction = pToolBar->addAction(QString::fromUtf8("Import Image"));
    m_pDuplicateSnippetAction = pToolBar->addAction(uiText(QString::fromUtf8("????"), QString::fromUtf8("Duplicate Snippet")));
    m_pMoveSnippetAction = pToolBar->addAction(uiText(QString::fromUtf8("????"), QString::fromUtf8("Move Snippet")));
    m_pManageTagsAction = pToolBar->addAction(uiText(QString::fromUtf8("????"), QString::fromUtf8("Snippet Tags")));
    m_pTagLibraryAction = pToolBar->addAction(uiText(QString::fromUtf8("???"), QString::fromUtf8("Tag Library")));
    m_pDeleteSnippetAction = pToolBar->addAction(uiText(QString::fromUtf8("????"), QString::fromUtf8("Delete Snippet")));
    m_pToggleArchiveSnippetAction = pToolBar->addAction(uiText(QString::fromUtf8("?? / ??"), QString::fromUtf8("Archive / Restore")));
    m_pDeleteSessionAction = pToolBar->addAction(uiText(QString::fromUtf8("????"), QString::fromUtf8("Delete Session")));
    m_pSummarizeSnippetAction = pToolBar->addAction(uiText(QString::fromUtf8("????"), QString::fromUtf8("Summarize Snippet")));
    m_pRetrySnippetSummaryAction = pToolBar->addAction(uiText(QString::fromUtf8("???? AI"), QString::fromUtf8("Retry Snippet AI")));
    m_pSummarizeSessionAction = pToolBar->addAction(uiText(QString::fromUtf8("????"), QString::fromUtf8("Summarize Session")));
    m_pRetrySessionSummaryAction = pToolBar->addAction(uiText(QString::fromUtf8("???? AI"), QString::fromUtf8("Retry Session AI")));
    m_pAiSettingsAction = pToolBar->addAction(uiText(QString::fromUtf8("??"), QString::fromUtf8("Settings")));
    m_pExportMarkdownAction = pToolBar->addAction(uiText(QString::fromUtf8("?? Markdown"), QString::fromUtf8("Export Markdown")));
    m_pRefreshAction = pToolBar->addAction(uiText(QString::fromUtf8("??"), QString::fromUtf8("Refresh")));

    m_pEditCurrentAction = new QAction(uiText(QString::fromUtf8("??????"), QString::fromUtf8("Edit Current")), this);
    m_pDeleteCurrentAction = new QAction(uiText(QString::fromUtf8("??????"), QString::fromUtf8("Delete Current")), this);
    m_pFocusSearchAction = new QAction(uiText(QString::fromUtf8("????"), QString::fromUtf8("Focus Search")), this);

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
    m_pManageTagsAction->setShortcut(QKeySequence(QString::fromUtf8("Ctrl+T")));
    m_pTagLibraryAction->setShortcut(QKeySequence(QString::fromUtf8("Ctrl+Shift+T")));
    m_pSummarizeSnippetAction->setShortcut(QKeySequence(QString::fromUtf8("Ctrl+Shift+M")));
    m_pRetrySnippetSummaryAction->setShortcut(QKeySequence(QString::fromUtf8("Ctrl+Shift+Y")));
    m_pSummarizeSessionAction->setShortcut(QKeySequence(QString::fromUtf8("Ctrl+Alt+M")));
    m_pRetrySessionSummaryAction->setShortcut(QKeySequence(QString::fromUtf8("Ctrl+Alt+Y")));
    m_pAiSettingsAction->setShortcut(QKeySequence(QString::fromUtf8("Ctrl+,")));
    m_pExportMarkdownAction->setShortcut(QKeySequence(QString::fromUtf8("Ctrl+Shift+E")));
    m_pRefreshAction->setShortcuts(QList<QKeySequence>() << QKeySequence::Refresh << QKeySequence(QString::fromUtf8("Ctrl+R")));
    m_pEditCurrentAction->setShortcut(QKeySequence(Qt::Key_F2));
    m_pDeleteCurrentAction->setShortcut(QKeySequence::Delete);
    m_pFocusSearchAction->setShortcuts(QList<QKeySequence>() << QKeySequence::Find << QKeySequence(QString::fromUtf8("/")));

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
        << m_pTagLibraryAction
        << m_pDeleteSnippetAction
        << m_pToggleArchiveSnippetAction
        << m_pDeleteSessionAction
        << m_pSummarizeSnippetAction
        << m_pRetrySnippetSummaryAction
        << m_pSummarizeSessionAction
        << m_pRetrySessionSummaryAction
        << m_pAiSettingsAction
        << m_pExportMarkdownAction
        << m_pRefreshAction
        << m_pEditCurrentAction
        << m_pDeleteCurrentAction
        << m_pFocusSearchAction;
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
    connect(m_pTagLibraryAction, &QAction::triggered, this, [this]() { onOpenTagLibrary(); });
    connect(m_pDeleteSnippetAction, &QAction::triggered, this, [this]() { onDeleteSnippet(); });
    connect(m_pToggleArchiveSnippetAction, &QAction::triggered, this, [this]() { onToggleArchiveSnippet(); });
    connect(m_pDeleteSessionAction, &QAction::triggered, this, [this]() { onDeleteSession(); });
    connect(m_pSummarizeSnippetAction, &QAction::triggered, this, [this]() { onSummarizeSnippet(); });
    connect(m_pRetrySnippetSummaryAction, &QAction::triggered, this, [this]() { onRetrySnippetSummary(); });
    connect(m_pSummarizeSessionAction, &QAction::triggered, this, [this]() { onSummarizeSession(); });
    connect(m_pRetrySessionSummaryAction, &QAction::triggered, this, [this]() { onRetrySessionSummary(); });
    connect(m_pAiSettingsAction, &QAction::triggered, this, [this]() { onAiSettings(); });
    connect(m_pExportMarkdownAction, &QAction::triggered, this, [this]() { onExportMarkdown(); });
    connect(m_pRefreshAction, &QAction::triggered, this, [this]() { onRefresh(); });
    connect(m_pEditCurrentAction, &QAction::triggered, this, [this]() { onEditCurrentItem(); });
    connect(m_pDeleteCurrentAction, &QAction::triggered, this, [this]() { onDeleteCurrentItem(); });
    connect(m_pFocusSearchAction, &QAction::triggered, this, [this]() { onFocusSearch(); });

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
    m_pTagLibraryAction->setObjectName(QString::fromUtf8("tagLibraryAction"));
    m_pSummarizeSnippetAction->setObjectName(QString::fromUtf8("summarizeSnippetAction"));
    m_pRetrySnippetSummaryAction->setObjectName(QString::fromUtf8("retrySnippetSummaryAction"));
    m_pSummarizeSessionAction->setObjectName(QString::fromUtf8("summarizeSessionAction"));
    m_pRetrySessionSummaryAction->setObjectName(QString::fromUtf8("retrySessionSummaryAction"));
    m_pAiSettingsAction->setObjectName(QString::fromUtf8("aiSettingsAction"));
    m_pExportMarkdownAction->setObjectName(QString::fromUtf8("exportMarkdownAction"));
    m_pRefreshAction->setObjectName(QString::fromUtf8("refreshAction"));
    m_pEditCurrentAction->setObjectName(QString::fromUtf8("editCurrentAction"));
    m_pDeleteCurrentAction->setObjectName(QString::fromUtf8("deleteCurrentAction"));
    m_pFocusSearchAction->setObjectName(QString::fromUtf8("focusSearchAction"));

    QMenu *pSessionMenu = menuBar()->addMenu(uiText(QString::fromUtf8("??"), QString::fromUtf8("Session")));
    pSessionMenu->addAction(m_pNewSessionAction);
    pSessionMenu->addAction(m_pEditSessionAction);
    pSessionMenu->addAction(m_pFinishSessionAction);
    pSessionMenu->addAction(m_pReopenSessionAction);
    pSessionMenu->addSeparator();
    pSessionMenu->addAction(m_pSummarizeSessionAction);
    pSessionMenu->addAction(m_pExportMarkdownAction);
    pSessionMenu->addSeparator();
    pSessionMenu->addAction(m_pDeleteSessionAction);

    QMenu *pSnippetMenu = menuBar()->addMenu(uiText(QString::fromUtf8("??"), QString::fromUtf8("Snippet")));
    pSnippetMenu->addAction(m_pNewTextSnippetAction);
    pSnippetMenu->addAction(m_pCaptureScreenAction);
    pSnippetMenu->addAction(m_pCaptureRegionAction);
    pSnippetMenu->addAction(m_pImportImageAction);
    pSnippetMenu->addSeparator();
    pSnippetMenu->addAction(m_pEditSnippetAction);
    pSnippetMenu->addAction(m_pDuplicateSnippetAction);
    pSnippetMenu->addAction(m_pMoveSnippetAction);
    pSnippetMenu->addAction(m_pManageTagsAction);
    pSnippetMenu->addAction(m_pToggleArchiveSnippetAction);
    pSnippetMenu->addAction(m_pDeleteSnippetAction);

    QMenu *pAiMenu = menuBar()->addMenu(QString::fromUtf8("AI"));
    pAiMenu->addAction(m_pSummarizeSnippetAction);
    pAiMenu->addAction(m_pRetrySnippetSummaryAction);
    pAiMenu->addAction(m_pSummarizeSessionAction);
    pAiMenu->addAction(m_pRetrySessionSummaryAction);
    pAiMenu->addSeparator();
    pAiMenu->addAction(m_pAiSettingsAction);

    QMenu *pToolsMenu = menuBar()->addMenu(uiText(QString::fromUtf8("??"), QString::fromUtf8("Tools")));
    pToolsMenu->addAction(m_pTagLibraryAction);
    pToolsMenu->addAction(m_pRefreshAction);
    pToolsMenu->addAction(m_pFocusSearchAction);

    if (nullptr != m_pSessionListWidget)
    {
        m_pSessionListWidget->addAction(m_pEditSessionAction);
        m_pSessionListWidget->addAction(m_pFinishSessionAction);
        m_pSessionListWidget->addAction(m_pReopenSessionAction);
        m_pSessionListWidget->addAction(m_pSummarizeSessionAction);
        m_pSessionListWidget->addAction(m_pExportMarkdownAction);
        m_pSessionListWidget->addAction(m_pDeleteSessionAction);
    }

    if (nullptr != m_pSnippetListWidget)
    {
        m_pSnippetListWidget->addAction(m_pEditSnippetAction);
        m_pSnippetListWidget->addAction(m_pDuplicateSnippetAction);
        m_pSnippetListWidget->addAction(m_pMoveSnippetAction);
        m_pSnippetListWidget->addAction(m_pManageTagsAction);
        m_pSnippetListWidget->addAction(m_pSummarizeSnippetAction);
        m_pSnippetListWidget->addAction(m_pToggleArchiveSnippetAction);
        m_pSnippetListWidget->addAction(m_pDeleteSnippetAction);
    }

    updateActionState();
}
void QCMainWindow::loadSessions()
{
    const qint64 nPreviousSessionId = currentSessionId();
    QSignalBlocker sessionBlocker(m_pSessionListWidget);
    m_pSessionListWidget->clear();
    loadTagFilterOptions();

    const QVector<QCStudySession> vecSessions = m_pSessionService->listSessions();
    if (!m_pSessionService->lastError().isEmpty())
    {
        QMessageBox::warning(this, QString::fromUtf8("Load Sessions"), m_pSessionService->lastError());
        return;
    }

    for (int i = 0; i < vecSessions.size(); ++i)
    {
        const QCStudySession& session = vecSessions.at(i);
        QListWidgetItem *pItem = new QListWidgetItem(SessionItemText(session), m_pSessionListWidget);
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
    m_pSessionSummaryTextEdit->setPlainText(uiText(QString::fromUtf8("???????"), QString::fromUtf8("No session summary available.")));
    updateViewSummary(0, 0);
    updateActionState();
}

void QCMainWindow::loadTagFilterOptions()
{
    if (nullptr == m_pTagFilterComboBox)
        return;

    const qint64 nCurrentTagId = currentTagFilterId();
    m_pTagFilterComboBox->blockSignals(true);
    m_pTagFilterComboBox->clear();
    m_pTagFilterComboBox->addItem(QString::fromUtf8("All Tags"), 0);

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
    const qint64 nSessionId = currentSessionId();
    const QVector<qint64> vecPreviousSnippetIds = selectedSnippetIds();

    QSignalBlocker snippetBlocker(m_pSnippetListWidget);
    m_pSnippetListWidget->clear();
    clearSnippetDetails();

    if (nSessionId <= 0)
    {
        updateViewSummary(0, 0);
        updateActionState();
        return;
    }

    const QVector<QCSnippet> vecAllSnippets = m_pSnippetService->listSnippetsBySession(nSessionId);
    if (!m_pSnippetService->lastError().isEmpty())
    {
        QMessageBox::warning(this, QString::fromUtf8("Load Snippets"), m_pSnippetService->lastError());
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
        QMessageBox::warning(this, QString::fromUtf8("Load Snippets"), m_pSnippetService->lastError());
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
    m_pSessionSummaryTextEdit->setPlainText(uiText(QString::fromUtf8("???????"), QString::fromUtf8("No session summary available.")));

    if (nSessionId <= 0)
        return;

    const QVector<QCAiRecord> vecAiRecords = m_pAiService->listAiRecordsBySession(nSessionId);
    if (!m_pAiService->lastError().isEmpty())
        return;

    const QString strSessionSummary = findSessionSummaryFromAiRecords(vecAiRecords);
    if (!strSessionSummary.trimmed().isEmpty())
        m_pSessionSummaryTextEdit->setPlainText(strSessionSummary);
}
void QCMainWindow::showSnippetDetails(qint64 nSnippetId)
{
    clearSnippetDetails();

    if (nSnippetId <= 0)
        return;

    QCSnippet snippet;
    if (!m_pSnippetService->getSnippetById(nSnippetId, &snippet))
    {
        QMessageBox::warning(this, QString::fromUtf8("Load Snippet"), m_pSnippetService->lastError());
        return;
    }

    const QString strSearchText = currentSearchText();
    m_pSnippetTitleValueLabel->setTextFormat(Qt::RichText);
    m_pSnippetTitleValueLabel->setText(buildHighlightedHtml(snippet.title().trimmed().isEmpty()
        ? QString::fromUtf8("Untitled Snippet")
        : snippet.title(),
        strSearchText));
    m_pSnippetNoteTextEdit->setPlainText(snippet.note());
    m_pSnippetContentTextEdit->setPlainText(snippet.contentText());

    refreshSnippetTagsDisplay(nSnippetId);

    QString strSummary = snippet.summary();
    const QVector<QCAiRecord> vecAiRecords = m_pAiService->listAiRecordsBySnippet(nSnippetId);
    if (m_pAiService->lastError().isEmpty() && strSummary.trimmed().isEmpty())
        strSummary = findSummaryFromAiRecords(vecAiRecords);
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
            m_pImagePathValueLabel->setText(QString::fromUtf8("Image attachment unavailable."));
            m_pImagePreviewLabel->setText(uiText(QString::fromUtf8("???????"), QString::fromUtf8("No image preview available.")));
        }
    }
    else
    {
        m_pImagePathValueLabel->setText(QString::fromUtf8("No image attachment."));
        m_pImagePreviewLabel->setText(uiText(QString::fromUtf8("???????"), QString::fromUtf8("No image preview available.")));
    }

    updateActionState();
}

void QCMainWindow::clearSnippetDetails()
{
    m_pSnippetTitleValueLabel->setTextFormat(Qt::PlainText);
    m_pSnippetTitleValueLabel->clear();
    m_pSnippetTagsValueLabel->setText(QString::fromUtf8("No tags."));
    m_pSnippetNoteTextEdit->clear();
    m_pSnippetContentTextEdit->clear();
    m_pSnippetSummaryTextEdit->clear();
    applyPlainTextHighlight(m_pSnippetNoteTextEdit, QString());
    applyPlainTextHighlight(m_pSnippetContentTextEdit, QString());
    applyPlainTextHighlight(m_pSnippetSummaryTextEdit, QString());
    m_pImagePathValueLabel->clear();
    m_pImagePreviewLabel->clear();
    m_pImagePreviewLabel->setPixmap(QPixmap());
    m_pImagePreviewLabel->setText(uiText(QString::fromUtf8("???????"), QString::fromUtf8("No image preview available.")));

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
    m_pImagePreviewLabel->clear();
    m_pImagePreviewLabel->setPixmap(QPixmap());
    m_pImagePathValueLabel->setText(strImagePath);

    QPixmap pixmapImage(strImagePath);
    if (pixmapImage.isNull())
    {
        m_pImagePreviewLabel->setText(QString::fromUtf8("Image preview unavailable."));
        return;
    }

    m_pImagePreviewLabel->setPixmap(pixmapImage.scaled(320,
                                                       220,
                                                       Qt::KeepAspectRatio,
                                                       Qt::SmoothTransformation));
}

void QCMainWindow::updateAiStatusDisplay()
{
    if (nullptr == m_pAiStatusLabel)
        return;

    QString strStatusMessage = m_strAiStatusMessage.trimmed();
    if (strStatusMessage.isEmpty())
        strStatusMessage = uiText(QString::fromUtf8("AI ???"), QString::fromUtf8("AI idle."));

    if (m_bSnippetSummaryRunning)
        strStatusMessage = QString::fromUtf8("Snippet AI running. The current summary will update when the background task finishes.");
    else if (m_bSessionSummaryRunning)
        strStatusMessage = QString::fromUtf8("Session AI running. The current session summary will update when the background task finishes.");

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
    const int nSelectedSnippetCount = vecSnippetIds.size();

    if (!bHasSession)
        return uiText(QString::fromUtf8("????????????"), QString::fromUtf8("Current context: no session selected."));

    const QString strSessionTitle = session.title().trimmed().isEmpty()
        ? QString::fromUtf8("Untitled Session")
        : session.title().trimmed();
    const QString strSessionStatus = session.status() == QCSessionStatus::FinishedSessionStatus
        ? QString::fromUtf8("Finished")
        : QString::fromUtf8("Active");

    if (nSelectedSnippetCount <= 0)
    {
        return uiText(QString::fromUtf8("?????????%1? [%2]?????????????? AI?"), QString::fromUtf8("Current context: session '%1' [%2]. Session actions, export, and session AI are available."))
            .arg(strSessionTitle, strSessionStatus);
    }

    if (1 == nSelectedSnippetCount)
    {
        return uiText(QString::fromUtf8("?????????%1? [%2] ???? 1 ???????????????????"), QString::fromUtf8("Current context: 1 selected snippet in '%1' [%2]. Single-item actions and session actions are available."))
            .arg(strSessionTitle, strSessionStatus);
    }

    return uiText(QString::fromUtf8("?????????%2? [%3] ???? %1 ????????????????????????????"), QString::fromUtf8("Current context: %1 selected snippets in '%2' [%3]. Batch organize actions are available; edit and summarize stay single-snippet actions."))
        .arg(nSelectedSnippetCount)
        .arg(strSessionTitle)
        .arg(strSessionStatus);
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
            ? uiText(QString::fromUtf8("?? %1 ???"), QString::fromUtf8("Duplicate %1 Snippets")).arg(nSelectedSnippetCount)
            : uiText(QString::fromUtf8("????"), QString::fromUtf8("Duplicate Snippet")));
    }
    if (nullptr != m_pMoveSnippetAction)
    {
        m_pMoveSnippetAction->setEnabled(bCanOrganizeSnippets);
        m_pMoveSnippetAction->setText(nSelectedSnippetCount > 1
            ? uiText(QString::fromUtf8("?? %1 ???"), QString::fromUtf8("Move %1 Snippets")).arg(nSelectedSnippetCount)
            : uiText(QString::fromUtf8("????"), QString::fromUtf8("Move Snippet")));
    }
    if (nullptr != m_pToggleArchiveSnippetAction)
    {
        m_pToggleArchiveSnippetAction->setEnabled(bCanOrganizeSnippets);
        if (nSelectedSnippetCount > 1)
        {
            m_pToggleArchiveSnippetAction->setText(bAllSelectedArchived
                ? uiText(QString::fromUtf8("?? %1 ???"), QString::fromUtf8("Restore %1 Snippets")).arg(nSelectedSnippetCount)
                : uiText(QString::fromUtf8("?? %1 ???"), QString::fromUtf8("Archive %1 Snippets")).arg(nSelectedSnippetCount));
        }
        else
        {
            m_pToggleArchiveSnippetAction->setText((bHasSnippet && snippet.isArchived())
                ? uiText(QString::fromUtf8("????"), QString::fromUtf8("Restore Snippet"))
                : uiText(QString::fromUtf8("????"), QString::fromUtf8("Archive Snippet")));
        }
    }
    if (nullptr != m_pDeleteSnippetAction)
    {
        m_pDeleteSnippetAction->setEnabled(bCanOrganizeSnippets);
        m_pDeleteSnippetAction->setText(nSelectedSnippetCount > 1
            ? uiText(QString::fromUtf8("?? %1 ???"), QString::fromUtf8("Delete %1 Snippets")).arg(nSelectedSnippetCount)
            : uiText(QString::fromUtf8("????"), QString::fromUtf8("Delete Snippet")));
    }
    if (nullptr != m_pManageTagsAction)
    {
        m_pManageTagsAction->setEnabled(bCanManageTags);
        m_pManageTagsAction->setText(nSelectedSnippetCount > 1
            ? uiText(QString::fromUtf8("? %1 ???????"), QString::fromUtf8("Tag %1 Snippets")).arg(nSelectedSnippetCount)
            : uiText(QString::fromUtf8("????"), QString::fromUtf8("Snippet Tags")));
    }
    if (nullptr != m_pTagLibraryAction)
        m_pTagLibraryAction->setEnabled(nullptr != m_pTagService && !bAnyAiTaskRunning);
    if (nullptr != m_pSummarizeSnippetAction)
        m_pSummarizeSnippetAction->setEnabled(bCanSummarizeSnippet);
    if (nullptr != m_pRetrySnippetSummaryAction)
        m_pRetrySnippetSummaryAction->setEnabled(m_bHasRetryableSnippetSummary && !bAnyAiTaskRunning);
    if (nullptr != m_pSummarizeSessionAction)
        m_pSummarizeSessionAction->setEnabled(bHasSession && !bAnyAiTaskRunning);
    if (nullptr != m_pRetrySessionSummaryAction)
        m_pRetrySessionSummaryAction->setEnabled(m_bHasRetryableSessionSummary && !bAnyAiTaskRunning);
    if (nullptr != m_pExportMarkdownAction)
        m_pExportMarkdownAction->setEnabled(bHasSession && !bAnyAiTaskRunning);
    if (nullptr != m_pEditCurrentAction)
    {
        m_pEditCurrentAction->setEnabled(bCanEditSnippet || (nSelectedSnippetCount == 0 && bCanEditSession));
        m_pEditCurrentAction->setText(bCanEditSnippet
            ? uiText(QString::fromUtf8("??????"), QString::fromUtf8("Edit Selected Snippet"))
            : ((nSelectedSnippetCount == 0 && bCanEditSession)
                ? uiText(QString::fromUtf8("??????"), QString::fromUtf8("Edit Current Session"))
                : uiText(QString::fromUtf8("??????"), QString::fromUtf8("Edit Current"))));
    }
    if (nullptr != m_pDeleteCurrentAction)
    {
        m_pDeleteCurrentAction->setEnabled(bCanOrganizeSnippets || (nSelectedSnippetCount == 0 && bCanDeleteSession));
        if (nSelectedSnippetCount > 1)
            m_pDeleteCurrentAction->setText(uiText(QString::fromUtf8("?? %1 ???"), QString::fromUtf8("Delete %1 Snippets")).arg(nSelectedSnippetCount));
        else if (nSelectedSnippetCount == 1)
            m_pDeleteCurrentAction->setText(uiText(QString::fromUtf8("??????"), QString::fromUtf8("Delete Selected Snippet")));
        else if (bCanDeleteSession)
            m_pDeleteCurrentAction->setText(uiText(QString::fromUtf8("??????"), QString::fromUtf8("Delete Current Session")));
        else
            m_pDeleteCurrentAction->setText(uiText(QString::fromUtf8("??????"), QString::fromUtf8("Delete Current")));
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
        m_pViewSummaryLabel->setText(QString::fromUtf8("No session selected."));
        return;
    }

    QStringList vecTokens;
    vecTokens.append(session.status() == QCSessionStatus::FinishedSessionStatus
        ? QString::fromUtf8("Finished")
        : QString::fromUtf8("Active"));
    vecTokens.append(QString::fromUtf8("Visible %1/%2 snippets").arg(nVisibleSnippetCount).arg(nTotalSnippetCount));
    if (nullptr != m_pFavoriteOnlyCheckBox && m_pFavoriteOnlyCheckBox->isChecked())
        vecTokens.append(QString::fromUtf8("Favorites"));
    if (nullptr != m_pReviewOnlyCheckBox && m_pReviewOnlyCheckBox->isChecked())
        vecTokens.append(QString::fromUtf8("Review"));
    if (nullptr != m_pSnippetTypeFilterComboBox && currentSnippetTypeFilter() != QString::fromUtf8("all"))
        vecTokens.append(QString::fromUtf8("Type: %1").arg(m_pSnippetTypeFilterComboBox->currentText()));
    if (nullptr != m_pShowArchivedCheckBox && m_pShowArchivedCheckBox->isChecked())
        vecTokens.append(QString::fromUtf8("Archived Visible"));
    if (currentTagFilterId() > 0 && nullptr != m_pTagFilterComboBox)
        vecTokens.append(QString::fromUtf8("Tag: %1").arg(m_pTagFilterComboBox->currentText()));
    if (!currentSearchText().isEmpty())
        vecTokens.append(QString::fromUtf8("Search: %1").arg(currentSearchText()));
    if (nullptr != m_pSearchHistoryComboBox && m_pSearchHistoryComboBox->count() > 1)
        vecTokens.append(QString::fromUtf8("History: %1").arg(m_pSearchHistoryComboBox->count() - 1));
    if (selectedSnippetCount() > 0)
        vecTokens.append(QString::fromUtf8("Selected: %1").arg(selectedSnippetCount()));

    m_pViewSummaryLabel->setText(QString::fromUtf8("%1 | %2")
        .arg(session.title().trimmed().isEmpty() ? QString::fromUtf8("Untitled Session") : session.title().trimmed(),
             vecTokens.join(QString::fromUtf8(" | "))));
}

QString QCMainWindow::currentSearchText() const
{
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
    m_pSearchHistoryComboBox->addItem(uiText(QString::fromUtf8("????"), QString::fromUtf8("Recent Searches")), QString());
    for (int i = 0; i < vecSearchHistory.size(); ++i)
        m_pSearchHistoryComboBox->addItem(vecSearchHistory.at(i), vecSearchHistory.at(i));
    m_pSearchHistoryComboBox->setCurrentIndex(0);
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
        m_strAiStatusMessage = QString::fromUtf8("Snippet AI failed. Use Retry Snippet AI to run the same request again.\n%1").arg(strErrorMessage);
        updateAiStatusDisplay();
        updateActionState();
        if (bAutomaticSnippetSummary)
            QMessageBox::warning(this, QString::fromUtf8("Auto Summarize Image Snippet"), QString::fromUtf8("Snippet was saved, but auto summary failed:\n%1").arg(strErrorMessage));
        else
            QMessageBox::warning(this, QString::fromUtf8("Summarize Snippet"), strErrorMessage);

        statusBar()->showMessage(bAutomaticSnippetSummary
            ? QString::fromUtf8("Image snippet saved, but auto summary failed.")
            : QString::fromUtf8("Snippet summary failed."), 5000);
        return;
    }

    m_bHasRetryableSnippetSummary = false;
    m_nRetrySnippetId = executionResult.m_context.m_nSnippetId;
    m_strAiStatusMessage = QString::fromUtf8("Snippet AI succeeded for snippet %1.").arg(executionResult.m_context.m_nSnippetId);
    updateAiStatusDisplay();
    updateActionState();
    loadSnippets(executionResult.m_context.m_nSessionId);
    if (!selectSnippetById(executionResult.m_context.m_nSnippetId))
    {
        QMessageBox::warning(this,
                             bAutomaticSnippetSummary ? QString::fromUtf8("Auto Summarize Image Snippet") : QString::fromUtf8("Summarize Snippet"),
                             QString::fromUtf8("Snippet summary finished, but automatic selection failed."));
        return;
    }

    showSnippetDetails(executionResult.m_context.m_nSnippetId);
    statusBar()->showMessage(bAutomaticSnippetSummary
        ? QString::fromUtf8("Image snippet saved and summarized.")
        : QString::fromUtf8("Snippet summary generated."), 5000);
}

void QCMainWindow::handleSessionSummaryFinished()
{
    m_bSessionSummaryRunning = false;

    const QCAiTaskExecutionResult executionResult = m_pSessionSummaryWatcher->result();
    if (!m_pAiProcessService->applyTaskResult(executionResult))
    {
        m_bHasRetryableSessionSummary = executionResult.m_context.m_nSessionId > 0;
        m_nRetrySessionId = executionResult.m_context.m_nSessionId;
        m_strAiStatusMessage = QString::fromUtf8("Session AI failed. Use Retry Session AI to run the same request again.\n%1").arg(m_pAiProcessService->lastError());
        updateAiStatusDisplay();
        updateActionState();
        QMessageBox::warning(this, QString::fromUtf8("Summarize Session"), m_pAiProcessService->lastError());
        statusBar()->showMessage(QString::fromUtf8("Session summary failed."), 5000);
        return;
    }

    m_bHasRetryableSessionSummary = false;
    m_nRetrySessionId = executionResult.m_context.m_nSessionId;
    m_strAiStatusMessage = QString::fromUtf8("Session AI succeeded for session %1.").arg(executionResult.m_context.m_nSessionId);
    updateAiStatusDisplay();
    updateActionState();
    showSessionSummary(executionResult.m_context.m_nSessionId);
    statusBar()->showMessage(QString::fromUtf8("Session summary generated."), 5000);
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
    m_strAiStatusMessage = bAutomatic
        ? QString::fromUtf8("Image snippet AI started in background. Retry stays available if this run fails.")
        : QString::fromUtf8("Snippet AI started in background. Retry stays available if this run fails.");
    updateAiStatusDisplay();
    updateActionState();
    statusBar()->showMessage(bAutomatic
        ? QString::fromUtf8("Generating image snippet summary in background...")
        : QString::fromUtf8("Generating snippet summary in background..."));
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
    m_pSnippetTagsValueLabel->setText(QString::fromUtf8("No tags."));

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
    const QVector<qint64> vecSnippetIds = selectedSnippetIds();
    if (vecSnippetIds.isEmpty())
    {
        QMessageBox::information(this, uiText(QString::fromUtf8("????"), QString::fromUtf8("Duplicate Snippet")), QString::fromUtf8("Select one or more snippets first."));
        return;
    }

    const qint64 nTargetSessionId = currentSessionId();
    if (nTargetSessionId <= 0)
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("????"), QString::fromUtf8("Duplicate Snippet")), QString::fromUtf8("Current session is invalid."));
        return;
    }

    QVector<qint64> vecNewSnippetIds;
    for (int i = 0; i < vecSnippetIds.size(); ++i)
    {
        qint64 nNewSnippetId = 0;
        if (!m_pSnippetService->duplicateSnippet(vecSnippetIds.at(i), nTargetSessionId, &nNewSnippetId))
        {
            QMessageBox::warning(this, uiText(QString::fromUtf8("????"), QString::fromUtf8("Duplicate Snippet")), m_pSnippetService->lastError());
            return;
        }

        if (nullptr != m_pTagService)
        {
            const QVector<QCTag> vecSourceTags = m_pTagService->listTagsBySnippet(vecSnippetIds.at(i));
            if (!m_pTagService->lastError().isEmpty())
            {
                QMessageBox::warning(this, uiText(QString::fromUtf8("????"), QString::fromUtf8("Duplicate Snippet")), m_pTagService->lastError());
                return;
            }

            QVector<qint64> vecTagIds;
            for (int j = 0; j < vecSourceTags.size(); ++j)
                vecTagIds.append(vecSourceTags.at(j).id());

            if (!vecTagIds.isEmpty() && !m_pTagService->replaceSnippetTags(nNewSnippetId, vecTagIds))
            {
                QMessageBox::warning(this, uiText(QString::fromUtf8("????"), QString::fromUtf8("Duplicate Snippet")), m_pTagService->lastError());
                return;
            }
        }

        vecNewSnippetIds.append(nNewSnippetId);
    }

    loadSnippets(nTargetSessionId);
    selectSnippetsByIds(vecNewSnippetIds);
    onSnippetSelectionChanged();
    statusBar()->showMessage(QString::fromUtf8("Duplicated %1 snippet(s).").arg(vecNewSnippetIds.size()), 4000);
}

void QCMainWindow::onMoveSnippet()
{
    const QVector<qint64> vecSnippetIds = selectedSnippetIds();
    if (vecSnippetIds.isEmpty())
    {
        QMessageBox::information(this, uiText(QString::fromUtf8("????"), QString::fromUtf8("Move Snippet")), QString::fromUtf8("Select one or more snippets first."));
        return;
    }

    if (nullptr == m_pSnippetService || nullptr == m_pSessionService)
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("????"), QString::fromUtf8("Move Snippet")), QString::fromUtf8("Required services are unavailable."));
        return;
    }

    const qint64 nCurrentSessionId = currentSessionId();
    const QVector<QCStudySession> vecSessions = m_pSessionService->listSessions();
    if (!m_pSessionService->lastError().isEmpty())
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("????"), QString::fromUtf8("Move Snippet")), m_pSessionService->lastError());
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
            ? QString::fromUtf8("Untitled Session")
            : session.title().trimmed();
        const QString strStatus = session.status() == QCSessionStatus::FinishedSessionStatus
            ? QString::fromUtf8("Finished")
            : QString::fromUtf8("Active");
        vecSessionLabels.append(QString::fromUtf8("%1 [%2]").arg(strTitle, strStatus));
        vecSessionIds.append(session.id());
    }

    if (vecSessionIds.isEmpty())
    {
        QMessageBox::information(this, uiText(QString::fromUtf8("????"), QString::fromUtf8("Move Snippet")), QString::fromUtf8("No other session is available."));
        return;
    }

    bool bAccepted = false;
    const QString strSelectedLabel = QInputDialog::getItem(this,
                                                           uiText(QString::fromUtf8("????"), QString::fromUtf8("Move Snippet")),
                                                           vecSnippetIds.size() > 1 ? QString::fromUtf8("Move selected snippets to session") : QString::fromUtf8("Move to session"),
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
    if (!m_pSnippetService->moveSnippetsToSession(vecSnippetIds, nTargetSessionId))
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("????"), QString::fromUtf8("Move Snippet")), m_pSnippetService->lastError());
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
    selectSnippetsByIds(vecSnippetIds);
    onSnippetSelectionChanged();
    statusBar()->showMessage(QString::fromUtf8("Moved %1 snippet(s) to another session.").arg(vecSnippetIds.size()), 4000);
}

void QCMainWindow::onManageTags()
{
    const QVector<qint64> vecSnippetIds = selectedSnippetIds();
    if (vecSnippetIds.isEmpty())
    {
        QMessageBox::information(this, uiText(QString::fromUtf8("????"), QString::fromUtf8("Snippet Tags")), QString::fromUtf8("Select one or more snippets first."));
        return;
    }

    if (nullptr == m_pTagService)
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("????"), QString::fromUtf8("Snippet Tags")), QString::fromUtf8("Tag service is unavailable."));
        return;
    }

    const QVector<QCTag> vecAvailableTags = m_pTagService->listTags();
    if (!m_pTagService->lastError().isEmpty())
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("????"), QString::fromUtf8("Snippet Tags")), m_pTagService->lastError());
        return;
    }

    const QHash<qint64, int> hashUsageCounts = buildTagUsageCounts(vecAvailableTags);
    if (!m_pTagService->lastError().isEmpty())
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("????"), QString::fromUtf8("Snippet Tags")), m_pTagService->lastError());
        return;
    }

    const QHash<qint64, Qt::CheckState> hashTagStates = buildTagSelectionStatesForSnippets(vecSnippetIds, vecAvailableTags);
    if (!m_pTagService->lastError().isEmpty())
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("????"), QString::fromUtf8("Snippet Tags")), m_pTagService->lastError());
        return;
    }

    QCSnippetTagDialog dialog(this);
    dialog.setWindowTitle(uiText(QString::fromUtf8("????"), QString::fromUtf8("Snippet Tags")));
    dialog.setBindingEnabled(true);
    dialog.setContextText(vecSnippetIds.size() > 1
        ? QString::fromUtf8("Checked tags will be applied to all selected snippets. Unchecked tags will be removed from all selected snippets. Mixed tags stay unchanged until you click them.")
        : QString::fromUtf8("Checked tags stay bound to the selected snippet. You can also create, rename, or delete reusable tags here."));
    dialog.setAvailableTags(vecAvailableTags);
    dialog.setTagUsageCounts(hashUsageCounts);
    dialog.setTagSelectionStates(hashTagStates);
    if (QDialog::Accepted != dialog.exec())
        return;

    qint64 nCreatedTagId = 0;
    QVector<qint64> vecDeletedTagIds;
    if (!applyTagDialogChanges(dialog, &nCreatedTagId, &vecDeletedTagIds, uiText(QString::fromUtf8("????"), QString::fromUtf8("Snippet Tags"))))
        return;

    QHash<qint64, Qt::CheckState> hashFinalStates = dialog.tagSelectionStates();
    for (int i = 0; i < vecDeletedTagIds.size(); ++i)
        hashFinalStates.remove(vecDeletedTagIds.at(i));
    if (nCreatedTagId > 0)
        hashFinalStates.insert(nCreatedTagId, Qt::Checked);

    if (!applyTagSelectionStatesToSnippets(vecSnippetIds, hashFinalStates))
    {
        QMessageBox::warning(this, uiText(QString::fromUtf8("????"), QString::fromUtf8("Snippet Tags")), m_pTagService->lastError());
        return;
    }

    const qint64 nSessionId = currentSessionId();
    loadTagFilterOptions();
    loadSnippets(nSessionId);
    selectSnippetsByIds(vecSnippetIds);
    onSnippetSelectionChanged();
    statusBar()->showMessage(QString::fromUtf8("Tags updated for %1 snippet(s).") .arg(vecSnippetIds.size()), 4000);
}

void QCMainWindow::onOpenTagLibrary()
{
    if (nullptr == m_pTagService)
    {
        QMessageBox::warning(this, QString::fromUtf8("Tag Library"), QString::fromUtf8("Tag service is unavailable."));
        return;
    }

    const QVector<QCTag> vecAvailableTags = m_pTagService->listTags();
    if (!m_pTagService->lastError().isEmpty())
    {
        QMessageBox::warning(this, QString::fromUtf8("Tag Library"), m_pTagService->lastError());
        return;
    }

    const QHash<qint64, int> hashUsageCounts = buildTagUsageCounts(vecAvailableTags);
    if (!m_pTagService->lastError().isEmpty())
    {
        QMessageBox::warning(this, QString::fromUtf8("Tag Library"), m_pTagService->lastError());
        return;
    }

    QCSnippetTagDialog dialog(this);
    dialog.setWindowTitle(QString::fromUtf8("Tag Library"));
    dialog.setBindingEnabled(false);
    dialog.setContextText(QString::fromUtf8("Manage reusable tags here. Usage counts show how many snippets currently reference each tag."));
    dialog.setAvailableTags(vecAvailableTags);
    dialog.setTagUsageCounts(hashUsageCounts);
    if (QDialog::Accepted != dialog.exec())
        return;

    qint64 nCreatedTagId = 0;
    QVector<qint64> vecDeletedTagIds;
    if (!applyTagDialogChanges(dialog, &nCreatedTagId, &vecDeletedTagIds, QString::fromUtf8("Tag Library")))
        return;

    loadTagFilterOptions();
    applySnippetFilters();
    onSnippetSelectionChanged();
    statusBar()->showMessage(QString::fromUtf8("Tag library updated."), 4000);
}

void QCMainWindow::onDeleteSnippet()
{
    const QVector<qint64> vecSnippetIds = selectedSnippetIds();
    if (vecSnippetIds.isEmpty())
    {
        QMessageBox::information(this, uiText(QString::fromUtf8("????"), QString::fromUtf8("Delete Snippet")), QString::fromUtf8("Select one or more snippets first."));
        return;
    }

    QString strMessage = vecSnippetIds.size() > 1
        ? QString::fromUtf8("Delete %1 selected snippets\nThis action cannot be undone.").arg(vecSnippetIds.size())
        : QString::fromUtf8("Delete selected snippet\nThis action cannot be undone.");
    const QMessageBox::StandardButton button = QMessageBox::question(this,
                                                                     uiText(QString::fromUtf8("????"), QString::fromUtf8("Delete Snippet")),
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
            QMessageBox::warning(this, uiText(QString::fromUtf8("????"), QString::fromUtf8("Delete Snippet")), m_pSnippetService->lastError());
            return;
        }
    }

    loadSnippets(nSessionId);
    statusBar()->showMessage(QString::fromUtf8("Deleted %1 snippet(s).").arg(vecSnippetIds.size()), 4000);
}

void QCMainWindow::onToggleArchiveSnippet()
{
    const QVector<qint64> vecSnippetIds = selectedSnippetIds();
    if (vecSnippetIds.isEmpty())
    {
        QMessageBox::information(this, uiText(QString::fromUtf8("????"), QString::fromUtf8("Archive Snippet")), QString::fromUtf8("Select one or more snippets first."));
        return;
    }

    int nArchivedCount = 0;
    for (int i = 0; i < vecSnippetIds.size(); ++i)
    {
        QCSnippet snippet;
        if (!m_pSnippetService->getSnippetById(vecSnippetIds.at(i), &snippet))
        {
            QMessageBox::warning(this, uiText(QString::fromUtf8("????"), QString::fromUtf8("Archive Snippet")), m_pSnippetService->lastError());
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
            QMessageBox::warning(this, uiText(QString::fromUtf8("????"), QString::fromUtf8("Archive Snippet")), m_pSnippetService->lastError());
            return;
        }
    }

    loadSnippets(currentSessionId());
    if (bRestore)
        selectSnippetsByIds(vecSnippetIds);
    statusBar()->showMessage(bRestore
        ? QString::fromUtf8("Restored %1 snippet(s).").arg(vecSnippetIds.size())
        : QString::fromUtf8("Archived %1 snippet(s).").arg(vecSnippetIds.size()), 4000);
}

void QCMainWindow::onDeleteSession()
{
    const qint64 nSessionId = currentSessionId();
    if (nSessionId <= 0)
    {
        QMessageBox::information(this, QString::fromUtf8("Delete Session"), QString::fromUtf8("Select a session first."));
        return;
    }

    QCStudySession session;
    if (!m_pSessionService->getSessionById(nSessionId, &session))
    {
        QMessageBox::warning(this, QString::fromUtf8("Delete Session"), m_pSessionService->lastError());
        return;
    }

    const QString strTitle = session.title().trimmed().isEmpty() ? QString::fromUtf8("Untitled Session") : session.title().trimmed();
    const QMessageBox::StandardButton button = QMessageBox::question(
        this,
        QString::fromUtf8("Delete Session"),
        QString::fromUtf8("Delete session: %1\nIts snippets will also be removed and this action cannot be undone.").arg(strTitle),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (QMessageBox::Yes != button)
        return;

    if (!m_pSessionService->deleteSession(nSessionId))
    {
        QMessageBox::warning(this, QString::fromUtf8("Delete Session"), m_pSessionService->lastError());
        return;
    }

    loadSessions();
    statusBar()->showMessage(QString::fromUtf8("Session deleted."), 4000);
}

void QCMainWindow::onSessionSelectionChanged()
{
    const qint64 nSessionId = currentSessionId();
    showSessionSummary(nSessionId);
    loadSnippets(nSessionId);
}

void QCMainWindow::onSnippetSelectionChanged()
{
    const QVector<qint64> vecSnippetIds = selectedSnippetIds();
    if (vecSnippetIds.size() == 1)
    {
        showSnippetDetails(vecSnippetIds.first());
        return;
    }

    clearSnippetDetails();
    if (vecSnippetIds.size() > 1)
    {
        m_pSnippetTitleValueLabel->setText(QString::fromUtf8("%1 snippets selected.").arg(vecSnippetIds.size()));
        m_pSnippetTagsValueLabel->setText(QString::fromUtf8("Batch organize and batch tag actions are available from the toolbar, menus, and right-click menu."));
        m_pSnippetNoteTextEdit->setPlainText(QString::fromUtf8("Use Duplicate, Move, Tag, Archive/Restore, or Delete for batch organization."));
        m_pSnippetContentTextEdit->setPlainText(QString::fromUtf8("Edit and Summarize stay single-snippet actions. Session actions remain available from the session list and menu bar."));
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
    statusBar()->showMessage(QString::fromUtf8("Search cleared."), 3000);
}

void QCMainWindow::restoreDefaultFilters()
{
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
    statusBar()->showMessage(QString::fromUtf8("Filters reset."), 3000);
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
            statusBar()->showMessage(QString::fromUtf8("Session created."), 4000);
            break;
        }
    }
}

void QCMainWindow::onEditSession()
{
    const qint64 nSessionId = currentSessionId();
    if (nSessionId <= 0)
    {
        QMessageBox::information(this, QString::fromUtf8("Edit Session"), QString::fromUtf8("Select a session first."));
        return;
    }

    QCStudySession session;
    if (!m_pSessionService->getSessionById(nSessionId, &session))
    {
        QMessageBox::warning(this, QString::fromUtf8("Edit Session"), m_pSessionService->lastError());
        return;
    }

    QCCreateSessionDialog dialog(this);
    dialog.setWindowTitle(QString::fromUtf8("Edit Session"));
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
        QMessageBox::warning(this, QString::fromUtf8("Edit Session"), m_pSessionService->lastError());
        return;
    }

    loadSessions();
    statusBar()->showMessage(QString::fromUtf8("Session updated."), 4000);
}

void QCMainWindow::onFinishSession()
{
    const qint64 nSessionId = currentSessionId();
    if (nSessionId <= 0)
    {
        QMessageBox::information(this, QString::fromUtf8("Finish Session"), QString::fromUtf8("Select a session first."));
        return;
    }

    QCStudySession session;
    if (!m_pSessionService->getSessionById(nSessionId, &session))
    {
        QMessageBox::warning(this, QString::fromUtf8("Finish Session"), m_pSessionService->lastError());
        return;
    }

    if (session.status() == QCSessionStatus::FinishedSessionStatus)
    {
        QMessageBox::information(this, QString::fromUtf8("Finish Session"), QString::fromUtf8("The selected session is already finished."));
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
        QMessageBox::warning(this, QString::fromUtf8("Finish Session"), m_pSessionService->lastError());
        return;
    }

    loadSessions();
    statusBar()->showMessage(QString::fromUtf8("Session finished."), 4000);
}

void QCMainWindow::onReopenSession()
{
    const qint64 nSessionId = currentSessionId();
    if (nSessionId <= 0)
    {
        QMessageBox::information(this, QString::fromUtf8("Reopen Session"), QString::fromUtf8("Select a session first."));
        return;
    }

    QCStudySession session;
    if (!m_pSessionService->getSessionById(nSessionId, &session))
    {
        QMessageBox::warning(this, QString::fromUtf8("Reopen Session"), m_pSessionService->lastError());
        return;
    }

    if (session.status() == QCSessionStatus::ActiveSessionStatus)
    {
        QMessageBox::information(this, QString::fromUtf8("Reopen Session"), QString::fromUtf8("The selected session is already active."));
        return;
    }

    session.setStatus(QCSessionStatus::ActiveSessionStatus);
    session.setEndedAt(QDateTime());
    if (!m_pSessionService->updateSession(&session))
    {
        QMessageBox::warning(this, QString::fromUtf8("Reopen Session"), m_pSessionService->lastError());
        return;
    }

    loadSessions();
    statusBar()->showMessage(QString::fromUtf8("Session reopened."), 4000);
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
        QMessageBox::information(this, QString::fromUtf8("New Text Snippet"), QString::fromUtf8("Select a session first before creating a text snippet."));
        return;
    }

    QCCreateTextSnippetDialog dialog(this);
    if (QDialog::Accepted != dialog.exec())
        return;

    QCSnippet snippet;
    snippet.setSessionId(nSessionId);
    snippet.setCaptureType(QCCaptureType::ManualCaptureType);
    snippet.setTitle(dialog.title());
    snippet.setNote(dialog.note());
    snippet.setContentText(dialog.content());
    snippet.setNoteKind(QCNoteKind::ConceptNoteKind);
    snippet.setNoteLevel(QCNoteLevel::ImportantNoteLevel);
    snippet.setSource(QString::fromUtf8("ui"));
    snippet.setLanguage(QString::fromUtf8("en"));

    if (!m_pSnippetService->createTextSnippet(&snippet))
    {
        QMessageBox::warning(this, QString::fromUtf8("New Text Snippet"), m_pSnippetService->lastError());
        return;
    }

    loadSnippets(nSessionId);
    if (!selectSnippetById(snippet.id()))
        QMessageBox::warning(this, QString::fromUtf8("New Text Snippet"), QString::fromUtf8("Snippet was saved, but automatic selection failed."));
    else
        statusBar()->showMessage(QString::fromUtf8("Text snippet created."), 4000);
}

void QCMainWindow::onEditSnippet()
{
    if (!hasSingleSelectedSnippet())
    {
        QMessageBox::information(this, QString::fromUtf8("Edit Snippet"), QString::fromUtf8("Select exactly one snippet first."));
        return;
    }

    QCSnippet snippet;
    if (!currentSnippetForAction(&snippet))
    {
        QMessageBox::warning(this, QString::fromUtf8("Edit Snippet"), m_pSnippetService->lastError());
        return;
    }

    if (snippet.type() == QCSnippetType::TextSnippetType)
    {
        QCCreateTextSnippetDialog dialog(this);
        dialog.setWindowTitle(QString::fromUtf8("Edit Text Snippet"));
        dialog.setTitle(snippet.title());
        dialog.setNote(snippet.note());
        dialog.setContent(snippet.contentText());
        if (QDialog::Accepted != dialog.exec())
            return;

        snippet.setTitle(dialog.title());
        snippet.setNote(dialog.note());
        snippet.setContentText(dialog.content());
    }
    else if (snippet.type() == QCSnippetType::ImageSnippetType)
    {
        QCAttachment primaryAttachment;
        if (!m_pSnippetService->getPrimaryAttachmentBySnippetId(snippet.id(), &primaryAttachment))
        {
            QMessageBox::warning(this, QString::fromUtf8("Edit Snippet"), m_pSnippetService->lastError());
            return;
        }

        QCQuickCaptureDialog dialog(primaryAttachment.filePath(), this);
        dialog.setWindowTitle(QString::fromUtf8("Edit Image Snippet"));
        dialog.setTitle(snippet.title());
        dialog.setNote(snippet.note());
        if (QDialog::Accepted != dialog.exec())
            return;

        snippet.setTitle(dialog.title());
        snippet.setNote(dialog.note());
    }
    else
    {
        QMessageBox::information(this, QString::fromUtf8("Edit Snippet"), QString::fromUtf8("Editing is currently available for text and image snippets."));
        return;
    }

    if (!m_pSnippetService->updateSnippet(&snippet))
    {
        QMessageBox::warning(this, QString::fromUtf8("Edit Snippet"), m_pSnippetService->lastError());
        return;
    }

    loadSnippets(snippet.sessionId());
    if (selectSnippetById(snippet.id()))
        showSnippetDetails(snippet.id());

    statusBar()->showMessage(QString::fromUtf8("Snippet updated."), 4000);
}

void QCMainWindow::onCaptureScreen()
{
    const qint64 nSessionId = currentSessionId();
    if (nSessionId <= 0)
    {
        QMessageBox::information(this, QString::fromUtf8("Capture Screen"), QString::fromUtf8("Select a session first before capturing the screen."));
        return;
    }

    if (nullptr == m_pScreenCaptureService)
    {
        QMessageBox::warning(this, QString::fromUtf8("Capture Screen"), QString::fromUtf8("Screen capture service is unavailable."));
        return;
    }

    statusBar()->showMessage(QString::fromUtf8("Capturing primary screen..."));
    QCScreenCaptureResult captureResult;
    if (!captureScreenToFile(&captureResult))
    {
        QMessageBox::warning(this, QString::fromUtf8("Capture Screen"), m_pScreenCaptureService->lastError());
        statusBar()->showMessage(QString::fromUtf8("Screen capture failed."), 4000);
        return;
    }

    QCQuickCaptureDialog dialog(captureResult.m_strFilePath, this);
    if (QDialog::Accepted != dialog.exec())
    {
        statusBar()->showMessage(QString::fromUtf8("Screen capture cancelled."), 3000);
        return;
    }

    QCSnippet snippet;
    snippet.setSessionId(nSessionId);
    snippet.setType(QCSnippetType::ImageSnippetType);
    snippet.setCaptureType(QCCaptureType::ScreenCaptureType);
    snippet.setTitle(dialog.title());
    snippet.setNote(dialog.note());
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
        statusBar()->showMessage(QString::fromUtf8("Captured image could not be saved."), 5000);
        return;
    }

    handleSavedImageSnippet(nSessionId, snippet.id(), QString::fromUtf8("Screenshot saved and selected."));
}
void QCMainWindow::onCaptureRegion()
{
    const qint64 nSessionId = currentSessionId();
    if (nSessionId <= 0)
    {
        QMessageBox::information(this, QString::fromUtf8("Capture Region"), QString::fromUtf8("Select a session first before capturing a region."));
        return;
    }

    if (nullptr == m_pScreenCaptureService)
    {
        QMessageBox::warning(this, QString::fromUtf8("Capture Region"), QString::fromUtf8("Screen capture service is unavailable."));
        return;
    }

    statusBar()->showMessage(QString::fromUtf8("Select a capture region on the primary screen..."));
    QCScreenCaptureResult captureResult;
    bool bCancelled = false;
    QString strCaptureFailureMessage;
    if (!captureRegionToFile(&captureResult, &bCancelled, &strCaptureFailureMessage))
    {
        if (bCancelled)
        {
            statusBar()->showMessage(QString::fromUtf8("Region capture cancelled."), 3000);
            return;
        }

        const QString strMessage = strCaptureFailureMessage.trimmed().isEmpty()
            ? QString::fromUtf8("Region capture failed.")
            : strCaptureFailureMessage;
        QMessageBox::warning(this, QString::fromUtf8("Capture Region"), strMessage);
        statusBar()->showMessage(strMessage, 4000);
        return;
    }

    statusBar()->showMessage(QString::fromUtf8("Region captured: %1 x %2").arg(captureResult.m_nWidth).arg(captureResult.m_nHeight), 3000);

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
    snippet.setNote(dialog.note());
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
        statusBar()->showMessage(QString::fromUtf8("Region image could not be saved."), 5000);
        return;
    }

    handleSavedImageSnippet(nSessionId, snippet.id(), QString::fromUtf8("Region capture saved and selected."));
}

void QCMainWindow::onImportImageSnippet()
{
    const qint64 nSessionId = currentSessionId();
    if (nSessionId <= 0)
    {
        QMessageBox::information(this, QString::fromUtf8("Import Image"), QString::fromUtf8("Select a session first before importing an image."));
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
            QMessageBox::warning(this, QString::fromUtf8("Import Image"), QString::fromUtf8("Screen capture service is unavailable."));
            return;
        }

        QString strCopiedFilePath;
        if (!m_pScreenCaptureService->copyImportedImageToCaptureDirectory(dialog.filePath(), &strCopiedFilePath))
        {
            QMessageBox::warning(this, QString::fromUtf8("Import Image"), m_pScreenCaptureService->lastError());
            return;
        }

        strAttachmentFilePath = strCopiedFilePath;
    }

    QCSnippet snippet;
    snippet.setSessionId(nSessionId);
    snippet.setType(QCSnippetType::ImageSnippetType);
    snippet.setCaptureType(QCCaptureType::ImportCaptureType);
    snippet.setTitle(dialog.title());
    snippet.setNote(dialog.note());
    snippet.setNoteKind(QCNoteKind::ReviewNoteKind);
    snippet.setNoteLevel(QCNoteLevel::ReviewNoteLevel);
    snippet.setSource(QString::fromUtf8("file"));

    QCAttachment primaryAttachment;
    primaryAttachment.setFilePath(strAttachmentFilePath);
    primaryAttachment.setFileName(QFileInfo(strAttachmentFilePath).fileName());
    primaryAttachment.setMimeType(QString::fromUtf8("image/png"));

    if (!m_pSnippetService->createImageSnippetWithPrimaryAttachment(&snippet, &primaryAttachment))
    {
        QMessageBox::warning(this, QString::fromUtf8("Import Image"), m_pSnippetService->lastError());
        return;
    }

    handleSavedImageSnippet(nSessionId, snippet.id(), QString::fromUtf8("Imported image saved."));
    if (dialog.shouldCopyImportedImageToDefaultCaptureDirectory())
    {
        statusBar()->showMessage(QString::fromUtf8("Imported image saved using copied file: %1").arg(strAttachmentFilePath), 6000);
    }
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
        QMessageBox::information(this, QString::fromUtf8("Summarize Snippet"), QString::fromUtf8("Select exactly one snippet first."));
        return;
    }

    const qint64 nSnippetId = selectedSnippetIds().first();
    if (nullptr == m_pAiProcessService)
    {
        QMessageBox::warning(this, QString::fromUtf8("Summarize Snippet"), QString::fromUtf8("AI process service is unavailable."));
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
        QMessageBox::information(this, QString::fromUtf8("Summarize Session"), QString::fromUtf8("Select a session first."));
        return;
    }

    if (nullptr == m_pAiProcessService)
    {
        QMessageBox::warning(this, QString::fromUtf8("Summarize Session"), QString::fromUtf8("AI process service is unavailable."));
        return;
    }

    QCAiTaskExecutionContext executionContext;
    if (!m_pAiProcessService->prepareSessionSummary(nSessionId, &executionContext))
    {
        QMessageBox::warning(this, QString::fromUtf8("Summarize Session"), m_pAiProcessService->lastError());
        return;
    }

    m_bSessionSummaryRunning = true;
    m_nRetrySessionId = nSessionId;
    m_strAiStatusMessage = QString::fromUtf8("Session AI started in background. Retry stays available if this run fails.");
    updateAiStatusDisplay();
    updateActionState();
    statusBar()->showMessage(QString::fromUtf8("Generating session summary in background..."));
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
            QMessageBox::warning(this, QString::fromUtf8("Retry Snippet AI"), strErrorMessage);
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
        QMessageBox::warning(this, QString::fromUtf8("Retry Session AI"), m_pAiProcessService->lastError());
        return;
    }

    m_bSessionSummaryRunning = true;
    m_strAiStatusMessage = QString::fromUtf8("Session AI retry started in background.");
    updateAiStatusDisplay();
    updateActionState();
    statusBar()->showMessage(QString::fromUtf8("Retrying session summary in background..."));
    m_pSessionSummaryWatcher->setFuture(QtConcurrent::run(RunAiTaskInBackground,
                                                          m_pAiProcessService,
                                                          executionContext));
}

void QCMainWindow::onAiSettings()
{
    if (nullptr == m_pSettingsService)
    {
        QMessageBox::warning(this, QString::fromUtf8("Settings"), QString::fromUtf8("Settings service is unavailable."));
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

    m_strAppLanguage = dialog.appLanguage();
    qApp->setProperty("qtclip.uiLanguage", m_strAppLanguage);
    applyLocalizedTexts();
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

    statusBar()->showMessage(uiText(QString::fromUtf8("??????"), QString::fromUtf8("Settings saved.")), 4000);
}

void QCMainWindow::onExportMarkdown()
{
    const qint64 nSessionId = currentSessionId();
    if (nSessionId <= 0)
    {
        QMessageBox::information(this, QString::fromUtf8("Export Markdown"), QString::fromUtf8("Select a session first."));
        return;
    }

    QString strExportDirectory;
    if (nullptr != m_pSettingsService)
        m_pSettingsService->getExportDirectory(&strExportDirectory);

    QString strSuggestedFileName = QString::fromUtf8("qtclip_export.md");
    if (nullptr != m_pMdExportService)
    {
        QCMdExportPreview exportPreview;
        if (m_pMdExportService->buildExportPreview(nSessionId, &exportPreview))
        {
            QString strBaseName = exportPreview.m_strSessionTitle.trimmed();
            if (strBaseName.isEmpty())
                strBaseName = QString::fromUtf8("qtclip_export");

            strBaseName.replace(QRegularExpression(QString::fromUtf8("[^A-Za-z0-9_-]+")),
                                QString::fromUtf8("_"));
            strBaseName.replace(QRegularExpression(QString::fromUtf8("_+")),
                                QString::fromUtf8("_"));
            strBaseName = strBaseName.trimmed();
            if (strBaseName.isEmpty())
                strBaseName = QString::fromUtf8("qtclip_export");

            strSuggestedFileName = strBaseName + QString::fromUtf8(".md");

            const QString strPreviewText = QString::fromUtf8(
                                               "Session: %1\n"
                                               "Course: %2\n"
                                               "Snippets: %3\n"
                                               "Image snippets: %4\n"
                                               "Archived snippets: %5\n"
                                               "Favorite snippets: %6\n"
                                               "Summarized snippets: %7\n"
                                               "Session summary: %8\n\n"
                                               "Continue to choose an export file path?")
                                               .arg(exportPreview.m_strSessionTitle.trimmed().isEmpty()
                                                        ? QString::fromUtf8("Untitled Session")
                                                        : exportPreview.m_strSessionTitle.trimmed())
                                               .arg(exportPreview.m_strCourseName.trimmed().isEmpty()
                                                        ? QString::fromUtf8("N/A")
                                                        : exportPreview.m_strCourseName.trimmed())
                                               .arg(exportPreview.m_nSnippetCount)
                                               .arg(exportPreview.m_nImageSnippetCount)
                                               .arg(exportPreview.m_nArchivedSnippetCount)
                                               .arg(exportPreview.m_nFavoriteSnippetCount)
                                               .arg(exportPreview.m_nSummarizedSnippetCount)
                                               .arg(exportPreview.m_bHasSessionSummary
                                                        ? QString::fromUtf8("Available")
                                                        : QString::fromUtf8("Not available"));
            if (QMessageBox::Yes != QMessageBox::question(this,
                                                          QString::fromUtf8("Export Markdown"),
                                                          strPreviewText,
                                                          QMessageBox::Yes | QMessageBox::Cancel,
                                                          QMessageBox::Yes))
            {
                return;
            }
        }
    }

    const QString strSuggestedOutputPath = strExportDirectory.trimmed().isEmpty()
        ? strSuggestedFileName
        : QDir(strExportDirectory).filePath(strSuggestedFileName);

    const QString strOutputFilePath = QFileDialog::getSaveFileName(this,
                                                                   QString::fromUtf8("Export Markdown"),
                                                                   strSuggestedOutputPath,
                                                                   QString::fromUtf8("Markdown (*.md)"));
    if (strOutputFilePath.isEmpty())
        return;

    if (!m_pMdExportService->exportSessionToFile(nSessionId, strOutputFilePath))
    {
        QMessageBox::warning(this, QString::fromUtf8("Export Markdown"), m_pMdExportService->lastError());
        return;
    }

    statusBar()->showMessage(QString::fromUtf8("Markdown exported to %1").arg(strOutputFilePath), 5000);
    QMessageBox messageBox(QMessageBox::Information,
                           QString::fromUtf8("Export Markdown"),
                           QString::fromUtf8("Markdown exported to:\n%1").arg(strOutputFilePath),
                           QMessageBox::NoButton,
                           this);
    QPushButton *pOpenFileButton = messageBox.addButton(QString::fromUtf8("Open File"), QMessageBox::AcceptRole);
    QPushButton *pOpenFolderButton = messageBox.addButton(QString::fromUtf8("Open Folder"), QMessageBox::ActionRole);
    messageBox.addButton(QMessageBox::Close);
    messageBox.exec();

    if (messageBox.clickedButton() == pOpenFileButton)
    {
        QDesktopServices::openUrl(QUrl::fromLocalFile(strOutputFilePath));
    }
    else if (messageBox.clickedButton() == pOpenFolderButton)
    {
        QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(strOutputFilePath).absolutePath()));
    }
}

void QCMainWindow::onFocusSearch()
{
    if (nullptr == m_pSnippetSearchLineEdit)
        return;

    m_pSnippetSearchLineEdit->setFocus(Qt::ShortcutFocusReason);
    m_pSnippetSearchLineEdit->selectAll();
}

void QCMainWindow::onRefresh()
{
    loadSessions();
    statusBar()->showMessage(uiText(QString::fromUtf8("????"), QString::fromUtf8("Refreshed.")), 3000);
}

















































