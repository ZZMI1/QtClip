#ifndef QTCLIP_QCCREATEIMAGESNIPPETDIALOG_H_
#define QTCLIP_QCCREATEIMAGESNIPPETDIALOG_H_
// File: qccreateimagesnippetdialog.h
// Author: ZZMI1
// Created: 2026-03-23
// Description: Declares the minimal dialog used to create an image snippet in the QtClip demo UI.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.
#include <QDialog>
class QCheckBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QCScreenCaptureService;
class QCCreateImageSnippetDialog : public QDialog
{
public:
    explicit QCCreateImageSnippetDialog(const QString& strInitialDirectory = QString(),
                                        bool bDefaultCopyToCaptureDirectory = false,
                                        const QString& strPreviewDirectory = QString(),
                                        const QCScreenCaptureService *pScreenCaptureService = nullptr,
                                        QWidget *pParent = nullptr);
    virtual ~QCCreateImageSnippetDialog() override;
    QString title() const;
    QString note() const;
    QString filePath() const;
    bool shouldCopyImportedImageToDefaultCaptureDirectory() const;
private:
    QCCreateImageSnippetDialog(const QCCreateImageSnippetDialog& other);
    QCCreateImageSnippetDialog& operator=(const QCCreateImageSnippetDialog& other);
    void chooseImageFile();
    void updateImportModeDescription();
    void updateTargetPreview();
private:
    QString m_strInitialDirectory;
    QString m_strPreviewDirectory;
    const QCScreenCaptureService *m_pScreenCaptureService;
    QLineEdit *m_pTitleLineEdit;
    QPlainTextEdit *m_pNoteTextEdit;
    QLineEdit *m_pFilePathLineEdit;
    QCheckBox *m_pCopyToDefaultCaptureDirectoryCheckBox;
    QLabel *m_pImportModeDescriptionLabel;
    QLabel *m_pTargetPreviewLabel;
    QPushButton *m_pBrowseButton;
};
#endif // QTCLIP_QCCREATEIMAGESNIPPETDIALOG_H_
