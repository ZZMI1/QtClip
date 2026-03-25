// File: qccreatetextsnippetdialog.cpp
// Author: ZZMI1
// Created: 2026-03-23
// Description: Implements the minimal dialog used to create a text snippet in the QtClip demo UI.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include "qccreatetextsnippetdialog.h"

#include <QApplication>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include "../common/qcuilocalization.h"

QCCreateTextSnippetDialog::QCCreateTextSnippetDialog(QWidget *pParent)
    : QDialog(pParent)
    , m_pTitleLineEdit(new QLineEdit(this))
    , m_pNoteTextEdit(new QPlainTextEdit(this))
    , m_pContentTextEdit(new QPlainTextEdit(this))
{
    setWindowTitle(QCUiText(QString::fromUtf8("新建文本 Snippet"), QString::fromUtf8("New Text Snippet")));
    m_pNoteTextEdit->setMinimumHeight(90);
    m_pContentTextEdit->setMinimumHeight(120);

    QFormLayout *pFormLayout = new QFormLayout();
    pFormLayout->addRow(QCUiText(QString::fromUtf8("标题"), QString::fromUtf8("Title")), m_pTitleLineEdit);
    pFormLayout->addRow(QCUiText(QString::fromUtf8("备注"), QString::fromUtf8("Note")), m_pNoteTextEdit);
    pFormLayout->addRow(QCUiText(QString::fromUtf8("内容"), QString::fromUtf8("Content")), m_pContentTextEdit);

    QDialogButtonBox *pButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(pButtonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(pButtonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    QVBoxLayout *pMainLayout = new QVBoxLayout();
    pMainLayout->addLayout(pFormLayout);
    pMainLayout->addWidget(pButtonBox);
    setLayout(pMainLayout);

    resize(460, 340);
}

QCCreateTextSnippetDialog::~QCCreateTextSnippetDialog()
{
}

QString QCCreateTextSnippetDialog::title() const
{
    return m_pTitleLineEdit->text().trimmed();
}

QString QCCreateTextSnippetDialog::note() const
{
    return m_pNoteTextEdit->toPlainText().trimmed();
}

QString QCCreateTextSnippetDialog::content() const
{
    return m_pContentTextEdit->toPlainText().trimmed();
}

void QCCreateTextSnippetDialog::setTitle(const QString& strTitle)
{
    m_pTitleLineEdit->setText(strTitle.trimmed());
}

void QCCreateTextSnippetDialog::setNote(const QString& strNote)
{
    m_pNoteTextEdit->setPlainText(strNote.trimmed());
}

void QCCreateTextSnippetDialog::setContent(const QString& strContent)
{
    m_pContentTextEdit->setPlainText(strContent.trimmed());
}


