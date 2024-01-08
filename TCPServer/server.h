#ifndef SERVER_H
#define SERVER_H

#include <QWidget>
//#include <QTcpSocket>
//#include <QTcpServer>
#include <QVector>
#include <QMap>

class QTcpServer;
class QTcpSocket;


/////////////////////////////////////////////////////////////////////////////////////////
///         CONTRACT
/// 0 Message
/// 1 Socket name
/// 2 Userlist
/// 3 File
/// /////////////////////////////////////////////////////////////////////////////////////

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Server : public QWidget
{
    Q_OBJECT

public:
    Server(QWidget *parent = nullptr);
    ~Server();

private:
    Ui::Widget *ui; // интерфейс
    QTcpServer* _server;// текущий сервер
    QTcpSocket* _socket; // текущий клиент
    QMap<QTcpSocket*,QString> _users; // подключенные клиенты

    bool _abort;
    int _blockSize;

   void serverNewConnection(); // новое подключение
   void socketReadyRead();  // обработка нового сообщения
   void socketDisconnected(); // отключение клиента
   void writeLog(const QString&, QTcpSocket* ); // запись логов
   void sendMessageToUsers(const QString& ); // рассылка сообщений клиентам
   void sendUserList(); // отправка списка пользователей клиентам
   void sendFileToUser( QByteArray& ); // отправка файла для пользователей
   public slots:
   void on_sendButton();



};
#endif // SERVER_H
