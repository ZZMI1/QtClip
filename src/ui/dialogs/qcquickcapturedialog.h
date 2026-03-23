#ifndef QTCLIP_QCQUICKCAPTUREDIALOG_H_
#define QTCLIP_QCQUICKCAPTUREDIALOG_H_

// File: qcquickcapturedialog.h
// Author: ZZMI1
// Created: 2026-03-23
// Description: Declares the minimal quick capture dialog used by the first QtClip screenshot workflow.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include <QDialog>

class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;

class QCQuickCaptureDialog : public QDialog
{
public:
    explicit QCQuickCaptureDialog(const QString& strImagePath, QWidget *pParent = nullptr);
    virtual ~QCQuickCaptureDialog() override;

    QString title() const;
    QString note() const;
    QString imagePath() const;

private:
    QCQuickCaptureDialog(const QCQuickCaptureDialog& other);
    QCQuickCaptureDialog& operator=(const QCQuickCaptureDialog& other);

private:
    QString m_strImagePath;
    QLabel *m_pPreviewLabel;
    QLineEdit *m_pTitleLineEdit;
    QPlainTextEdit *m_pNoteTextEdit;
    QPushButton *m_pSaveButton;
};

#endif // QTCLIP_QCQUICKCAPTUREDIALOG_H_
