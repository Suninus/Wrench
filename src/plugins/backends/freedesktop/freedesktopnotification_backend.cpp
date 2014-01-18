#include "freedesktopnotification_backend.h"

#include <QtGlobal>
#include <QDebug>
#include "core/notification/notification.h"
#include "core/notification/notification_p.h"
#include "core/snore_p.h"
#include <QtCore>
#include <QImage>
#include "fredesktopnotification.h"
#include "core/snore.h"


using namespace Snore;

Q_EXPORT_PLUGIN2 ( freedesktopnotificationbackend,FreedesktopBackend )


FreedesktopBackend::FreedesktopBackend () :
    SnoreBackend ( "FreedesktopNotification_Backend",true,true)
{
}

bool FreedesktopBackend::initialize(SnoreCore *snore){

    m_interface = new org::freedesktop::Notifications( "org.freedesktop.Notifications", "/org/freedesktop/Notifications",
                                                       QDBusConnection::sessionBus(), this );

    QDBusPendingReply<QStringList> reply = m_interface->GetCapabilities();
    reply.waitForFinished();
    QStringList caps  = reply.value();
    setSupportsRichtext(caps.contains( "body-markup" ));
    connect(m_interface, SIGNAL(ActionInvoked(uint,QString)), this, SLOT(slotActionInvoked(uint,QString)));
    connect(m_interface, SIGNAL(NotificationClosed(uint,uint)), this , SLOT(slotNotificationClosed(uint,uint)));

    return SnoreBackend::initialize(snore);
}

bool FreedesktopBackend::deinitialize()
{
    if(SnoreBackend::deinitialize())
    {
        disconnect(m_interface, SIGNAL(ActionInvoked(uint,QString)), this, SLOT(slotActionInvoked(uint,QString)));
        disconnect(m_interface, SIGNAL(NotificationClosed(uint,uint)), this , SLOT(slotNotificationClosed(uint,uint)));
        m_interface->deleteLater();
        m_interface = NULL;
        return true;
    }
    return false;
}

void  FreedesktopBackend::slotNotify ( Notification noti )
{    
    QStringList actions;
    foreach(int k,noti.actions().keys())
    {
        actions << QString::number(k) << noti.actions()[k].name();
    }
    QVariantMap hints;
    if(noti.icon().isValid())
    {
        FreedesktopImageHint image(noti.icon().image());
        hints["image_data"] = QVariant::fromValue(image);
    }

    if(noti.priority() != NotificationEnums::Prioritys::NORMAL)
    {
        hints["urgency"] = (char)noti.priority()+1;
    }

    if(noti.application().constHints().contains("desktop-entry"))
    {
        hints["desktop-entry"] = noti.application().constHints().value("desktop-entry");
    }

    uint updateId = 0;
    if(noti.updateID() != 0)
    {
        updateId = m_snoreIdMap[noti.updateID()];
        m_dbusIdMap[updateId] = noti.id();
        m_snoreIdMap[noti.id()] = updateId;
    }

    QString title = QString("%1 - %2").arg(noti.application().name(), noti.title());
    QString body(noti.text());
    if(!supportsRichtext())
    {
        title = Snore::toPlainText(title);
        body = Snore::toPlainText(body);
    }
    QDBusPendingReply<uint>  id = m_interface->Notify(noti.application().name(), updateId, "", title,
                                                      body, actions, hints, noti.sticky()?-1:noti.timeout()*1000);

    if(noti.updateID() == 0)
    {
        id.waitForFinished();
        m_snoreIdMap[noti.id()] = id.value();
        m_dbusIdMap[id.value()] = noti.id();
    }
}
void FreedesktopBackend::slotActionInvoked(const uint &id, const QString &actionID){
    Notification noti = getActiveNotificationByID(m_dbusIdMap[id]);
    if(!noti.isValid())
    {
        return;
    }
    noti.data()->setActionInvoked ( actionID.toInt() );
    snore()->d()->notificationActionInvoked ( noti );
}

void FreedesktopBackend::slotCloseNotification ( Notification notification )
{
    if(!m_snoreIdMap.contains(notification.id()))
    {
        return;
    }
    uint id = m_snoreIdMap.take(notification.id());
    m_dbusIdMap.remove(id);
    m_interface->CloseNotification(id);
}



void FreedesktopBackend::slotNotificationClosed ( const uint &id,const uint &reason )
{
    NotificationEnums::CloseReasons::closeReasons closeReason = NotificationEnums::CloseReasons::closeReasons(reason);
    qDebug() << Q_FUNC_INFO << "Closed" << id << "|" << closeReason << reason;
    if(id == 0)
    {
        return;
    }
    Notification noti =  getActiveNotificationByID(m_dbusIdMap.take(id));
    if(noti.isValid())
    {
        m_snoreIdMap.remove(noti.id());
        closeNotification(noti, closeReason);
    }
}




