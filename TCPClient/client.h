#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QColor>

class QFile;

class Client: public QTcpSocket
{
public:
    QString _nick;
    int _blocksize;
    QColor _color;
    QFile* _file;
    Client(QObject* parent = nullptr);

};

#endif // CLIENT_H
