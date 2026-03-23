
// File: qcmainwindow.cpp
// Author: ZZMI1
// Created: 2026-03-23
// Description: Implements the minimal Qt Widgets main window used by the QtClip demo application.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include "qcmainwindow.h"

#include <QtConcurrent/QtConcurrent>

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QEventLoop>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QStatusBar>
#include <QTimer>
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
    if (session.courseName().trimmed().isEmpty())
        return session.title();

    return QString::fromUtf8("%1\n%2").arg(session.title(), session.courseName());
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
    , m_pNewTextSnippetAction(nullptr)
    , m_pCaptureScreenAction(nullptr)
    , m_pCaptureRegionAction(nullptr)
    , m_pImportImageAction(nullptr)
    , m_pManageTagsAction(nullptr)
    , m_pSummarizeSnippetAction(nullptr)
    , m_pSummarizeSessionAction(nullptr)
    , m_pAiSettingsAction(nullptr)
    , m_pExportMarkdownAction(nullptr)
    , m_pRefreshAction(nullptr)
    , m_pSessionListWidget(nullptr)
    , m_pSnippetListWidget(nullptr)
    , m_pSnippetSearchLineEdit(nullptr)
    , m_pQuickFavoriteCheckBox(nullptr)
    , m_pQuickReviewCheckBox(nullptr)
    , m_pFavoriteOnlyCheckBox(nullptr)
    , m_pReviewOnlyCheckBox(nullptr)
    , m_pTagFilterComboBox(nullptr)
    , m_pSessionSummaryTextEdit(nullptr)
    , m_pSnippetTitleValueLabel(nullptr)
    , m_pSnippetTagsValueLabel(nullptr)
    , m_pSnippetNoteTextEdit(nullptr)
    , m_pSnippetContentTextEdit(nullptr)
    , m_pSnippetSummaryTextEdit(nullptr)
    , m_pImagePathValueLabel(nullptr)
    , m_pImagePreviewLabel(nullptr)
    , m_bSnippetSummaryRunning(false)
    , m_bSessionSummaryRunning(false)
    , m_bAutomaticSnippetSummary(false)
    , m_bUpdatingSnippetStateControls(false)
    , m_pSnippetSummaryWatcher(new QFutureWatcher<QCAiTaskExecutionResult>(this))
    , m_pSessionSummaryWatcher(new QFutureWatcher<QCAiTaskExecutionResult>(this))
{
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

void QCMainWindow::setupUi()
{
    setWindowTitle(QString::fromUtf8("QtClip"));
    resize(1280, 760);

    m_pSessionListWidget = new QListWidget(this);
    m_pSnippetListWidget = new QListWidget(this);
    m_pSnippetSearchLineEdit = new QLineEdit(this);
    m_pClearSearchButton = new QPushButton(QString::fromUtf8("Clear"), this);
    m_pResetFiltersButton = new QPushButton(QString::fromUtf8("Reset"), this);
    m_pQuickFavoriteCheckBox = new QCheckBox(QString::fromUtf8("Selected Favorite"), this);
    m_pQuickReviewCheckBox = new QCheckBox(QString::fromUtf8("Selected Review"), this);
    m_pFavoriteOnlyCheckBox = new QCheckBox(QString::fromUtf8("Favorites Only"), this);
    m_pReviewOnlyCheckBox = new QCheckBox(QString::fromUtf8("Review Only"), this);
    m_pTagFilterComboBox = new QComboBox(this);
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
    m_pSnippetSearchLineEdit->setPlaceholderText(QString::fromUtf8("Search title, content, summary, note"));
    m_pTagFilterComboBox->setMinimumWidth(140);

    m_pSessionSummaryTextEdit->setReadOnly(true);
    m_pSnippetTitleValueLabel->setWordWrap(true);
    m_pSnippetTagsValueLabel->setWordWrap(true);
    m_pImagePathValueLabel->setWordWrap(true);
    m_pImagePreviewLabel->setMinimumSize(320, 220);
    m_pImagePreviewLabel->setAlignment(Qt::AlignCenter);
    m_pImagePreviewLabel->setFrameShape(QFrame::StyledPanel);
    m_pImagePreviewLabel->setText(QString::fromUtf8("No image preview available."));

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
    pDetailLayout->addRow(QString::fromUtf8("Session Summary"), m_pSessionSummaryTextEdit);
    pDetailLayout->addRow(QString::fromUtf8("Title"), m_pSnippetTitleValueLabel);
    pDetailLayout->addRow(QString::fromUtf8("Tags"), m_pSnippetTagsValueLabel);
    pDetailLayout->addRow(QString::fromUtf8("State"), pStateWidget);
    pDetailLayout->addRow(QString::fromUtf8("Note"), m_pSnippetNoteTextEdit);
    pDetailLayout->addRow(QString::fromUtf8("Content"), m_pSnippetContentTextEdit);
    pDetailLayout->addRow(QString::fromUtf8("Snippet Summary"), m_pSnippetSummaryTextEdit);
    pDetailLayout->addRow(QString::fromUtf8("Image Path"), m_pImagePathValueLabel);
    pDetailLayout->addRow(QString::fromUtf8("Preview"), m_pImagePreviewLabel);
    pDetailWidget->setLayout(pDetailLayout);

    QWidget *pSnippetPanelWidget = new QWidget(this);
    QVBoxLayout *pSnippetPanelLayout = new QVBoxLayout();
    QHBoxLayout *pFilterLayout = new QHBoxLayout();
    QHBoxLayout *pQuickStateLayout = new QHBoxLayout();
    pFilterLayout->addWidget(m_pSnippetSearchLineEdit, 1);
    pFilterLayout->addWidget(m_pClearSearchButton);
    pFilterLayout->addWidget(m_pResetFiltersButton);
    pFilterLayout->addWidget(m_pFavoriteOnlyCheckBox);
    pFilterLayout->addWidget(m_pReviewOnlyCheckBox);
    pFilterLayout->addWidget(new QLabel(QString::fromUtf8("Tag"), this));
    pFilterLayout->addWidget(m_pTagFilterComboBox);
    pQuickStateLayout->addWidget(new QLabel(QString::fromUtf8("Selected"), this));
    pQuickStateLayout->addWidget(m_pQuickFavoriteCheckBox);
    pQuickStateLayout->addWidget(m_pQuickReviewCheckBox);
    pQuickStateLayout->addStretch(1);
    pSnippetPanelLayout->addLayout(pFilterLayout);
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
    connect(m_pClearSearchButton, &QPushButton::clicked, this, [this]() { onClearSearch(); });
    connect(m_pResetFiltersButton, &QPushButton::clicked, this, [this]() { onResetFilters(); });
    connect(m_pFavoriteOnlyCheckBox, &QCheckBox::toggled, this, [this](bool) { onSnippetFilterChanged(); });
    connect(m_pReviewOnlyCheckBox, &QCheckBox::toggled, this, [this](bool) { onSnippetFilterChanged(); });
    connect(m_pTagFilterComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) { onSnippetFilterChanged(); });
    connect(m_pQuickFavoriteCheckBox, &QCheckBox::toggled, this, [this](bool bChecked) { onFavoriteToggled(bChecked); });
    connect(m_pQuickReviewCheckBox, &QCheckBox::toggled, this, [this](bool bChecked) { onReviewToggled(bChecked); });
    connect(m_pSnippetFavoriteCheckBox, &QCheckBox::toggled, this, [this](bool bChecked) { onFavoriteToggled(bChecked); });
    connect(m_pSnippetReviewCheckBox, &QCheckBox::toggled, this, [this](bool bChecked) { onReviewToggled(bChecked); });

    statusBar()->showMessage(QString::fromUtf8("Ready."));
    clearSnippetDetails();
    m_pSessionSummaryTextEdit->setPlainText(QString::fromUtf8("No session summary available."));
    loadTagFilterOptions();
}
void QCMainWindow::setupActions()
{
    QToolBar *pToolBar = addToolBar(QString::fromUtf8("Main"));
    pToolBar->setMovable(false);

    m_pNewSessionAction = pToolBar->addAction(QString::fromUtf8("New Session"));
    m_pNewTextSnippetAction = pToolBar->addAction(QString::fromUtf8("New Text"));
    m_pCaptureScreenAction = pToolBar->addAction(QString::fromUtf8("Capture Screen"));
    m_pCaptureRegionAction = pToolBar->addAction(QString::fromUtf8("Capture Region"));
    m_pImportImageAction = pToolBar->addAction(QString::fromUtf8("Import Image"));
    m_pManageTagsAction = pToolBar->addAction(QString::fromUtf8("Manage Tags"));
    m_pSummarizeSnippetAction = pToolBar->addAction(QString::fromUtf8("Summarize Snippet"));
    m_pSummarizeSessionAction = pToolBar->addAction(QString::fromUtf8("Summarize Session"));
    m_pAiSettingsAction = pToolBar->addAction(QString::fromUtf8("Settings"));
    m_pExportMarkdownAction = pToolBar->addAction(QString::fromUtf8("Export Markdown"));
    m_pRefreshAction = pToolBar->addAction(QString::fromUtf8("Refresh"));

    connect(m_pNewSessionAction, &QAction::triggered, this, [this]() { onCreateSession(); });
    connect(m_pNewTextSnippetAction, &QAction::triggered, this, [this]() { onCreateTextSnippet(); });
    connect(m_pCaptureScreenAction, &QAction::triggered, this, [this]() { onCaptureScreen(); });
    connect(m_pCaptureRegionAction, &QAction::triggered, this, [this]() { onCaptureRegion(); });
    connect(m_pImportImageAction, &QAction::triggered, this, [this]() { onImportImageSnippet(); });
    connect(m_pManageTagsAction, &QAction::triggered, this, [this]() { onManageTags(); });
    connect(m_pSummarizeSnippetAction, &QAction::triggered, this, [this]() { onSummarizeSnippet(); });
    connect(m_pSummarizeSessionAction, &QAction::triggered, this, [this]() { onSummarizeSession(); });
    connect(m_pAiSettingsAction, &QAction::triggered, this, [this]() { onAiSettings(); });
    connect(m_pExportMarkdownAction, &QAction::triggered, this, [this]() { onExportMarkdown(); });
    connect(m_pRefreshAction, &QAction::triggered, this, [this]() { onRefresh(); });

    updateAiActionState();
}

void QCMainWindow::loadSessions()
{
    const qint64 nPreviousSessionId = currentSessionId();
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
    else
    {
        m_pSnippetListWidget->clear();
        clearSnippetDetails();
        m_pSessionSummaryTextEdit->setPlainText(QString::fromUtf8("No session summary available."));
    }
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
    const qint64 nPreviousSnippetId = currentSnippetId();

    m_pSnippetListWidget->clear();
    clearSnippetDetails();

    if (nSessionId <= 0)
        return;

    const QVector<QCSnippet> vecSnippets = m_pSnippetService->querySnippets(nSessionId,
                                                                            (nullptr != m_pSnippetSearchLineEdit) ? m_pSnippetSearchLineEdit->text() : QString(),
                                                                            (nullptr != m_pFavoriteOnlyCheckBox) ? m_pFavoriteOnlyCheckBox->isChecked() : false,
                                                                            (nullptr != m_pReviewOnlyCheckBox) ? m_pReviewOnlyCheckBox->isChecked() : false,
                                                                            currentTagFilterId());
    if (!m_pSnippetService->lastError().isEmpty())
    {
        QMessageBox::warning(this, QString::fromUtf8("Load Snippets"), m_pSnippetService->lastError());
        return;
    }

    for (int i = 0; i < vecSnippets.size(); ++i)
    {
        const QCSnippet& snippet = vecSnippets.at(i);
        QListWidgetItem *pItem = new QListWidgetItem(SnippetItemText(snippet), m_pSnippetListWidget);
        pItem->setData(Qt::UserRole, snippet.id());
    }

    if (m_pSnippetListWidget->count() <= 0)
        return;

    if (nPreviousSnippetId > 0 && selectSnippetById(nPreviousSnippetId))
        return;

    m_pSnippetListWidget->setCurrentRow(0);
}

void QCMainWindow::showSessionSummary(qint64 nSessionId)
{
    m_pSessionSummaryTextEdit->setPlainText(QString::fromUtf8("No session summary available."));

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

    m_pSnippetTitleValueLabel->setText(snippet.title().trimmed().isEmpty()
        ? QString::fromUtf8("Untitled Snippet")
        : snippet.title());
    m_pSnippetNoteTextEdit->setPlainText(snippet.note());
    m_pSnippetContentTextEdit->setPlainText(snippet.contentText());

    refreshSnippetTagsDisplay(nSnippetId);

    QString strSummary = snippet.summary();
    const QVector<QCAiRecord> vecAiRecords = m_pAiService->listAiRecordsBySnippet(nSnippetId);
    if (m_pAiService->lastError().isEmpty() && strSummary.trimmed().isEmpty())
        strSummary = findSummaryFromAiRecords(vecAiRecords);
    m_pSnippetSummaryTextEdit->setPlainText(strSummary);
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
            m_pImagePreviewLabel->setText(QString::fromUtf8("No image preview available."));
        }
    }
    else
    {
        m_pImagePathValueLabel->setText(QString::fromUtf8("No image attachment."));
        m_pImagePreviewLabel->setText(QString::fromUtf8("No image preview available."));
    }
}

void QCMainWindow::clearSnippetDetails()
{
    m_pSnippetTitleValueLabel->clear();
    m_pSnippetTagsValueLabel->setText(QString::fromUtf8("No tags."));
    m_pSnippetNoteTextEdit->clear();
    m_pSnippetContentTextEdit->clear();
    m_pSnippetSummaryTextEdit->clear();
    m_pImagePathValueLabel->clear();
    m_pImagePreviewLabel->clear();
    m_pImagePreviewLabel->setPixmap(QPixmap());
    m_pImagePreviewLabel->setText(QString::fromUtf8("No image preview available."));

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

void QCMainWindow::updateAiActionState()
{
    const bool bAnyAiTaskRunning = m_bSnippetSummaryRunning || m_bSessionSummaryRunning;
    if (nullptr != m_pSummarizeSnippetAction)
        m_pSummarizeSnippetAction->setEnabled(!bAnyAiTaskRunning);
    if (nullptr != m_pSummarizeSessionAction)
        m_pSummarizeSessionAction->setEnabled(!bAnyAiTaskRunning);
}

void QCMainWindow::handleSnippetSummaryFinished()
{
    const bool bAutomaticSnippetSummary = m_bAutomaticSnippetSummary;
    m_bSnippetSummaryRunning = false;
    m_bAutomaticSnippetSummary = false;
    updateAiActionState();

    const QCAiTaskExecutionResult executionResult = m_pSnippetSummaryWatcher->result();
    if (!m_pAiProcessService->applyTaskResult(executionResult))
    {
        const QString strErrorMessage = m_pAiProcessService->lastError();
        if (bAutomaticSnippetSummary)
            QMessageBox::warning(this, QString::fromUtf8("Auto Summarize Image Snippet"), QString::fromUtf8("Snippet was saved, but auto summary failed:\n%1").arg(strErrorMessage));
        else
            QMessageBox::warning(this, QString::fromUtf8("Summarize Snippet"), strErrorMessage);

        statusBar()->showMessage(bAutomaticSnippetSummary
            ? QString::fromUtf8("Image snippet saved, but auto summary failed.")
            : QString::fromUtf8("Snippet summary failed."), 5000);
        return;
    }

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
    updateAiActionState();

    const QCAiTaskExecutionResult executionResult = m_pSessionSummaryWatcher->result();
    if (!m_pAiProcessService->applyTaskResult(executionResult))
    {
        QMessageBox::warning(this, QString::fromUtf8("Summarize Session"), m_pAiProcessService->lastError());
        statusBar()->showMessage(QString::fromUtf8("Session summary failed."), 5000);
        return;
    }

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
    for (int i = 0; i < m_pSnippetListWidget->count(); ++i)
    {
        QListWidgetItem *pItem = m_pSnippetListWidget->item(i);
        if (pItem->data(Qt::UserRole).toLongLong() == nSnippetId)
        {
            m_pSnippetListWidget->setCurrentItem(pItem);
            return true;
        }
    }

    return false;
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
    updateAiActionState();
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
void QCMainWindow::onManageTags()
{
    const qint64 nSnippetId = currentSnippetId();
    if (nSnippetId <= 0)
    {
        QMessageBox::information(this, QString::fromUtf8("Manage Tags"), QString::fromUtf8("Select a snippet first."));
        return;
    }

    if (nullptr == m_pTagService)
    {
        QMessageBox::warning(this, QString::fromUtf8("Manage Tags"), QString::fromUtf8("Tag service is unavailable."));
        return;
    }

    const QVector<QCTag> vecAvailableTags = m_pTagService->listTags();
    if (!m_pTagService->lastError().isEmpty())
    {
        QMessageBox::warning(this, QString::fromUtf8("Manage Tags"), m_pTagService->lastError());
        return;
    }

    const QVector<QCTag> vecBoundTags = m_pTagService->listTagsBySnippet(nSnippetId);
    if (!m_pTagService->lastError().isEmpty())
    {
        QMessageBox::warning(this, QString::fromUtf8("Manage Tags"), m_pTagService->lastError());
        return;
    }

    QVector<qint64> vecSelectedIds;
    for (int i = 0; i < vecBoundTags.size(); ++i)
        vecSelectedIds.append(vecBoundTags.at(i).id());

    QCSnippetTagDialog dialog(this);
    dialog.setAvailableTags(vecAvailableTags);
    dialog.setSelectedTagIds(vecSelectedIds);
    if (QDialog::Accepted != dialog.exec())
        return;

    QVector<qint64> vecFinalIds = dialog.selectedTagIds();
    const QString strNewTagName = dialog.newTagName().trimmed();
    if (!strNewTagName.isEmpty())
    {
        qint64 nExistingTagId = 0;
        for (int i = 0; i < vecAvailableTags.size(); ++i)
        {
            if (vecAvailableTags.at(i).name().compare(strNewTagName, Qt::CaseInsensitive) == 0)
            {
                nExistingTagId = vecAvailableTags.at(i).id();
                break;
            }
        }

        if (nExistingTagId > 0)
        {
            vecFinalIds.append(nExistingTagId);
        }
        else
        {
            QCTag newTag;
            newTag.setName(strNewTagName);
            if (!m_pTagService->createTag(&newTag))
            {
                QMessageBox::warning(this, QString::fromUtf8("Manage Tags"), m_pTagService->lastError());
                return;
            }

            vecFinalIds.append(newTag.id());
        }
    }

    if (!m_pTagService->replaceSnippetTags(nSnippetId, vecFinalIds))
    {
        QMessageBox::warning(this, QString::fromUtf8("Manage Tags"), m_pTagService->lastError());
        return;
    }

    loadTagFilterOptions();
    refreshSnippetTagsDisplay(nSnippetId);
    statusBar()->showMessage(QString::fromUtf8("Snippet tags updated."), 4000);
}

void QCMainWindow::onSessionSelectionChanged()
{
    const qint64 nSessionId = currentSessionId();
    showSessionSummary(nSessionId);
    loadSnippets(nSessionId);
}

void QCMainWindow::onSnippetSelectionChanged()
{
    showSnippetDetails(currentSnippetId());
}

void QCMainWindow::onSnippetFilterChanged()
{
    applySnippetFilters();
}

void QCMainWindow::onClearSearch()
{
    if (nullptr == m_pSnippetSearchLineEdit)
        return;

    m_pSnippetSearchLineEdit->clear();
    applySnippetFilters();
    statusBar()->showMessage(QString::fromUtf8("Search cleared."), 3000);
}

void QCMainWindow::restoreDefaultFilters()
{
    if (nullptr != m_pSnippetSearchLineEdit)
        m_pSnippetSearchLineEdit->clear();
    if (nullptr != m_pFavoriteOnlyCheckBox)
        m_pFavoriteOnlyCheckBox->setChecked(false);
    if (nullptr != m_pReviewOnlyCheckBox)
        m_pReviewOnlyCheckBox->setChecked(false);
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
    QCCreateSessionDialog dialog(this);
    if (QDialog::Accepted != dialog.exec())
        return;

    QCStudySession session;
    session.setTitle(dialog.title());
    session.setCourseName(dialog.courseName());
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

void QCMainWindow::onSummarizeSnippet()
{
    const qint64 nSnippetId = currentSnippetId();
    if (nSnippetId <= 0)
    {
        QMessageBox::information(this, QString::fromUtf8("Summarize Snippet"), QString::fromUtf8("Select a snippet first."));
        return;
    }

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
    updateAiActionState();
    statusBar()->showMessage(QString::fromUtf8("Generating session summary in background..."));
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

    const QCAiRuntimeSettings defaultAiSettings = m_pSettingsService->defaultAiSettings();
    QCAiRuntimeSettings aiSettings;
    if (!m_pSettingsService->loadAiSettings(&aiSettings))
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
    dialog.setDefaultState(defaultAiSettings,
                           m_pSettingsService->defaultScreenshotSaveDirectory(),
                           m_pSettingsService->defaultExportDirectory(),
                           m_pSettingsService->defaultCopyImportedImageToCaptureDirectory());
    dialog.setDialogState(aiSettings,
                          strScreenshotDirectory,
                          strExportDirectory,
                          bDefaultCopyImportedImage);
    if (QDialog::Accepted != dialog.exec())
        return;

    if (!m_pSettingsService->saveAiSettings(dialog.settings()))
    {
        QMessageBox::warning(this, QString::fromUtf8("Settings"), m_pSettingsService->lastError());
        return;
    }

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

    statusBar()->showMessage(QString::fromUtf8("Settings saved."), 4000);
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

    const QString strSuggestedOutputPath = strExportDirectory.trimmed().isEmpty()
        ? QString::fromUtf8("qtclip_export.md")
        : QDir(strExportDirectory).filePath(QString::fromUtf8("qtclip_export.md"));

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
    QMessageBox::information(this,
                             QString::fromUtf8("Export Markdown"),
                             QString::fromUtf8("Markdown exported to:\n%1").arg(strOutputFilePath));
}

void QCMainWindow::onRefresh()
{
    loadSessions();
    statusBar()->showMessage(QString::fromUtf8("Refreshed."), 3000);
}
















































