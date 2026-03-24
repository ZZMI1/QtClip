#ifndef QTCLIP_QCSNIPPETTAGDIALOG_H_
#define QTCLIP_QCSNIPPETTAGDIALOG_H_

// File: qcsnippettagdialog.h
// Author: ZZMI1
// Created: 2026-03-23
// Description: Declares the minimal snippet tag dialog used by the QtClip UI workflow.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include <QDialog>
#include <QHash>
#include <QVector>

#include "../../domain/entities/qctag.h"

class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPushButton;

class QCSnippetTagDialog : public QDialog
{
public:
    explicit QCSnippetTagDialog(QWidget *pParent = nullptr);
    virtual ~QCSnippetTagDialog() override;

    void setBindingEnabled(bool bEnabled);
    void setContextText(const QString& strText);
    void setAvailableTags(const QVector<QCTag>& vecTags);
    void setTagUsageCounts(const QHash<qint64, int>& hashTagUsageCounts);
    void setSelectedTagIds(const QVector<qint64>& vecTagIds);
    void setTagSelectionStates(const QHash<qint64, Qt::CheckState>& hashTagStates);
    QVector<qint64> selectedTagIds() const;
    QHash<qint64, Qt::CheckState> tagSelectionStates() const;
    QString newTagName() const;
    QHash<qint64, QString> renamedTags() const;
    QVector<qint64> deletedTagIds() const;

private:
    QCSnippetTagDialog(const QCSnippetTagDialog& other);
    QCSnippetTagDialog& operator=(const QCSnippetTagDialog& other);

    QListWidgetItem *currentTagItem() const;
    QString tagNameForItem(const QListWidgetItem *pItem) const;
    int usageCountForItem(const QListWidgetItem *pItem) const;
    QString buildDisplayText(const QString& strTagName, int nUsageCount) const;
    void updateSelectionState();
    void refreshItemDisplay(QListWidgetItem *pItem);
    void renameSelectedTag();
    void deleteSelectedTag();

private:
    bool m_bBindingEnabled;
    QLabel *m_pContextLabel;
    QListWidget *m_pTagListWidget;
    QLineEdit *m_pSelectedTagNameLineEdit;
    QLabel *m_pSelectedTagUsageLabel;
    QLineEdit *m_pNewTagLineEdit;
    QPushButton *m_pRenameTagButton;
    QPushButton *m_pDeleteTagButton;
    QPushButton *m_pSaveButton;
    QPushButton *m_pCancelButton;
    QHash<qint64, QString> m_hashRenamedTags;
    QVector<qint64> m_vecDeletedTagIds;
};

#endif // QTCLIP_QCSNIPPETTAGDIALOG_H_
