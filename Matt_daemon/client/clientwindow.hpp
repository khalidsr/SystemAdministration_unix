#ifndef CLIENTWINDOW_HPP
#define CLIENTWINDOW_HPP

#include <QWidget>
#include <QTcpSocket>

class QLineEdit;
class QTextEdit;
class QPushButton;

class ClientWindow : public QWidget 
{
    Q_OBJECT

public:
    ClientWindow(QWidget *parent = nullptr);

private slots:
    void sendToDaemon();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError);

private:
    QLineEdit *usernameEdit;
    QLineEdit *passwordEdit;
    QLineEdit *commandEdit;
    QTextEdit *responseBox;
    QPushButton *sendButton;
    QTcpSocket *socket;
};

#endif