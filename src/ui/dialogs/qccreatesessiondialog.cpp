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
#include <QPlainTextEdit>
#include <QVBoxLayout>

QCCreateSessionDialog::QCCreateSessionDialog(QWidget *pParent)
    : QDialog(pParent)
    , m_pTitleLineEdit(new QLineEdit(this))
    , m_pCourseNameLineEdit(new QLineEdit(this))
    , m_pDescriptionTextEdit(new QPlainTextEdit(this))
{
    setWindowTitle(QString::fromUtf8("New Session"));
    m_pDescriptionTextEdit->setMinimumHeight(90);

    QFormLayout *pFormLayout = new QFormLayout();
    pFormLayout->addRow(QString::fromUtf8("Title"), m_pTitleLineEdit);
    pFormLayout->addRow(QString::fromUtf8("Course"), m_pCourseNameLineEdit);
    pFormLayout->addRow(QString::fromUtf8("Description"), m_pDescriptionTextEdit);

    QDialogButtonBox *pButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(pButtonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(pButtonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    QVBoxLayout *pMainLayout = new QVBoxLayout();
    pMainLayout->addLayout(pFormLayout);
    pMainLayout->addWidget(pButtonBox);
    setLayout(pMainLayout);

    resize(420, 240);
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

QString QCCreateSessionDialog::description() const
{
    return m_pDescriptionTextEdit->toPlainText().trimmed();
}

void QCCreateSessionDialog::setTitle(const QString& strTitle)
{
    m_pTitleLineEdit->setText(strTitle.trimmed());
}

void QCCreateSessionDialog::setCourseName(const QString& strCourseName)
{
    m_pCourseNameLineEdit->setText(strCourseName.trimmed());
}

void QCCreateSessionDialog::setDescription(const QString& strDescription)
{
    m_pDescriptionTextEdit->setPlainText(strDescription.trimmed());
}
