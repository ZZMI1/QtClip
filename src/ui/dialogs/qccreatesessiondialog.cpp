// File: qccreatesessiondialog.cpp
// Author: ZZMI1
// Created: 2026-03-23
// Description: Implements the minimal dialog used to create a study session in the QtClip demo UI.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include "qccreatesessiondialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QVBoxLayout>

QCCreateSessionDialog::QCCreateSessionDialog(QWidget *pParent)
    : QDialog(pParent)
    , m_pTitleLineEdit(new QLineEdit(this))
    , m_pCourseNameLineEdit(new QLineEdit(this))
{
    setWindowTitle(QString::fromUtf8("New Session"));

    QFormLayout *pFormLayout = new QFormLayout();
    pFormLayout->addRow(QString::fromUtf8("Title"), m_pTitleLineEdit);
    pFormLayout->addRow(QString::fromUtf8("Course"), m_pCourseNameLineEdit);

    QDialogButtonBox *pButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(pButtonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(pButtonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    QVBoxLayout *pMainLayout = new QVBoxLayout();
    pMainLayout->addLayout(pFormLayout);
    pMainLayout->addWidget(pButtonBox);
    setLayout(pMainLayout);

    resize(360, 120);
}

QCCreateSessionDialog::~QCCreateSessionDialog()
{
}

QString QCCreateSessionDialog::title() const
{
    return m_pTitleLineEdit->text().trimmed();
}

QString QCCreateSessionDialog::courseName() const
{
    return m_pCourseNameLineEdit->text().trimmed();
}
