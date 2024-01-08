#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <client.h>
QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Client client; // текущий клиент
    Widget(QWidget *parent = nullptr);
    ~Widget();
private:
    void writeLog(const QString & str); // запись событий
    void sendFile(const QString& );
public slots:

    void on_connectToServerButton_clicked();  // подключение к серверу
    void socketConnected();  // подключились к серверу
    void socketDisconnected(); // отключились от сервера
    void socketReadyRead(); // обработка нового сообщения
    void on_sendButton_clicked(); //отправка нового сообщения
    void on_colorButton_clicked(); // выбор цвета ника
    void on_sendFileButton_clicked(); // отправка файла


private:
    Ui::Widget *ui;
};
#endif // WIDGET_H
