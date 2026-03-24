// File: qcaisettingsdialog.cpp
// Author: ZZMI1
// Created: 2026-03-23
// Description: Implements the minimal AI settings dialog used by the first QtClip UI workflow.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include "qcaisettingsdialog.h"

#include <QtConcurrent/QtConcurrent>

#include <QCheckBox>
#include <QComboBox>
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

bool AreAiSettingsEqual(const QCAiRuntimeSettings& leftAiSettings,
                        const QCAiRuntimeSettings& rightAiSettings)
{
    return leftAiSettings.m_bUseMockProvider == rightAiSettings.m_bUseMockProvider
        && leftAiSettings.m_bAutoSummarizeImageSnippet == rightAiSettings.m_bAutoSummarizeImageSnippet
        && leftAiSettings.m_strBaseUrl == rightAiSettings.m_strBaseUrl
        && leftAiSettings.m_strApiKey == rightAiSettings.m_strApiKey
        && leftAiSettings.m_strModel == rightAiSettings.m_strModel;
}

int NormalizeProfileIndex(int nProfileIndex, int nProfileCount)
{
    if (nProfileCount <= 0)
        return 0;
    if (nProfileIndex < 0 || nProfileIndex >= nProfileCount)
        return 0;

    return nProfileIndex;
}

QVector<QCAiRuntimeSettings> EnsureProfileCount(const QVector<QCAiRuntimeSettings>& vecAiSettingsProfiles,
                                                int nProfileCount)
{
    QVector<QCAiRuntimeSettings> vecNormalizedProfiles = vecAiSettingsProfiles;
    while (vecNormalizedProfiles.size() < nProfileCount)
        vecNormalizedProfiles.append(QCAiRuntimeSettings());
    while (vecNormalizedProfiles.size() > nProfileCount)
        vecNormalizedProfiles.removeLast();

    return vecNormalizedProfiles;
}

QString DefaultConnectionTestHint(bool bChineseUi)
{
    return bChineseUi
        ? QString::fromUtf8("?????????????? AI ???")
        : QString::fromUtf8("Run a connection test to verify the selected AI configuration.");
}

QString BuildProfileDisplayLabel(const QCAiRuntimeSettings& aiSettings, int nProfileIndex)
{
    QString strSummary;
    if (aiSettings.m_bUseMockProvider)
    {
        strSummary = aiSettings.m_strModel.trimmed().isEmpty()
            ? QString::fromUtf8("Mock")
            : QString::fromUtf8("Mock / %1").arg(aiSettings.m_strModel.trimmed());
    }
    else if (!aiSettings.m_strModel.trimmed().isEmpty())
    {
        strSummary = aiSettings.m_strModel.trimmed();
    }
    else if (!aiSettings.m_strBaseUrl.trimmed().isEmpty())
    {
        strSummary = QString::fromUtf8("Configured Endpoint");
    }
    else
    {
        strSummary = QString::fromUtf8("Empty");
    }

    return QString::fromUtf8("Profile %1 - %2").arg(nProfileIndex + 1).arg(strSummary);
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
    , m_pAiProfileComboBox(new QComboBox(this))
    , m_pAppLanguageComboBox(new QComboBox(this))
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
    , m_vecAiSettingsProfiles(EnsureProfileCount(QVector<QCAiRuntimeSettings>(), 3))
    , m_vecInitialAiSettingsProfiles(EnsureProfileCount(QVector<QCAiRuntimeSettings>(), 3))
    , m_vecDefaultAiSettingsProfiles(EnsureProfileCount(QVector<QCAiRuntimeSettings>(), 3))
    , m_strInitialAppLanguage()
    , m_strDefaultAppLanguage()
    , m_strInitialScreenshotSaveDirectory()
    , m_strInitialExportDirectory()
    , m_strDefaultScreenshotSaveDirectory()
    , m_strDefaultExportDirectory()
    , m_bInitialDefaultCopyImportedImageToCaptureDirectory(false)
    , m_bDefaultCopyImportedImageToCaptureDirectory(false)
    , m_nCurrentAiProfileIndex(0)
    , m_nInitialActiveAiProfileIndex(0)
    , m_nDefaultActiveAiProfileIndex(0)
    , m_bLoadingState(false)
    , m_bConnectionTestRunning(false)
{
    setWindowTitle(QString::fromUtf8("?? / Settings"));
    resize(720, 600);

    m_pAppLanguageComboBox->addItem(QString::fromUtf8("??"), QString::fromUtf8("zh-CN"));
    m_pAppLanguageComboBox->addItem(QString::fromUtf8("English"), QString::fromUtf8("en-US"));

    for (int i = 0; i < m_vecAiSettingsProfiles.size(); ++i)
        m_pAiProfileComboBox->addItem(BuildProfileDisplayLabel(m_vecAiSettingsProfiles.at(i), i));

    m_pApiKeyLineEdit->setEchoMode(QLineEdit::Password);
    m_pApiKeyLineEdit->setPlaceholderText(QString::fromUtf8("Optional for mock provider"));
    m_pBaseUrlLineEdit->setPlaceholderText(QString::fromUtf8("https://api.example.com/v1"));
    m_pModelLineEdit->setPlaceholderText(QString::fromUtf8("mock-summary-v1"));
    m_pScreenshotSaveDirectoryLineEdit->setClearButtonEnabled(true);
    m_pExportDirectoryLineEdit->setClearButtonEnabled(true);
    m_pConnectionTestResultLabel->setWordWrap(true);
    m_pConnectionTestResultLabel->setText(DefaultConnectionTestHint(true));

    QLabel *pAiHintLabel = new QLabel(this);
    pAiHintLabel->setObjectName(QString::fromUtf8("aiHintLabel"));
    pAiHintLabel->setWordWrap(true);
    QFormLayout *pAiFormLayout = new QFormLayout();
    pAiFormLayout->setObjectName(QString::fromUtf8("aiFormLayout"));
    pAiFormLayout->addRow(QString(), m_pAppLanguageComboBox);
    pAiFormLayout->addRow(QString(), m_pAiProfileComboBox);
    pAiFormLayout->addRow(QString(), m_pUseMockCheckBox);
    pAiFormLayout->addRow(QString(), m_pAutoSummarizeImageSnippetCheckBox);
    pAiFormLayout->addRow(QString(), m_pBaseUrlLineEdit);
    pAiFormLayout->addRow(QString(), m_pApiKeyLineEdit);
    pAiFormLayout->addRow(QString(), m_pModelLineEdit);

    QHBoxLayout *pAiTestLayout = new QHBoxLayout();
    pAiTestLayout->addWidget(m_pTestConnectionButton, 0);
    pAiTestLayout->addWidget(m_pConnectionTestResultLabel, 1);

    QVBoxLayout *pAiLayout = new QVBoxLayout();
    pAiLayout->addWidget(pAiHintLabel);
    pAiLayout->addLayout(pAiFormLayout);
    pAiLayout->addLayout(pAiTestLayout);
    QGroupBox *pAiGroupBox = new QGroupBox(QString::fromUtf8("AI"), this);
    pAiGroupBox->setLayout(pAiLayout);

    QLabel *pPathsHintLabel = new QLabel(this);
    pPathsHintLabel->setObjectName(QString::fromUtf8("pathsHintLabel"));
    pPathsHintLabel->setWordWrap(true);
    QHBoxLayout *pScreenshotDirectoryLayout = new QHBoxLayout();
    pScreenshotDirectoryLayout->addWidget(m_pScreenshotSaveDirectoryLineEdit, 1);
    pScreenshotDirectoryLayout->addWidget(m_pScreenshotBrowseButton);
    QHBoxLayout *pExportDirectoryLayout = new QHBoxLayout();
    pExportDirectoryLayout->addWidget(m_pExportDirectoryLineEdit, 1);
    pExportDirectoryLayout->addWidget(m_pExportBrowseButton);
    QFormLayout *pPathsFormLayout = new QFormLayout();
    pPathsFormLayout->setObjectName(QString::fromUtf8("pathsFormLayout"));
    pPathsFormLayout->addRow(QString(), pScreenshotDirectoryLayout);
    pPathsFormLayout->addRow(QString(), pExportDirectoryLayout);
    QVBoxLayout *pPathsLayout = new QVBoxLayout();
    pPathsLayout->addWidget(pPathsHintLabel);
    pPathsLayout->addLayout(pPathsFormLayout);
    QGroupBox *pPathsGroupBox = new QGroupBox(this);
    pPathsGroupBox->setObjectName(QString::fromUtf8("pathsGroupBox"));
    pPathsGroupBox->setLayout(pPathsLayout);

    QLabel *pImportHintLabel = new QLabel(this);
    pImportHintLabel->setObjectName(QString::fromUtf8("importHintLabel"));
    pImportHintLabel->setWordWrap(true);
    QVBoxLayout *pImportLayout = new QVBoxLayout();
    pImportLayout->addWidget(pImportHintLabel);
    pImportLayout->addWidget(m_pDefaultCopyImportedImageCheckBox);
    QGroupBox *pImportGroupBox = new QGroupBox(this);
    pImportGroupBox->setObjectName(QString::fromUtf8("importGroupBox"));
    pImportGroupBox->setLayout(pImportLayout);


    QDialogButtonBox *pButtonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    m_pRestoreDefaultsButton = pButtonBox->addButton(QString::fromUtf8("???? / Restore Defaults"), QDialogButtonBox::ResetRole);
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

    connect(m_pAppLanguageComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) { applyLocalizedTexts(); updateDirtyState(); updateConnectionTestState(); });
    connect(m_pAiProfileComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int nIndex) {
        switchAiProfile(nIndex);
    });
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

    applyLocalizedTexts();
    updateControlState();
    updateDirtyState();
    updateConnectionTestState();
}

QCAiSettingsDialog::~QCAiSettingsDialog()
{
}

void QCAiSettingsDialog::setDialogState(const QVector<QCAiRuntimeSettings>& vecAiSettingsProfiles,
                                       int nActiveAiProfileIndex,
                                       const QString& strAppLanguage,
                                       const QString& strScreenshotDirectory,
                                       const QString& strExportDirectory,
                                       bool bDefaultCopyImportedImageToCaptureDirectory)
{
    m_bLoadingState = true;
    m_vecAiSettingsProfiles = EnsureProfileCount(vecAiSettingsProfiles, m_pAiProfileComboBox->count());
    m_nCurrentAiProfileIndex = NormalizeProfileIndex(nActiveAiProfileIndex, m_vecAiSettingsProfiles.size());
    const QString strNormalizedAppLanguage = strAppLanguage.trimmed().compare(QString::fromUtf8("en-US"), Qt::CaseInsensitive) == 0
        ? QString::fromUtf8("en-US")
        : QString::fromUtf8("zh-CN");
    const int nLanguageIndex = m_pAppLanguageComboBox->findData(strNormalizedAppLanguage);
    m_pAppLanguageComboBox->setCurrentIndex(nLanguageIndex >= 0 ? nLanguageIndex : 0);
    m_pAiProfileComboBox->setCurrentIndex(m_nCurrentAiProfileIndex);
    applyProfileToEditors(m_vecAiSettingsProfiles.at(m_nCurrentAiProfileIndex));
    m_pScreenshotSaveDirectoryLineEdit->setText(strScreenshotDirectory.trimmed());
    m_pExportDirectoryLineEdit->setText(strExportDirectory.trimmed());
    m_pDefaultCopyImportedImageCheckBox->setChecked(bDefaultCopyImportedImageToCaptureDirectory);
    m_pConnectionTestResultLabel->setText(DefaultConnectionTestHint(isChineseUi()));
    m_bLoadingState = false;

    markCurrentStateAsSaved();
    updateControlState();
    updateDirtyState();
    updateConnectionTestState();
}

void QCAiSettingsDialog::setDefaultState(const QVector<QCAiRuntimeSettings>& vecAiSettingsProfiles,
                                        int nActiveAiProfileIndex,
                                        const QString& strAppLanguage,
                                        const QString& strScreenshotDirectory,
                                        const QString& strExportDirectory,
                                        bool bDefaultCopyImportedImageToCaptureDirectory)
{
    m_vecDefaultAiSettingsProfiles = EnsureProfileCount(vecAiSettingsProfiles, m_pAiProfileComboBox->count());
    m_nDefaultActiveAiProfileIndex = NormalizeProfileIndex(nActiveAiProfileIndex, m_vecDefaultAiSettingsProfiles.size());
    m_strDefaultAppLanguage = strAppLanguage.trimmed().compare(QString::fromUtf8("en-US"), Qt::CaseInsensitive) == 0 ? QString::fromUtf8("en-US") : QString::fromUtf8("zh-CN");
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

QVector<QCAiRuntimeSettings> QCAiSettingsDialog::aiSettingsProfiles() const
{
    QVector<QCAiRuntimeSettings> vecAiSettingsProfiles = EnsureProfileCount(m_vecAiSettingsProfiles, m_pAiProfileComboBox->count());
    const int nCurrentProfileIndex = NormalizeProfileIndex(activeAiProfileIndex(), vecAiSettingsProfiles.size());
    vecAiSettingsProfiles[nCurrentProfileIndex] = settings();
    return vecAiSettingsProfiles;
}

int QCAiSettingsDialog::activeAiProfileIndex() const
{
    return NormalizeProfileIndex(m_pAiProfileComboBox->currentIndex(), m_pAiProfileComboBox->count());
}

QString QCAiSettingsDialog::appLanguage() const
{
    return m_pAppLanguageComboBox->currentData().toString().trimmed().compare(QString::fromUtf8("en-US"), Qt::CaseInsensitive) == 0
        ? QString::fromUtf8("en-US")
        : QString::fromUtf8("zh-CN");
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
    m_vecAiSettingsProfiles = EnsureProfileCount(m_vecDefaultAiSettingsProfiles, m_pAiProfileComboBox->count());
    m_nCurrentAiProfileIndex = NormalizeProfileIndex(m_nDefaultActiveAiProfileIndex, m_vecAiSettingsProfiles.size());
    {
        const int nLanguageIndex = m_pAppLanguageComboBox->findData(m_strDefaultAppLanguage.isEmpty() ? QString::fromUtf8("zh-CN") : m_strDefaultAppLanguage);
        m_pAppLanguageComboBox->setCurrentIndex(nLanguageIndex >= 0 ? nLanguageIndex : 0);
    }
    m_pAiProfileComboBox->setCurrentIndex(m_nCurrentAiProfileIndex);
    applyProfileToEditors(m_vecAiSettingsProfiles.at(m_nCurrentAiProfileIndex));
    m_pScreenshotSaveDirectoryLineEdit->setText(m_strDefaultScreenshotSaveDirectory);
    m_pExportDirectoryLineEdit->setText(m_strDefaultExportDirectory);
    m_pDefaultCopyImportedImageCheckBox->setChecked(m_bDefaultCopyImportedImageToCaptureDirectory);
    m_pConnectionTestResultLabel->setText(DefaultConnectionTestHint(isChineseUi()));
    m_bLoadingState = false;

    updateControlState();
    updateDirtyState();
    updateConnectionTestState();
}

void QCAiSettingsDialog::startConnectionTest()
{
    if (m_bConnectionTestRunning)
        return;

    storeEditorStateToCurrentProfile();
    m_bConnectionTestRunning = true;
    m_pConnectionTestResultLabel->setText(uiText(QString::fromUtf8("???? AI ??..."), QString::fromUtf8("Testing AI connection...")));
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
            ? uiText(QString::fromUtf8("AI ???????"), QString::fromUtf8("AI connection test failed."))
            : testResult.m_strMessage;
        m_pConnectionTestResultLabel->setText(uiText(QString::fromUtf8("?????%1"), QString::fromUtf8("Test failed: %1")).arg(strMessage));
    }

    updateConnectionTestState();
}

void QCAiSettingsDialog::switchAiProfile(int nProfileIndex)
{
    if (m_bLoadingState)
        return;

    storeEditorStateToCurrentProfile();
    m_nCurrentAiProfileIndex = NormalizeProfileIndex(nProfileIndex, m_vecAiSettingsProfiles.size());

    m_bLoadingState = true;
    applyProfileToEditors(m_vecAiSettingsProfiles.at(m_nCurrentAiProfileIndex));
    m_pConnectionTestResultLabel->setText(DefaultConnectionTestHint(isChineseUi()));
    m_bLoadingState = false;

    updateControlState();
    updateDirtyState();
    updateConnectionTestState();
}

void QCAiSettingsDialog::applyProfileToEditors(const QCAiRuntimeSettings& aiSettings)
{
    m_pUseMockCheckBox->setChecked(aiSettings.m_bUseMockProvider);
    m_pAutoSummarizeImageSnippetCheckBox->setChecked(aiSettings.m_bAutoSummarizeImageSnippet);
    m_pBaseUrlLineEdit->setText(aiSettings.m_strBaseUrl);
    m_pApiKeyLineEdit->setText(aiSettings.m_strApiKey);
    m_pModelLineEdit->setText(aiSettings.m_strModel);
}

void QCAiSettingsDialog::storeEditorStateToCurrentProfile()
{
    if (m_bLoadingState || m_vecAiSettingsProfiles.isEmpty())
        return;

    const int nCurrentProfileIndex = NormalizeProfileIndex(m_nCurrentAiProfileIndex, m_vecAiSettingsProfiles.size());
    m_vecAiSettingsProfiles[nCurrentProfileIndex] = settings();
}

void QCAiSettingsDialog::updateAiProfileLabels()
{
    const QVector<QCAiRuntimeSettings> vecAiSettingsProfiles = aiSettingsProfiles();
    const int nCurrentProfileIndex = activeAiProfileIndex();
    m_pAiProfileComboBox->blockSignals(true);
    for (int i = 0; i < m_pAiProfileComboBox->count() && i < vecAiSettingsProfiles.size(); ++i)
        m_pAiProfileComboBox->setItemText(i, BuildProfileDisplayLabel(vecAiSettingsProfiles.at(i), i));
    m_pAiProfileComboBox->setCurrentIndex(nCurrentProfileIndex);
    m_pAiProfileComboBox->blockSignals(false);
}

void QCAiSettingsDialog::updateControlState()
{
    const bool bUseMockProvider = m_pUseMockCheckBox->isChecked();
    m_pAppLanguageComboBox->setEnabled(!m_bConnectionTestRunning);
    m_pAiProfileComboBox->setEnabled(!m_bConnectionTestRunning);
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

    storeEditorStateToCurrentProfile();
    const QVector<QCAiRuntimeSettings> vecCurrentAiSettingsProfiles = aiSettingsProfiles();
    bool bAiSettingsDirty = activeAiProfileIndex() != m_nInitialActiveAiProfileIndex
        || vecCurrentAiSettingsProfiles.size() != m_vecInitialAiSettingsProfiles.size();

    if (!bAiSettingsDirty)
    {
        for (int i = 0; i < vecCurrentAiSettingsProfiles.size(); ++i)
        {
            if (!AreAiSettingsEqual(vecCurrentAiSettingsProfiles.at(i), m_vecInitialAiSettingsProfiles.at(i)))
            {
                bAiSettingsDirty = true;
                break;
            }
        }
    }

    const bool bLanguageDirty = appLanguage() != (m_strInitialAppLanguage.trimmed().compare(QString::fromUtf8("en-US"), Qt::CaseInsensitive) == 0 ? QString::fromUtf8("en-US") : QString::fromUtf8("zh-CN"));
    const bool bDirty = bLanguageDirty
        || bAiSettingsDirty
        || !IsSamePath(screenshotSaveDirectory(), m_strInitialScreenshotSaveDirectory)
        || !IsSamePath(exportDirectory(), m_strInitialExportDirectory)
        || defaultCopyImportedImageToCaptureDirectory() != m_bInitialDefaultCopyImportedImageToCaptureDirectory;

    updateAiProfileLabels();
    m_pSaveButton->setEnabled(bDirty && !m_bConnectionTestRunning);
    m_pStateLabel->setText(bDirty ? uiText(QString::fromUtf8("????????"), QString::fromUtf8("Unsaved changes."))
                                  : uiText(QString::fromUtf8("????????"), QString::fromUtf8("All changes saved.")));
    updateWindowTitle();
}

void QCAiSettingsDialog::updateWindowTitle()
{
    const bool bDirty = nullptr != m_pSaveButton && m_pSaveButton->isEnabled();
    setWindowTitle(bDirty ? uiText(QString::fromUtf8("?? *"), QString::fromUtf8("Settings *"))
                          : uiText(QString::fromUtf8("??"), QString::fromUtf8("Settings")));
}

void QCAiSettingsDialog::updateConnectionTestState()
{
    const bool bCanTest = !m_bConnectionTestRunning
        && nullptr != m_pAiProcessService;

    m_pTestConnectionButton->setEnabled(bCanTest);
    m_pTestConnectionButton->setText(m_bConnectionTestRunning
        ? uiText(QString::fromUtf8("???..."), QString::fromUtf8("Testing..."))
        : uiText(QString::fromUtf8("?? AI ??"), QString::fromUtf8("Test AI Connection")));
    updateControlState();
    updateDirtyState();
}


bool QCAiSettingsDialog::isChineseUi() const
{
    return appLanguage().compare(QString::fromUtf8("en-US"), Qt::CaseInsensitive) != 0;
}

QString QCAiSettingsDialog::uiText(const QString& strChinese, const QString& strEnglish) const
{
    return isChineseUi() ? strChinese : strEnglish;
}

void QCAiSettingsDialog::applyLocalizedTexts()
{
    setWindowTitle(uiText(QString::fromUtf8("??"), QString::fromUtf8("Settings")));
    m_pUseMockCheckBox->setText(uiText(QString::fromUtf8("?? Mock Provider"), QString::fromUtf8("Use Mock Provider")));
    m_pAutoSummarizeImageSnippetCheckBox->setText(uiText(QString::fromUtf8("???????????"), QString::fromUtf8("Auto Summarize Image Snippet After Save")));
    m_pDefaultCopyImportedImageCheckBox->setText(uiText(QString::fromUtf8("??????????????"), QString::fromUtf8("Copy imported image to screenshot save directory by default")));
    m_pScreenshotBrowseButton->setText(uiText(QString::fromUtf8("??"), QString::fromUtf8("Browse")));
    m_pExportBrowseButton->setText(uiText(QString::fromUtf8("??"), QString::fromUtf8("Browse")));
    m_pRestoreDefaultsButton->setText(uiText(QString::fromUtf8("????"), QString::fromUtf8("Restore Defaults")));

    m_pApiKeyLineEdit->setPlaceholderText(uiText(QString::fromUtf8("Mock Provider ???"), QString::fromUtf8("Optional for mock provider")));
    m_pTestConnectionButton->setToolTip(uiText(QString::fromUtf8("??????????????????"), QString::fromUtf8("Tests the selected profile using the values currently shown in this dialog.")));
    m_pAiProfileComboBox->setToolTip(uiText(QString::fromUtf8("?????????? provider?base URL?API key ? model?"), QString::fromUtf8("Each profile stores its own provider mode, base URL, API key, and model.")));
    m_pScreenshotSaveDirectoryLineEdit->setToolTip(uiText(QString::fromUtf8("Capture Screen ? Capture Region ????????"), QString::fromUtf8("Used by Capture Screen and Capture Region output files.")));
    m_pExportDirectoryLineEdit->setToolTip(uiText(QString::fromUtf8("?? Markdown ???????????"), QString::fromUtf8("Used as the default folder when choosing a Markdown export file.")));
    m_pDefaultCopyImportedImageCheckBox->setToolTip(uiText(QString::fromUtf8("Import Image ??????????????????"), QString::fromUtf8("Uses the screenshot save directory as the default copy target for Import Image.")));

    if (QLabel *pAiHintLabel = findChild<QLabel *>(QString::fromUtf8("aiHintLabel")))
        pAiHintLabel->setText(uiText(QString::fromUtf8("AI ???????? provider ? mock provider?????? 3 ? AI ????????????????"), QString::fromUtf8("Use the AI section to configure a real provider or the mock provider. You can keep up to three AI configurations and switch the active one here.")));
    if (QLabel *pPathsHintLabel = findChild<QLabel *>(QString::fromUtf8("pathsHintLabel")))
        pPathsHintLabel->setText(uiText(QString::fromUtf8("????????? Capture Screen ? Capture Region?Markdown ??????????????????"), QString::fromUtf8("Screenshot save directory is used by Capture Screen and Capture Region. Markdown export directory is the default folder suggested when exporting Markdown.")));
    if (QLabel *pImportHintLabel = findChild<QLabel *>(QString::fromUtf8("importHintLabel")))
        pImportHintLabel->setText(uiText(QString::fromUtf8("????Import Image ??????????????????????? snippet?"), QString::fromUtf8("When enabled, Import Image starts in copy mode and copies the image into the screenshot save directory before saving the snippet.")));
    if (QGroupBox *pPathsGroupBox = findChild<QGroupBox *>(QString::fromUtf8("pathsGroupBox")))
        pPathsGroupBox->setTitle(uiText(QString::fromUtf8("??"), QString::fromUtf8("Paths")));
    if (QGroupBox *pImportGroupBox = findChild<QGroupBox *>(QString::fromUtf8("importGroupBox")))
        pImportGroupBox->setTitle(uiText(QString::fromUtf8("??"), QString::fromUtf8("Import")));

    if (QFormLayout *pAiFormLayout = findChild<QFormLayout *>(QString::fromUtf8("aiFormLayout")))
    {
        pAiFormLayout->setWidget(0, QFormLayout::LabelRole, new QLabel(uiText(QString::fromUtf8("????"), QString::fromUtf8("App Language")), this));
        pAiFormLayout->setWidget(1, QFormLayout::LabelRole, new QLabel(uiText(QString::fromUtf8("????"), QString::fromUtf8("Configuration Slot")), this));
        pAiFormLayout->setWidget(2, QFormLayout::LabelRole, new QLabel(uiText(QString::fromUtf8("Provider"), QString::fromUtf8("Provider")), this));
        pAiFormLayout->setWidget(3, QFormLayout::LabelRole, new QLabel(uiText(QString::fromUtf8("??????"), QString::fromUtf8("Auto Image Summary")), this));
        pAiFormLayout->setWidget(4, QFormLayout::LabelRole, new QLabel(QString::fromUtf8("Base URL"), this));
        pAiFormLayout->setWidget(5, QFormLayout::LabelRole, new QLabel(QString::fromUtf8("API Key"), this));
        pAiFormLayout->setWidget(6, QFormLayout::LabelRole, new QLabel(QString::fromUtf8("Model"), this));
    }

    if (QFormLayout *pPathsFormLayout = findChild<QFormLayout *>(QString::fromUtf8("pathsFormLayout")))
    {
        pPathsFormLayout->setWidget(0, QFormLayout::LabelRole, new QLabel(uiText(QString::fromUtf8("??????"), QString::fromUtf8("Screenshot Save Directory")), this));
        pPathsFormLayout->setWidget(1, QFormLayout::LabelRole, new QLabel(uiText(QString::fromUtf8("Markdown ????"), QString::fromUtf8("Markdown Export Directory")), this));
    }

    updateWindowTitle();
    if (!m_bConnectionTestRunning && (m_pConnectionTestResultLabel->text().trimmed().isEmpty() || m_pConnectionTestResultLabel->text() == DefaultConnectionTestHint(true) || m_pConnectionTestResultLabel->text() == DefaultConnectionTestHint(false)))
        m_pConnectionTestResultLabel->setText(DefaultConnectionTestHint(isChineseUi()));
}

void QCAiSettingsDialog::markCurrentStateAsSaved()
{
    storeEditorStateToCurrentProfile();
    m_strInitialAppLanguage = appLanguage();
    m_vecInitialAiSettingsProfiles = aiSettingsProfiles();
    m_strInitialScreenshotSaveDirectory = screenshotSaveDirectory();
    m_strInitialExportDirectory = exportDirectory();
    m_bInitialDefaultCopyImportedImageToCaptureDirectory = defaultCopyImportedImageToCaptureDirectory();
    m_nInitialActiveAiProfileIndex = activeAiProfileIndex();
}
