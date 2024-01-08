#include "server.h"
#include "ui_server.h"
#include <QDebug>
#include <QTcpSocket>
#include <QTcpServer>
#include <QMessageBox>
#include <QDataStream>
#include <QFile>
#include <QDateTime>
#include <QPushButton>
#include <QTextStream>


Server::Server(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    _server = new QTcpServer(this);
    connect(_server, &QTcpServer::newConnection, this, &Server::serverNewConnection);
    connect(ui->sendButton,&QPushButton::clicked,this,&Server::on_sendButton);
    // Адрес и порт хорошо бы настроить в GUI
    _server->listen(QHostAddress::Any, 40000);
    ui->sendButton->setEnabled(false);
}

void Server::serverNewConnection()
{
    qDebug() << "serverNewConnection";
    while(_server->hasPendingConnections())
    {
        _socket = (QTcpSocket*) _server->nextPendingConnection(); // -> clients
        _users[_socket];
        ui->label_active_clients->setText("Активных клиентов: " + QString::number(_users.size()));
        connect(_socket, &QTcpSocket::readyRead, this, &Server::socketReadyRead);
        connect(_socket, &QTcpSocket::disconnected, this, &Server::socketDisconnected);

    }

    ui->sendButton->setEnabled(true);
}

void Server::socketDisconnected()
{
    QTcpSocket *socket = (QTcpSocket* ) sender();
    QString str = "Disconnected: ";
    writeLog(str,socket);

    auto bg = _users.begin();

    while (bg != _users.end())
    {

        if (bg.key() == socket)
        {
            if(!_abort)
            {
                auto res = ui->listWidget->findItems( bg.value(),Qt::MatchContains);
                if(!res.isEmpty())
                {
                    ui->listWidget->takeItem(ui->listWidget->row(res.first()));
                }
            }
            _users.erase(bg); //удаляем сокет из map
            break;
        }
        bg++;
    }

    ui->label_active_clients->setText("Активных клиентов: " + QString::number(_users.size()));

    if(_abort)
        _abort = false;

    sendUserList();

    if (_users.size() == 0)
    {
        ui->sendButton->setEnabled(false);
    }

}

void Server::writeLog(const QString & str, QTcpSocket* socket)
{

    QFile logs(PRO_FILE_PWD"/ServerLogs.txt");
    logs.open(QIODevice::Append| QIODevice::Text);

    if(logs.isOpen())
    {
        QTextStream stream(&logs);
        stream << str << _users[socket] << " " << QDateTime::currentDateTime().toString() << " IP: " <<  socket->peerAddress().toString()  << " State: ";

        switch (socket->state()) {
        case 0:
            stream << "UnconnectedState\n";
            break;
        case 1:
            stream << "HostLookupState\n";
            break;
        case 2:
            stream << "ConnectingState\n";
            break;
        case 3:
            stream << "ConnectedState\n";
            break;
        case 4:
            stream << "BoundState\n";
            break;
        case 5:
            stream << "ListeningState\n";
            break;
        case 6:
            stream << "ClosingState\n";
            break;
        default:
            break;
        }
    }
    else {
        qDebug() << "Log file not open!";
    }

    logs.close();
}

void Server::sendMessageToUsers(const QString & msg)
{
    QByteArray buffer;
    QDataStream stream(&buffer,QIODevice::ReadWrite);
    stream.setVersion(QDataStream::Qt_5_5);
    stream << (qint32)0;
    stream << (qint32)0;
    stream << msg;
    stream.device()->seek(0);
    stream << (qint32)(buffer.size() - sizeof(qint32));

    auto bg = _users.begin();
    while(bg != _users.end())
    {
        if(bg.key() != _socket)
        {
            bg.key()->write(buffer);
        }
        bg++;
    }
}

void Server::sendFileToUser( QByteArray& data)
{
    qDebug() << "Send file" ;
    QByteArray buffer;
    QDataStream stream(&buffer,QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_5_5);
    stream << (quint32)0; // Space for size of data
    stream << (qint32)3;
    stream << data ;// Actual data
    stream.device()->seek(0);
    stream << (quint32)(buffer.size() - sizeof(quint32));

    auto bg = _users.begin();

    while(bg != _users.end())
    {
        if(bg.key() != _socket)
        {
            bg.key()->write(buffer);
        }
        bg++;

    }
}

void Server::sendUserList()
{
    QByteArray buffer;
    QDataStream stream(&buffer,QIODevice::WriteOnly);
     stream.setVersion(QDataStream::Qt_5_5);
    stream << (qint32)0;
    stream << (quint32)2;
    stream << _users.size();
    foreach(auto& str,_users)
    {
        stream << str;
    }

    stream.device()->seek(0);
    stream << (qint32)(buffer.size() - sizeof(qint32));
    auto bg = _users.begin();

    while(bg != _users.end())
    {

        bg.key()->write(buffer);
        bg++;

    }
}

void Server::on_sendButton()
{
    if(ui->lineEdit->text() != QString())
    {
        QByteArray buffer;
        QDataStream stream(&buffer,QIODevice::ReadWrite);
        stream.setVersion(QDataStream::Qt_5_5);
        stream << (qint32)0;
        stream << (qint32)0;
        QString msg = "Сервер: " +  ui->lineEdit->text();
        stream<< msg;
        stream.device()->seek(0);
        stream << (qint32)(buffer.size() - sizeof(qint32));
        ui->textEdit->append(msg);

        auto bg = _users.begin();

        while(bg != _users.end())
        {
           bg.key()->write(buffer);

            bg++;
        }
    }
}


void Server::socketReadyRead()
{
    QByteArray buffer;
    _blockSize = 0;
    _socket = (QTcpSocket*) sender(); //то, от какого сокета серверу приходит сообщение
    //используется для определения количества байт, доступных для чтения из сокета в данный момент.
    //Этот метод полезен для асинхронного чтения данных из сокета.При использовании TCP-соединения, данные могут приходить блоками,
    //и _socket->bytesAvailable() предоставляет информацию о количестве доступных байт,
    //готовых для чтения без блокирования исполнения программы
    QDataStream stream(_socket);
    stream.setVersion(QDataStream::Qt_5_5);
    while(_socket->bytesAvailable() > 0)
    {
        if(_blockSize == 0) // Новое сообщение
        {
            if(_socket->bytesAvailable() < sizeof(_blockSize))
                break;
            // Читаем размер сообщения в blockSize
            stream >> _blockSize;
        }

        if(_socket->bytesAvailable() >= _blockSize)
        { // Очередное сообщение готово для чтения

            int i;
            stream >> i;
            QString msg;

            switch (i) {
            case 0:
                stream >> msg;
                ui->textEdit->append(msg);
                sendMessageToUsers(msg);
                break;
            case 1:
                stream >> msg;
                ui->listWidget->addItem(msg);
                _users[_socket] = msg;
                writeLog("Connection: ", _socket);
                sendUserList();
                break;
            case 3:
                stream >> buffer;
                sendFileToUser(buffer);
                break;
            default:
                qDebug() << "Error the contract is not described";
                break;
            }

            _blockSize = 0;
        }
        else
        {
            break;
        }
    }



}



Server::~Server()
{
    delete ui;
}

