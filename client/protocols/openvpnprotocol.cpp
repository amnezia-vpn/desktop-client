#include <QCoreApplication>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QTcpSocket>

#include "debug.h"
#include "defines.h"
#include "utils.h"
#include "openvpnprotocol.h"


OpenVpnProtocol::OpenVpnProtocol(const QJsonObject &configuration, QObject* parent) :
    VpnProtocol(configuration, parent)
{
    readOpenVpnConfiguration(configuration);
    connect(&m_managementServer, &ManagementServer::readyRead, this, &OpenVpnProtocol::onReadyReadDataFromManagementServer);
}

OpenVpnProtocol::~OpenVpnProtocol()
{
    qDebug() << "OpenVpnProtocol::~OpenVpnProtocol()";
    OpenVpnProtocol::stop();
    QThread::msleep(200);
}

QString OpenVpnProtocol::defaultConfigFileName()
{
    //qDebug() << "OpenVpnProtocol::defaultConfigFileName" << defaultConfigPath() + QString("/%1.ovpn").arg(APPLICATION_NAME);
    return defaultConfigPath() + QString("/%1.ovpn").arg(APPLICATION_NAME);
}

QString OpenVpnProtocol::defaultConfigPath()
{
    QString p = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/config";
    Utils::initializePath(p);

    return p;
}

void OpenVpnProtocol::stop()
{
    qDebug() << "OpenVpnProtocol::stop()";

    // TODO: need refactoring
    // sendTermSignal() will even return true while server connected ???
    if ((m_connectionState == VpnProtocol::Preparing) ||
            (m_connectionState == VpnProtocol::Connecting) ||
            (m_connectionState == VpnProtocol::Connected) ||
            (m_connectionState == VpnProtocol::Reconnecting)) {
        if (!sendTermSignal()) {
            killOpenVpnProcess();
        }
        m_managementServer.stop();
        qApp->processEvents();
        setConnectionState(VpnProtocol::Disconnecting);
    }
}

ErrorCode OpenVpnProtocol::checkAndSetupTapDriver()
{
    if (!IpcClient::Interface()) {
        return ErrorCode::AmneziaServiceConnectionFailed;
    }

    QRemoteObjectPendingReply<QStringList> resultCheck = IpcClient::Interface()->getTapList();
    resultCheck.waitForFinished();

    if (resultCheck.returnValue().isEmpty()){
        QRemoteObjectPendingReply<bool> resultInstall = IpcClient::Interface()->checkAndInstallDriver();
        resultInstall.waitForFinished();

        if (!resultInstall.returnValue()) return ErrorCode::OpenVpnTapAdapterError;
    }
    return ErrorCode::NoError;
}

void OpenVpnProtocol::killOpenVpnProcess()
{
    if (m_openVpnProcess){
        m_openVpnProcess->close();
    }
}

void OpenVpnProtocol::readOpenVpnConfiguration(const QJsonObject &configuration)
{
    if (configuration.contains(config::key_openvpn_config_data)) {
        m_configFile.open();
        m_configFile.write(configuration.value(config::key_openvpn_config_data).toString().toUtf8());
        m_configFile.close();
        m_configFileName = m_configFile.fileName();

        qDebug().noquote() << QString("Set config data") << m_configFileName;
    }
    else if (configuration.contains(config::key_openvpn_config_path)) {
        m_configFileName = configuration.value(config::key_openvpn_config_path).toString();
        QFileInfo file(m_configFileName);

        if (file.fileName().isEmpty()) {
            m_configFileName = defaultConfigFileName();
        }

        qDebug().noquote() << QString("Set config file: '%1'").arg(configPath());
    }
}

bool OpenVpnProtocol::openVpnProcessIsRunning() const
{
    return Utils::processIsRunning("openvpn");
}

void OpenVpnProtocol::disconnectFromManagementServer()
{
    m_managementServer.stop();
}

QString OpenVpnProtocol::configPath() const
{
    return m_configFileName;
}

void OpenVpnProtocol::sendManagementCommand(const QString& command)
{
    QIODevice *device = dynamic_cast<QIODevice*>(m_managementServer.socket().data());
    if (device) {
        QTextStream stream(device);
        stream << command << Qt::endl;
    }
}

void OpenVpnProtocol::updateRouteGateway(QString line)
{
    // TODO: fix for macos
    line = line.split("ROUTE_GATEWAY", QString::SkipEmptyParts).at(1);
    if (!line.contains("/")) return;
    m_routeGateway = line.split("/", QString::SkipEmptyParts).first();
    m_routeGateway.replace(" ", "");
    qDebug() << "Set VPN route gateway" << m_routeGateway;
}

QString OpenVpnProtocol::openVpnExecPath() const
{
#ifdef Q_OS_WIN
    return Utils::executable("openvpn/openvpn", true);
#elif defined Q_OS_LINUX
    return Utils::usrExecutable("openvpn");
#else
    return Utils::executable("/openvpn", true);
#endif
}

ErrorCode OpenVpnProtocol::start()
{
    //qDebug() << "Start OpenVPN connection";
    OpenVpnProtocol::stop();

    if (!QFileInfo::exists(openVpnExecPath())) {
        setLastError(ErrorCode::OpenVpnExecutableMissing);
        return lastError();
    }

    if (!QFileInfo::exists(configPath())) {
        setLastError(ErrorCode::OpenVpnConfigMissing);
        return lastError();
    }

    QString vpnLogFileNamePath = Utils::systemLogPath() + "/openvpn.log";
    Utils::createEmptyFile(vpnLogFileNamePath);

    if (!m_managementServer.start(m_managementHost, m_managementPort)) {
        setLastError(ErrorCode::OpenVpnManagementServerError);
        return lastError();
    }

    setConnectionState(ConnectionState::Connecting);

    m_openVpnProcess = IpcClient::CreatePrivilegedProcess();

    if (!m_openVpnProcess) {
        //qWarning() << "IpcProcess replica is not created!";
        setLastError(ErrorCode::AmneziaServiceConnectionFailed);
        return ErrorCode::AmneziaServiceConnectionFailed;
    }

    m_openVpnProcess->waitForSource(1000);
    if (!m_openVpnProcess->isInitialized()) {
        qWarning() << "IpcProcess replica is not connected!";
        setLastError(ErrorCode::AmneziaServiceConnectionFailed);
        return ErrorCode::AmneziaServiceConnectionFailed;
    }
    m_openVpnProcess->setProgram(openVpnExecPath());
    QStringList arguments({"--config" , configPath(),
                      "--management", m_managementHost, QString::number(m_managementPort),
                      "--management-client",
                      "--log", vpnLogFileNamePath
                     });
    m_openVpnProcess->setArguments(arguments);

    qDebug() << arguments.join(" ");
    connect(m_openVpnProcess.data(), &IpcProcessInterfaceReplica::errorOccurred, [&](QProcess::ProcessError error) {
        qDebug() << "IpcProcessInterfaceReplica errorOccurred" << error;
    });

    connect(m_openVpnProcess.data(), &IpcProcessInterfaceReplica::stateChanged, [&](QProcess::ProcessState newState) {
        qDebug() << "IpcProcessInterfaceReplica stateChanged" << newState;
    });

    connect(m_openVpnProcess.data(), &IpcProcessInterfaceReplica::finished, this, [&]() {
        setConnectionState(ConnectionState::Disconnected);
    });

    m_openVpnProcess->start();

    //startTimeoutTimer();

    return ErrorCode::NoError;
}

bool OpenVpnProtocol::sendTermSignal()
{
    return m_managementServer.writeCommand("signal SIGTERM");
}

void OpenVpnProtocol::sendByteCount()
{
    m_managementServer.writeCommand("bytecount 1");
}

void OpenVpnProtocol::sendInitialData()
{
    m_managementServer.writeCommand("state on");
    m_managementServer.writeCommand("log on");
}

void OpenVpnProtocol::onReadyReadDataFromManagementServer()
{
    for (;;)  {
        QString line = m_managementServer.readLine().simplified();

        if (line.isEmpty()) {
            return;
        }

        if (!line.contains(">BYTECOUNT")) {
            qDebug().noquote() << line;
        }

        if (line.contains(">INFO:OpenVPN Management Interface")) {
            sendInitialData();
        }  else if (line.startsWith(">STATE")) {
            if (line.contains("CONNECTED,SUCCESS")) {
                sendByteCount();
                stopTimeoutTimer();
                setConnectionState(VpnProtocol::Connected);
                continue;
            } else if (line.contains("EXITING,SIGTER")) {
                //openVpnStateSigTermHandler();
                setConnectionState(VpnProtocol::Disconnecting);
                continue;
            } else if (line.contains("RECONNECTING")) {
                setConnectionState(VpnProtocol::Reconnecting);
                continue;
            }
        }

        if (line.contains("ROUTE_GATEWAY")) {
            updateRouteGateway(line);
        }

        if (line.contains("PUSH: Received control message")) {
            updateVpnGateway(line);
        }

        if (line.contains("FATAL")) {
            if (line.contains("tap-windows6 adapters on this system are currently in use or disabled")) {
                emit protocolError(ErrorCode::OpenVpnAdaptersInUseError);
            }
            else {
                emit protocolError(ErrorCode::OpenVpnUnknownError);
            }
            return;
        }

        QByteArray data(line.toStdString().c_str());
        if (data.contains(">BYTECOUNT:")) {
            int beg = data.lastIndexOf(">BYTECOUNT:");
            int end = data.indexOf("\n", beg);

            beg += sizeof(">BYTECOUNT:") - 1;
            QList<QByteArray> count = data.mid(beg, end - beg + 1).split(',');

            quint64 r = static_cast<quint64>(count.at(0).trimmed().toULongLong());
            quint64 s = static_cast<quint64>(count.at(1).trimmed().toULongLong());

            setBytesChanged(r, s);
        }
    }
}

void OpenVpnProtocol::updateVpnGateway(const QString &line)
{
    // line looks like
    // PUSH: Received control message: 'PUSH_REPLY,route 10.8.0.1,topology net30,ping 10,ping-restart 120,ifconfig 10.8.0.6 10.8.0.5,peer-id 0,cipher AES-256-GCM'

    QStringList params = line.split(",");
    for (const QString &l : params) {
        if (l.contains("ifconfig")) {
            if (l.split(" ").size() == 3) {
                m_vpnLocalAddress = l.split(" ").at(1);
                m_vpnGateway = l.split(" ").at(2);

                qDebug() << QString("Set vpn local address %1, gw %2").arg(m_vpnLocalAddress).arg(vpnGateway());
            }
        }
    }
}
