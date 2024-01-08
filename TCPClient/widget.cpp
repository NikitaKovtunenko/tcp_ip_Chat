#include "widget.h"
#include "ui_widget.h"
#include <QHostAddress>
#include <QMessageBox>
#include <QLineEdit>
#include <QColorDialog>
#include <QFile>
#include <QDateTime>
#include <QFileDialog>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    connect(&client, &QTcpSocket::readyRead, this, &Widget::socketReadyRead);
    connect(&client, &QTcpSocket::disconnected, this, &Widget::socketDisconnected);
    connect(&client, &QTcpSocket::connected, this, &Widget::socketConnected);
    connect(&client, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(socketDisconnected()));
    connect(ui->connect,SIGNAL(clicked()),this,SLOT(on_connectToServerButton_clicked()));
    connect(ui->sendMsg,SIGNAL(clicked()),this,SLOT(on_sendButton_clicked()));
    connect(ui->colorButtn,SIGNAL(clicked()),this,SLOT(on_colorButton_clicked()));
    connect(ui->selectFile,SIGNAL(clicked(bool)),this,SLOT(on_sendFileButton_clicked()));
}

Widget::~Widget()
{
    delete ui;
}
// Клиентская часть
void Widget::on_connectToServerButton_clicked()
{

    if (ui->port->value() !=0 && ui->ipAdrs->text().length() != 0 && ui->nickname->text().length() !=0)
    {
        client._nick = ui->nickname->text();
        ui->port->setEnabled(false);
        ui->ipAdrs->setEnabled(false);
        ui->connect->setEnabled(false);
        ui->nickname->setEnabled(false);
        ui->sendMsg->setEnabled(true);
        QHostAddress serverAddress(ui->ipAdrs->text());
        ushort serverPort = ui->port->value();
        client.connectToHost(serverAddress, serverPort);
    }
    else
    {
        QMessageBox::information(this, "Внимание!", "Одно из полей не было заполнено");
    }
    if (client.isValid() == false)
    {
        qDebug() << "43";
        ui->sendMsg->setEnabled(false);

    }
}

void Widget::socketConnected()
{
    qDebug() << "Client :: Socket connected";
    QByteArray buffer;
    QDataStream stream(&buffer,QIODevice::WriteOnly);
     stream.setVersion(QDataStream::Qt_5_5);
    stream << (qint32)0;
    stream << (qint32)1;
    stream <<  client._nick;
    stream.device()->seek(0);
    stream << (qint32)(buffer.size() - sizeof(qint32));
    client.write(buffer);

}

void Widget::socketDisconnected()
{

    if (ui->ipAdrs->isEnabled() == true || ui->sendMsg->isEnabled() == false)
    {
        QMessageBox::information(this, "Внимание!", "Сервер не был найден");
    }
    ui->sendMsg->setEnabled(false);
    ui->connect->setEnabled(true);
    ui->port->setEnabled(true);
    ui->ipAdrs->setEnabled(true);

}

void Widget::on_sendButton_clicked()
{
    QByteArray buffer;
    QString msg = "<b style=\"color:" + client._color.name() + ";\">" + client._nick  + ": " "</b>" +   ui->message->text();
    QDataStream stream(&buffer,QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_5_5);
    stream<< (quint32)0;
    stream << (quint32)0;
    stream << msg;
    int size = buffer.size();
    stream.device()->seek(0);
    stream << (quint32)(size - sizeof(qint32));
    client.write(buffer);
    writeLog( ui->message->text());
    ui->textEdit->append(msg);
    ui->message->clear();
}

void Widget::on_colorButton_clicked()
{
    QColor ncolor = QColorDialog::getColor(QColor(255, 255, 255, 255), this);
    if (ncolor.isValid()) //выбран ли цвет
    {
        client._color = ncolor;
    }
}

void Widget::on_sendFileButton_clicked()
{
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::AnyFile);
    QStringList fileNames;
    if (dialog.exec())
        fileNames = dialog.selectedFiles();

    if(!fileNames.isEmpty())
    {
        QString path = fileNames.first();
        qDebug() << path;
        int index  = path.lastIndexOf('/');
        QString fileName  = path.mid(index);
        ui->message->setText("send File :" + fileName);
        on_sendButton_clicked();
        sendFile(path);
    }

}

void Widget::writeLog(const QString & str)
{

    QFile logs(PRO_FILE_PWD"/ClientLogs.txt");
    logs.open(QIODevice::Append| QIODevice::Text);
    qDebug()<< PRO_FILE_PWD ;
    if(logs.isOpen())
    {
        QTextStream stream(&logs);
        stream << str << ' ' <<  QDateTime::currentDateTime().toString() << '\n';
    }
    else {
        qDebug() << "Log file not open!";
    }

    logs.close();
}

void Widget::sendFile(const QString & path)
{
    QFile file(path);
    QString fileName = path.mid(path.lastIndexOf('/'));

    if (!file.open(QFile::ReadOnly))
    {
        qDebug() << "Could not open the file for reading";
    }

    QByteArray block; // Data that will be sent
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_5);
    out << (quint32)0; // Space for size of data
    out << (qint32)3;
    out << file.readAll(); // Actual data
    out.device()->seek(0);
    out << (quint32)(block.size() - sizeof(quint32));

    // write the string into the socket
    client.write(block);
}

void Widget::socketReadyRead()
{

    client._blocksize = 0;
    QDataStream stream(&client);
    stream.setVersion(QDataStream::Qt_5_5);
    while(client.bytesAvailable() > 0)
    {
        if(client._blocksize == 0) // Новое сообщение
        {
            if(client.bytesAvailable() < sizeof(client._blocksize))
                break;
            // Читаем размер сообщения в blockSize
            stream >>  client._blocksize;
        }

        if(client.bytesAvailable() >= client._blocksize)
        { // Очередное сообщение готово для чтения

            int i;
            stream >> i;
            QString msg;
            // msg.reserve(buffer.size());

            switch (i) {
            case 0:
                stream >> msg;
                ui->textEdit->append(msg);
                writeLog(msg.mid(msg.lastIndexOf("\">")));
                break;
            case 2:
                int count ;
                stream >> count;
                ui->listWidget->clear();
                while(count--)
                {
                    QString name;
                    stream >> name;
                    ui->listWidget->addItem(name);
                }
                break;
            case 3:
            {
                QString fileName = ui->textEdit->toPlainText().mid(ui->textEdit->toPlainText().lastIndexOf('/'));
                QFile file( PRO_FILE_PWD + fileName); // download path
                QByteArray buffer;
                stream >> buffer;
                file.open(QIODevice::WriteOnly);
                file.write(buffer);
                file.close();
            }
                break;
            default:
                qDebug() << "Error the contract is not described";
                break;
            }


            client._blocksize = 0;

        }
        else
        {
            break;
        }
    }


}
