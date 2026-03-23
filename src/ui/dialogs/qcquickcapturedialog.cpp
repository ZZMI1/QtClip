// File: qcquickcapturedialog.cpp
// Author: ZZMI1
// Created: 2026-03-23
// Description: Implements the minimal quick capture dialog used by the first QtClip screenshot workflow.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include "qcquickcapturedialog.h"

#include <QDialogButtonBox>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

QCQuickCaptureDialog::QCQuickCaptureDialog(const QString& strImagePath, QWidget *pParent)
    : QDialog(pParent)
    , m_strImagePath(strImagePath)
    , m_pPreviewLabel(new QLabel(this))
    , m_pTitleLineEdit(new QLineEdit(this))
    , m_pNoteTextEdit(new QPlainTextEdit(this))
    , m_pSaveButton(nullptr)
{
    setWindowTitle(QString::fromUtf8("Quick Capture"));

    m_pPreviewLabel->setMinimumSize(420, 240);
    m_pPreviewLabel->setAlignment(Qt::AlignCenter);
    m_pPreviewLabel->setFrameShape(QFrame::StyledPanel);
    m_pNoteTextEdit->setMinimumHeight(100);

    QPixmap pixmapPreview(m_strImagePath);
    if (!pixmapPreview.isNull())
    {
        m_pPreviewLabel->setPixmap(pixmapPreview.scaled(420,
                                                        240,
                                                        Qt::KeepAspectRatio,
                                                        Qt::SmoothTransformation));
    }
    else
    {
        m_pPreviewLabel->setText(QString::fromUtf8("Preview unavailable."));
    }

    m_pTitleLineEdit->setPlaceholderText(QFileInfo(m_strImagePath).baseName());

    QFormLayout *pFormLayout = new QFormLayout();
    pFormLayout->addRow(QString::fromUtf8("Preview"), m_pPreviewLabel);
    pFormLayout->addRow(QString::fromUtf8("Title"), m_pTitleLineEdit);
    pFormLayout->addRow(QString::fromUtf8("Note"), m_pNoteTextEdit);

    QDialogButtonBox *pButtonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    m_pSaveButton = pButtonBox->button(QDialogButtonBox::Save);
    if (nullptr != m_pSaveButton)
    {
        m_pSaveButton->setDefault(true);
        m_pSaveButton->setAutoDefault(true);
    }

    connect(pButtonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(pButtonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    QVBoxLayout *pMainLayout = new QVBoxLayout();
    pMainLayout->addLayout(pFormLayout);
    pMainLayout->addWidget(pButtonBox);
    setLayout(pMainLayout);

    m_pTitleLineEdit->setFocus();
    resize(620, 460);
}

QCQuickCaptureDialog::~QCQuickCaptureDialog()
{
}

QString QCQuickCaptureDialog::title() const
{
    return m_pTitleLineEdit->text().trimmed();
}

QString QCQuickCaptureDialog::note() const
{
    return m_pNoteTextEdit->toPlainText().trimmed();
}

QString QCQuickCaptureDialog::imagePath() const
{
    return m_strImagePath;
}
