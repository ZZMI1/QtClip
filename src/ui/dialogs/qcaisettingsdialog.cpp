// File: qcaisettingsdialog.cpp
// Author: ZZMI1
// Created: 2026-03-23
// Description: Implements the minimal AI settings dialog used by the first QtClip UI workflow.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include "qcaisettingsdialog.h"

#include <QtConcurrent/QtConcurrent>

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace
{
bool IsSamePath(const QString& strLeftPath, const QString& strRightPath)
{
    return QDir::cleanPath(strLeftPath).compare(QDir::cleanPath(strRightPath), Qt::CaseInsensitive) == 0;
}

QCAiConnectionTestResult RunConnectionTest(QCAiProcessService *pAiProcessService,
                                           const QCAiRuntimeSettings& aiSettings)
{
    QCAiConnectionTestResult testResult;
    testResult.m_bSuccess = false;
    testResult.m_strProviderName.clear();
    testResult.m_strModelName = aiSettings.m_strModel;
    testResult.m_strRawResponse.clear();
    testResult.m_strMessage = QString::fromUtf8("AI process service is unavailable.");

    if (nullptr == pAiProcessService)
        return testResult;

    pAiProcessService->testConnection(aiSettings, &testResult);
    return testResult;
}
}

QCAiSettingsDialog::QCAiSettingsDialog(QCAiProcessService *pAiProcessService,
                                       QWidget *pParent)
    : QDialog(pParent)
    , m_pAiProcessService(pAiProcessService)
    , m_pUseMockCheckBox(new QCheckBox(QString::fromUtf8("Use Mock Provider"), this))
    , m_pAutoSummarizeImageSnippetCheckBox(new QCheckBox(QString::fromUtf8("Auto Summarize Image Snippet After Save"), this))
    , m_pBaseUrlLineEdit(new QLineEdit(this))
    , m_pApiKeyLineEdit(new QLineEdit(this))
    , m_pModelLineEdit(new QLineEdit(this))
    , m_pScreenshotSaveDirectoryLineEdit(new QLineEdit(this))
    , m_pExportDirectoryLineEdit(new QLineEdit(this))
    , m_pDefaultCopyImportedImageCheckBox(new QCheckBox(QString::fromUtf8("Copy imported image to screenshot save directory by default"), this))
    , m_pStateLabel(new QLabel(this))
    , m_pConnectionTestResultLabel(new QLabel(this))
    , m_pScreenshotBrowseButton(new QPushButton(QString::fromUtf8("Browse"), this))
    , m_pExportBrowseButton(new QPushButton(QString::fromUtf8("Browse"), this))
    , m_pRestoreDefaultsButton(nullptr)
    , m_pTestConnectionButton(new QPushButton(QString::fromUtf8("Test AI Connection"), this))
    , m_pSaveButton(nullptr)
    , m_pCancelButton(nullptr)
    , m_pConnectionTestWatcher(new QFutureWatcher<QCAiConnectionTestResult>(this))
    , m_initialAiSettings()
    , m_defaultAiSettings()
    , m_strInitialScreenshotSaveDirectory()
    , m_strInitialExportDirectory()
    , m_strDefaultScreenshotSaveDirectory()
    , m_strDefaultExportDirectory()
    , m_bInitialDefaultCopyImportedImageToCaptureDirectory(false)
    , m_bDefaultCopyImportedImageToCaptureDirectory(false)
    , m_bLoadingState(false)
    , m_bConnectionTestRunning(false)
{
    setWindowTitle(QString::fromUtf8("Settings"));
    resize(700, 520);

    m_pApiKeyLineEdit->setEchoMode(QLineEdit::Password);
    m_pApiKeyLineEdit->setPlaceholderText(QString::fromUtf8("Optional for mock provider"));
    m_pBaseUrlLineEdit->setPlaceholderText(QString::fromUtf8("https://api.example.com/v1"));
    m_pModelLineEdit->setPlaceholderText(QString::fromUtf8("mock-summary-v1"));
    m_pScreenshotSaveDirectoryLineEdit->setClearButtonEnabled(true);
    m_pExportDirectoryLineEdit->setClearButtonEnabled(true);
    m_pConnectionTestResultLabel->setWordWrap(true);
    m_pConnectionTestResultLabel->setText(QString::fromUtf8("Run a connection test to verify the current AI provider settings."));

    QLabel *pAiHintLabel = new QLabel(QString::fromUtf8("Controls the current AI provider, model, and whether image snippets summarize automatically after save."), this);
    pAiHintLabel->setWordWrap(true);
    QFormLayout *pAiFormLayout = new QFormLayout();
    pAiFormLayout->addRow(QString::fromUtf8("Provider"), m_pUseMockCheckBox);
    pAiFormLayout->addRow(QString::fromUtf8("Auto Image Summary"), m_pAutoSummarizeImageSnippetCheckBox);
    pAiFormLayout->addRow(QString::fromUtf8("Base URL"), m_pBaseUrlLineEdit);
    pAiFormLayout->addRow(QString::fromUtf8("API Key"), m_pApiKeyLineEdit);
    pAiFormLayout->addRow(QString::fromUtf8("Model"), m_pModelLineEdit);

    QHBoxLayout *pAiTestLayout = new QHBoxLayout();
    pAiTestLayout->addWidget(m_pTestConnectionButton, 0);
    pAiTestLayout->addWidget(m_pConnectionTestResultLabel, 1);

    QVBoxLayout *pAiLayout = new QVBoxLayout();
    pAiLayout->addWidget(pAiHintLabel);
    pAiLayout->addLayout(pAiFormLayout);
    pAiLayout->addLayout(pAiTestLayout);
    QGroupBox *pAiGroupBox = new QGroupBox(QString::fromUtf8("AI"), this);
    pAiGroupBox->setLayout(pAiLayout);

    QLabel *pPathsHintLabel = new QLabel(QString::fromUtf8("Screenshot save directory is used by Capture Screen and Capture Region. Markdown export directory is the default folder suggested when exporting Markdown."), this);
    pPathsHintLabel->setWordWrap(true);
    QHBoxLayout *pScreenshotDirectoryLayout = new QHBoxLayout();
    pScreenshotDirectoryLayout->addWidget(m_pScreenshotSaveDirectoryLineEdit, 1);
    pScreenshotDirectoryLayout->addWidget(m_pScreenshotBrowseButton);
    QHBoxLayout *pExportDirectoryLayout = new QHBoxLayout();
    pExportDirectoryLayout->addWidget(m_pExportDirectoryLineEdit, 1);
    pExportDirectoryLayout->addWidget(m_pExportBrowseButton);
    QFormLayout *pPathsFormLayout = new QFormLayout();
    pPathsFormLayout->addRow(QString::fromUtf8("Screenshot Save Directory"), pScreenshotDirectoryLayout);
    pPathsFormLayout->addRow(QString::fromUtf8("Markdown Export Directory"), pExportDirectoryLayout);
    QVBoxLayout *pPathsLayout = new QVBoxLayout();
    pPathsLayout->addWidget(pPathsHintLabel);
    pPathsLayout->addLayout(pPathsFormLayout);
    QGroupBox *pPathsGroupBox = new QGroupBox(QString::fromUtf8("Paths"), this);
    pPathsGroupBox->setLayout(pPathsLayout);

    QLabel *pImportHintLabel = new QLabel(QString::fromUtf8("When enabled, Import Image starts in copy mode and copies the image into the screenshot save directory before saving the snippet."), this);
    pImportHintLabel->setWordWrap(true);
    QVBoxLayout *pImportLayout = new QVBoxLayout();
    pImportLayout->addWidget(pImportHintLabel);
    pImportLayout->addWidget(m_pDefaultCopyImportedImageCheckBox);
    QGroupBox *pImportGroupBox = new QGroupBox(QString::fromUtf8("Import"), this);
    pImportGroupBox->setLayout(pImportLayout);

    m_pScreenshotSaveDirectoryLineEdit->setToolTip(QString::fromUtf8("Used by Capture Screen and Capture Region output files."));
    m_pExportDirectoryLineEdit->setToolTip(QString::fromUtf8("Used as the default folder when choosing a Markdown export file."));
    m_pDefaultCopyImportedImageCheckBox->setToolTip(QString::fromUtf8("Uses the screenshot save directory as the default copy target for Import Image."));
    m_pTestConnectionButton->setToolTip(QString::fromUtf8("Tests the current provider using the values currently shown in this dialog."));

    QDialogButtonBox *pButtonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    m_pRestoreDefaultsButton = pButtonBox->addButton(QString::fromUtf8("Restore Defaults"), QDialogButtonBox::ResetRole);
    m_pSaveButton = pButtonBox->button(QDialogButtonBox::Save);
    m_pCancelButton = pButtonBox->button(QDialogButtonBox::Cancel);
    m_pSaveButton->setDefault(true);

    m_pStateLabel->setMinimumWidth(180);

    QHBoxLayout *pFooterLayout = new QHBoxLayout();
    pFooterLayout->addWidget(m_pStateLabel, 1);
    pFooterLayout->addWidget(pButtonBox);

    QVBoxLayout *pLayout = new QVBoxLayout();
    pLayout->addWidget(pAiGroupBox);
    pLayout->addWidget(pPathsGroupBox);
    pLayout->addWidget(pImportGroupBox);
    pLayout->addLayout(pFooterLayout);
    setLayout(pLayout);

    connect(m_pUseMockCheckBox, &QCheckBox::toggled, this, [this]() {
        updateControlState();
        updateDirtyState();
    });
    connect(m_pAutoSummarizeImageSnippetCheckBox, &QCheckBox::toggled, this, [this]() { updateDirtyState(); });
    connect(m_pDefaultCopyImportedImageCheckBox, &QCheckBox::toggled, this, [this]() { updateDirtyState(); });
    connect(m_pBaseUrlLineEdit, &QLineEdit::textChanged, this, [this](const QString&) { updateDirtyState(); updateConnectionTestState(); });
    connect(m_pApiKeyLineEdit, &QLineEdit::textChanged, this, [this](const QString&) { updateDirtyState(); updateConnectionTestState(); });
    connect(m_pModelLineEdit, &QLineEdit::textChanged, this, [this](const QString&) { updateDirtyState(); updateConnectionTestState(); });
    connect(m_pScreenshotSaveDirectoryLineEdit, &QLineEdit::textChanged, this, [this](const QString&) { updateDirtyState(); });
    connect(m_pExportDirectoryLineEdit, &QLineEdit::textChanged, this, [this](const QString&) { updateDirtyState(); });
    connect(m_pScreenshotBrowseButton, &QPushButton::clicked, this, [this]() { chooseScreenshotSaveDirectory(); });
    connect(m_pExportBrowseButton, &QPushButton::clicked, this, [this]() { chooseExportDirectory(); });
    connect(m_pRestoreDefaultsButton, &QPushButton::clicked, this, [this]() { restoreDefaults(); });
    connect(m_pTestConnectionButton, &QPushButton::clicked, this, [this]() { startConnectionTest(); });
    connect(m_pConnectionTestWatcher, &QFutureWatcher<QCAiConnectionTestResult>::finished, this, [this]() { handleConnectionTestFinished(); });
    connect(pButtonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(pButtonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    updateControlState();
    updateDirtyState();
    updateConnectionTestState();
}

QCAiSettingsDialog::~QCAiSettingsDialog()
{
}

void QCAiSettingsDialog::setDialogState(const QCAiRuntimeSettings& aiSettings,
                                       const QString& strScreenshotDirectory,
                                       const QString& strExportDirectory,
                                       bool bDefaultCopyImportedImageToCaptureDirectory)
{
    m_bLoadingState = true;
    m_pUseMockCheckBox->setChecked(aiSettings.m_bUseMockProvider);
    m_pAutoSummarizeImageSnippetCheckBox->setChecked(aiSettings.m_bAutoSummarizeImageSnippet);
    m_pBaseUrlLineEdit->setText(aiSettings.m_strBaseUrl);
    m_pApiKeyLineEdit->setText(aiSettings.m_strApiKey);
    m_pModelLineEdit->setText(aiSettings.m_strModel);
    m_pScreenshotSaveDirectoryLineEdit->setText(strScreenshotDirectory.trimmed());
    m_pExportDirectoryLineEdit->setText(strExportDirectory.trimmed());
    m_pDefaultCopyImportedImageCheckBox->setChecked(bDefaultCopyImportedImageToCaptureDirectory);
    m_bLoadingState = false;

    markCurrentStateAsSaved();
    updateControlState();
    updateDirtyState();
    updateConnectionTestState();
}

void QCAiSettingsDialog::setDefaultState(const QCAiRuntimeSettings& aiSettings,
                                        const QString& strScreenshotDirectory,
                                        const QString& strExportDirectory,
                                        bool bDefaultCopyImportedImageToCaptureDirectory)
{
    m_defaultAiSettings = aiSettings;
    m_strDefaultScreenshotSaveDirectory = strScreenshotDirectory.trimmed();
    m_strDefaultExportDirectory = strExportDirectory.trimmed();
    m_bDefaultCopyImportedImageToCaptureDirectory = bDefaultCopyImportedImageToCaptureDirectory;
}

QCAiRuntimeSettings QCAiSettingsDialog::settings() const
{
    QCAiRuntimeSettings aiSettings;
    aiSettings.m_bUseMockProvider = m_pUseMockCheckBox->isChecked();
    aiSettings.m_bAutoSummarizeImageSnippet = m_pAutoSummarizeImageSnippetCheckBox->isChecked();
    aiSettings.m_strBaseUrl = m_pBaseUrlLineEdit->text().trimmed();
    aiSettings.m_strApiKey = m_pApiKeyLineEdit->text().trimmed();
    aiSettings.m_strModel = m_pModelLineEdit->text().trimmed();
    return aiSettings;
}

QString QCAiSettingsDialog::screenshotSaveDirectory() const
{
    return m_pScreenshotSaveDirectoryLineEdit->text().trimmed();
}

QString QCAiSettingsDialog::exportDirectory() const
{
    return m_pExportDirectoryLineEdit->text().trimmed();
}

bool QCAiSettingsDialog::defaultCopyImportedImageToCaptureDirectory() const
{
    return m_pDefaultCopyImportedImageCheckBox->isChecked();
}

void QCAiSettingsDialog::chooseScreenshotSaveDirectory()
{
    const QString strCurrentDirectory = screenshotSaveDirectory();
    const QString strSelectedDirectory = QFileDialog::getExistingDirectory(this,
                                                                           QString::fromUtf8("Select Screenshot Save Directory"),
                                                                           strCurrentDirectory,
                                                                           QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!strSelectedDirectory.isEmpty())
        m_pScreenshotSaveDirectoryLineEdit->setText(strSelectedDirectory);
}

void QCAiSettingsDialog::chooseExportDirectory()
{
    const QString strCurrentDirectory = exportDirectory();
    const QString strSelectedDirectory = QFileDialog::getExistingDirectory(this,
                                                                           QString::fromUtf8("Select Markdown Export Directory"),
                                                                           strCurrentDirectory,
                                                                           QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!strSelectedDirectory.isEmpty())
        m_pExportDirectoryLineEdit->setText(strSelectedDirectory);
}

void QCAiSettingsDialog::restoreDefaults()
{
    m_bLoadingState = true;
    m_pUseMockCheckBox->setChecked(m_defaultAiSettings.m_bUseMockProvider);
    m_pAutoSummarizeImageSnippetCheckBox->setChecked(m_defaultAiSettings.m_bAutoSummarizeImageSnippet);
    m_pBaseUrlLineEdit->setText(m_defaultAiSettings.m_strBaseUrl);
    m_pApiKeyLineEdit->setText(m_defaultAiSettings.m_strApiKey);
    m_pModelLineEdit->setText(m_defaultAiSettings.m_strModel);
    m_pScreenshotSaveDirectoryLineEdit->setText(m_strDefaultScreenshotSaveDirectory);
    m_pExportDirectoryLineEdit->setText(m_strDefaultExportDirectory);
    m_pDefaultCopyImportedImageCheckBox->setChecked(m_bDefaultCopyImportedImageToCaptureDirectory);
    m_bLoadingState = false;

    updateControlState();
    updateDirtyState();
    updateConnectionTestState();
}

void QCAiSettingsDialog::startConnectionTest()
{
    if (m_bConnectionTestRunning)
        return;

    m_bConnectionTestRunning = true;
    m_pConnectionTestResultLabel->setText(QString::fromUtf8("Testing AI connection..."));
    updateConnectionTestState();
    m_pConnectionTestWatcher->setFuture(QtConcurrent::run(RunConnectionTest,
                                                          m_pAiProcessService,
                                                          settings()));
}

void QCAiSettingsDialog::handleConnectionTestFinished()
{
    m_bConnectionTestRunning = false;
    const QCAiConnectionTestResult testResult = m_pConnectionTestWatcher->result();
    if (testResult.m_bSuccess)
    {
        m_pConnectionTestResultLabel->setText(testResult.m_strMessage);
    }
    else
    {
        const QString strMessage = testResult.m_strMessage.trimmed().isEmpty()
            ? QString::fromUtf8("AI connection test failed.")
            : testResult.m_strMessage;
        m_pConnectionTestResultLabel->setText(QString::fromUtf8("Test failed: %1").arg(strMessage));
    }

    updateConnectionTestState();
}

void QCAiSettingsDialog::updateControlState()
{
    const bool bUseMockProvider = m_pUseMockCheckBox->isChecked();
    m_pBaseUrlLineEdit->setEnabled(!bUseMockProvider && !m_bConnectionTestRunning);
    m_pApiKeyLineEdit->setEnabled(!bUseMockProvider && !m_bConnectionTestRunning);
    m_pModelLineEdit->setEnabled(!m_bConnectionTestRunning);
    m_pUseMockCheckBox->setEnabled(!m_bConnectionTestRunning);
    m_pAutoSummarizeImageSnippetCheckBox->setEnabled(!m_bConnectionTestRunning);
    m_pScreenshotSaveDirectoryLineEdit->setEnabled(!m_bConnectionTestRunning);
    m_pExportDirectoryLineEdit->setEnabled(!m_bConnectionTestRunning);
    m_pDefaultCopyImportedImageCheckBox->setEnabled(!m_bConnectionTestRunning);
    m_pScreenshotBrowseButton->setEnabled(!m_bConnectionTestRunning);
    m_pExportBrowseButton->setEnabled(!m_bConnectionTestRunning);
    if (nullptr != m_pRestoreDefaultsButton)
        m_pRestoreDefaultsButton->setEnabled(!m_bConnectionTestRunning);
    if (nullptr != m_pCancelButton)
        m_pCancelButton->setEnabled(!m_bConnectionTestRunning);
}

void QCAiSettingsDialog::updateDirtyState()
{
    if (m_bLoadingState)
        return;

    const QCAiRuntimeSettings currentAiSettings = settings();
    const bool bDirty = currentAiSettings.m_bUseMockProvider != m_initialAiSettings.m_bUseMockProvider
        || currentAiSettings.m_bAutoSummarizeImageSnippet != m_initialAiSettings.m_bAutoSummarizeImageSnippet
        || currentAiSettings.m_strBaseUrl != m_initialAiSettings.m_strBaseUrl
        || currentAiSettings.m_strApiKey != m_initialAiSettings.m_strApiKey
        || currentAiSettings.m_strModel != m_initialAiSettings.m_strModel
        || !IsSamePath(screenshotSaveDirectory(), m_strInitialScreenshotSaveDirectory)
        || !IsSamePath(exportDirectory(), m_strInitialExportDirectory)
        || defaultCopyImportedImageToCaptureDirectory() != m_bInitialDefaultCopyImportedImageToCaptureDirectory;

    m_pSaveButton->setEnabled(bDirty && !m_bConnectionTestRunning);
    m_pStateLabel->setText(bDirty ? QString::fromUtf8("Unsaved changes.")
                                  : QString::fromUtf8("All changes saved."));
    updateWindowTitle();
}

void QCAiSettingsDialog::updateWindowTitle()
{
    const bool bDirty = nullptr != m_pSaveButton && m_pSaveButton->isEnabled();
    setWindowTitle(bDirty ? QString::fromUtf8("Settings *")
                          : QString::fromUtf8("Settings"));
}

void QCAiSettingsDialog::updateConnectionTestState()
{
    const bool bCanTest = !m_bConnectionTestRunning
        && nullptr != m_pAiProcessService;

    m_pTestConnectionButton->setEnabled(bCanTest);
    m_pTestConnectionButton->setText(m_bConnectionTestRunning
        ? QString::fromUtf8("Testing...")
        : QString::fromUtf8("Test AI Connection"));
    updateControlState();
    updateDirtyState();
}


void QCAiSettingsDialog::markCurrentStateAsSaved()
{
    m_initialAiSettings = settings();
    m_strInitialScreenshotSaveDirectory = screenshotSaveDirectory();
    m_strInitialExportDirectory = exportDirectory();
    m_bInitialDefaultCopyImportedImageToCaptureDirectory = defaultCopyImportedImageToCaptureDirectory();
}


