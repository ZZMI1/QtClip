// File: qcscreenshotoverlay.cpp
// Author: ZZMI1
// Created: 2026-03-23
// Description: Implements the minimal region selection overlay used by the first QtClip area capture workflow.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include "qcscreenshotoverlay.h"

#include <QApplication>
#include <QApplication>
#include <QEventLoop>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPen>
#include <QScreen>

namespace
{
const int g_nMinimumSelectionSize = 16;
const int g_nOverlayPadding = 12;

bool IsChineseUi()
{
    return qApp->property("qtclip.uiLanguage").toString().trimmed().compare(QString::fromUtf8("en-US"), Qt::CaseInsensitive) != 0;
}

QString UiText(const QString& strChinese, const QString& strEnglish)
{
    return IsChineseUi() ? strChinese : strEnglish;
}
}

QCScreenshotOverlay::QCScreenshotOverlay(QWidget *pParent)
    : QWidget(pParent)
    , m_bSelecting(false)
    , m_bAccepted(false)
    , m_bCancelled(false)
    , m_bSelectionTooSmall(false)
    , m_pointSelectionStart()
    , m_pointSelectionEnd()
    , m_rectScreenGeometry()
    , m_pEventLoop(nullptr)
    , m_strLastError()
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setCursor(Qt::CrossCursor);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
}

QCScreenshotOverlay::~QCScreenshotOverlay()
{
}

bool QCScreenshotOverlay::selectRegion(QRect *pSelectedRect)
{
    clearError();

    if (nullptr == pSelectedRect)
    {
        setLastError(UiText(QString::fromUtf8("?????????"), QString::fromUtf8("Selection output pointer is null.")));
        return false;
    }

    QScreen *pPrimaryScreen = QGuiApplication::primaryScreen();
    if (nullptr == pPrimaryScreen)
    {
        setLastError(UiText(QString::fromUtf8("???????"), QString::fromUtf8("Primary screen is unavailable.")));
        return false;
    }

    m_bSelecting = false;
    m_bAccepted = false;
    m_bCancelled = false;
    m_bSelectionTooSmall = false;
    m_pointSelectionStart = QPoint();
    m_pointSelectionEnd = QPoint();
    m_rectScreenGeometry = pPrimaryScreen->geometry();

    setGeometry(m_rectScreenGeometry);
    showFullScreen();
    raise();
    activateWindow();
    setFocus(Qt::ActiveWindowFocusReason);
    QApplication::processEvents();

    QEventLoop eventLoop;
    m_pEventLoop = &eventLoop;
    eventLoop.exec();
    m_pEventLoop = nullptr;

    hide();
    QApplication::processEvents();

    if (!m_bAccepted)
        return false;

    *pSelectedRect = selectionRectGlobal();
    return pSelectedRect->isValid() && !pSelectedRect->isEmpty();
}

bool QCScreenshotOverlay::wasCancelled() const
{
    return m_bCancelled;
}

bool QCScreenshotOverlay::wasSelectionTooSmall() const
{
    return m_bSelectionTooSmall;
}

QString QCScreenshotOverlay::lastError() const
{
    return m_strLastError;
}

void QCScreenshotOverlay::clearError() const
{
    m_strLastError.clear();
}

void QCScreenshotOverlay::paintEvent(QPaintEvent *pEvent)
{
    Q_UNUSED(pEvent);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), QColor(0, 0, 0, 110));

    const QRect rectSelection = selectionRect();
    if (rectSelection.isValid() && !rectSelection.isEmpty())
    {
        painter.setCompositionMode(QPainter::CompositionMode_Clear);
        painter.fillRect(rectSelection, Qt::transparent);
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

        painter.setPen(QPen(QColor(77, 184, 255), 2));
        painter.setBrush(QColor(77, 184, 255, 40));
        painter.drawRect(rectSelection);

        painter.setPen(QPen(QColor(255, 255, 255, 180), 1, Qt::DashLine));
        painter.drawLine(QPoint(rectSelection.center().x(), rectSelection.top()), QPoint(rectSelection.center().x(), rectSelection.bottom()));
        painter.drawLine(QPoint(rectSelection.left(), rectSelection.center().y()), QPoint(rectSelection.right(), rectSelection.center().y()));

        const QString strSizeText = selectionSizeText(rectSelection);
        QFontMetrics fontMetrics = painter.fontMetrics();
        const QSize textSize(fontMetrics.horizontalAdvance(strSizeText) + 18, fontMetrics.height() + 10);
        QPoint pointLabelTopLeft = rectSelection.topLeft() + QPoint(0, -textSize.height() - 8);
        if (pointLabelTopLeft.y() < g_nOverlayPadding)
            pointLabelTopLeft = rectSelection.topLeft() + QPoint(0, 8);
        if (pointLabelTopLeft.x() + textSize.width() > width() - g_nOverlayPadding)
            pointLabelTopLeft.setX(width() - g_nOverlayPadding - textSize.width());
        if (pointLabelTopLeft.x() < g_nOverlayPadding)
            pointLabelTopLeft.setX(g_nOverlayPadding);

        const QRect rectLabel(pointLabelTopLeft, textSize);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(24, 24, 24, 220));
        painter.drawRoundedRect(rectLabel, 6, 6);
        painter.setPen(Qt::white);
        painter.drawText(rectLabel, Qt::AlignCenter, strSizeText);
    }

    const QString strHint = UiText(QString::fromUtf8("?????????? Esc ???"), QString::fromUtf8("Drag to capture a region. Press Esc to cancel."));
    QFontMetrics hintMetrics = painter.fontMetrics();
    const QRect rectHint(g_nOverlayPadding,
                         g_nOverlayPadding,
                         hintMetrics.horizontalAdvance(strHint) + 20,
                         hintMetrics.height() + 10);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(24, 24, 24, 200));
    painter.drawRoundedRect(rectHint, 6, 6);
    painter.setPen(Qt::white);
    painter.drawText(rectHint, Qt::AlignCenter, strHint);
}

void QCScreenshotOverlay::mousePressEvent(QMouseEvent *pEvent)
{
    if (nullptr == pEvent || pEvent->button() != Qt::LeftButton)
        return;

    clearError();
    m_bCancelled = false;
    m_bSelectionTooSmall = false;
    m_bSelecting = true;
    m_pointSelectionStart = clampToOverlay(pEvent->pos());
    m_pointSelectionEnd = m_pointSelectionStart;
    update();
}

void QCScreenshotOverlay::mouseMoveEvent(QMouseEvent *pEvent)
{
    if (nullptr == pEvent || !m_bSelecting)
        return;

    m_pointSelectionEnd = clampToOverlay(pEvent->pos());
    update();
}

void QCScreenshotOverlay::mouseReleaseEvent(QMouseEvent *pEvent)
{
    if (nullptr == pEvent || pEvent->button() != Qt::LeftButton || !m_bSelecting)
        return;

    m_bSelecting = false;
    m_pointSelectionEnd = clampToOverlay(pEvent->pos());
    update();

    const QRect rectSelected = selectionRect();
    if (rectSelected.width() < g_nMinimumSelectionSize || rectSelected.height() < g_nMinimumSelectionSize)
    {
        m_bSelectionTooSmall = true;
        setLastError(UiText(QString::fromUtf8("?????????? %1 x %1 ???"), QString::fromUtf8("Selected region is too small. Please select at least %1 x %1 pixels.")).arg(g_nMinimumSelectionSize));
        finishSelection(false);
        return;
    }

    m_bAccepted = true;
    finishSelection(true);
}

void QCScreenshotOverlay::keyPressEvent(QKeyEvent *pEvent)
{
    if (nullptr == pEvent)
        return;

    if (pEvent->key() == Qt::Key_Escape)
    {
        clearError();
        m_bCancelled = true;
        m_bSelecting = false;
        finishSelection(false);
        return;
    }

    QWidget::keyPressEvent(pEvent);
}

QRect QCScreenshotOverlay::selectionRect() const
{
    return QRect(m_pointSelectionStart, m_pointSelectionEnd).normalized();
}

QRect QCScreenshotOverlay::selectionRectGlobal() const
{
    const QRect rectLocal = selectionRect().intersected(rect());
    return QRect(m_rectScreenGeometry.topLeft() + rectLocal.topLeft(), rectLocal.size());
}

QPoint QCScreenshotOverlay::clampToOverlay(const QPoint& pointPosition) const
{
    const int nClampedX = qBound(rect().left(), pointPosition.x(), rect().right());
    const int nClampedY = qBound(rect().top(), pointPosition.y(), rect().bottom());
    return QPoint(nClampedX, nClampedY);
}

QString QCScreenshotOverlay::selectionSizeText(const QRect& rectSelection) const
{
    return QString::fromUtf8("%1 x %2").arg(rectSelection.width()).arg(rectSelection.height());
}

void QCScreenshotOverlay::finishSelection(bool bAccepted)
{
    m_bAccepted = bAccepted;
    if (nullptr != m_pEventLoop)
        m_pEventLoop->quit();
}

void QCScreenshotOverlay::setLastError(const QString& strError) const
{
    m_strLastError = strError;
}
