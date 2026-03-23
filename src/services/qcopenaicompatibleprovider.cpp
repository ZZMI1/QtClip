// File: qcopenaicompatibleprovider.cpp
// Author: ZZMI1
// Created: 2026-03-23
// Description: Implements the minimal OpenAI-compatible HTTP provider used by the first QtClip AI workflow.
// Email: 1633467942@qq.com
// Copyright (c) 2026 QtClip. All rights reserved.

#include "qcopenaicompatibleprovider.h"

#include <QByteArray>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>

namespace
{
QString ExtractErrorMessage(const QJsonObject& jsonObject)
{
    const QJsonObject jsonError = jsonObject.value(QString::fromUtf8("error")).toObject();
    QString strError = jsonError.value(QString::fromUtf8("message")).toString().trimmed();
    if (!strError.isEmpty())
        return strError;

    strError = jsonError.value(QString::fromUtf8("code")).toString().trimmed();
    if (!strError.isEmpty())
        return strError;

    return QString();
}

QString ExtractMessageContent(const QJsonValue& jsonValue)
{
    if (jsonValue.isString())
        return jsonValue.toString().trimmed();

    if (!jsonValue.isArray())
        return QString();

    QString strText;
    const QJsonArray jsonArray = jsonValue.toArray();
    for (int i = 0; i < jsonArray.size(); ++i)
    {
        const QJsonObject jsonPart = jsonArray.at(i).toObject();
        const QString strType = jsonPart.value(QString::fromUtf8("type")).toString();
        if (strType == QString::fromUtf8("text") || strType.isEmpty())
        {
            const QString strPartText = jsonPart.value(QString::fromUtf8("text")).toString().trimmed();
            if (!strPartText.isEmpty())
            {
                if (!strText.isEmpty())
                    strText += QString::fromUtf8("\n");
                strText += strPartText;
            }
        }
    }

    return strText.trimmed();
}

QString ExtractResponseText(const QJsonObject& jsonObject)
{
    const QJsonArray jsonChoices = jsonObject.value(QString::fromUtf8("choices")).toArray();
    if (jsonChoices.isEmpty())
        return QString();

    const QJsonObject jsonChoice = jsonChoices.at(0).toObject();
    const QJsonObject jsonMessage = jsonChoice.value(QString::fromUtf8("message")).toObject();
    return ExtractMessageContent(jsonMessage.value(QString::fromUtf8("content")));
}

QString DetectMimeType(const QString& strFilePath)
{
    const QString strSuffix = QFileInfo(strFilePath).suffix().toLower();
    if (strSuffix == QString::fromUtf8("jpg") || strSuffix == QString::fromUtf8("jpeg"))
        return QString::fromUtf8("image/jpeg");
    if (strSuffix == QString::fromUtf8("webp"))
        return QString::fromUtf8("image/webp");
    if (strSuffix == QString::fromUtf8("gif"))
        return QString::fromUtf8("image/gif");

    return QString::fromUtf8("image/png");
}

bool BuildImageDataUrl(const QString& strFilePath,
                       QString *pstrDataUrl,
                       QString *pstrError)
{
    if (nullptr == pstrDataUrl)
    {
        if (nullptr != pstrError)
            *pstrError = QString::fromUtf8("Image data URL output pointer is null.");
        return false;
    }

    pstrDataUrl->clear();
    if (nullptr != pstrError)
        pstrError->clear();

    const QString strNormalizedPath = strFilePath.trimmed();
    if (strNormalizedPath.isEmpty())
    {
        if (nullptr != pstrError)
            *pstrError = QString::fromUtf8("Image file path is empty.");
        return false;
    }

    QFile imageFile(strNormalizedPath);
    if (!imageFile.exists())
    {
        if (nullptr != pstrError)
            *pstrError = QString::fromUtf8("Image file does not exist: %1").arg(strNormalizedPath);
        return false;
    }

    if (!imageFile.open(QIODevice::ReadOnly))
    {
        if (nullptr != pstrError)
            *pstrError = QString::fromUtf8("Failed to open image file: %1").arg(strNormalizedPath);
        return false;
    }

    const QByteArray byteArrayImage = imageFile.readAll();
    imageFile.close();
    if (byteArrayImage.isEmpty())
    {
        if (nullptr != pstrError)
            *pstrError = QString::fromUtf8("Image file is empty: %1").arg(strNormalizedPath);
        return false;
    }

    const QString strMimeType = DetectMimeType(strNormalizedPath);
    *pstrDataUrl = QString::fromUtf8("data:%1;base64,%2")
        .arg(strMimeType, QString::fromUtf8(byteArrayImage.toBase64()));
    return true;
}

QString BuildBaseUrlError(const QString& strBaseUrl)
{
    return QString::fromUtf8("AI base URL is invalid: %1").arg(strBaseUrl.trimmed());
}

QString ClassifyHttpError(int nHttpStatusCode, const QString& strProviderMessage)
{
    const QString strMessage = strProviderMessage.trimmed();
    if (nHttpStatusCode == 400)
        return QString::fromUtf8("HTTP 400: request was rejected. Check the model name and request format. %1").arg(strMessage);
    if (nHttpStatusCode == 401)
        return QString::fromUtf8("HTTP 401: authentication failed. Check the API key. %1").arg(strMessage);
    if (nHttpStatusCode == 403)
        return QString::fromUtf8("HTTP 403: request was forbidden. The model may be unavailable for this key or access may be denied. %1").arg(strMessage);
    if (nHttpStatusCode == 404)
        return QString::fromUtf8("HTTP 404: endpoint or model was not found. Check the base URL and model name. %1").arg(strMessage);
    if (nHttpStatusCode == 408)
        return QString::fromUtf8("HTTP 408: request timed out. %1").arg(strMessage);
    if (nHttpStatusCode == 409)
        return QString::fromUtf8("HTTP 409: request conflicted with the provider state. %1").arg(strMessage);
    if (nHttpStatusCode == 422)
        return QString::fromUtf8("HTTP 422: request was understood but rejected. Check the model name and payload fields. %1").arg(strMessage);
    if (nHttpStatusCode == 429)
        return QString::fromUtf8("HTTP 429: rate limited or quota exceeded. %1").arg(strMessage);
    if (nHttpStatusCode >= 500 && nHttpStatusCode <= 599)
        return QString::fromUtf8("HTTP %1: provider service error. %2").arg(nHttpStatusCode).arg(strMessage);
    if (nHttpStatusCode > 0)
        return QString::fromUtf8("HTTP %1: %2").arg(nHttpStatusCode).arg(strMessage);
    return strMessage;
}

QString ClassifyNetworkError(QNetworkReply::NetworkError networkError,
                             int nHttpStatusCode,
                             bool bTimedOut,
                             const QString& strReplyError,
                             const QString& strProviderMessage)
{
    if (bTimedOut || networkError == QNetworkReply::TimeoutError)
        return QString::fromUtf8("AI request timed out. Check the network connection, provider latency, or base URL.");

    if (nHttpStatusCode > 0)
        return ClassifyHttpError(nHttpStatusCode, strProviderMessage.isEmpty() ? strReplyError : strProviderMessage);

    switch (networkError)
    {
    case QNetworkReply::ConnectionRefusedError:
    case QNetworkReply::HostNotFoundError:
    case QNetworkReply::NetworkSessionFailedError:
        return QString::fromUtf8("Network connection failed. Check the base URL host and network availability.");
    case QNetworkReply::SslHandshakeFailedError:
        return QString::fromUtf8("SSL/TLS handshake failed. Check the HTTPS endpoint and local certificate trust.");
    case QNetworkReply::ProxyConnectionRefusedError:
    case QNetworkReply::ProxyConnectionClosedError:
    case QNetworkReply::ProxyNotFoundError:
    case QNetworkReply::ProxyTimeoutError:
        return QString::fromUtf8("Proxy connection failed while reaching the AI provider.");
    case QNetworkReply::RemoteHostClosedError:
        return QString::fromUtf8("The provider closed the network connection unexpectedly.");
    default:
        break;
    }

    if (!strProviderMessage.trimmed().isEmpty())
        return strProviderMessage.trimmed();
    if (!strReplyError.trimmed().isEmpty())
        return QString::fromUtf8("Network request failed: %1").arg(strReplyError.trimmed());
    return QString::fromUtf8("OpenAI-compatible request failed.");
}
}

QCOpenAiCompatibleProvider::QCOpenAiCompatibleProvider(const QString& strBaseUrl,
                                                       const QString& strApiKey,
                                                       const QString& strDefaultModel)
    : m_strBaseUrl(strBaseUrl.trimmed())
    , m_strApiKey(strApiKey.trimmed())
    , m_strDefaultModel(strDefaultModel.trimmed())
    , m_strLastError()
{
}

QCOpenAiCompatibleProvider::~QCOpenAiCompatibleProvider()
{
}

bool QCOpenAiCompatibleProvider::generateText(const QCAiProviderRequest& aiRequest,
                                              QCAiProviderResponse *pAiResponse)
{
    clearError();

    if (nullptr == pAiResponse)
    {
        setLastError(QString::fromUtf8("AI response output pointer is null."));
        return false;
    }

    if (m_strBaseUrl.isEmpty())
    {
        setLastError(QString::fromUtf8("AI base URL is missing."));
        return false;
    }

    if (m_strApiKey.isEmpty())
    {
        setLastError(QString::fromUtf8("AI API key is missing."));
        return false;
    }

    const QString strModelName = aiRequest.m_strModelName.trimmed().isEmpty()
        ? m_strDefaultModel
        : aiRequest.m_strModelName.trimmed();
    if (strModelName.isEmpty())
    {
        setLastError(QString::fromUtf8("AI model is missing."));
        return false;
    }

    const QString strRequestUrl = resolveChatCompletionsUrl();
    const QUrl urlRequest(strRequestUrl);
    if (!urlRequest.isValid() || urlRequest.scheme().trimmed().isEmpty() || urlRequest.host().trimmed().isEmpty())
    {
        setLastError(BuildBaseUrlError(m_strBaseUrl));
        return false;
    }

    if (urlRequest.scheme() != QString::fromUtf8("http") && urlRequest.scheme() != QString::fromUtf8("https"))
    {
        setLastError(QString::fromUtf8("AI base URL must start with http:// or https://: %1").arg(m_strBaseUrl));
        return false;
    }

    QJsonArray jsonMessages;
    if (!aiRequest.m_strSystemPrompt.trimmed().isEmpty())
    {
        QJsonObject jsonSystemMessage;
        jsonSystemMessage.insert(QString::fromUtf8("role"), QString::fromUtf8("system"));
        jsonSystemMessage.insert(QString::fromUtf8("content"), aiRequest.m_strSystemPrompt);
        jsonMessages.append(jsonSystemMessage);
    }

    QJsonObject jsonUserMessage;
    jsonUserMessage.insert(QString::fromUtf8("role"), QString::fromUtf8("user"));

    if (aiRequest.m_strLocalImageFilePath.trimmed().isEmpty())
    {
        jsonUserMessage.insert(QString::fromUtf8("content"), aiRequest.m_strUserPrompt);
    }
    else
    {
        QString strImageDataUrl;
        QString strImageError;
        if (!BuildImageDataUrl(aiRequest.m_strLocalImageFilePath, &strImageDataUrl, &strImageError))
        {
            setLastError(strImageError);
            return false;
        }

        QJsonArray jsonContent;

        QJsonObject jsonTextPart;
        jsonTextPart.insert(QString::fromUtf8("type"), QString::fromUtf8("text"));
        jsonTextPart.insert(QString::fromUtf8("text"), aiRequest.m_strUserPrompt);
        jsonContent.append(jsonTextPart);

        QJsonObject jsonImageUrl;
        jsonImageUrl.insert(QString::fromUtf8("url"), strImageDataUrl);

        QJsonObject jsonImagePart;
        jsonImagePart.insert(QString::fromUtf8("type"), QString::fromUtf8("image_url"));
        jsonImagePart.insert(QString::fromUtf8("image_url"), jsonImageUrl);
        jsonContent.append(jsonImagePart);

        jsonUserMessage.insert(QString::fromUtf8("content"), jsonContent);
    }

    jsonMessages.append(jsonUserMessage);

    QJsonObject jsonPayload;
    jsonPayload.insert(QString::fromUtf8("model"), strModelName);
    jsonPayload.insert(QString::fromUtf8("messages"), jsonMessages);
    jsonPayload.insert(QString::fromUtf8("temperature"), 0.2);

    QNetworkRequest networkRequest{urlRequest};
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, QString::fromUtf8("application/json"));
    networkRequest.setRawHeader("Accept", "application/json");
    networkRequest.setRawHeader("Authorization", QString::fromUtf8("Bearer %1").arg(m_strApiKey).toUtf8());

    QNetworkAccessManager networkAccessManager;
    QNetworkReply *pReply = networkAccessManager.post(networkRequest, QJsonDocument(jsonPayload).toJson(QJsonDocument::Compact));
    if (nullptr == pReply)
    {
        setLastError(QString::fromUtf8("Failed to create AI network request."));
        return false;
    }

    QEventLoop eventLoop;
    bool bTimedOut = false;
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    QObject::connect(&timeoutTimer, &QTimer::timeout, &eventLoop, [&eventLoop, pReply, &bTimedOut]() {
        bTimedOut = true;
        if (nullptr != pReply && pReply->isRunning())
            pReply->abort();
        eventLoop.quit();
    });
    QObject::connect(pReply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);

    timeoutTimer.start(aiRequest.m_nTimeoutMs > 0 ? aiRequest.m_nTimeoutMs : 30000);
    eventLoop.exec();
    timeoutTimer.stop();

    const QByteArray byteArrayResponse = pReply->readAll();
    const QString strRawResponse = QString::fromUtf8(byteArrayResponse);
    QJsonParseError jsonParseError;
    const QJsonDocument jsonDocument = QJsonDocument::fromJson(byteArrayResponse, &jsonParseError);
    const QJsonObject jsonObject = jsonDocument.object();
    const int nHttpStatusCode = pReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    pAiResponse->m_strProviderName = providerName();
    pAiResponse->m_strModelName = strModelName;
    pAiResponse->m_strRawResponse = strRawResponse;
    pAiResponse->m_strEndpoint = strRequestUrl;
    pAiResponse->m_nHttpStatusCode = nHttpStatusCode;
    pAiResponse->m_strErrorSummary.clear();

    if (pReply->error() != QNetworkReply::NoError)
    {
        const QString strError = ClassifyNetworkError(pReply->error(),
                                                      nHttpStatusCode,
                                                      bTimedOut,
                                                      pReply->errorString(),
                                                      ExtractErrorMessage(jsonObject));
        pAiResponse->m_strErrorSummary = ExtractErrorMessage(jsonObject).trimmed();
        pReply->deleteLater();
        setLastError(strError);
        return false;
    }

    if (nHttpStatusCode < 200 || nHttpStatusCode >= 300)
    {
        const QString strProviderError = ExtractErrorMessage(jsonObject).trimmed();
        QString strError = ClassifyHttpError(nHttpStatusCode, strProviderError);
        if (strError.trimmed().isEmpty())
            strError = QString::fromUtf8("HTTP %1: request failed.").arg(nHttpStatusCode);
        pAiResponse->m_strErrorSummary = strProviderError;
        pReply->deleteLater();
        setLastError(strError);
        return false;
    }

    if (!byteArrayResponse.trimmed().isEmpty() && jsonParseError.error != QJsonParseError::NoError)
    {
        pAiResponse->m_strErrorSummary = jsonParseError.errorString();
        pReply->deleteLater();
        setLastError(QString::fromUtf8("Failed to parse AI provider response JSON: %1").arg(jsonParseError.errorString()));
        return false;
    }

    const QString strText = ExtractResponseText(jsonObject);
    if (strText.isEmpty())
    {
        QString strError = ExtractErrorMessage(jsonObject);
        if (strError.isEmpty())
            strError = QString::fromUtf8("AI response parsing failed: no text answer was returned by the provider.");
        pAiResponse->m_strErrorSummary = strError;
        pReply->deleteLater();
        setLastError(strError);
        return false;
    }

    pAiResponse->m_strModelName = jsonObject.value(QString::fromUtf8("model")).toString(strModelName);
    pAiResponse->m_strText = strText;

    pReply->deleteLater();
    return true;
}

QString QCOpenAiCompatibleProvider::providerName() const
{
    return QString::fromUtf8("openai-compatible");
}

QString QCOpenAiCompatibleProvider::lastError() const
{
    return m_strLastError;
}

void QCOpenAiCompatibleProvider::clearError() const
{
    m_strLastError.clear();
}

QString QCOpenAiCompatibleProvider::resolveChatCompletionsUrl() const
{
    QString strBaseUrl = m_strBaseUrl;
    while (strBaseUrl.endsWith('/'))
        strBaseUrl.chop(1);

    return QString::fromUtf8("%1/chat/completions").arg(strBaseUrl);
}

void QCOpenAiCompatibleProvider::setLastError(const QString& strError) const
{
    m_strLastError = strError;
}

