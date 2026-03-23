// File: qcscreencaptureservice.cpp
// Author: ZZMI1
// Created: 2026-03-23
// Description: Implements the screen capture service used by the first QtClip capture workflow.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include "qcscreencaptureservice.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QPixmap>
#include <QScreen>
#include <QStandardPaths>

#include "qcsettingsservice.h"

namespace
{
const int g_nMinimumCaptureWidth = 16;
const int g_nMinimumCaptureHeight = 16;
}

QCScreenCaptureService::QCScreenCaptureService(QCSettingsService *pSettingsService)
    : m_pSettingsService(pSettingsService)
    , m_strLastError()
{
}

QCScreenCaptureService::~QCScreenCaptureService()
{
}

bool QCScreenCaptureService::capturePrimaryScreen(QCScreenCaptureResult *pCaptureResult) const
{
    clearError();

    if (nullptr == pCaptureResult)
    {
        setLastError(QString::fromUtf8("QCScreenCaptureResult output pointer is null."));
        return false;
    }

    QScreen *pPrimaryScreen = QGuiApplication::primaryScreen();
    if (nullptr == pPrimaryScreen)
    {
        setLastError(QString::fromUtf8("Primary screen is unavailable."));
        return false;
    }

    const QDateTime dateTimeCapturedAt = QDateTime::currentDateTimeUtc();
    const QPixmap pixmapCapture = pPrimaryScreen->grabWindow(0);
    if (pixmapCapture.isNull())
    {
        setLastError(QString::fromUtf8("Failed to capture the primary screen."));
        return false;
    }

    return saveCapturePixmap(pixmapCapture, dateTimeCapturedAt, pCaptureResult);
}

bool QCScreenCaptureService::capturePrimaryScreenRegion(const QRect& rectCaptureRegion,
                                                        QCScreenCaptureResult *pCaptureResult) const
{
    clearError();

    if (nullptr == pCaptureResult)
    {
        setLastError(QString::fromUtf8("QCScreenCaptureResult output pointer is null."));
        return false;
    }

    QScreen *pPrimaryScreen = QGuiApplication::primaryScreen();
    if (nullptr == pPrimaryScreen)
    {
        setLastError(QString::fromUtf8("Primary screen is unavailable."));
        return false;
    }

    const QRect rectScreenGeometry = pPrimaryScreen->geometry();
    const QRect rectNormalizedRegion = rectCaptureRegion.normalized();
    const QRect rectClippedRegion = rectNormalizedRegion.intersected(rectScreenGeometry);
    if (!rectClippedRegion.isValid() || rectClippedRegion.isEmpty())
    {
        setLastError(QString::fromUtf8("Capture region is outside the primary screen."));
        return false;
    }

    if (rectClippedRegion.width() < g_nMinimumCaptureWidth || rectClippedRegion.height() < g_nMinimumCaptureHeight)
    {
        setLastError(QString::fromUtf8("Selected region is too small. Please select at least %1 x %2 pixels.")
                         .arg(g_nMinimumCaptureWidth)
                         .arg(g_nMinimumCaptureHeight));
        return false;
    }

    const QDateTime dateTimeCapturedAt = QDateTime::currentDateTimeUtc();
    const QPixmap pixmapFullScreen = pPrimaryScreen->grabWindow(0);
    if (pixmapFullScreen.isNull())
    {
        setLastError(QString::fromUtf8("Failed to capture the primary screen before cropping the region."));
        return false;
    }

    const QRect rectLocalCrop(rectClippedRegion.topLeft() - rectScreenGeometry.topLeft(), rectClippedRegion.size());
    const QPixmap pixmapCropped = pixmapFullScreen.copy(rectLocalCrop);
    if (pixmapCropped.isNull())
    {
        setLastError(QString::fromUtf8("Failed to crop the selected capture region."));
        return false;
    }

    return saveCapturePixmap(pixmapCropped, dateTimeCapturedAt, pCaptureResult);
}

bool QCScreenCaptureService::previewImportedImageCopyPath(const QString& strSourceFilePath,
                                                          QString *pstrPreviewFilePath) const
{
    clearError();

    if (nullptr == pstrPreviewFilePath)
    {
        setLastError(QString::fromUtf8("Preview file path output pointer is null."));
        return false;
    }

    const QFileInfo sourceFileInfo(strSourceFilePath.trimmed());
    if (!sourceFileInfo.exists() || !sourceFileInfo.isFile())
    {
        setLastError(QString::fromUtf8("Imported image file does not exist."));
        return false;
    }

    const QString strTargetFilePath = buildImportedImageFilePath(sourceFileInfo.absoluteFilePath());
    if (strTargetFilePath.isEmpty())
    {
        setLastError(QString::fromUtf8("Failed to resolve copied import image preview path."));
        return false;
    }

    *pstrPreviewFilePath = strTargetFilePath;
    return true;
}

bool QCScreenCaptureService::copyImportedImageToCaptureDirectory(const QString& strSourceFilePath,
                                                                 QString *pstrCopiedFilePath) const
{
    clearError();

    if (nullptr == pstrCopiedFilePath)
    {
        setLastError(QString::fromUtf8("Copied file path output pointer is null."));
        return false;
    }

    const QFileInfo sourceFileInfo(strSourceFilePath.trimmed());
    if (!sourceFileInfo.exists() || !sourceFileInfo.isFile())
    {
        setLastError(QString::fromUtf8("Imported image file does not exist."));
        return false;
    }

    const QString strTargetFilePath = buildImportedImageFilePath(sourceFileInfo.absoluteFilePath());
    if (strTargetFilePath.isEmpty())
    {
        setLastError(QString::fromUtf8("Failed to resolve copied import image path."));
        return false;
    }

    if (!QFile::copy(sourceFileInfo.absoluteFilePath(), strTargetFilePath))
    {
        setLastError(QString::fromUtf8("Failed to copy imported image to %1").arg(strTargetFilePath));
        return false;
    }

    *pstrCopiedFilePath = strTargetFilePath;
    return true;
}

QString QCScreenCaptureService::lastError() const
{
    return m_strLastError;
}

void QCScreenCaptureService::clearError() const
{
    m_strLastError.clear();
}

QString QCScreenCaptureService::resolveCaptureRootDirectory() const
{
    if (nullptr != m_pSettingsService)
    {
        QString strConfiguredDirectory;
        if (m_pSettingsService->getScreenshotSaveDirectory(&strConfiguredDirectory)
            && !strConfiguredDirectory.trimmed().isEmpty())
        {
            return strConfiguredDirectory.trimmed();
        }
    }

    QString strCaptureRoot = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    if (strCaptureRoot.isEmpty())
        strCaptureRoot = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (strCaptureRoot.isEmpty())
        strCaptureRoot = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (strCaptureRoot.isEmpty())
        return QString();

    return QDir(strCaptureRoot).filePath(QString::fromUtf8("QtClip/Captures"));
}

QString QCScreenCaptureService::buildCaptureFilePath(const QDateTime& dateTimeCapturedAt) const
{
    const QString strCaptureRoot = resolveCaptureRootDirectory();
    if (strCaptureRoot.isEmpty())
        return QString();

    QDir rootDirectory(strCaptureRoot);
    if (!rootDirectory.exists() && !rootDirectory.mkpath(QString::fromUtf8(".")))
        return QString();

    const QString strFileName = QString::fromUtf8("capture_%1.png")
        .arg(dateTimeCapturedAt.toString(QString::fromUtf8("yyyyMMdd_HHmmss_zzz")));
    return rootDirectory.filePath(strFileName);
}

QString QCScreenCaptureService::buildImportedImageFilePath(const QString& strSourceFilePath) const
{
    const QString strCaptureRoot = resolveCaptureRootDirectory();
    if (strCaptureRoot.isEmpty())
        return QString();

    QDir rootDirectory(strCaptureRoot);
    if (!rootDirectory.exists() && !rootDirectory.mkpath(QString::fromUtf8(".")))
        return QString();

    const QFileInfo sourceFileInfo(strSourceFilePath);
    const QString strBaseName = sourceFileInfo.completeBaseName().trimmed().isEmpty()
        ? QString::fromUtf8("imported_image")
        : sourceFileInfo.completeBaseName().trimmed();
    const QString strSuffix = sourceFileInfo.completeSuffix().trimmed();
    const QString strExtension = strSuffix.isEmpty() ? QString() : QString::fromUtf8(".%1").arg(strSuffix);

    QString strCandidateFilePath = rootDirectory.filePath(sourceFileInfo.fileName());
    if (!QFileInfo::exists(strCandidateFilePath)
        && QDir::cleanPath(strCandidateFilePath).compare(QDir::cleanPath(sourceFileInfo.absoluteFilePath()), Qt::CaseInsensitive) != 0)
    {
        return strCandidateFilePath;
    }

    for (int nCounter = 1; nCounter < 10000; ++nCounter)
    {
        strCandidateFilePath = rootDirectory.filePath(QString::fromUtf8("%1_copy_%2%3")
            .arg(strBaseName)
            .arg(nCounter)
            .arg(strExtension));
        if (!QFileInfo::exists(strCandidateFilePath))
            return strCandidateFilePath;
    }

    return QString();
}

bool QCScreenCaptureService::saveCapturePixmap(const QPixmap& pixmapCapture,
                                               const QDateTime& dateTimeCapturedAt,
                                               QCScreenCaptureResult *pCaptureResult) const
{
    if (nullptr == pCaptureResult)
    {
        setLastError(QString::fromUtf8("QCScreenCaptureResult output pointer is null."));
        return false;
    }

    const QString strFilePath = buildCaptureFilePath(dateTimeCapturedAt);
    if (strFilePath.isEmpty())
    {
        setLastError(QString::fromUtf8("Failed to resolve screen capture file path."));
        return false;
    }

    if (!pixmapCapture.save(strFilePath, "PNG"))
    {
        setLastError(QString::fromUtf8("Failed to save screen capture image: %1").arg(strFilePath));
        return false;
    }

    pCaptureResult->m_strFilePath = strFilePath;
    pCaptureResult->m_nWidth = pixmapCapture.width();
    pCaptureResult->m_nHeight = pixmapCapture.height();
    pCaptureResult->m_dateTimeCreatedAt = dateTimeCapturedAt;
    return true;
}

void QCScreenCaptureService::setLastError(const QString& strError) const
{
    m_strLastError = strError;
}
