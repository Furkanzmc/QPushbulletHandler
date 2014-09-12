QPushBulletHandler
==================
QPushBulletHandler is a library written in Qt/C++ to easily communicate with Pushbullet service. QPushBulletHandler uses signals and slots to achieve POST and GET requests.

Features
========
* Get push history
* Push to device
* Push to contact
* Push to all devices
* Update push
* Delete push
* Get device list
* Create device
* Update device
* Delete device
* Create contact
* Get contact list
* Update contact
* Delete contact
* Subscribe to real time event stream to be notified about mobile notifications and new pushes/devices 

Usage
======
##Authentication
Get the API key from your account page on Pushbullet.

```C++
QPushBulletHandler handler(<APIKEY>);
```

##Connecting signals and slots
Every request has a signal that gets emitted when the request is over. So If you want to get feedback on your requests, you have to connect the signal to your slot.
```C++
connect(&handler, SIGNAL(didReceiveDevices(DeviceList)), this, SLOT(receiveDevices(DeviceList)));
```
After that connection, whenever you request the device list the didReceiveDevices(DeviceList) signal gets emitted with the device list you requested. Every request that has a return value works that way. For some requests, it's not necessary because it only returns what you already posted, like pushing notes.

#Pushing Stuff
There are no seperate signals for each push type, but only one which contains the information about that push.
```C++
connect(&handler, SIGNAL(didPush(Push)), this, SLOT(pushResponse(Push)));

void pushResponse(Push push)
{
  //The Push struct contains all the necessary fields for a push. Push::type is a PUSH_TYPE enum which contains the types of pushes.
  std::cout << push.title.toStdString() << std::endl;
  std::cout << push.body.toStdString() << std::endl;
  if (push.type == PUSH_TYPE::NOTE)
    std::cout << "Push type is note" << std::endl;
}
```
Every push reqest requires a Push struct with the necessary fields filled in. You don't have to fill the whole variables since they are not needed to push it.

You have three ways to send a push:

1. QPushBulletHandler::requestPushToAllDevices -> Pushes the note to all available devices
2. QPushBulletHandler::requestPushToContact -> Pushes the note to a contact with the given email
3. QPushBulletHandler::requestPushToDevice -> Pushes the note to a device with the given device ID

###Push Note
You create a new Push to fill the information.
```C++
Push p;
p.type = PUSH_TYPE::NOTE;
p.title = "Hello World!";
p.body = "And Hello to you, too!"
//Then you can use of the three ways to send the push
handler.requestPushToAllDevices(p);
handler.requestPushToContact(p, <EMAIL>);
handler.requestPushToDevice(p, <DEVICE_ID>);
```
When the request is done, it will emit the didPush signal.

###Push Address
```C++
Push p;
p.type = PUSH_TYPE::ADDRESS;
p.addressName = "Address Name";
p.address = "The address";
//Then you can use of the three ways to send the push
handler.requestPushToAllDevices(p);
handler.requestPushToContact(p, <EMAIL>);
handler.requestPushToDevice(p, <DEVICE_ID>);
```
###Pushing A List
```C++
Push p;
p.type = QPushBulletHandler::PUSH_TYPE::LIST;
p.title = "My List";
QStringList items;
items.append("Item1");
items.append("Item2");
p.listItems = items;
//Then you can use of the three ways to send the push
handler.requestPushToAllDevices(p);
handler.requestPushToContact(p, <EMAIL>);
handler.requestPushToDevice(p, <DEVICE_ID>);
```

###Pushing A Link
```C++
Push p;
p.type = QPushBulletHandler::PUSH_TYPE::LINK;
p.title = "My Link";
p.url = "www.somesite.com";
p.body = "Optional Body";
//Then you can use of the three ways to send the push
handler.requestPushToAllDevices(p);
handler.requestPushToContact(p, <EMAIL>);
handler.requestPushToDevice(p, <DEVICE_ID>);
```

##Operations on Pushes
###Get Push History
You can fetch the push history after making the following connection
```C++
connect(&handler, SIGNAL(didReceivePushHistory(PushList)), this, SLOT(pushesReceived(PushList)));
handler.requestPushHistory();
//You can alternatively use a timestamp to get the pushes after that time. Make sure to use Push type's modified or created variable.
handler.requestPushHistory(p.modified);
```
###Update a Push
Updating a push only changes its dismissed value.
```C++
//Connect the signal to your slot to b notified of the updated push
connect(&handler, SIGNAL(didPushUpdate(Push)), this, SLOT(pushUpdated(Push)));
```
handler.requestPushUpdate(p.ID, true);

###Delete a Push
```C++
//Connect the signal to your slot to b notified of the deletion
connect(&handler, SIGNAL(didPushDelete()), this, SLOT(pushDeleted()));
handler.requestPushDelete(p.ID);
```

##Working with Contacts
Contacts work like devices, but instead of device ID contacts have email.

###Getting Contact List
Getting the contact list requires the call of the QPushBulletHandler::requestContactList() method. Like pushes, you need to connect the didReceiveContacts(const ContactList&) signal to a slot to access the list.
```C++
//Connect the signal to your slot to b notified of the deletion
connect(&handler, SIGNAL(didReceiveContacts(const ContactList&)), this, SLOT(contactListReceived(const ContactList&)));
handler.requestContactList();

//Then inside the contactListReceived function you can access the contacts
void contactListReceived(const ContactList &contacts)
{
    for (int i = 0; i < contacts.count(); i++)
        std::cout << contacts.at(i).name.toStdString() << " - " << contacts.at(i).ID.toStdString() << std::endl;
}
```

###Updating a Contact
Contact updates can only change the name of the contact. The signal returns the updated contact
```C++
connect(&handler, SIGNAL(didContactUpdate(const Contact&)), this, SLOT(contactUpdated(const Contact&)));
handler.requestContactUpdate(Contact::email, "New Name");
```

###Deleting a Contact
```C++
//Connect the signal to your slot to b notified of the deletion
connect(&handler, SIGNAL(didContactDelete()), this, SLOT(contactDeleted()));
handler.requestContactDelete(Contact::email);
```

##Working with Devices
###Getting Device List
```C++
connect(&handler, SIGNAL(didReceiveDevices(const DeviceList&)), this, SLOT(receivedDevices(const DeviceList&)));
handler.requestDeviceList();
```

###Create a Device
```C++
connect(&handler, SIGNAL(didDeviceCreate(const Device&)), this, SLOT(deviceCreated(const Device&)));
handler.requestCreateDevice("Device Name", "Model");
```

###Update a Device
```C++
connect(&handler, SIGNAL(didDeviceUpdate(const Device&)), this, SLOT(deviceUpdated(const Device&)));
handler.requestDeviceUpdate(Device::ID, "New Device Name");
```

###Delete a Device
```C++
connect(&handler, SIGNAL(didDeviceDelete()), this, SLOT(deviceDeleted()));
handler.requestDeviceDelete(Device::ID);
```

##Subscribe to Real Time Event Stream
Subscribing to the stream is easy and only requires one function call.
```C++
handler.registerForRealTimeEventStream();
```
After you subscribe to the stream, you get tickles which tell you something has changed on the server and mirror notifications from your devices. To use the stream you must connect three signals to your slots.
```C++
//This is required for mirror notifications
connect(&handler, SIGNAL(didReceiveMirrorPush(const MirrorPush&)), this, SLOT(mirrorPush(const MirrorPush&)));
connect(&handler, SIGNAL(didReceivePushHistory(const PushList&)), this, SLOT(pushesReceived(const PushList&)));
connect(&handler, SIGNAL(didReceiveDevices(const DeviceList&)), this, SLOT(receivedDevices(const DeviceList&)));
```
These two connections are already used for the push and device operations. So tickles doesn't require extra connections. You only need the connection for the mirror notifications.

##Error Handling
The first variable returns the error messages from Pushbullet and the second one returs error coming from other sources.
```C++
connect(&handler, SIGNAL(didReceiveError(QString,QByteArray)), this, SLOT(receivedError(QString,QByteArray)));
```
