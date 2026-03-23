#ifndef QTCLIP_QCSNIPPET_H_
#define QTCLIP_QCSNIPPET_H_

// File: qcsnippet.h
// Author: ZZMI1
// Created: 2026-03-23
// Description: Declares the snippet entity used by the QtClip domain layer.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include <QDateTime>
#include <QString>

// Snippet content categories used by the first QtClip release.
enum class QCSnippetType
{
    ImageSnippetType,
    TextSnippetType,
    CodeSnippetType
};

// Capture origin categories used by the first QtClip release.
enum class QCCaptureType
{
    ScreenCaptureType,
    ManualCaptureType,
    ImportCaptureType
};

// Note categories aligned with the classroom note-taking workflow.
enum class QCNoteKind
{
    ConceptNoteKind,
    CodeNoteKind,
    HomeworkNoteKind,
    DefinitionNoteKind,
    QuestionNoteKind,
    ReviewNoteKind
};

// Note priority levels aligned with study review workflow.
enum class QCNoteLevel
{
    NormalNoteLevel,
    ImportantNoteLevel,
    ReviewNoteLevel
};

class QCSnippet
{
public:
    QCSnippet();
    ~QCSnippet();

    qint64 id() const;
    void setId(qint64 nId);

    qint64 sessionId() const;
    void setSessionId(qint64 nSessionId);

    QCSnippetType type() const;
    void setType(QCSnippetType snippetType);

    QCCaptureType captureType() const;
    void setCaptureType(QCCaptureType captureType);

    QString title() const;
    void setTitle(const QString& strTitle);

    QString contentText() const;
    void setContentText(const QString& strContentText);

    QString summary() const;
    void setSummary(const QString& strSummary);

    QString note() const;
    void setNote(const QString& strNote);

    QCNoteKind noteKind() const;
    void setNoteKind(QCNoteKind noteKind);

    QCNoteLevel noteLevel() const;
    void setNoteLevel(QCNoteLevel noteLevel);

    QString source() const;
    void setSource(const QString& strSource);

    QString language() const;
    void setLanguage(const QString& strLanguage);

    bool isFavorite() const;
    void setIsFavorite(bool bIsFavorite);

    bool isArchived() const;
    void setIsArchived(bool bIsArchived);

    QDateTime capturedAt() const;
    void setCapturedAt(const QDateTime& dateTimeCapturedAt);

    QDateTime createdAt() const;
    void setCreatedAt(const QDateTime& dateTimeCreatedAt);

    QDateTime updatedAt() const;
    void setUpdatedAt(const QDateTime& dateTimeUpdatedAt);

private:
    qint64 m_nId;
    qint64 m_nSessionId;
    QCSnippetType m_snippetType;
    QCCaptureType m_captureType;
    QString m_strTitle;
    QString m_strContentText;
    QString m_strSummary;
    QString m_strNote;
    QCNoteKind m_noteKind;
    QCNoteLevel m_noteLevel;
    QString m_strSource;
    QString m_strLanguage;
    bool m_bIsFavorite;
    bool m_bIsArchived;
    QDateTime m_dateTimeCapturedAt;
    QDateTime m_dateTimeCreatedAt;
    QDateTime m_dateTimeUpdatedAt;
};

#endif // QTCLIP_QCSNIPPET_H_
