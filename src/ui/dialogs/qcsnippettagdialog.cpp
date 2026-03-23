// File: qcsnippettagdialog.cpp
// Author: ZZMI1
// Created: 2026-03-23
// Description: Implements the minimal snippet tag dialog used by the QtClip UI workflow.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include "qcsnippettagdialog.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QVBoxLayout>

QCSnippetTagDialog::QCSnippetTagDialog(QWidget *pParent)
    : QDialog(pParent)
    , m_pTagListWidget(new QListWidget(this))
    , m_pNewTagLineEdit(new QLineEdit(this))
    , m_pSaveButton(nullptr)
    , m_pCancelButton(nullptr)
{
    setWindowTitle(QString::fromUtf8("Manage Tags"));
    resize(360, 420);

    m_pTagListWidget->setAlternatingRowColors(true);
    m_pNewTagLineEdit->setPlaceholderText(QString::fromUtf8("Create a new tag on save"));

    QDialogButtonBox *pButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_pSaveButton = pButtonBox->button(QDialogButtonBox::Ok);
    m_pCancelButton = pButtonBox->button(QDialogButtonBox::Cancel);
    m_pSaveButton->setText(QString::fromUtf8("Save"));
    m_pSaveButton->setDefault(true);

    QVBoxLayout *pLayout = new QVBoxLayout();
    pLayout->addWidget(new QLabel(QString::fromUtf8("Tags"), this));
    pLayout->addWidget(m_pTagListWidget);
    pLayout->addWidget(new QLabel(QString::fromUtf8("New Tag"), this));
    pLayout->addWidget(m_pNewTagLineEdit);
    pLayout->addWidget(pButtonBox);
    setLayout(pLayout);

    connect(pButtonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(pButtonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

QCSnippetTagDialog::~QCSnippetTagDialog()
{
}

void QCSnippetTagDialog::setAvailableTags(const QVector<QCTag>& vecTags)
{
    m_pTagListWidget->clear();

    for (int i = 0; i < vecTags.size(); ++i)
    {
        const QCTag& tag = vecTags.at(i);
        QListWidgetItem *pItem = new QListWidgetItem(tag.name(), m_pTagListWidget);
        pItem->setFlags(pItem->flags() | Qt::ItemIsUserCheckable);
        pItem->setCheckState(Qt::Unchecked);
        pItem->setData(Qt::UserRole, tag.id());
    }
}

void QCSnippetTagDialog::setSelectedTagIds(const QVector<qint64>& vecTagIds)
{
    for (int i = 0; i < m_pTagListWidget->count(); ++i)
    {
        QListWidgetItem *pItem = m_pTagListWidget->item(i);
        const qint64 nTagId = pItem->data(Qt::UserRole).toLongLong();
        pItem->setCheckState(vecTagIds.contains(nTagId) ? Qt::Checked : Qt::Unchecked);
    }
}

QVector<qint64> QCSnippetTagDialog::selectedTagIds() const
{
    QVector<qint64> vecTagIds;
    for (int i = 0; i < m_pTagListWidget->count(); ++i)
    {
        const QListWidgetItem *pItem = m_pTagListWidget->item(i);
        if (pItem->checkState() == Qt::Checked)
            vecTagIds.append(pItem->data(Qt::UserRole).toLongLong());
    }

    return vecTagIds;
}

QString QCSnippetTagDialog::newTagName() const
{
    return m_pNewTagLineEdit->text().trimmed();
}
