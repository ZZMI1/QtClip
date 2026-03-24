// File: qccreateimagesnippetdialog.cpp
// Author: ZZMI1
// Created: 2026-03-23
// Description: Implements the minimal dialog used to create an image snippet in the QtClip demo UI.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.
#include "qccreateimagesnippetdialog.h"
#include <QCheckBox>
#include <QApplication>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include "../../services/qcscreencaptureservice.h"
namespace
{
bool IsChineseUi()
{
    return qApp->property("qtclip.uiLanguage").toString().trimmed().compare(QString::fromUtf8("en-US"), Qt::CaseInsensitive) != 0;
}

QString UiText(const QString& strChinese, const QString& strEnglish)
{
    return IsChineseUi() ? strChinese : strEnglish;
}
}


QCCreateImageSnippetDialog::QCCreateImageSnippetDialog(const QString& strInitialDirectory,
                                                       bool bDefaultCopyToCaptureDirectory,
                                                       const QString& strPreviewDirectory,
                                                       const QCScreenCaptureService *pScreenCaptureService,
                                                       QWidget *pParent)
    : QDialog(pParent)
    , m_strInitialDirectory(strInitialDirectory)
    , m_strPreviewDirectory(strPreviewDirectory)
    , m_pScreenCaptureService(pScreenCaptureService)
    , m_pTitleLineEdit(new QLineEdit(this))
    , m_pNoteTextEdit(new QPlainTextEdit(this))
    , m_pFilePathLineEdit(new QLineEdit(this))
    , m_pCopyToDefaultCaptureDirectoryCheckBox(new QCheckBox(UiText(QString::fromUtf8("?????????????"), QString::fromUtf8("Copy imported image to default capture directory")), this))
    , m_pImportModeDescriptionLabel(new QLabel(this))
    , m_pTargetPreviewLabel(new QLabel(this))
    , m_pBrowseButton(new QPushButton(UiText(QString::fromUtf8("??"), QString::fromUtf8("Browse")), this))
{
    setWindowTitle(UiText(QString::fromUtf8("??????"), QString::fromUtf8("New Image Snippet")));
    m_pNoteTextEdit->setMinimumHeight(90);
    m_pCopyToDefaultCaptureDirectoryCheckBox->setChecked(bDefaultCopyToCaptureDirectory);
    m_pImportModeDescriptionLabel->setWordWrap(true);
    m_pTargetPreviewLabel->setWordWrap(true);
    m_pCopyToDefaultCaptureDirectoryCheckBox->setToolTip(UiText(QString::fromUtf8("???????????????????????????????"), QString::fromUtf8("Unchecked: use the original image file. Checked: copy the image to the default capture directory first.")));
    connect(m_pBrowseButton, &QPushButton::clicked, this, [this]() { chooseImageFile(); });
    connect(m_pCopyToDefaultCaptureDirectoryCheckBox, &QCheckBox::toggled, this, [this]() {
        updateImportModeDescription();
        updateTargetPreview();
    });
    connect(m_pFilePathLineEdit, &QLineEdit::textChanged, this, [this]() {
        updateTargetPreview();
    });
    QHBoxLayout *pFileLayout = new QHBoxLayout();
    pFileLayout->addWidget(m_pFilePathLineEdit);
    pFileLayout->addWidget(m_pBrowseButton);
    QFormLayout *pFormLayout = new QFormLayout();
    pFormLayout->addRow(UiText(QString::fromUtf8("??"), QString::fromUtf8("Title")), m_pTitleLineEdit);
    pFormLayout->addRow(UiText(QString::fromUtf8("??"), QString::fromUtf8("Note")), m_pNoteTextEdit);
    pFormLayout->addRow(UiText(QString::fromUtf8("??"), QString::fromUtf8("Image")), pFileLayout);
    pFormLayout->addRow(QString(), m_pCopyToDefaultCaptureDirectoryCheckBox);
    pFormLayout->addRow(QString(), m_pImportModeDescriptionLabel);
    pFormLayout->addRow(QString(), m_pTargetPreviewLabel);
    QDialogButtonBox *pButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(pButtonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(pButtonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    QVBoxLayout *pMainLayout = new QVBoxLayout();
    pMainLayout->addLayout(pFormLayout);
    pMainLayout->addWidget(pButtonBox);
    setLayout(pMainLayout);
    updateImportModeDescription();
    updateTargetPreview();
    resize(600, 340);
}
QCCreateImageSnippetDialog::~QCCreateImageSnippetDialog()
{
}
QString QCCreateImageSnippetDialog::title() const
{
    return m_pTitleLineEdit->text().trimmed();
}
QString QCCreateImageSnippetDialog::note() const
{
    return m_pNoteTextEdit->toPlainText().trimmed();
}
QString QCCreateImageSnippetDialog::filePath() const
{
    return m_pFilePathLineEdit->text().trimmed();
}
bool QCCreateImageSnippetDialog::shouldCopyImportedImageToDefaultCaptureDirectory() const
{
    return m_pCopyToDefaultCaptureDirectoryCheckBox->isChecked();
}
void QCCreateImageSnippetDialog::chooseImageFile()
{
    QString strBrowseDirectory = m_pFilePathLineEdit->text().trimmed();
    if (strBrowseDirectory.isEmpty())
        strBrowseDirectory = m_strInitialDirectory.trimmed();
    if (!strBrowseDirectory.isEmpty() && QFileInfo(strBrowseDirectory).isFile())
        strBrowseDirectory = QFileInfo(strBrowseDirectory).absolutePath();
    const QString strFilePath = QFileDialog::getOpenFileName(this,
                                                             UiText(QString::fromUtf8("????"), QString::fromUtf8("Select Image")),
                                                             strBrowseDirectory,
                                                             QString::fromUtf8("Images (*.png *.jpg *.jpeg *.bmp *.gif)"));
    if (!strFilePath.isEmpty())
        m_pFilePathLineEdit->setText(strFilePath);
}
void QCCreateImageSnippetDialog::updateImportModeDescription()
{
    if (m_pCopyToDefaultCaptureDirectoryCheckBox->isChecked())
    {
        m_pImportModeDescriptionLabel->setText(UiText(QString::fromUtf8("???????????????????????????"), QString::fromUtf8("Checked: copy the image to the default capture directory, then use the copied file.")));
    }
    else
    {
        m_pImportModeDescriptionLabel->setText(UiText(QString::fromUtf8("???????????????????????"), QString::fromUtf8("Unchecked: use the original image file directly. The file is not moved or copied.")));
    }
}
void QCCreateImageSnippetDialog::updateTargetPreview()
{
    if (!m_pCopyToDefaultCaptureDirectoryCheckBox->isChecked())
    {
        m_pTargetPreviewLabel->setVisible(false);
        return;
    }
    m_pTargetPreviewLabel->setVisible(true);
    const QString strSelectedFilePath = filePath();
    if (strSelectedFilePath.isEmpty())
    {
        m_pTargetPreviewLabel->setText(QString::fromUtf8("Copy target directory: %1\nSelect an image to preview the copied file path.")
            .arg(m_strPreviewDirectory));
        return;
    }
    QString strPreviewPath;
    if (nullptr != m_pScreenCaptureService
        && m_pScreenCaptureService->previewImportedImageCopyPath(strSelectedFilePath, &strPreviewPath)
        && !strPreviewPath.trimmed().isEmpty())
    {
        m_pTargetPreviewLabel->setText(QString::fromUtf8("Copy target directory: %1\nCopy target path: %2")
            .arg(QFileInfo(strPreviewPath).absolutePath(), strPreviewPath));
        return;
    }
    const QFileInfo selectedFileInfo(strSelectedFilePath);
    strPreviewPath = QDir(m_strPreviewDirectory).filePath(selectedFileInfo.fileName());
    m_pTargetPreviewLabel->setText(QString::fromUtf8("Copy target directory: %1\nCopy target path: %2")
        .arg(m_strPreviewDirectory, strPreviewPath));
}
