#ifndef QTCLIP_QCSCREENSHOTOVERLAY_H_
#define QTCLIP_QCSCREENSHOTOVERLAY_H_

// File: qcscreenshotoverlay.h
// Author: ZZMI1
// Created: 2026-03-23
// Description: Declares the minimal region selection overlay used by the first QtClip area capture workflow.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include <QPoint>
#include <QRect>
#include <QString>
#include <QWidget>

class QEventLoop;

class QCScreenshotOverlay : public QWidget
{
public:
    explicit QCScreenshotOverlay(QWidget *pParent = nullptr);
    virtual ~QCScreenshotOverlay() override;

    bool selectRegion(QRect *pSelectedRect);
    bool wasCancelled() const;
    bool wasSelectionTooSmall() const;
    QString lastError() const;
    void clearError() const;

protected:
    virtual void paintEvent(QPaintEvent *pEvent) override;
    virtual void mousePressEvent(QMouseEvent *pEvent) override;
    virtual void mouseMoveEvent(QMouseEvent *pEvent) override;
    virtual void mouseReleaseEvent(QMouseEvent *pEvent) override;
    virtual void keyPressEvent(QKeyEvent *pEvent) override;

private:
    QCScreenshotOverlay(const QCScreenshotOverlay& other);
    QCScreenshotOverlay& operator=(const QCScreenshotOverlay& other);

    QRect selectionRect() const;
    QRect selectionRectGlobal() const;
    QPoint clampToOverlay(const QPoint& pointPosition) const;
    QString selectionSizeText(const QRect& rectSelection) const;
    void finishSelection(bool bAccepted);
    void setLastError(const QString& strError) const;

private:
    bool m_bSelecting;
    bool m_bAccepted;
    bool m_bCancelled;
    bool m_bSelectionTooSmall;
    QPoint m_pointSelectionStart;
    QPoint m_pointSelectionEnd;
    QRect m_rectScreenGeometry;
    QEventLoop *m_pEventLoop;
    mutable QString m_strLastError;
};

#endif // QTCLIP_QCSCREENSHOTOVERLAY_H_
