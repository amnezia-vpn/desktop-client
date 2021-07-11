#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QHostAddress>
#include <QHostInfo>
#include <QProcess>
#include <QRandomGenerator>
#include <QStandardPaths>

#include "defines.h"
#include "utils.h"

QString Utils::getRandomString(int len)
{
    const QString possibleCharacters("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");

    QString randomString;
    for(int i=0; i<len; ++i) {
        quint32 index = QRandomGenerator::global()->generate() % possibleCharacters.length();
        QChar nextChar = possibleCharacters.at(index);
        randomString.append(nextChar);
    }
    return randomString;
}

QString Utils::systemLogPath()
{
#ifdef Q_OS_WIN
    QStringList locationList = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
    QString primaryLocation = "ProgramData";
    foreach (const QString& location, locationList) {
        if (location.contains(primaryLocation)) {
            return QString("%1/%2/log").arg(location).arg(APPLICATION_NAME);
        }
    }
    return QString();
#else
    return QString("/var/log/%1").arg(APPLICATION_NAME);
#endif
}

bool Utils::initializePath(const QString& path)
{
    QDir dir;
    if (!dir.mkpath(path)) {
        qWarning().noquote() << QString("Cannot initialize path: '%1'").arg(path);
        return false;
    }
    return true;
}

bool Utils::createEmptyFile(const QString& path)
{
    QFile f(path);
    return f.open(QIODevice::WriteOnly | QIODevice::Truncate);
}

QString Utils::executable(const QString& baseName, bool absPath)
{
    QString ext;
#ifdef Q_OS_WIN
    ext = ".exe";
#endif
    const QString fileName = baseName + ext;
    if (!absPath) {
        return fileName;
    }
    return QCoreApplication::applicationDirPath() + "/" + fileName;
}

QString Utils::usrExecutable(const QString& baseName)
{
    if (QFileInfo::exists("/usr/sbin/" + baseName))
        return ("/usr/sbin/" + baseName);
    else
        return ("/usr/bin/" + baseName);
}

bool Utils::processIsRunning(const QString& fileName)
{
#ifdef Q_OS_WIN
    QProcess process;
    process.setReadChannel(QProcess::StandardOutput);
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(QString("wmic.exe /OUTPUT:STDOUT PROCESS get %1").arg("Caption"));
    process.waitForStarted();
    process.waitForFinished();
    QString processData(process.readAll());
    QStringList processList = processData.split(QRegExp("[\r\n]"),QString::SkipEmptyParts);
    foreach (const QString& rawLine, processList) {
        const QString line = rawLine.simplified();
        if (line.isEmpty()) {
            continue;
        }

        if (line == fileName) {
            return true;
        }

    }
    return false;
#else
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start("pgrep", QStringList({fileName}));
    process.waitForFinished();
    if (process.exitStatus() == QProcess::NormalExit) {
        return (process.readAll().toUInt() > 0);
    }
    return false;
#endif
}

QString Utils::getIPAddress(const QString& host)
{
    if (ipAddressRegExp().exactMatch(host)) {
        return host;
    }

    QList<QHostAddress> adresses = QHostInfo::fromName(host).addresses();
    if (!adresses.isEmpty()) {
        qDebug() << "Resolved address for" << host << adresses.first().toString();
        return adresses.first().toString();
    }
    qDebug() << "Unable to resolve address for " << host;
    return "";
}

QString Utils::getStringBetween(const QString& s, const QString& a, const QString& b)
{
    int ap = s.indexOf(a), bp = s.indexOf(b, ap + a.length());
    if(ap < 0 || bp < 0)
        return QString();
    ap += a.length();
    if(bp - ap <= 0)
        return QString();
    return s.mid(ap, bp - ap).trimmed();
}

bool Utils::checkIPv4Format(const QString& ip)
{
    if (ip.isEmpty()) return false;
    int count = ip.count(".");
    if(count != 3) return false;

    QHostAddress addr(ip);
    return  (addr.protocol() == QAbstractSocket::NetworkLayerProtocol::IPv4Protocol);
}

bool Utils::checkIpSubnetFormat(const QString &ip)
{
    if (!ip.contains("/")) return checkIPv4Format(ip);

    QStringList parts = ip.split("/");
    if (parts.size() != 2) return false;

    bool ok;
    int subnet = parts.at(1).toInt(&ok);
    if (subnet >= 0 && subnet <= 32 && ok) return checkIPv4Format(parts.at(0));
    else return false;
}

void Utils::killProcessByName(const QString &name)
{
    qDebug().noquote() << "Kill process" << name;
#ifdef Q_OS_WIN
    QProcess::execute(QString("taskkill /im %1 /f").arg(name));
#else
    QProcess::execute(QString("pkill %1").arg(name));
#endif
}

QString Utils::netMaskFromIpWithSubnet(const QString ip)
{
    if (!ip.contains("/")) return "255.255.255.255";

    bool ok;
    int prefix = ip.split("/").at(1).toInt(&ok);
    if (!ok) return "255.255.255.255";

    unsigned long mask = (0xFFFFFFFF << (32 - prefix)) & 0xFFFFFFFF;

    return QString("%1.%2.%3.%4")
            .arg(mask >> 24)
            .arg((mask >> 16) & 0xFF)
            .arg((mask >> 8) & 0xFF)
            .arg( mask & 0xFF);
}

QString Utils::ipAddressFromIpWithSubnet(const QString ip)
{
    if (ip.count(".") != 3) return "";
    return ip.split("/").first();
}

QStringList Utils::summarizeRoutes(const QStringList &ips, const QString cidr)
{
//    QMap<int, int>
//    QHostAddress

//    QMap<QString, QStringList> subnets; // <"a.b", <list subnets>>

//    for (const QString &ip : ips) {
//        if (ip.count(".") != 3) continue;

//        const QStringList &parts = ip.split(".");
//        subnets[parts.at(0) + "." + parts.at(1)].append(ip);
//    }

    return QStringList();
}

#ifdef Q_OS_WIN
// Inspired from http://stackoverflow.com/a/15281070/1529139
// and http://stackoverflow.com/q/40059902/1529139
bool Utils::signalCtrl(DWORD dwProcessId, DWORD dwCtrlEvent)
{
    bool success = false;
    DWORD thisConsoleId = GetCurrentProcessId();
    // Leave current console if it exists
    // (otherwise AttachConsole will return ERROR_ACCESS_DENIED)
    bool consoleDetached = (FreeConsole() != FALSE);

    if (AttachConsole(dwProcessId) != FALSE)
    {
        // Add a fake Ctrl-C handler for avoid instant kill is this console
        // WARNING: do not revert it or current program will be also killed
        SetConsoleCtrlHandler(nullptr, true);
        success = (GenerateConsoleCtrlEvent(dwCtrlEvent, 0) != FALSE);
        FreeConsole();
    }

    if (consoleDetached)
    {
        // Create a new console if previous was deleted by OS
        if (AttachConsole(thisConsoleId) == FALSE)
        {
            int errorCode = GetLastError();
            if (errorCode == 31) // 31=ERROR_GEN_FAILURE
            {
                AllocConsole();
            }
        }
    }
    return success;
}
#endif
