#ifndef QTCLIP_QCAISETTINGSDIALOG_H_
#define QTCLIP_QCAISETTINGSDIALOG_H_

// File: qcaisettingsdialog.h
// Author: ZZMI1
// Created: 2026-03-23
// Description: Declares the minimal AI settings dialog used by the first QtClip UI workflow.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include <QFutureWatcher>
#include <QDialog>

#include "../../services/qcaiprocessservice.h"
#include "../../services/qcsettingsservice.h"

class QCheckBox;
class QLabel;
class QLineEdit;
class QPushButton;

class QCAiSettingsDialog : public QDialog
{
public:
    explicit QCAiSettingsDialog(QCAiProcessService *pAiProcessService,
                                QWidget *pParent = nullptr);
    virtual ~QCAiSettingsDialog() override;

    void setDialogState(const QCAiRuntimeSettings& aiSettings,
                        const QString& strScreenshotDirectory,
                        const QString& strExportDirectory,
                        bool bDefaultCopyImportedImageToCaptureDirectory);
    void setDefaultState(const QCAiRuntimeSettings& aiSettings,
                         const QString& strScreenshotDirectory,
                         const QString& strExportDirectory,
                         bool bDefaultCopyImportedImageToCaptureDirectory);
    QCAiRuntimeSettings settings() const;
    QString screenshotSaveDirectory() const;
    QString exportDirectory() const;
    bool defaultCopyImportedImageToCaptureDirectory() const;

private:
    QCAiSettingsDialog(const QCAiSettingsDialog& other);
    QCAiSettingsDialog& operator=(const QCAiSettingsDialog& other);

    void chooseScreenshotSaveDirectory();
    void chooseExportDirectory();
    void restoreDefaults();
    void startConnectionTest();
    void handleConnectionTestFinished();
    void updateControlState();
    void updateDirtyState();
    void updateWindowTitle();
    void updateConnectionTestState();
    void markCurrentStateAsSaved();

private:
    QCAiProcessService *m_pAiProcessService;
    QCheckBox *m_pUseMockCheckBox;
    QCheckBox *m_pAutoSummarizeImageSnippetCheckBox;
    QLineEdit *m_pBaseUrlLineEdit;
    QLineEdit *m_pApiKeyLineEdit;
    QLineEdit *m_pModelLineEdit;
    QLineEdit *m_pScreenshotSaveDirectoryLineEdit;
    QLineEdit *m_pExportDirectoryLineEdit;
    QCheckBox *m_pDefaultCopyImportedImageCheckBox;
    QLabel *m_pStateLabel;
    QLabel *m_pConnectionTestResultLabel;
    QPushButton *m_pScreenshotBrowseButton;
    QPushButton *m_pExportBrowseButton;
    QPushButton *m_pRestoreDefaultsButton;
    QPushButton *m_pTestConnectionButton;
    QPushButton *m_pSaveButton;
    QPushButton *m_pCancelButton;
    QFutureWatcher<QCAiConnectionTestResult> *m_pConnectionTestWatcher;

    QCAiRuntimeSettings m_initialAiSettings;
    QCAiRuntimeSettings m_defaultAiSettings;
    QString m_strInitialScreenshotSaveDirectory;
    QString m_strInitialExportDirectory;
    QString m_strDefaultScreenshotSaveDirectory;
    QString m_strDefaultExportDirectory;
    bool m_bInitialDefaultCopyImportedImageToCaptureDirectory;
    bool m_bDefaultCopyImportedImageToCaptureDirectory;
    bool m_bLoadingState;
    bool m_bConnectionTestRunning;
};

#endif // QTCLIP_QCAISETTINGSDIALOG_H_
