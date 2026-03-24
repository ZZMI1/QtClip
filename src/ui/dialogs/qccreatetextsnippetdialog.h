#ifndef QTCLIP_QCCREATETEXTSNIPPETDIALOG_H_
#define QTCLIP_QCCREATETEXTSNIPPETDIALOG_H_

// File: qccreatetextsnippetdialog.h
// Author: ZZMI1
// Created: 2026-03-23
// Description: Declares the minimal dialog used to create a text snippet in the QtClip demo UI.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include <QDialog>

class QLineEdit;
class QPlainTextEdit;

class QCCreateTextSnippetDialog : public QDialog
{
public:
    explicit QCCreateTextSnippetDialog(QWidget *pParent = nullptr);
    virtual ~QCCreateTextSnippetDialog() override;

    QString title() const;
    QString note() const;
    QString content() const;

    void setTitle(const QString& strTitle);
    void setNote(const QString& strNote);
    void setContent(const QString& strContent);

private:
    QCCreateTextSnippetDialog(const QCCreateTextSnippetDialog& other);
    QCCreateTextSnippetDialog& operator=(const QCCreateTextSnippetDialog& other);

private:
    QLineEdit *m_pTitleLineEdit;
    QPlainTextEdit *m_pNoteTextEdit;
    QPlainTextEdit *m_pContentTextEdit;
};

#endif // QTCLIP_QCCREATETEXTSNIPPETDIALOG_H_
