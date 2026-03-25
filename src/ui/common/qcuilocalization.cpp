// File: qcuilocalization.cpp
// Author: ZZMI1
// Created: 2026-03-25
// Description: Implements shared UI language helpers for bilingual text selection.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include "qcuilocalization.h"

#include <QApplication>

QString QCNormalizeUiLanguage(const QString& strLanguage)
{
    return strLanguage.trimmed().compare(QString::fromUtf8("en-US"), Qt::CaseInsensitive) == 0
        ? QString::fromUtf8("en-US")
        : QString::fromUtf8("zh-CN");
}

bool QCIsChineseUiLanguage(const QString& strLanguage)
{
    return QCNormalizeUiLanguage(strLanguage) != QString::fromUtf8("en-US");
}

QString QCResolveUiLanguage()
{
    if (nullptr == qApp)
        return QString::fromUtf8("zh-CN");

    return QCNormalizeUiLanguage(qApp->property("qtclip.uiLanguage").toString());
}

bool QCIsChineseUi()
{
    return QCIsChineseUiLanguage(QCResolveUiLanguage());
}

QString QCUiText(const QString& strChinese, const QString& strEnglish)
{
    return QCIsChineseUi() ? strChinese : strEnglish;
}
