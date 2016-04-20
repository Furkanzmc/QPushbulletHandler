#ifndef PUSHBULLETHANDLER_H
#define PUSHBULLETHANDLER_H
#include <QObject>
#include <QtNetwork>
#include <QtWebSockets>

enum class PUSH_TYPE {
    NOTE,
    LINK,
    LIST,
    ADDRESS,
    FILE,
    NONE
};

struct Device {
    QString ID, pushToken;
    int appVersion;
    bool active;
    QString nickname, manufacturer, type;
    bool pushable;
};
struct Contact {
    QString ID, name, email;
};
struct Push {
    QString ID, title, body, url, targetDeviceID, senderEmail, receiverEmail, addressName, address, fileName, fileType,
            fileURL;
    PUSH_TYPE type;
    double modified, created;
    QStringList listItems;
    bool isActive;
};
struct MirrorPush {
    QString type = "", subtype = "";
};

typedef QList<Device> DeviceList;
typedef QList<Contact> ContactList;
typedef QList<Push> PushList;

//NOTE: Do not call any requests before the current request is finished
class QPushbulletHandler : public QObject
{
    Q_OBJECT

public:
    QPushbulletHandler(QString apiKey);

public:
    enum class CURRENT_OPERATION {
        GET_DEVICE_LIST,
        GET_CONTACT_LIST,
        GET_PUSH_HISTORY,
        PUSH,
        PUSH_UPDATE,
        DELETE_PUSH,
        UPDATE_PUSH_LIST,
        CREATE_DEVICE,
        UPDATE_DEVICE,
        DELETE_DEVICE,
        CREATE_CONTACT,
        UPDATE_CONTACT,
        DELETE_CONTACT,
        REQUEST_UPLOAD_FILE,
        UPLOAD_FILE,
        NONE
    };

private:
    DeviceList m_Devices;
    ContactList m_Contacts;
    PushList m_Pushes;
    CURRENT_OPERATION m_CurrentOperation;

    QNetworkAccessManager m_NetworkManager;
    QWebSocket m_WebSocket;
    const QUrl m_URLContacts,
          m_URLDevices,
          m_URLMe,
          m_URLPushes,
          m_URLUploadRequest;
    const QString m_APIKey;
    QNetworkAccessManager::NetworkAccessibility m_NetworkAccessibility;

    QString m_FilePath, m_FileName;
    QHttpMultiPart *m_MultiPart;
    QFile *m_File;

signals:
    void didReceiveDevices(const DeviceList &devices);
    void didDeviceCreate(const Device &device);
    void didDeviceUpdate(const Device &device);
    void didDeviceDelete();

    void didReceiveContacts(const ContactList &contacts);
    void didContactCreate(const Contact &contact);
    void didContactUpdate(const Contact &contact);
    void didContactDelete();

    void didReceivePushHistory(const PushList &pushes);
    void didPush(const Push &push);
    void didPushUpdate(const Push &push);
    void didPushDelete();

    void didReceiveMirrorPush(const MirrorPush &mirror);

    /**
     * @brief Gets emitted when QNetworkAccessManager encounters a problem
     * @param pushBulletErrorString
     * @param errorFromServer
     */
    void didReceiveError(const QNetworkReply *networkReply);

private slots:
    void handleNetworkData(QNetworkReply *networkReply);
    void sessionConnected();
    void handleNetworkAccessibilityChange(QNetworkAccessManager::NetworkAccessibility change);
    void webSocketConnected();
    void webSocketDisconnected();
    void textMessageReceived(QString message);

private:
    void getRequest(QUrl url);
    void postRequest(QUrl url, const QByteArray &data);

    void parseDeviceResponse(const QByteArray &data);
    void parseCreateDeviceResponse(const QByteArray &data);
    void parseUpdateDeviceResponce(const QByteArray &data);

    void parseContactResponse(const QByteArray &data);
    void parseCreateContactResponse(const QByteArray &data);
    void parseUpdateContactResponse(const QByteArray &data);

    void parsePushHistoryResponse(const QByteArray &data);
    void parsePushResponse(const QByteArray &data);

    void parseMirrorPush(QString data);
    void parseTickle(QJsonObject jsonObject);

    void requestPush(Push &push, QString deviceID, QString email);

    PUSH_TYPE getPushTypeFromString(QString type);
    QString getDeviceNameFromDeviceID(QString deviceID);

    void requestUploadFile(QString fileName, QString filePath);
    void parseUploadRequestResponse(const QByteArray &data);

    void postMultipart(QUrl url, QUrlQuery query);

public:
    void requestDeviceList();
    void requestCreateDevice(QString deviceName, QString model);
    void requestDeviceUpdate(QString deviceID, QString newNickname);
    void requestDeviceDelete(QString deviceID);

    void requestContactList();
    void requestCreateContact(QString name, QString email);
    void requestContactUpdate(QString contactID, QString newName);
    void requestContactDelete(QString contactID);

    void requestPushHistory();
    void requestPushHistory(double modifiedAfter);
    void requestPushToDevice(Push &push, QString deviceID);
    void requestPushToContact(Push &push, QString email);
    void requestPushToAllDevices(Push &push);
    void requestPushUpdate(QString pushID, bool dismissed);
    void requestPushDelete(QString pushID);

    /**
     * @brief registerForRealTimeEventStream to be notified about new pushes/devices and mobile notifications
     */
    void registerForRealTimeEventStream();

    QNetworkAccessManager::NetworkAccessibility getNetworkAccessibility();
    /**
     * @brief Returns the local DeviceList without any requests to the server
     * @return
     */
    const DeviceList getDeviceList();
    /**
     * @brief Returns the local ContactList without any requests to the server
     * @return
     */
    const ContactList getContactList();
    /**
     * @brief Returns the local PushList without any requests to the server
     * @return
     */
    const PushList getPushList();

};

#endif // PUSHBULLETHANDLER_H

