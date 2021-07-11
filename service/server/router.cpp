#include "router.h"

#ifdef Q_OS_WIN
#include "router_win.h"
#elif defined (Q_OS_MAC)
#include "router_mac.h"
#elif defined Q_OS_LINUX
#include "router_linux.h"
#endif


int Router::routeAddList(const QString &gw, const QStringList &ips)
{
#ifdef Q_OS_WIN
    return RouterWin::Instance().routeAddList(gw, ips);
#elif defined (Q_OS_MAC)
    return RouterMac::Instance().routeAddList(gw, ips);
#elif defined Q_OS_LINUX
    return RouterLinux::Instance().routeAddList(gw, ips);
#endif
}

bool Router::clearSavedRoutes()
{
#ifdef Q_OS_WIN
    return RouterWin::Instance().clearSavedRoutes();
#elif defined (Q_OS_MAC)
    return RouterMac::Instance().clearSavedRoutes();
#elif defined Q_OS_LINUX
    return RouterLinux::Instance().clearSavedRoutes();
#endif
}

int Router::routeDeleteList(const QString &gw, const QStringList &ips)
{
#ifdef Q_OS_WIN
    return RouterWin::Instance().routeDeleteList(gw, ips);
#elif defined (Q_OS_MAC)
    return RouterMac::Instance().routeDeleteList(gw, ips);
#elif defined Q_OS_LINUX
    return RouterLinux::Instance().routeDeleteList(gw, ips);
#endif
}

void Router::flushDns()
{
#ifdef Q_OS_WIN
    RouterWin::Instance().flushDns();
#elif defined (Q_OS_MAC)
    RouterMac::Instance().flushDns();
#elif defined Q_OS_LINUX
    RouterLinux::Instance().flushDns();
#endif
}

