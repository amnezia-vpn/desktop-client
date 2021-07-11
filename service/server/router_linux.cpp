#include "router_linux.h"

#include <QProcess>
#include <QThread>
#include <utils.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/route.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <paths.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <QFileInfo>

RouterLinux &RouterLinux::Instance()
{
    static RouterLinux s;
    return s;
}

bool RouterLinux::routeAdd(const QString &ipWithSubnet, const QString &gw, const int &sock)
{
    QString ip = Utils::ipAddressFromIpWithSubnet(ipWithSubnet);
    QString mask = Utils::netMaskFromIpWithSubnet(ipWithSubnet);

    if (!Utils::checkIPv4Format(ip) || !Utils::checkIPv4Format(gw)) {
        qCritical().noquote() << "Critical, trying to add invalid route: " << ip << gw;
        return false;
    }

    struct rtentry route;
    memset(&route, 0, sizeof( route ));

    // set gateway
    ((struct sockaddr_in *)&route.rt_gateway)->sin_family = AF_INET;
    ((struct sockaddr_in *)&route.rt_gateway)->sin_addr.s_addr = inet_addr(gw.toStdString().c_str());
    ((struct sockaddr_in *)&route.rt_gateway)->sin_port = 0;
    // set host rejecting
    ((struct sockaddr_in *)&route.rt_dst)->sin_family = AF_INET;
    ((struct sockaddr_in *)&route.rt_dst)->sin_addr.s_addr = inet_addr(ip.toStdString().c_str());
    ((struct sockaddr_in *)&route.rt_dst)->sin_port = 0;
    // set mask
    ((struct sockaddr_in *)&route.rt_genmask)->sin_family = AF_INET;
    ((struct sockaddr_in *)&route.rt_genmask)->sin_addr.s_addr = inet_addr(mask.toStdString().c_str());
    ((struct sockaddr_in *)&route.rt_genmask)->sin_port = 0;

    route.rt_flags = RTF_UP | RTF_GATEWAY;
    route.rt_metric = 0;
    //route.rt_dev = "ens33";

    if (int err = ioctl(sock, SIOCADDRT, &route) < 0)
    {
        qDebug().noquote() << "route add error: gw "
        << ((struct sockaddr_in *)&route.rt_gateway)->sin_addr.s_addr
        << " ip " << ((struct sockaddr_in *)&route.rt_dst)->sin_addr.s_addr
        << " mask " << ((struct sockaddr_in *)&route.rt_genmask)->sin_addr.s_addr << " " << err;
        return false;
    }
    return true;
}

int RouterLinux::routeAddList(const QString &gw, const QStringList &ips)
{
    int temp_sock = socket(AF_INET, SOCK_DGRAM,  IPPROTO_IP);
    int cnt = 0;
    for (const QString &ip: ips) {
        if (routeAdd(ip, gw, temp_sock)) cnt++;
    }
    close(temp_sock);
    return cnt;
}

bool RouterLinux::clearSavedRoutes()
{
    // No need to delete routes after iface down
    return true;

//    int cnt = 0;
//    for (const QString &ip: m_addedRoutes) {
//        if (routeDelete(ip)) cnt++;
//    }
    //    return (cnt == m_addedRoutes.count());
}

bool RouterLinux::routeDelete(const QString &ipWithSubnet, const QString &gw, const int &sock)
{
    QString ip = Utils::ipAddressFromIpWithSubnet(ipWithSubnet);
    QString mask = Utils::netMaskFromIpWithSubnet(ipWithSubnet);

    if (!Utils::checkIPv4Format(ip) || !Utils::checkIPv4Format(gw)) {
        qCritical().noquote() << "Critical, trying to remove invalid route: " << ip << gw;
        return false;
    }

    if (ip == "0.0.0.0") {
        qDebug().noquote() << "Warning, trying to remove default route, skipping: " << ip << gw;
        return true;
    }

    struct rtentry route;
    memset(&route, 0, sizeof( route ));

    // set gateway
    ((struct sockaddr_in *)&route.rt_gateway)->sin_family = AF_INET;
    ((struct sockaddr_in *)&route.rt_gateway)->sin_addr.s_addr = inet_addr(gw.toStdString().c_str());
    ((struct sockaddr_in *)&route.rt_gateway)->sin_port = 0;
    // set host rejecting
    ((struct sockaddr_in *)&route.rt_dst)->sin_family = AF_INET;
    ((struct sockaddr_in *)&route.rt_dst)->sin_addr.s_addr = inet_addr(ip.toStdString().c_str());
    ((struct sockaddr_in *)&route.rt_dst)->sin_port = 0;
    // set mask
    ((struct sockaddr_in *)&route.rt_genmask)->sin_family = AF_INET;
    ((struct sockaddr_in *)&route.rt_genmask)->sin_addr.s_addr = inet_addr(mask.toStdString().c_str());
    ((struct sockaddr_in *)&route.rt_genmask)->sin_port = 0;

    route.rt_flags = RTF_UP | RTF_GATEWAY;
    route.rt_metric = 0;
    //route.rt_dev = "ens33";

    if (ioctl(sock, SIOCDELRT, &route) < 0)
    {
        qDebug().noquote() << "route delete error: gw " << gw << " ip " << ip << " mask " << mask;
        return false;
    }
    return true;
}

bool RouterLinux::routeDeleteList(const QString &gw, const QStringList &ips)
{
    int temp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    int cnt = 0;
    for (const QString &ip: ips) {
        if (routeDelete(ip, gw, temp_sock)) cnt++;
    }
    close(temp_sock);
    return cnt;
}

void RouterLinux::flushDns()
{
    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);

    //check what the dns manager use
    if (QFileInfo::exists("/usr/bin/nscd")
        || QFileInfo::exists("/usr/sbin/nscd")
        || QFileInfo::exists("/usr/lib/systemd/system/nscd.service"))
    {
        p.start("systemctl restart nscd");
    }
    else
    {
        p.start("systemctl restart systemd-resolved");
    }

    p.waitForFinished();
    QByteArray output(p.readAll());
    if (output.isEmpty())
        qDebug().noquote() << "Flush dns completed";
    else
        qDebug().noquote() << "OUTPUT systemctl restart nscd/systemd-resolved: " + output;
}
