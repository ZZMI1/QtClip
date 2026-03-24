// File: qcsnippettagdialog.cpp
// Author: ZZMI1
// Created: 2026-03-23
// Description: Implements the minimal snippet tag dialog used by the QtClip UI workflow.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include "qcsnippettagdialog.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

namespace
{
const int g_nTagNameRole = Qt::UserRole + 1;
const int g_nTagUsageCountRole = Qt::UserRole + 2;
}

QCSnippetTagDialog::QCSnippetTagDialog(QWidget *pParent)
    : QDialog(pParent)
    , m_bBindingEnabled(true)
    , m_pContextLabel(new QLabel(this))
    , m_pTagListWidget(new QListWidget(this))
    , m_pSelectedTagNameLineEdit(new QLineEdit(this))
    , m_pSelectedTagUsageLabel(new QLabel(this))
    , m_pNewTagLineEdit(new QLineEdit(this))
    , m_pRenameTagButton(new QPushButton(QString::fromUtf8("Rename Selected"), this))
    , m_pDeleteTagButton(new QPushButton(QString::fromUtf8("Delete Selected"), this))
    , m_pSaveButton(nullptr)
    , m_pCancelButton(nullptr)
    , m_hashRenamedTags()
    , m_vecDeletedTagIds()
{
    setWindowTitle(QString::fromUtf8("Manage Tags"));
    resize(460, 560);

    m_pContextLabel->setWordWrap(true);
    m_pTagListWidget->setAlternatingRowColors(true);
    m_pSelectedTagNameLineEdit->setPlaceholderText(QString::fromUtf8("Select one existing tag to rename"));
    m_pNewTagLineEdit->setPlaceholderText(QString::fromUtf8("Create a new tag on save"));
    m_pSelectedTagUsageLabel->setText(QString::fromUtf8("Used by 0 snippets."));

    QHBoxLayout *pEditButtonsLayout = new QHBoxLayout();
    pEditButtonsLayout->addWidget(m_pRenameTagButton);
    pEditButtonsLayout->addWidget(m_pDeleteTagButton);

    QDialogButtonBox *pButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_pSaveButton = pButtonBox->button(QDialogButtonBox::Ok);
    m_pCancelButton = pButtonBox->button(QDialogButtonBox::Cancel);
    m_pSaveButton->setText(QString::fromUtf8("Save"));
    m_pSaveButton->setDefault(true);

    QVBoxLayout *pLayout = new QVBoxLayout();
    pLayout->addWidget(m_pContextLabel);
    pLayout->addWidget(new QLabel(QString::fromUtf8("Tags"), this));
    pLayout->addWidget(m_pTagListWidget);
    pLayout->addWidget(new QLabel(QString::fromUtf8("Rename Selected Tag"), this));
    pLayout->addWidget(m_pSelectedTagNameLineEdit);
    pLayout->addWidget(m_pSelectedTagUsageLabel);
    pLayout->addLayout(pEditButtonsLayout);
    pLayout->addWidget(new QLabel(QString::fromUtf8("New Tag"), this));
    pLayout->addWidget(m_pNewTagLineEdit);
    pLayout->addWidget(pButtonBox);
    setLayout(pLayout);

    connect(m_pTagListWidget, &QListWidget::itemSelectionChanged, this, [this]() { updateSelectionState(); });
    connect(m_pRenameTagButton, &QPushButton::clicked, this, [this]() { renameSelectedTag(); });
    connect(m_pDeleteTagButton, &QPushButton::clicked, this, [this]() { deleteSelectedTag(); });
    connect(pButtonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(pButtonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    setContextText(QString::fromUtf8("Checked tags stay bound. You can also create, rename, or delete tags here."));
    updateSelectionState();
}

QCSnippetTagDialog::~QCSnippetTagDialog()
{
}

void QCSnippetTagDialog::setBindingEnabled(bool bEnabled)
{
    m_bBindingEnabled = bEnabled;
    for (int i = 0; i < m_pTagListWidget->count(); ++i)
    {
        QListWidgetItem *pItem = m_pTagListWidget->item(i);
        if (nullptr == pItem)
            continue;

        Qt::ItemFlags itemFlags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
        if (m_bBindingEnabled)
            itemFlags |= Qt::ItemIsUserCheckable;
        pItem->setFlags(itemFlags);
        if (!m_bBindingEnabled)
            pItem->setCheckState(Qt::Unchecked);
    }
}

void QCSnippetTagDialog::setContextText(const QString& strText)
{
    m_pContextLabel->setText(strText.trimmed());
}

void QCSnippetTagDialog::setAvailableTags(const QVector<QCTag>& vecTags)
{
    m_pTagListWidget->clear();
    m_hashRenamedTags.clear();
    m_vecDeletedTagIds.clear();

    for (int i = 0; i < vecTags.size(); ++i)
    {
        const QCTag& tag = vecTags.at(i);
        QListWidgetItem *pItem = new QListWidgetItem(m_pTagListWidget);
        Qt::ItemFlags itemFlags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
        if (m_bBindingEnabled)
            itemFlags |= Qt::ItemIsUserCheckable;
        pItem->setFlags(itemFlags);
        pItem->setCheckState(Qt::Unchecked);
        pItem->setData(Qt::UserRole, tag.id());
        pItem->setData(g_nTagNameRole, tag.name());
        pItem->setData(g_nTagUsageCountRole, 0);
        refreshItemDisplay(pItem);
    }

    updateSelectionState();
}

void QCSnippetTagDialog::setTagUsageCounts(const QHash<qint64, int>& hashTagUsageCounts)
{
    for (int i = 0; i < m_pTagListWidget->count(); ++i)
    {
        QListWidgetItem *pItem = m_pTagListWidget->item(i);
        if (nullptr == pItem)
            continue;

        const qint64 nTagId = pItem->data(Qt::UserRole).toLongLong();
        pItem->setData(g_nTagUsageCountRole, hashTagUsageCounts.value(nTagId, 0));
        refreshItemDisplay(pItem);
    }

    updateSelectionState();
}

void QCSnippetTagDialog::setSelectedTagIds(const QVector<qint64>& vecTagIds)
{
    for (int i = 0; i < m_pTagListWidget->count(); ++i)
    {
        QListWidgetItem *pItem = m_pTagListWidget->item(i);
        if (nullptr == pItem || !m_bBindingEnabled)
            continue;

        const qint64 nTagId = pItem->data(Qt::UserRole).toLongLong();
        pItem->setCheckState(vecTagIds.contains(nTagId) ? Qt::Checked : Qt::Unchecked);
    }
}

void QCSnippetTagDialog::setTagSelectionStates(const QHash<qint64, Qt::CheckState>& hashTagStates)
{
    for (int i = 0; i < m_pTagListWidget->count(); ++i)
    {
        QListWidgetItem *pItem = m_pTagListWidget->item(i);
        if (nullptr == pItem || !m_bBindingEnabled)
            continue;

        const qint64 nTagId = pItem->data(Qt::UserRole).toLongLong();
        pItem->setCheckState(hashTagStates.value(nTagId, Qt::Unchecked));
    }
}

QVector<qint64> QCSnippetTagDialog::selectedTagIds() const
{
    QVector<qint64> vecTagIds;
    for (int i = 0; i < m_pTagListWidget->count(); ++i)
    {
        const QListWidgetItem *pItem = m_pTagListWidget->item(i);
        if (nullptr != pItem && pItem->checkState() == Qt::Checked)
            vecTagIds.append(pItem->data(Qt::UserRole).toLongLong());
    }

    return vecTagIds;
}

QHash<qint64, Qt::CheckState> QCSnippetTagDialog::tagSelectionStates() const
{
    QHash<qint64, Qt::CheckState> hashTagStates;
    for (int i = 0; i < m_pTagListWidget->count(); ++i)
    {
        const QListWidgetItem *pItem = m_pTagListWidget->item(i);
        if (nullptr == pItem)
            continue;

        hashTagStates.insert(pItem->data(Qt::UserRole).toLongLong(), pItem->checkState());
    }

    return hashTagStates;
}

QString QCSnippetTagDialog::newTagName() const
{
    return m_pNewTagLineEdit->text().trimmed();
}

QHash<qint64, QString> QCSnippetTagDialog::renamedTags() const
{
    return m_hashRenamedTags;
}

QVector<qint64> QCSnippetTagDialog::deletedTagIds() const
{
    return m_vecDeletedTagIds;
}

QListWidgetItem *QCSnippetTagDialog::currentTagItem() const
{
    return m_pTagListWidget->currentItem();
}

QString QCSnippetTagDialog::tagNameForItem(const QListWidgetItem *pItem) const
{
    return nullptr == pItem ? QString() : pItem->data(g_nTagNameRole).toString().trimmed();
}

int QCSnippetTagDialog::usageCountForItem(const QListWidgetItem *pItem) const
{
    return nullptr == pItem ? 0 : pItem->data(g_nTagUsageCountRole).toInt();
}

QString QCSnippetTagDialog::buildDisplayText(const QString& strTagName, int nUsageCount) const
{
    return QString::fromUtf8("%1 (%2)").arg(strTagName.trimmed().isEmpty() ? QString::fromUtf8("Untitled Tag") : strTagName.trimmed()).arg(nUsageCount);
}

void QCSnippetTagDialog::updateSelectionState()
{
    QListWidgetItem *pCurrentItem = currentTagItem();
    const bool bHasCurrentItem = nullptr != pCurrentItem;
    if (bHasCurrentItem)
    {
        m_pSelectedTagNameLineEdit->setText(tagNameForItem(pCurrentItem));
        m_pSelectedTagUsageLabel->setText(QString::fromUtf8("Used by %1 snippet(s).").arg(usageCountForItem(pCurrentItem)));
    }
    else
    {
        m_pSelectedTagNameLineEdit->clear();
        m_pSelectedTagUsageLabel->setText(QString::fromUtf8("Used by 0 snippets."));
    }

    m_pSelectedTagNameLineEdit->setEnabled(bHasCurrentItem);
    m_pRenameTagButton->setEnabled(bHasCurrentItem);
    m_pDeleteTagButton->setEnabled(bHasCurrentItem);
}

void QCSnippetTagDialog::refreshItemDisplay(QListWidgetItem *pItem)
{
    if (nullptr == pItem)
        return;

    pItem->setText(buildDisplayText(tagNameForItem(pItem), usageCountForItem(pItem)));
}

void QCSnippetTagDialog::renameSelectedTag()
{
    QListWidgetItem *pCurrentItem = currentTagItem();
    if (nullptr == pCurrentItem)
        return;

    const QString strNewName = m_pSelectedTagNameLineEdit->text().trimmed();
    if (strNewName.isEmpty())
    {
        QMessageBox::warning(this, QString::fromUtf8("Manage Tags"), QString::fromUtf8("Tag name cannot be empty."));
        return;
    }

    for (int i = 0; i < m_pTagListWidget->count(); ++i)
    {
        QListWidgetItem *pItem = m_pTagListWidget->item(i);
        if (pItem == pCurrentItem)
            continue;

        if (tagNameForItem(pItem).compare(strNewName, Qt::CaseInsensitive) == 0)
        {
            QMessageBox::warning(this, QString::fromUtf8("Manage Tags"), QString::fromUtf8("A tag with the same name already exists."));
            return;
        }
    }

    const qint64 nTagId = pCurrentItem->data(Qt::UserRole).toLongLong();
    pCurrentItem->setData(g_nTagNameRole, strNewName);
    refreshItemDisplay(pCurrentItem);
    m_hashRenamedTags.insert(nTagId, strNewName);
}

void QCSnippetTagDialog::deleteSelectedTag()
{
    QListWidgetItem *pCurrentItem = currentTagItem();
    if (nullptr == pCurrentItem)
        return;

    const QString strTagName = tagNameForItem(pCurrentItem);
    const int nUsageCount = usageCountForItem(pCurrentItem);
    const QString strMessage = nUsageCount > 0
        ? QString::fromUtf8("Delete tag '%1'?\nIt is currently used by %2 snippet(s) and will be removed from them.").arg(strTagName, QString::number(nUsageCount))
        : QString::fromUtf8("Delete tag '%1'?").arg(strTagName);
    if (QMessageBox::Yes != QMessageBox::question(this,
                                                  QString::fromUtf8("Manage Tags"),
                                                  strMessage,
                                                  QMessageBox::Yes | QMessageBox::No,
                                                  QMessageBox::No))
    {
        return;
    }

    const qint64 nTagId = pCurrentItem->data(Qt::UserRole).toLongLong();
    if (!m_vecDeletedTagIds.contains(nTagId))
        m_vecDeletedTagIds.append(nTagId);

    m_hashRenamedTags.remove(nTagId);
    delete m_pTagListWidget->takeItem(m_pTagListWidget->row(pCurrentItem));
    updateSelectionState();
}
