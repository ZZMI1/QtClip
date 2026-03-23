// File: qcsnippet.cpp
// Author: ZZMI1
// Created: 2026-03-23
// Description: Implements the snippet entity used by the QtClip domain layer.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include "qcsnippet.h"

QCSnippet::QCSnippet()
    : m_nId(0)
    , m_nSessionId(0)
    , m_snippetType(QCSnippetType::ImageSnippetType)
    , m_captureType(QCCaptureType::ScreenCaptureType)
    , m_strTitle()
    , m_strContentText()
    , m_strSummary()
    , m_strNote()
    , m_noteKind(QCNoteKind::ConceptNoteKind)
    , m_noteLevel(QCNoteLevel::NormalNoteLevel)
    , m_strSource()
    , m_strLanguage()
    , m_bIsFavorite(false)
    , m_bIsArchived(false)
    , m_dateTimeCapturedAt()
    , m_dateTimeCreatedAt()
    , m_dateTimeUpdatedAt()
{
}

QCSnippet::~QCSnippet()
{
}

qint64 QCSnippet::id() const
{
    return m_nId;
}

void QCSnippet::setId(qint64 nId)
{
    m_nId = nId;
}

qint64 QCSnippet::sessionId() const
{
    return m_nSessionId;
}

void QCSnippet::setSessionId(qint64 nSessionId)
{
    m_nSessionId = nSessionId;
}

QCSnippetType QCSnippet::type() const
{
    return m_snippetType;
}

void QCSnippet::setType(QCSnippetType snippetType)
{
    m_snippetType = snippetType;
}

QCCaptureType QCSnippet::captureType() const
{
    return m_captureType;
}

void QCSnippet::setCaptureType(QCCaptureType captureType)
{
    m_captureType = captureType;
}

QString QCSnippet::title() const
{
    return m_strTitle;
}

void QCSnippet::setTitle(const QString& strTitle)
{
    m_strTitle = strTitle;
}

QString QCSnippet::contentText() const
{
    return m_strContentText;
}

void QCSnippet::setContentText(const QString& strContentText)
{
    m_strContentText = strContentText;
}

QString QCSnippet::summary() const
{
    return m_strSummary;
}

void QCSnippet::setSummary(const QString& strSummary)
{
    m_strSummary = strSummary;
}

QString QCSnippet::note() const
{
    return m_strNote;
}

void QCSnippet::setNote(const QString& strNote)
{
    m_strNote = strNote;
}

QCNoteKind QCSnippet::noteKind() const
{
    return m_noteKind;
}

void QCSnippet::setNoteKind(QCNoteKind noteKind)
{
    m_noteKind = noteKind;
}

QCNoteLevel QCSnippet::noteLevel() const
{
    return m_noteLevel;
}

void QCSnippet::setNoteLevel(QCNoteLevel noteLevel)
{
    m_noteLevel = noteLevel;
}

QString QCSnippet::source() const
{
    return m_strSource;
}

void QCSnippet::setSource(const QString& strSource)
{
    m_strSource = strSource;
}

QString QCSnippet::language() const
{
    return m_strLanguage;
}

void QCSnippet::setLanguage(const QString& strLanguage)
{
    m_strLanguage = strLanguage;
}

bool QCSnippet::isFavorite() const
{
    return m_bIsFavorite;
}

void QCSnippet::setIsFavorite(bool bIsFavorite)
{
    m_bIsFavorite = bIsFavorite;
}

bool QCSnippet::isArchived() const
{
    return m_bIsArchived;
}

void QCSnippet::setIsArchived(bool bIsArchived)
{
    m_bIsArchived = bIsArchived;
}

QDateTime QCSnippet::capturedAt() const
{
    return m_dateTimeCapturedAt;
}

void QCSnippet::setCapturedAt(const QDateTime& dateTimeCapturedAt)
{
    m_dateTimeCapturedAt = dateTimeCapturedAt;
}

QDateTime QCSnippet::createdAt() const
{
    return m_dateTimeCreatedAt;
}

void QCSnippet::setCreatedAt(const QDateTime& dateTimeCreatedAt)
{
    m_dateTimeCreatedAt = dateTimeCreatedAt;
}

QDateTime QCSnippet::updatedAt() const
{
    return m_dateTimeUpdatedAt;
}

void QCSnippet::setUpdatedAt(const QDateTime& dateTimeUpdatedAt)
{
    m_dateTimeUpdatedAt = dateTimeUpdatedAt;
}
