#include "QPushbulletHandler.h"
#include <QDebug>
#include <iostream>

QPushbulletHandler::QPushbulletHandler(QString apiKey)
    : m_NetworkManager()
    , m_WebSocket()
    , m_URLContacts("https://api.pushbullet.com/v2/contacts")
    , m_URLDevices("https://api.pushbullet.com/v2/devices")
    , m_URLMe("https://api.pushbullet.com/v2/users/me")
    , m_URLPushes("https://api.pushbullet.com/v2/pushes")
    , m_URLUploadRequest("https://api.pushbullet.com/v2/upload-request")
    , m_APIKey(apiKey)
    , m_NetworkAccessibility(QNetworkAccessManager::NetworkAccessibility::UnknownAccessibility)
{
    //Connect the QNetworkAccessManager signals
    connect(&m_NetworkManager, SIGNAL(finished(QNetworkReply *)), this, SLOT(handleNetworkData(QNetworkReply *)));
    connect(&m_NetworkManager, SIGNAL(networkSessionConnected()), this, SLOT(sessionConnected()));
    connect(&m_NetworkManager, SIGNAL(networkAccessibleChanged(QNetworkAccessManager::NetworkAccessibility)), this
            , SLOT(handleNetworkAccessibilityChange(QNetworkAccessManager::NetworkAccessibility)));
}

void QPushbulletHandler::getRequest(QUrl url)
{
    qDebug() << "Get Request";
    url.setUserName(m_APIKey);
    m_NetworkManager.get(QNetworkRequest(url));
}

void QPushbulletHandler::postRequest(QUrl url, const QByteArray &data)
{
    qDebug() << "Post Request: " << QString(data);
    QNetworkRequest request(url);
    if (m_CurrentOperation == CURRENT_OPERATION::UPLOAD_FILE) {
        request.setRawHeader(QString("Content-Type").toUtf8(), QString("multipart/form-data; boundary=margin").toUtf8());
    }
    else {
        url.setUserName(m_APIKey);
        request.setUrl(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    }
    if (m_CurrentOperation == CURRENT_OPERATION::DELETE_CONTACT || m_CurrentOperation == CURRENT_OPERATION::DELETE_DEVICE
        || m_CurrentOperation == CURRENT_OPERATION::DELETE_PUSH)
        m_NetworkManager.deleteResource(QNetworkRequest(url));
    else
        m_NetworkManager.post(request, data);
}

void QPushbulletHandler::handleNetworkData(QNetworkReply *networkReply)
{
    if (networkReply->error()) {
        qDebug() << "Error String: " << networkReply->errorString();
        QByteArray response(networkReply->readAll());
        qDebug() << QString(response);
        emit didReceiveError(networkReply);
        return;
    }

    QByteArray response(networkReply->readAll());
    if (m_CurrentOperation == CURRENT_OPERATION::GET_DEVICE_LIST) {
        parseDeviceResponse(response);
    }
    else if (m_CurrentOperation == CURRENT_OPERATION::CREATE_DEVICE) {
        parseCreateDeviceResponse(response);
    }
    else if (m_CurrentOperation == CURRENT_OPERATION::DELETE_DEVICE) {
        emit didDeviceDelete();
    }
    else if (m_CurrentOperation == CURRENT_OPERATION::UPDATE_DEVICE) {
        parseUpdateDeviceResponce(response);
    }
    else if (m_CurrentOperation == CURRENT_OPERATION::GET_CONTACT_LIST) {
        parseContactResponse(response);
    }
    else if (m_CurrentOperation == CURRENT_OPERATION::CREATE_CONTACT) {
        parseCreateContactResponse(response);
    }
    else if (m_CurrentOperation == CURRENT_OPERATION::UPDATE_CONTACT) {
        parseUpdateContactResponse(response);
    }
    else if (m_CurrentOperation == CURRENT_OPERATION::DELETE_CONTACT) {
        emit didContactDelete();
    }
    else if (m_CurrentOperation == CURRENT_OPERATION::GET_PUSH_HISTORY) {
        parsePushHistoryResponse(response);
    }
    else if (m_CurrentOperation == CURRENT_OPERATION::PUSH) {
        parsePushResponse(response);
    }
    else if (m_CurrentOperation == CURRENT_OPERATION::PUSH_UPDATE) {
        parsePushResponse(response);
    }
    else if (m_CurrentOperation == CURRENT_OPERATION::UPDATE_PUSH_LIST) {
        parsePushHistoryResponse(response);
    }
    else if (m_CurrentOperation == CURRENT_OPERATION::DELETE_PUSH) {
        emit didPushDelete();
    }
    else if (m_CurrentOperation == CURRENT_OPERATION::REQUEST_UPLOAD_FILE) {
        parseUploadRequestResponse(response);
    }
    else if (m_CurrentOperation == CURRENT_OPERATION::UPLOAD_FILE) {
        QString str(response);
        qDebug() << str;
    }

    m_CurrentOperation = CURRENT_OPERATION::NONE;
}

void QPushbulletHandler::sessionConnected()
{
    qDebug() << "Connected";
}

void QPushbulletHandler::handleNetworkAccessibilityChange(QNetworkAccessManager::NetworkAccessibility change)
{
    m_NetworkAccessibility = change;
    if (m_NetworkAccessibility == QNetworkAccessManager::UnknownAccessibility)
        qDebug() << "Unknown network accessibility";
    else if (m_NetworkAccessibility == QNetworkAccessManager::Accessible)
        qDebug() << "Network is accessible";
    else if (m_NetworkAccessibility == QNetworkAccessManager::NotAccessible)
        qDebug() << "Network is not accessible";
}

void QPushbulletHandler::webSocketConnected()
{
    qDebug() << "Web Socket Connected";
    connect(&m_WebSocket, SIGNAL(textMessageReceived(QString)), this, SLOT(textMessageReceived(QString)));
}

void QPushbulletHandler::webSocketDisconnected()
{
    qDebug() << "Web Socket Disconnected";
}

void QPushbulletHandler::textMessageReceived(QString message)
{
    parseMirrorPush(message);
}

void QPushbulletHandler::requestDeviceList()
{
    m_CurrentOperation = CURRENT_OPERATION::GET_DEVICE_LIST;
    getRequest(m_URLDevices);
}

void QPushbulletHandler::requestCreateDevice(QString deviceName, QString model)
{
    QJsonDocument doc;
    QJsonObject obj;
    obj["nickname"] = deviceName;
    obj["type"] = "stream";

    if (!model.isEmpty())
        obj["manufacturer"] = model;
    doc.setObject(obj);
    m_CurrentOperation = CURRENT_OPERATION::CREATE_DEVICE;
    postRequest(m_URLDevices, doc.toJson());
}

void QPushbulletHandler::requestDeviceUpdate(QString deviceID, QString newNickname)
{
    QString url = m_URLDevices.toString();
    url.append("/");
    url.append(deviceID);
    QUrl modifiedURL(url);
    QUrlQuery query;
    query.addQueryItem("nickname", newNickname);
    m_CurrentOperation = CURRENT_OPERATION::UPDATE_DEVICE;
    postRequest(modifiedURL, query.toString(QUrl::FullyEncoded).toUtf8());
}

void QPushbulletHandler::requestDeviceDelete(QString deviceID)
{
    QString url = m_URLDevices.toString();
    url.append("/");
    url.append(deviceID);
    QUrl modifiedURL(url);
    m_CurrentOperation = CURRENT_OPERATION::DELETE_DEVICE;
    QUrlQuery query;
    query.setQueryDelimiters(' ', '&');
    query.addQueryItem("-X", "DELETE");
    postRequest(modifiedURL, query.toString(QUrl::FullyEncoded).toUtf8());
}

void QPushbulletHandler::requestContactList()
{
    m_CurrentOperation = CURRENT_OPERATION::GET_CONTACT_LIST;
    getRequest(m_URLContacts);
}

void QPushbulletHandler::requestCreateContact(QString name, QString email)
{
    QUrlQuery query;
    query.addQueryItem("name", name);
    query.addQueryItem("email", email);
    m_CurrentOperation = CURRENT_OPERATION::CREATE_CONTACT;
    postRequest(m_URLContacts, query.toString(QUrl::FullyEncoded).toUtf8());
}

void QPushbulletHandler::requestContactUpdate(QString contactID, QString newName)
{
    QString url = m_URLContacts.toString();
    url.append("/");
    url.append(contactID);
    QUrl modifiedURL(url);
    QUrlQuery query;
    query.addQueryItem("name", newName);
    m_CurrentOperation = CURRENT_OPERATION::UPDATE_CONTACT;
    postRequest(modifiedURL, query.toString(QUrl::FullyEncoded).toUtf8());
}

void QPushbulletHandler::requestContactDelete(QString contactID)
{
    QString url = m_URLContacts.toString();
    url.append("/");
    url.append(contactID);
    QUrl modifiedURL(url);
    m_CurrentOperation = CURRENT_OPERATION::DELETE_CONTACT;
    QUrlQuery query;
    query.setQueryDelimiters(' ', '&');
    query.addQueryItem("-X", "DELETE");

    postRequest(modifiedURL, query.toString(QUrl::FullyEncoded).toUtf8());
}

void QPushbulletHandler::requestPushHistory()
{
    m_CurrentOperation = CURRENT_OPERATION::GET_PUSH_HISTORY;
    getRequest(m_URLPushes);
}

void QPushbulletHandler::requestPushHistory(double modifiedAfter)
{
    m_CurrentOperation = CURRENT_OPERATION::GET_PUSH_HISTORY;
    QString url(m_URLPushes.toString());
    url.append("?modified_after=");
    url.append(QString::number(modifiedAfter));
    getRequest(QUrl(url));
}

void QPushbulletHandler::requestPush(Push &push, QString deviceID, QString email)
{
    /* [x] Note
     * [x] Link
     * [x] Address
     * [x] List
     * [] File
     */
    QJsonDocument jsonDocument;
    QJsonObject jsonObject;
    if (push.type == PUSH_TYPE::ADDRESS)
        jsonObject["type"] = "address";
    else if (push.type == PUSH_TYPE::FILE)
        jsonObject["type"] = "file";
    else if (push.type == PUSH_TYPE::LINK)
        jsonObject["type"] = "link";
    else if (push.type == PUSH_TYPE::LIST)
        jsonObject["type"] = "list";
    else if (push.type == PUSH_TYPE::NOTE)
        jsonObject["type"] = "note";

    if (!deviceID.isEmpty())
        jsonObject["device_iden"] = deviceID;
    else if (!email.isEmpty())
        jsonObject["email"] = email;
    if (push.type == PUSH_TYPE::NOTE) {
        jsonObject["title"] = push.title;
        jsonObject["body"] = push.body;
    }
    else if (push.type == PUSH_TYPE::LINK) {
        jsonObject["url"] = push.url;
        jsonObject["title"] = push.title;
        jsonObject["body"] = push.body;
    }
    else if (push.type == PUSH_TYPE::ADDRESS) {
        jsonObject["name"] = push.addressName;
        jsonObject["address"] = push.address;
    }
    else if (push.type == PUSH_TYPE::LIST) {
        jsonObject["type"] = "list";
        jsonObject["title"] = push.title;
        QJsonArray jsonArray;
        for (QString item : push.listItems) {
            jsonArray.append(QJsonValue(item));
        }
        jsonObject["items"] = jsonArray;
    }
    else if (push.type == PUSH_TYPE::FILE) {
        jsonObject["file_name"] = push.fileName;
        jsonObject["file_type"] = push.fileType;
        jsonObject["file_url"] = push.fileURL;
        jsonObject["body"] = push.body;
    }
    m_CurrentOperation = CURRENT_OPERATION::PUSH;

    jsonDocument.setObject(jsonObject);
    qDebug() << QString(jsonDocument.toJson());
    postRequest(m_URLPushes, jsonDocument.toJson());
}

void QPushbulletHandler::requestPushToDevice(Push &push, QString deviceID)
{
    requestPush(push, deviceID, "");
}

void QPushbulletHandler::requestPushToContact(Push &push, QString email)
{
    requestPush(push, "", email);
}

void QPushbulletHandler::requestPushToAllDevices(Push &push)
{
    requestPush(push, "", "");
}

void QPushbulletHandler::requestPushUpdate(QString pushID, bool dismissed)
{
    QUrlQuery query;
    query.addQueryItem("dismissed", dismissed ? "true" : "false");
    QString url = m_URLPushes.toString();
    url.append("/");
    url.append(pushID);
    QUrl modifiedURL(url);
    m_CurrentOperation = CURRENT_OPERATION::PUSH_UPDATE;
    postRequest(modifiedURL, query.toString(QUrl::FullyEncoded).toUtf8());
}

void QPushbulletHandler::requestPushDelete(QString pushID)
{
    QString url = m_URLPushes.toString();
    url.append("/");
    url.append(pushID);
    QUrl modifiedURL(url);
    m_CurrentOperation = CURRENT_OPERATION::DELETE_PUSH;
    QUrlQuery query;
    query.setQueryDelimiters(' ', '&');
    query.addQueryItem("-X", "DELETE");

    postRequest(modifiedURL, query.toString(QUrl::FullyEncoded).toUtf8());
}

void QPushbulletHandler::parseDeviceResponse(const QByteArray &data)
{
    m_Devices.clear();
    QString strReply = (QString)data;
    QJsonDocument jsonResponse = QJsonDocument::fromJson(strReply.toUtf8());
    QJsonObject jsonObject = jsonResponse.object();
    QJsonArray jsonArray = jsonObject["devices"].toArray();

    foreach (const QJsonValue &value, jsonArray) {
        QJsonObject obj = value.toObject();
        Device device;
        device.nickname = obj["nickname"].toString();
        if (device.nickname.isEmpty())
            continue;
        device.active = obj["active"].toBool();
        device.appVersion = obj["app_version"].toInt();
        device.ID = obj["iden"].toString();
        device.manufacturer = obj["manufacturer"].toString();
        device.type = obj["type"].toString();
        device.pushable = obj["pushable"].toBool();
        device.pushToken = obj["push_token"].toString();
        m_Devices.append(device);
    }
    emit didReceiveDevices(m_Devices);
}

void QPushbulletHandler::parseCreateDeviceResponse(const QByteArray &data)
{
    QString strReply = (QString)data;
    QJsonDocument jsonResponse = QJsonDocument::fromJson(strReply.toUtf8());
    QJsonObject jsonObject = jsonResponse.object();

    Device device;
    device.active = jsonObject["active"].toBool();
    device.appVersion = jsonObject["app_version"].toInt();
    device.ID = jsonObject["iden"].toString();
    device.manufacturer = jsonObject["manufacturer"].toString();
    device.type = jsonObject["type"].toString();
    device.pushable = jsonObject["pushable"].toBool();
    device.pushToken = jsonObject["push_token"].toString();
    device.nickname = jsonObject["nickname"].toString();

    emit didDeviceCreate(device);
}

void QPushbulletHandler::parseUpdateDeviceResponce(const QByteArray &data)
{
    QString strReply = (QString)data;
    QJsonDocument jsonResponse = QJsonDocument::fromJson(strReply.toUtf8());
    QJsonObject jsonObject = jsonResponse.object();

    Device device;
    device.active = jsonObject["active"].toBool();
    device.appVersion = jsonObject["app_version"].toInt();
    device.ID = jsonObject["iden"].toString();
    device.manufacturer = jsonObject["manufacturer"].toString();
    device.type = jsonObject["type"].toString();
    device.pushable = jsonObject["pushable"].toBool();
    device.pushToken = jsonObject["push_token"].toString();
    device.nickname = jsonObject["nickname"].toString();

    emit didDeviceUpdate(device);
}

void QPushbulletHandler::parseContactResponse(const QByteArray &data)
{
    m_Contacts.clear();

    QString strReply = (QString)data;
    QJsonDocument jsonResponse = QJsonDocument::fromJson(strReply.toUtf8());
    QJsonObject jsonObject = jsonResponse.object();
    QJsonArray jsonArray = jsonObject["contacts"].toArray();

    foreach (const QJsonValue &value, jsonArray) {
        QJsonObject obj = value.toObject();
        Contact contact;
        contact.name = obj["name"].toString();
        if (contact.name.isEmpty())
            continue;
        contact.email = obj["email"].toString();
        contact.ID = obj["iden"].toString();
        m_Contacts.append(contact);
    }
    emit didReceiveContacts(m_Contacts);
}

void QPushbulletHandler::parseCreateContactResponse(const QByteArray &data)
{
    QString strReply = (QString)data;
    QJsonDocument jsonResponse = QJsonDocument::fromJson(strReply.toUtf8());
    QJsonObject jsonObject = jsonResponse.object();

    Contact contact;
    contact.name = jsonObject["name"].toString();
    contact.email = jsonObject["email"].toString();
    contact.ID = jsonObject["iden"].toString();

    emit didContactCreate(contact);
}

void QPushbulletHandler::parseUpdateContactResponse(const QByteArray &data)
{
    QString strReply = (QString)data;
    QJsonDocument jsonResponse = QJsonDocument::fromJson(strReply.toUtf8());
    QJsonObject jsonObject = jsonResponse.object();

    Contact contact;
    contact.name = jsonObject["name"].toString();
    contact.email = jsonObject["email"].toString();
    contact.ID = jsonObject["iden"].toString();

    emit didContactUpdate(contact);
}

void QPushbulletHandler::parsePushHistoryResponse(const QByteArray &data)
{
    if (m_CurrentOperation != CURRENT_OPERATION::UPDATE_PUSH_LIST)
        m_Pushes.clear();

    QString strReply = (QString)data;
    QJsonDocument jsonResponse = QJsonDocument::fromJson(strReply.toUtf8());
    QJsonObject jsonObject = jsonResponse.object();
    QJsonArray jsonArray = jsonObject["pushes"].toArray();

    foreach (const QJsonValue &value, jsonArray) {
        QJsonObject jsonObject = value.toObject();
        Push push;

        if (jsonObject["active"].toBool() == false) {
            continue;
        }

        push.isActive = true;
        push.ID = jsonObject["iden"].toString();

        auto foundIt = std::find_if(m_Pushes.begin(), m_Pushes.end(), [&push](const Push & p) {
            return p.ID == push.ID;
        });

        if (foundIt != m_Pushes.end()) {
            continue;
        }

        push.type = getPushTypeFromString(jsonObject["type"].toString());
        push.targetDeviceID = jsonObject["target_device_iden"].toString();
        push.senderEmail = jsonObject["sender_email"].toString();
        push.receiverEmail = jsonObject["receiver_email"].toString();
        push.modified = jsonObject["modified"].toDouble();
        push.created = jsonObject["created"].toDouble();

        if (push.type == getPushTypeFromString("note")) {
            push.title = jsonObject["title"].toString();
            push.body = jsonObject["body"].toString();
        }
        else if (push.type == getPushTypeFromString("link")) {
            push.title = jsonObject["title"].toString();
            push.url = jsonObject["url"].toString();
            push.body = jsonObject["body"].toString();
        }
        else if (push.type == getPushTypeFromString("address")) {
            push.addressName = jsonObject["name"].toString();
            push.address = jsonObject["address"].toString();
        }
        else if (push.type == getPushTypeFromString("list")) {
            push.title = jsonObject["title"].toString();
            push.body = jsonObject["items"].toString();
            qDebug() << push.body;
        }
        else if (push.type == getPushTypeFromString("file")) {
            push.fileName = jsonObject["file_name"].toString();
            push.fileType = jsonObject["file_type"].toString();
            push.fileURL = jsonObject["file_url"].toString();
            push.body = jsonObject["body"].toString();
        }

        // Try to keep the last message at the last index
        if (m_CurrentOperation != CURRENT_OPERATION::UPDATE_PUSH_LIST) {
            m_Pushes.append(push);
        }
        else {
            m_Pushes.prepend(push);
        }
    }
    emit didReceivePushHistory(m_Pushes);
}

void QPushbulletHandler::parsePushResponse(const QByteArray &data)
{
    QString strReply = (QString)data;
    QJsonDocument jsonResponse = QJsonDocument::fromJson(strReply.toUtf8());
    QJsonObject jsonObject = jsonResponse.object();

    Push push;

    push.ID = jsonObject["iden"].toString();
    push.type = getPushTypeFromString(jsonObject["type"].toString());
    push.targetDeviceID = jsonObject["target_device_iden"].toString();
    push.senderEmail = jsonObject["sender_email"].toString();
    push.receiverEmail = jsonObject["receiver_email"].toString();
    push.modified = jsonObject["modified"].toDouble();
    push.created = jsonObject["created"].toDouble();

    if (push.type == getPushTypeFromString("note")) {
        push.title = jsonObject["title"].toString();
        push.body = jsonObject["body"].toString();
    }
    else if (push.type == getPushTypeFromString("link")) {
        push.title = jsonObject["title"].toString();
        push.url = jsonObject["url"].toString();
        push.body = jsonObject["body"].toString();
    }
    else if (push.type == getPushTypeFromString("address")) {
        push.addressName = jsonObject["name"].toString();
        push.address = jsonObject["address"].toString();
    }
    else if (push.type == getPushTypeFromString("list")) {
        push.title = jsonObject["title"].toString();
        push.body = jsonObject["items"].toString();
    }
    else if (push.type == getPushTypeFromString("file")) {
        push.fileName = jsonObject["file_name"].toString();
        push.fileType = jsonObject["file_type"].toString();
        push.fileURL = jsonObject["file_url"].toString();
        push.body = jsonObject["body"].toString();
    }
    if (m_CurrentOperation == CURRENT_OPERATION::PUSH_UPDATE)
        emit didPushUpdate(push);
    else
        emit didPush(push);
}

PUSH_TYPE QPushbulletHandler::getPushTypeFromString(QString type)
{
    PUSH_TYPE t = PUSH_TYPE::NONE;
    if (type == "address")
        t = PUSH_TYPE::ADDRESS;
    else if (type == "file")
        t = PUSH_TYPE::FILE;
    else if (type == "link")
        t = PUSH_TYPE::LINK;
    else if (type == "list")
        t = PUSH_TYPE::LIST;
    else if (type == "note")
        t = PUSH_TYPE::NOTE;
    return t;
}

void QPushbulletHandler::parseMirrorPush(QString data)
{
    QJsonDocument jsonResponse = QJsonDocument::fromJson(data.toUtf8());
    QJsonObject jsonObject = jsonResponse.object();
    if (jsonObject["type"] == "tickle") {
        parseTickle(jsonObject);
        return;
    }

    QJsonObject obj = jsonObject["push"].toObject();

    MirrorPush mirror;

    mirror.type = obj["type"].toString();
    mirror.subtype = obj["subtype"].toString();

    emit didReceiveMirrorPush(mirror);
}

void QPushbulletHandler::parseTickle(QJsonObject jsonObject)
{
    if (jsonObject["subtype"] == "push") {
        if (m_Pushes.isEmpty()) {
            requestPushHistory();
        }
        else {
            m_CurrentOperation = CURRENT_OPERATION::UPDATE_PUSH_LIST;
            QUrlQuery query;
            std::string created = std::to_string(m_Pushes.first().modified);
            query.addQueryItem("modified_after", QString::fromStdString(created));
            QUrl modifiedURL = m_URLPushes;
            modifiedURL.setQuery(query);
            getRequest(modifiedURL);
        }
    }
    else if (jsonObject["subtype"] == "device") {
        requestDeviceList();
    }
}

QString QPushbulletHandler::getDeviceNameFromDeviceID(QString deviceID)
{
    if (m_Devices.isEmpty())
        return "";
    QString name;
    for (int i = 0; i < m_Devices.count(); i++) {
        if (m_Devices.at(i).ID == deviceID) {
            name = m_Devices.at(i).nickname;
            break;
        }
    }
    return name;
}

void QPushbulletHandler::requestUploadFile(QString fileName, QString filePath)
{
    m_FilePath = filePath;
    m_FileName = fileName;
    QJsonDocument jsonDocument;
    QJsonObject jsonObject;
    jsonObject["file_name"] = fileName;
    jsonDocument.setObject(jsonObject);
    m_CurrentOperation = CURRENT_OPERATION::REQUEST_UPLOAD_FILE;
    postRequest(m_URLUploadRequest, jsonDocument.toJson());
}

void QPushbulletHandler::parseUploadRequestResponse(const QByteArray &data)
{
    QString strReply = (QString)data;
    QJsonDocument jsonResponse = QJsonDocument::fromJson(strReply.toUtf8());
    QJsonObject jsonObject = jsonResponse.object();
    QJsonObject obj = jsonObject["data"].toObject();

    QUrlQuery query;
//    query.addQueryItem("file", mFilePath.prepend('@'));
    query.addQueryItem("awsaccesskeyid", obj["awsaccesskeyid"].toString());
    query.addQueryItem("acl", obj["acl"].toString());
    query.addQueryItem("key", obj["key"].toString());
    query.addQueryItem("signature", obj["signature"].toString());
    query.addQueryItem("policy", obj["policy"].toString());
    query.addQueryItem("content-type", obj["content-type"].toString());

    //Upload file
    m_CurrentOperation = CURRENT_OPERATION::UPLOAD_FILE;
    postMultipart(QUrl(jsonObject["upload_url"].toString()), query);

    QUrl url(jsonObject["upload_url"].toString());
    url.setQuery(query);
    qDebug() << url.toString();
}

void QPushbulletHandler::postMultipart(QUrl url, QUrlQuery query)
{
    m_MultiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart imagePart;
    imagePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/json"));
    m_File = new QFile("/Users/Furkanzmc/Desktop/response.json");
    if (!m_File->open(QIODevice::ReadOnly))
        qDebug() << m_File->errorString();
    imagePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"response\";filename=\"response.json\""));
    imagePart.setBodyDevice(m_File);
    m_File->setParent(m_MultiPart); // we cannot delete the file now, so delete it with the multiPart

    m_MultiPart->append(imagePart);

    url.setQuery(query);
    QNetworkRequest request(url);
    request.setRawHeader(QString("Content-Type").toUtf8(), QString("multipart/form-data;boundary=\"margin\"").toUtf8());
    request.setRawHeader(QString("Content-Length").toUtf8(), QString(m_File->readAll().length()).toUtf8());

    qDebug() << "Length: " << QString(m_File->readAll());
    QNetworkReply *reply = m_NetworkManager.put(request, m_MultiPart);
    m_MultiPart->setParent(reply);
    connect(reply, SIGNAL(uploadProgress(qint64, qint64)), this, SLOT(uploadProgress(qint64, qint64)));
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(uploadError(QNetworkReply::NetworkError)));

}

void QPushbulletHandler::registerForRealTimeEventStream()
{
    //Connect QWebSocket signals
    connect(&m_WebSocket, SIGNAL(connected()), this, SLOT(webSocketConnected()));
    const QString socketURL = "wss://stream.pushbullet.com/websocket/" + m_APIKey;
    m_WebSocket.open(QUrl(socketURL));
}

QNetworkAccessManager::NetworkAccessibility QPushbulletHandler::getNetworkAccessibility()
{
    return m_NetworkAccessibility;
}

const DeviceList QPushbulletHandler::getDeviceList()
{
    return m_Devices;
}

const ContactList QPushbulletHandler::getContactList()
{
    return m_Contacts;
}

const PushList QPushbulletHandler::getPushList()
{
    return m_Pushes;
}
