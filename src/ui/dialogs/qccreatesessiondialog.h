#ifndef QTCLIP_QCCREATESESSIONDIALOG_H_
#define QTCLIP_QCCREATESESSIONDIALOG_H_

// File: qccreatesessiondialog.h
// Author: ZZMI1
// Created: 2026-03-23
// Description: Declares the minimal dialog used to create a study session in the QtClip demo UI.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include <QDialog>

class QLineEdit;

class QCCreateSessionDialog : public QDialog
{
public:
    explicit QCCreateSessionDialog(QWidget *pParent = nullptr);
    virtual ~QCCreateSessionDialog() override;

    QString title() const;
    QString courseName() const;

private:
    QCCreateSessionDialog(const QCCreateSessionDialog& other);
    QCCreateSessionDialog& operator=(const QCCreateSessionDialog& other);

private:
    QLineEdit *m_pTitleLineEdit;
    QLineEdit *m_pCourseNameLineEdit;
};

#endif // QTCLIP_QCCREATESESSIONDIALOG_H_
