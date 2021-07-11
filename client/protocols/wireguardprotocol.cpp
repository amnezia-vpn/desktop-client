#include <QCoreApplication>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QTcpSocket>
#include <QThread>

#include "debug.h"
#include "wireguardprotocol.h"
#include "utils.h"

WireguardProtocol::WireguardProtocol(const QJsonObject &configuration, QObject* parent) :
    VpnProtocol(configuration, parent)
{
    //m_configFile.setFileTemplate(QDir::tempPath() + QDir::separator() + serviceName() + ".conf");
    m_configFile.setFileName(QDir::tempPath() + QDir::separator() + serviceName() + ".conf");
    readWireguardConfiguration(configuration);
}

WireguardProtocol::~WireguardProtocol()
{
    qDebug() << "WireguardProtocol::~WireguardProtocol()";
    WireguardProtocol::stop();
    QThread::msleep(200);
}

void WireguardProtocol::stop()
{
    if (!QFileInfo::exists(wireguardExecPath())) {
        qCritical() << "Wireguard executable missing!";
        setLastError(ErrorCode::ExecutableMissing);
        return;
    }

    m_wireguardStopProcess = IpcClient::CreatePrivilegedProcess();

    if (!m_wireguardStopProcess) {
        qCritical() << "IpcProcess replica is not created!";
        setLastError(ErrorCode::AmneziaServiceConnectionFailed);
        return;
    }

    m_wireguardStopProcess->waitForSource(1000);
    if (!m_wireguardStopProcess->isInitialized()) {
        qWarning() << "IpcProcess replica is not connected!";
        setLastError(ErrorCode::AmneziaServiceConnectionFailed);
        return;
    }

    m_wireguardStopProcess->setProgram(wireguardExecPath());


    QStringList arguments({"/uninstalltunnelservice", serviceName(), });
    m_wireguardStopProcess->setArguments(arguments);

    qDebug() << arguments.join(" ");

    connect(m_wireguardStopProcess.data(), &IpcProcessInterfaceReplica::errorOccurred, this, [this](QProcess::ProcessError error) {
        qDebug() << "WireguardProtocol::WireguardProtocol errorOccurred" << error;
        setConnectionState(ConnectionState::Disconnected);
    });

    connect(m_wireguardStopProcess.data(), &IpcProcessInterfaceReplica::stateChanged, this, [this](QProcess::ProcessState newState) {
        qDebug() << "WireguardProtocol::WireguardProtocol Stop stateChanged" << newState;
    });

    m_wireguardStopProcess->start();

    setConnectionState(VpnProtocol::Disconnected);
}

void WireguardProtocol::readWireguardConfiguration(const QJsonObject &configuration)
{
    if (configuration.contains(config::key_wireguard_config_data)) {
        if (!m_configFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qCritical() << "Failed to save wireguard config to" << m_configFile.fileName();
            return;
        }

        m_isConfigLoaded = true;

        m_configFile.write(configuration.value(config::key_wireguard_config_data).toString().toUtf8());
        m_configFile.close();
        m_configFileName = m_configFile.fileName();

        qDebug().noquote() << QString("Set config data") << m_configFileName;
        qDebug().noquote() << QString("Set config data") << configuration.value(config::key_wireguard_config_data).toString().toUtf8();
    }
//    else if (configuration.contains(config::key_wireguard_config_path)) {
//        m_configFileName = configuration.value(config::key_wireguard_config_path).toString();
//        QFileInfo file(m_configFileName);

//        if (file.fileName().isEmpty()) {
//            m_configFileName = defaultConfigFileName();
//        }

//        qDebug().noquote() << QString("Set config file: '%1'").arg(configPath());
//    }
}

//bool WireguardProtocol::openVpnProcessIsRunning() const
//{
//    return Utils::processIsRunning("openvpn");
//}

QString WireguardProtocol::configPath() const
{
    return m_configFileName;
}

void WireguardProtocol::updateRouteGateway(QString line)
{
    // TODO: fix for macos
    line = line.split("ROUTE_GATEWAY", QString::SkipEmptyParts).at(1);
    if (!line.contains("/")) return;
    m_routeGateway = line.split("/", QString::SkipEmptyParts).first();
    m_routeGateway.replace(" ", "");
    qDebug() << "Set VPN route gateway" << m_routeGateway;
}

QString WireguardProtocol::wireguardExecPath() const
{
#ifdef Q_OS_WIN
    return Utils::executable("wireguard/wireguard", true);
#elif defined Q_OS_LINUX
    return Utils::usrExecutable("wg");
#else
    return Utils::executable("/wireguard", true);
#endif
}

ErrorCode WireguardProtocol::start()
{
    if (!m_isConfigLoaded) {
        setLastError(ErrorCode::ConfigMissing);
        return lastError();
    }

    //qDebug() << "Start OpenVPN connection";
    WireguardProtocol::stop();

    if (!QFileInfo::exists(wireguardExecPath())) {
        setLastError(ErrorCode::ExecutableMissing);
        return lastError();
    }

    if (!QFileInfo::exists(configPath())) {
        setLastError(ErrorCode::ConfigMissing);
        return lastError();
    }

    setConnectionState(ConnectionState::Connecting);

    m_wireguardStartProcess = IpcClient::CreatePrivilegedProcess();

    if (!m_wireguardStartProcess) {
        //qWarning() << "IpcProcess replica is not created!";
        setLastError(ErrorCode::AmneziaServiceConnectionFailed);
        return ErrorCode::AmneziaServiceConnectionFailed;
    }

    m_wireguardStartProcess->waitForSource(1000);
    if (!m_wireguardStartProcess->isInitialized()) {
        qWarning() << "IpcProcess replica is not connected!";
        setLastError(ErrorCode::AmneziaServiceConnectionFailed);
        return ErrorCode::AmneziaServiceConnectionFailed;
    }

    m_wireguardStartProcess->setProgram(wireguardExecPath());


    QStringList arguments({"/installtunnelservice", configPath(), });
    m_wireguardStartProcess->setArguments(arguments);

    qDebug() << arguments.join(" ");

    connect(m_wireguardStartProcess.data(), &IpcProcessInterfaceReplica::errorOccurred, this, [this](QProcess::ProcessError error) {
        qDebug() << "WireguardProtocol::WireguardProtocol errorOccurred" << error;
        setConnectionState(ConnectionState::Disconnected);
    });

    connect(m_wireguardStartProcess.data(), &IpcProcessInterfaceReplica::stateChanged, this, [this](QProcess::ProcessState newState) {
        qDebug() << "WireguardProtocol::WireguardProtocol stateChanged" << newState;
    });

    connect(m_wireguardStartProcess.data(), &IpcProcessInterfaceReplica::finished, this, [this]() {
        setConnectionState(ConnectionState::Connected);
    });

    connect(m_wireguardStartProcess.data(), &IpcProcessInterfaceReplica::readyRead, this, [this]() {
        QRemoteObjectPendingReply<QByteArray> reply = m_wireguardStartProcess->readAll();
        reply.waitForFinished(1000);
        qDebug() << "WireguardProtocol::WireguardProtocol readyRead" << reply.returnValue();
    });

    connect(m_wireguardStartProcess.data(), &IpcProcessInterfaceReplica::readyReadStandardOutput, this, [this]() {
        QRemoteObjectPendingReply<QByteArray> reply = m_wireguardStartProcess->readAllStandardOutput();
        reply.waitForFinished(1000);
        qDebug() << "WireguardProtocol::WireguardProtocol readAllStandardOutput" << reply.returnValue();
    });

    connect(m_wireguardStartProcess.data(), &IpcProcessInterfaceReplica::readyReadStandardError, this, [this]() {
        QRemoteObjectPendingReply<QByteArray> reply = m_wireguardStartProcess->readAllStandardError();
        reply.waitForFinished(1000);
        qDebug() << "WireguardProtocol::WireguardProtocol readAllStandardError" << reply.returnValue();
    });

    m_wireguardStartProcess->start();

    return ErrorCode::NoError;
}

void WireguardProtocol::updateVpnGateway(const QString &line)
{
//    // line looks like
//    // PUSH: Received control message: 'PUSH_REPLY,route 10.8.0.1,topology net30,ping 10,ping-restart 120,ifconfig 10.8.0.6 10.8.0.5,peer-id 0,cipher AES-256-GCM'

//    QStringList params = line.split(",");
//    for (const QString &l : params) {
//        if (l.contains("ifconfig")) {
//            if (l.split(" ").size() == 3) {
//                m_vpnLocalAddress = l.split(" ").at(1);
//                m_vpnGateway = l.split(" ").at(2);

//                qDebug() << QString("Set vpn local address %1, gw %2").arg(m_vpnLocalAddress).arg(vpnGateway());
//            }
//        }
    //    }
}

QString WireguardProtocol::serviceName() const
{
    return "AmneziaVPN.WireGuard0";
}
