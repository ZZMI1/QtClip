#ifndef QTCLIP_QCSNIPPETTAGDIALOG_H_
#define QTCLIP_QCSNIPPETTAGDIALOG_H_

// File: qcsnippettagdialog.h
// Author: ZZMI1
// Created: 2026-03-23
// Description: Declares the minimal snippet tag dialog used by the QtClip UI workflow.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include <QDialog>
#include <QVector>

#include "../../domain/entities/qctag.h"

class QLineEdit;
class QListWidget;
class QPushButton;

class QCSnippetTagDialog : public QDialog
{
public:
    explicit QCSnippetTagDialog(QWidget *pParent = nullptr);
    virtual ~QCSnippetTagDialog() override;

    void setAvailableTags(const QVector<QCTag>& vecTags);
    void setSelectedTagIds(const QVector<qint64>& vecTagIds);
    QVector<qint64> selectedTagIds() const;
    QString newTagName() const;

private:
    QCSnippetTagDialog(const QCSnippetTagDialog& other);
    QCSnippetTagDialog& operator=(const QCSnippetTagDialog& other);

private:
    QListWidget *m_pTagListWidget;
    QLineEdit *m_pNewTagLineEdit;
    QPushButton *m_pSaveButton;
    QPushButton *m_pCancelButton;
};

#endif // QTCLIP_QCSNIPPETTAGDIALOG_H_
