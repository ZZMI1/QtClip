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
#include <QVector>

#include "../../services/qcaiprocessservice.h"
#include "../../services/qcsettingsservice.h"

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;

class QCAiSettingsDialog : public QDialog
{
public:
    explicit QCAiSettingsDialog(QCAiProcessService *pAiProcessService,
                                QWidget *pParent = nullptr);
    virtual ~QCAiSettingsDialog() override;

    void setDialogState(const QVector<QCAiRuntimeSettings>& vecAiSettingsProfiles,
                        int nActiveAiProfileIndex,
                        const QString& strScreenshotDirectory,
                        const QString& strExportDirectory,
                        bool bDefaultCopyImportedImageToCaptureDirectory);
    void setDefaultState(const QVector<QCAiRuntimeSettings>& vecAiSettingsProfiles,
                         int nActiveAiProfileIndex,
                         const QString& strScreenshotDirectory,
                         const QString& strExportDirectory,
                         bool bDefaultCopyImportedImageToCaptureDirectory);
    QCAiRuntimeSettings settings() const;
    QVector<QCAiRuntimeSettings> aiSettingsProfiles() const;
    int activeAiProfileIndex() const;
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
    void switchAiProfile(int nProfileIndex);
    void applyProfileToEditors(const QCAiRuntimeSettings& aiSettings);
    void storeEditorStateToCurrentProfile();
    void updateAiProfileLabels();
    void updateControlState();
    void updateDirtyState();
    void updateWindowTitle();
    void updateConnectionTestState();
    void markCurrentStateAsSaved();

private:
    QCAiProcessService *m_pAiProcessService;
    QComboBox *m_pAiProfileComboBox;
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

    QVector<QCAiRuntimeSettings> m_vecAiSettingsProfiles;
    QVector<QCAiRuntimeSettings> m_vecInitialAiSettingsProfiles;
    QVector<QCAiRuntimeSettings> m_vecDefaultAiSettingsProfiles;
    QString m_strInitialScreenshotSaveDirectory;
    QString m_strInitialExportDirectory;
    QString m_strDefaultScreenshotSaveDirectory;
    QString m_strDefaultExportDirectory;
    bool m_bInitialDefaultCopyImportedImageToCaptureDirectory;
    bool m_bDefaultCopyImportedImageToCaptureDirectory;
    int m_nCurrentAiProfileIndex;
    int m_nInitialActiveAiProfileIndex;
    int m_nDefaultActiveAiProfileIndex;
    bool m_bLoadingState;
    bool m_bConnectionTestRunning;
};

#endif // QTCLIP_QCAISETTINGSDIALOG_H_
