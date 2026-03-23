#ifndef QTCLIP_QCSCREENCAPTURESERVICE_H_
#define QTCLIP_QCSCREENCAPTURESERVICE_H_

// File: qcscreencaptureservice.h
// Author: ZZMI1
// Created: 2026-03-23
// Description: Declares the screen capture service used by the first QtClip capture workflow.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include <QDateTime>
#include <QPixmap>
#include <QRect>
#include <QString>

class QCSettingsService;

struct QCScreenCaptureResult
{
    QString m_strFilePath;
    int m_nWidth;
    int m_nHeight;
    QDateTime m_dateTimeCreatedAt;
};

class QCScreenCaptureService
{
public:
    explicit QCScreenCaptureService(QCSettingsService *pSettingsService = nullptr);
    ~QCScreenCaptureService();

    bool capturePrimaryScreen(QCScreenCaptureResult *pCaptureResult) const;
    bool capturePrimaryScreenRegion(const QRect& rectCaptureRegion, QCScreenCaptureResult *pCaptureResult) const;
    bool previewImportedImageCopyPath(const QString& strSourceFilePath, QString *pstrPreviewFilePath) const;
    bool copyImportedImageToCaptureDirectory(const QString& strSourceFilePath, QString *pstrCopiedFilePath) const;

    QString lastError() const;
    void clearError() const;

private:
    QCScreenCaptureService(const QCScreenCaptureService& other);
    QCScreenCaptureService& operator=(const QCScreenCaptureService& other);

    QString resolveCaptureRootDirectory() const;
    QString buildCaptureFilePath(const QDateTime& dateTimeCapturedAt) const;
    QString buildImportedImageFilePath(const QString& strSourceFilePath) const;
    bool saveCapturePixmap(const QPixmap& pixmapCapture,
                           const QDateTime& dateTimeCapturedAt,
                           QCScreenCaptureResult *pCaptureResult) const;
    void setLastError(const QString& strError) const;

private:
    QCSettingsService *m_pSettingsService;
    mutable QString m_strLastError;
};

#endif // QTCLIP_QCSCREENCAPTURESERVICE_H_
