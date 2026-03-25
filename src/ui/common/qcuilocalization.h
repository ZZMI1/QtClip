// File: qcuilocalization.h
// Author: ZZMI1
// Created: 2026-03-25
// Description: Provides shared UI language helpers for bilingual text selection.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#ifndef QCUILOCALIZATION_H
#define QCUILOCALIZATION_H

#include <QString>

QString QCNormalizeUiLanguage(const QString& strLanguage);
bool QCIsChineseUiLanguage(const QString& strLanguage);
QString QCResolveUiLanguage();
bool QCIsChineseUi();
QString QCUiText(const QString& strChinese, const QString& strEnglish);

#endif // QCUILOCALIZATION_H
