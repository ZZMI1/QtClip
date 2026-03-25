// File: qcuitheme.cpp
// Author: ZZMI1
// Created: 2026-03-25
// Description: Implements app-wide Qt Widgets theme helpers for QtClip.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include "qcuitheme.h"

#include <QApplication>
#include <QFont>
#include <QString>

void QCApplyAppTheme(QApplication *pApplication)
{
    if (nullptr == pApplication)
        return;

    QFont appFont(QString::fromUtf8("Microsoft YaHei UI"), 10);
    appFont.setStyleStrategy(QFont::PreferAntialias);
    pApplication->setFont(appFont);

    const QString strStyleSheet = QString::fromUtf8(R"(
QWidget {
    background: #FAFAFA;
    color: #2A2A2A;
}

QMainWindow {
    background: #F7F8FA;
}

QMenuBar {
    background: #FFFFFF;
    border-bottom: 1px solid #E6E8EB;
    padding: 4px 6px;
}

QMenuBar::item {
    padding: 6px 10px;
    border-radius: 6px;
    background: transparent;
}

QMenuBar::item:selected {
    background: #F1F3F5;
}

QMenu {
    background: #FFFFFF;
    border: 1px solid #E4E7EC;
    padding: 6px;
}

QMenu::item {
    padding: 7px 18px;
    border-radius: 6px;
}

QMenu::item:selected {
    background: #EDF2F7;
}

QToolBar {
    background: #FFFFFF;
    border: none;
    border-bottom: 1px solid #E6E8EB;
    spacing: 6px;
    padding: 6px 10px;
}

QToolButton {
    background: transparent;
    border: 1px solid transparent;
    border-radius: 7px;
    padding: 6px 10px;
}

QToolButton:hover {
    background: #F5F7FA;
    border-color: #E4E7EC;
}

QToolButton:pressed {
    background: #EEF2F6;
}

QStatusBar {
    background: #FFFFFF;
    border-top: 1px solid #E6E8EB;
}

QPushButton {
    background: #FFFFFF;
    border: 1px solid #D9DEE5;
    border-radius: 8px;
    padding: 6px 12px;
    min-height: 18px;
}

QPushButton:hover {
    background: #F8FAFC;
    border-color: #C8D1DC;
}

QPushButton:pressed {
    background: #F1F4F8;
}

QPushButton:disabled {
    color: #9AA4AF;
    background: #F6F7F9;
    border-color: #E8EBEF;
}

QLineEdit,
QPlainTextEdit,
QComboBox,
QListWidget {
    background: #FFFFFF;
    border: 1px solid #DDE2E8;
    border-radius: 8px;
    selection-background-color: #DCEBFF;
    selection-color: #1F2D3D;
}

QLineEdit,
QComboBox {
    min-height: 22px;
    padding: 5px 8px;
}

QPlainTextEdit {
    padding: 8px;
}

QComboBox::drop-down {
    border: none;
    width: 24px;
}

QAbstractItemView {
    background: #FFFFFF;
    border: 1px solid #DDE2E8;
    selection-background-color: #DCEBFF;
}

QLabel#selectionContextLabel,
QLabel#viewSummaryLabel,
QLabel#aiStatusLabel {
    background: #FFFFFF;
    border: 1px solid #E5E8ED;
    border-radius: 8px;
    padding: 8px 10px;
    color: #4A5565;
}

QLabel#sessionPanelTitleLabel,
QLabel#snippetPanelTitleLabel,
QLabel#detailPanelTitleLabel {
    color: #111827;
    font-weight: 600;
    font-size: 14px;
    padding: 2px 2px 6px 2px;
}

QWidget#sessionPanelWidget,
QWidget#snippetPanelWidget,
QWidget#detailPanelWidget {
    background: #FFFFFF;
    border: 1px solid #E7EAF0;
    border-radius: 10px;
}

QWidget#sessionPanelWidget,
QWidget#snippetPanelWidget,
QWidget#detailPanelWidget,
QWidget#snippetCommandBarWidget,
QWidget#snippetFilterBarWidget,
QWidget#snippetQuickStateBarWidget {
    background: #FFFFFF;
}


QPushButton#sidebarNewSessionButton {
    background: #111827;
    color: #FFFFFF;
    border-color: #111827;
    font-weight: 600;
}

QPushButton#sidebarNewSessionButton:hover {
    background: #1F2937;
    border-color: #1F2937;
}

QPushButton#sidebarSettingsButton {
    background: #F8FAFC;
}

QPushButton#runSummaryButton {
    background: #EEF6FF;
    border-color: #C7DCF8;
}

QPushButton#topExportButton {
    background: #F3F4F6;
}
QSplitter::handle {
    background: #EEF1F5;
    width: 1px;
    margin: 10px 0;
}

QScrollBar:vertical,
QScrollBar:horizontal {
    background: transparent;
    border: none;
    margin: 2px;
}

QScrollBar::handle:vertical,
QScrollBar::handle:horizontal {
    background: #CDD5DF;
    border-radius: 5px;
    min-height: 24px;
    min-width: 24px;
}

QScrollBar::add-line,
QScrollBar::sub-line,
QScrollBar::add-page,
QScrollBar::sub-page {
    background: transparent;
    border: none;
}
)");

    pApplication->setStyleSheet(strStyleSheet);
}



