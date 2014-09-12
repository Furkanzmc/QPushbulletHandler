#include "QPushBulletHandler.h"
#include <QDebug>

//TODO: Set the api key on the contructor
QPushBulletHandler::QPushBulletHandler(QString apiKey)
    : mNetworkManager()
    , mWebSocket()
    , mURLContacts("https://api.pushbullet.com/v2/contacts")
    , mURLDevices("https://api.pushbullet.com/v2/devices")
    , mURLMe("https://api.pushbullet.com/v2/users/me")
    , mURLPushes("https://api.pushbullet.com/v2/pushes")
    , mURLUploadRequest("https://api.pushbullet.com/v2/upload-request")
    , mAPIKey(apiKey)
    , mNetworkAccessibility(QNetworkAccessManager::NetworkAccessibility::UnknownAccessibility)
{
    //Connect the QNetworkAccessManager signals
    connect(&mNetworkManager, SIGNAL(finished(QNetworkReply *)), this, SLOT(handleNetworkData(QNetworkReply *)));
    connect(&mNetworkManager, SIGNAL(networkSessionConnected()), this, SLOT(sessionConnected()));
    connect(&mNetworkManager, SIGNAL(networkAccessibleChanged(QNetworkAccessManager::NetworkAccessibility)), this
            , SLOT(handleNetworkAccessibilityChange(QNetworkAccessManager::NetworkAccessibility)));
}

void QPushBulletHandler::getRequest(QUrl url)
{
    qDebug() << "Get Request";
    url.setUserName(mAPIKey);
    mNetworkManager.get(QNetworkRequest(url));
}

void QPushBulletHandler::postRequest(QUrl url, const QByteArray &data)
{
    qDebug() << "Post Request";
    QNetworkRequest request(url);
    if (mCurrentOperation == CURRENT_OPERATION::UPLOAD_FILE) {
        request.setRawHeader(QString("Content-Type").toUtf8(), QString("multipart/form-data; boundary=margin").toUtf8());
    }
    else {
        url.setUserName(mAPIKey);
        request.setUrl(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    }
    if (mCurrentOperation == CURRENT_OPERATION::DELETE_CONTACT || mCurrentOperation == CURRENT_OPERATION::DELETE_DEVICE
        || mCurrentOperation == CURRENT_OPERATION::DELETE_PUSH)
        mNetworkManager.deleteResource(QNetworkRequest(url));
    else
        mNetworkManager.post(request, data);
}

void QPushBulletHandler::handleNetworkData(QNetworkReply *networkReply)
{
    if (networkReply->error()) {
        qDebug() << "Error String: " << networkReply->errorString();
        QByteArray response(networkReply->readAll());
        qDebug() << QString(response);
        emit didReceiveError(networkReply->errorString(), response);
        return;
    }

    QByteArray response(networkReply->readAll());
    if (mCurrentOperation == CURRENT_OPERATION::GET_DEVICE_LIST) {
        parseDeviceResponse(response);
    }
    else if (mCurrentOperation == CURRENT_OPERATION::CREATE_DEVICE) {
        parseCreateDeviceResponse(response);
    }
    else if (mCurrentOperation == CURRENT_OPERATION::DELETE_DEVICE) {
        emit didDeviceDelete();
    }
    else if (mCurrentOperation == CURRENT_OPERATION::UPDATE_DEVICE) {
        parseUpdateDeviceResponce(response);
    }
    else if (mCurrentOperation == CURRENT_OPERATION::GET_CONTACT_LIST) {
        parseContactResponse(response);
    }
    else if (mCurrentOperation == CURRENT_OPERATION::CREATE_CONTACT) {
        parseCreateContactResponse(response);
    }
    else if (mCurrentOperation == CURRENT_OPERATION::UPDATE_CONTACT) {
        parseUpdateContactResponse(response);
    }
    else if (mCurrentOperation == CURRENT_OPERATION::DELETE_CONTACT) {
        emit didContactDelete();
    }
    else if (mCurrentOperation == CURRENT_OPERATION::GET_PUSH_HISTORY) {
        parsePushHistoryResponse(response);
    }
    else if (mCurrentOperation == CURRENT_OPERATION::PUSH) {
        parsePushResponse(response);
    }
    else if (mCurrentOperation == CURRENT_OPERATION::PUSH_UPDATE) {
        parsePushResponse(response);
    }
    else if (mCurrentOperation == CURRENT_OPERATION::UPDATE_PUSH_LIST) {
        parsePushHistoryResponse(response);
    }
    else if (mCurrentOperation == CURRENT_OPERATION::DELETE_PUSH) {
        emit didPushDelete();
    }
    else if (mCurrentOperation == CURRENT_OPERATION::REQUEST_UPLOAD_FILE) {
        parseUploadRequestResponse(response);
    }
    else if (mCurrentOperation == CURRENT_OPERATION::UPLOAD_FILE) {
        QString str(response);
        qDebug() << str;
    }

    mCurrentOperation = CURRENT_OPERATION::NONE;
}

void QPushBulletHandler::sessionConnected()
{
    qDebug() << "Connected";
}

void QPushBulletHandler::handleNetworkAccessibilityChange(QNetworkAccessManager::NetworkAccessibility change)
{
    mNetworkAccessibility = change;
    if (mNetworkAccessibility == QNetworkAccessManager::UnknownAccessibility)
        qDebug() << "Unknown network accessibility";
    else if (mNetworkAccessibility == QNetworkAccessManager::Accessible)
        qDebug() << "Network is accessible";
    else if (mNetworkAccessibility == QNetworkAccessManager::NotAccessible)
        qDebug() << "Network is not accessible";
}

void QPushBulletHandler::webSocketConnected()
{
    qDebug() << "Web Socket Connected";
    connect(&mWebSocket, SIGNAL(textMessageReceived(QString)), this, SLOT(textMessageReceived(QString)));
}

void QPushBulletHandler::webSocketDisconnected()
{
    qDebug() << "Web Socket Disconnected";
}

void QPushBulletHandler::textMessageReceived(QString message)
{
    parseMirrorPush(message);
}

void QPushBulletHandler::requestDeviceList()
{
    mCurrentOperation = CURRENT_OPERATION::GET_DEVICE_LIST;
    getRequest(mURLDevices);
}

void QPushBulletHandler::requestCreateDevice(QString deviceName, QString model)
{
    QUrlQuery query;
    query.addQueryItem("nickname", deviceName);
    if (!model.isEmpty())
        query.addQueryItem("manufacturer", model);
    query.addQueryItem("type", "stream");
    mCurrentOperation = CURRENT_OPERATION::CREATE_DEVICE;
    postRequest(mURLDevices, query.toString(QUrl::FullyDecoded).toUtf8());
}

void QPushBulletHandler::requestDeviceUpdate(QString deviceID, QString newNickname)
{
    QString url = mURLDevices.toString();
    url.append("/");
    url.append(deviceID);
    QUrl modifiedURL(url);
    QUrlQuery query;
    query.addQueryItem("nickname", newNickname);
    mCurrentOperation = CURRENT_OPERATION::UPDATE_DEVICE;
    postRequest(modifiedURL, query.toString(QUrl::FullyEncoded).toUtf8());
}

void QPushBulletHandler::requestDeviceDelete(QString deviceID)
{
    QString url = mURLDevices.toString();
    url.append("/");
    url.append(deviceID);
    QUrl modifiedURL(url);
    mCurrentOperation = CURRENT_OPERATION::DELETE_DEVICE;
    QUrlQuery query;
    query.setQueryDelimiters(' ', '&');
    query.addQueryItem("-X", "DELETE");
    postRequest(modifiedURL, query.toString(QUrl::FullyEncoded).toUtf8());
}

void QPushBulletHandler::requestContactList()
{
    mCurrentOperation = CURRENT_OPERATION::GET_CONTACT_LIST;
    getRequest(mURLContacts);
}

void QPushBulletHandler::requestCreateContact(QString name, QString email)
{
    QUrlQuery query;
    query.addQueryItem("name", name);
    query.addQueryItem("email", email);
    mCurrentOperation = CURRENT_OPERATION::CREATE_CONTACT;
    postRequest(mURLContacts, query.toString(QUrl::FullyEncoded).toUtf8());
}

void QPushBulletHandler::requestContactUpdate(QString contactID, QString newName)
{
    QString url = mURLContacts.toString();
    url.append("/");
    url.append(contactID);
    QUrl modifiedURL(url);
    QUrlQuery query;
    query.addQueryItem("name", newName);
    mCurrentOperation = CURRENT_OPERATION::UPDATE_CONTACT;
    postRequest(modifiedURL, query.toString(QUrl::FullyEncoded).toUtf8());
}

void QPushBulletHandler::requestContactDelete(QString contactID)
{
    QString url = mURLContacts.toString();
    url.append("/");
    url.append(contactID);
    QUrl modifiedURL(url);
    mCurrentOperation = CURRENT_OPERATION::DELETE_CONTACT;
    QUrlQuery query;
    query.setQueryDelimiters(' ', '&');
    query.addQueryItem("-X", "DELETE");

    postRequest(modifiedURL, query.toString(QUrl::FullyEncoded).toUtf8());
}

void QPushBulletHandler::requestPushHistory()
{
    mCurrentOperation = CURRENT_OPERATION::GET_PUSH_HISTORY;
    getRequest(mURLPushes);
}

void QPushBulletHandler::requestPush(Push push, QString deviceID, QString email)
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
    mCurrentOperation = CURRENT_OPERATION::PUSH;

    jsonDocument.setObject(jsonObject);
    qDebug() << QString(jsonDocument.toJson());
    postRequest(mURLPushes, jsonDocument.toJson());
}

void QPushBulletHandler::requestPushToDevice(Push push, QString deviceID)
{
    requestPush(push, deviceID, "");
}

void QPushBulletHandler::requestPushToContact(Push push, QString email)
{
    requestPush(push, "", email);
}

void QPushBulletHandler::requestPushToAllDevices(Push push)
{
    requestPush(push, "", "");
}

void QPushBulletHandler::requestPushUpdate(QString pushID, bool dismissed)
{
    QUrlQuery query;
    query.addQueryItem("dismissed", dismissed ? "true" : "false");
    QString url = mURLPushes.toString();
    url.append("/");
    url.append(pushID);
    QUrl modifiedURL(url);
    mCurrentOperation = CURRENT_OPERATION::PUSH_UPDATE;
    postRequest(modifiedURL, query.toString(QUrl::FullyEncoded).toUtf8());
}

void QPushBulletHandler::requestPushDelete(QString pushID)
{
    QString url = mURLPushes.toString();
    url.append("/");
    url.append(pushID);
    QUrl modifiedURL(url);
    mCurrentOperation = CURRENT_OPERATION::DELETE_PUSH;
    QUrlQuery query;
    query.setQueryDelimiters(' ', '&');
    query.addQueryItem("-X", "DELETE");

    postRequest(modifiedURL, query.toString(QUrl::FullyEncoded).toUtf8());
}

void QPushBulletHandler::parseDeviceResponse(const QByteArray &data)
{
    mDevices.clear();
    QString strReply = (QString)data;
    QJsonDocument jsonResponse = QJsonDocument::fromJson(strReply.toUtf8());
    QJsonObject jsonObject = jsonResponse.object();
    QJsonArray jsonArray = jsonObject["devices"].toArray();

    foreach(const QJsonValue & value, jsonArray) {
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
        mDevices.append(device);
    }
    emit didReceiveDevices(mDevices);
}

void QPushBulletHandler::parseCreateDeviceResponse(const QByteArray &data)
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

void QPushBulletHandler::parseUpdateDeviceResponce(const QByteArray &data)
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

void QPushBulletHandler::parseContactResponse(const QByteArray &data)
{
    mContacts.clear();

    QString strReply = (QString)data;
    QJsonDocument jsonResponse = QJsonDocument::fromJson(strReply.toUtf8());
    QJsonObject jsonObject = jsonResponse.object();
    QJsonArray jsonArray = jsonObject["contacts"].toArray();

    foreach(const QJsonValue & value, jsonArray) {
        QJsonObject obj = value.toObject();
        Contact contact;
        contact.name = obj["name"].toString();
        if (contact.name.isEmpty())
            continue;
        contact.email = obj["email"].toString();
        contact.ID = obj["iden"].toString();
        mContacts.append(contact);
    }
    emit didReceiveContacts(mContacts);
}

void QPushBulletHandler::parseCreateContactResponse(const QByteArray &data)
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

void QPushBulletHandler::parseUpdateContactResponse(const QByteArray &data)
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

void QPushBulletHandler::parsePushHistoryResponse(const QByteArray &data)
{
    if (mCurrentOperation != CURRENT_OPERATION::UPDATE_PUSH_LIST)
        mPushes.clear();
    QString strReply = (QString)data;
    QJsonDocument jsonResponse = QJsonDocument::fromJson(strReply.toUtf8());
    QJsonObject jsonObject = jsonResponse.object();
    QJsonArray jsonArray = jsonObject["pushes"].toArray();

    foreach(const QJsonValue & value, jsonArray) {
        QJsonObject jsonObject = value.toObject();
        Push push;

        if (jsonObject["active"].toBool() == false)
            continue;
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
            qDebug() << push.body;
        }
        else if (push.type == getPushTypeFromString("file")) {
            push.fileName = jsonObject["file_name"].toString();
            push.fileType = jsonObject["file_type"].toString();
            push.fileURL = jsonObject["file_url"].toString();
            push.body = jsonObject["body"].toString();
        }

        mPushes.append(push);
    }
    emit didReceivePushHistory(mPushes);
}

void QPushBulletHandler::parsePushResponse(const QByteArray &data)
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
    if (mCurrentOperation == CURRENT_OPERATION::PUSH_UPDATE)
        emit didPushUpdate(push);
    else
        emit didPush(push);
}

QPushBulletHandler::PUSH_TYPE QPushBulletHandler::getPushTypeFromString(QString type)
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

void QPushBulletHandler::parseMirrorPush(QString data)
{
    QString strReply = (QString)data;
    QJsonDocument jsonResponse = QJsonDocument::fromJson(strReply.toUtf8());
    QJsonObject jsonObject = jsonResponse.object();
    if (jsonObject["type"] != "push") {
        if (jsonObject["type"] == "tickle") {
            parseTickle(jsonObject);
        }
        return;
    }
    QJsonObject obj = jsonObject["push"].toObject();

    MirrorPush mirror;

    mirror.type = obj["type"].toString();
    mirror.applicationName = obj["application_name"].toString();
    mirror.body = obj["body"].toString();
    mirror.dismissable = obj["dismissable"].toString() == "true" ? true : false;
    mirror.notificationID = obj["notification_id"].toInt();
    mirror.sourceDeviceID = obj["source_device_iden"].toString();
    mirror.sourceUserID = obj["source_user_iden"].toString();
    mirror.title = obj["title"].toString();
    mirror.sourceDeviceNickname = getDeviceNameFromDeviceID(mirror.sourceDeviceID);

    emit didReceiveMirrorPush(mirror);
}

void QPushBulletHandler::parseTickle(QJsonObject jsonObject)
{
    if (jsonObject["subtype"] == "push") {
        if (mPushes.isEmpty()) {
            requestPushHistory();
        }
        else {
            mCurrentOperation = CURRENT_OPERATION::UPDATE_PUSH_LIST;
            QUrlQuery query;
            std::string created = std::to_string(mPushes.first().modified);
            query.addQueryItem("modified_after", QString::fromStdString(created));
            QUrl modifiedURL = mURLPushes;
            modifiedURL.setQuery(query);
            getRequest(mURLPushes);
        }
    }
    else if (jsonObject["subtype"] == "device") {
        requestDeviceList();
    }
}

QString QPushBulletHandler::getDeviceNameFromDeviceID(QString deviceID)
{
    if (mDevices.isEmpty())
        return "";
    QString name;
    for (int i = 0; i < mDevices.count(); i++) {
        if (mDevices.at(i).ID == deviceID) {
            name = mDevices.at(i).nickname;
            break;
        }
    }
    return name;
}

void QPushBulletHandler::requestUploadFile(QString fileName, QString filePath)
{
    mFilePath = filePath;
    mFileName = fileName;
    QJsonDocument jsonDocument;
    QJsonObject jsonObject;
    jsonObject["file_name"] = fileName;
    jsonDocument.setObject(jsonObject);
    mCurrentOperation = CURRENT_OPERATION::REQUEST_UPLOAD_FILE;
    postRequest(mURLUploadRequest, jsonDocument.toJson());
}

void QPushBulletHandler::parseUploadRequestResponse(const QByteArray &data)
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
    mCurrentOperation = CURRENT_OPERATION::UPLOAD_FILE;
    postMultipart(QUrl(jsonObject["upload_url"].toString()), query);

    QUrl url(jsonObject["upload_url"].toString());
    url.setQuery(query);
    qDebug() << url.toString();
}

void QPushBulletHandler::postMultipart(QUrl url, QUrlQuery query)
{
    multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart imagePart;
    imagePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/json"));
    file = new QFile("/Users/Furkanzmc/Desktop/response.json");
    if (!file->open(QIODevice::ReadOnly))
        qDebug() << file->errorString();
    imagePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"response\";filename=\"response.json\""));
    imagePart.setBodyDevice(file);
    file->setParent(multiPart); // we cannot delete the file now, so delete it with the multiPart

    multiPart->append(imagePart);

    url.setQuery(query);
    QNetworkRequest request(url);
    request.setRawHeader(QString("Content-Type").toUtf8(), QString("multipart/form-data;boundary=\"margin\"").toUtf8());
    request.setRawHeader(QString("Content-Length").toUtf8(), QString(file->readAll().length()).toUtf8());

    qDebug() << "Length: " << QString(file->readAll());
    QNetworkReply *reply = mNetworkManager.put(request, multiPart);
    multiPart->setParent(reply);
    connect(reply, SIGNAL(uploadProgress(qint64, qint64)), this, SLOT(uploadProgress(qint64, qint64)));
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(uploadError(QNetworkReply::NetworkError)));

}

void QPushBulletHandler::registerForRealTimeEventStream()
{
    //Connect QWebSocket signals
    connect(&mWebSocket, SIGNAL(connected()), this, SLOT(webSocketConnected()));
    const QString socketURL = "wss://stream.pushbullet.com/websocket/" + mAPIKey;
    mWebSocket.open(QUrl(socketURL));
}

QNetworkAccessManager::NetworkAccessibility QPushBulletHandler::getNetworkAccessibility()
{
    return mNetworkAccessibility;
}
