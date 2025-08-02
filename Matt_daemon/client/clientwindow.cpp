#include "clientwindow.hpp"
#include <QVBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QMessageBox>
#include <QHostAddress>

ClientWindow::ClientWindow(QWidget *parent) : QWidget(parent) 
{
    setWindowTitle("MattDaemon Client");
    resize(400, 300);

    auto *layout = new QVBoxLayout(this);

    layout->addWidget(new QLabel("Username:"));
    usernameEdit = new QLineEdit();
    layout->addWidget(usernameEdit);

    layout->addWidget(new QLabel("Password:"));
    passwordEdit = new QLineEdit();
    passwordEdit->setEchoMode(QLineEdit::Password);
    layout->addWidget(passwordEdit);

    layout->addWidget(new QLabel("Command:"));
    commandEdit = new QLineEdit();
    layout->addWidget(commandEdit);

    sendButton = new QPushButton("Send");
    layout->addWidget(sendButton);

    responseBox = new QTextEdit();
    responseBox->setReadOnly(true);
    layout->addWidget(responseBox);

    socket = new QTcpSocket(this);

    connect(sendButton, &QPushButton::clicked, this, &ClientWindow::sendToDaemon);
    connect(socket, &QTcpSocket::readyRead, this, &ClientWindow::onReadyRead);
    connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
            this, &ClientWindow::onError);
}

void ClientWindow::sendToDaemon() 
{
    QString user = usernameEdit->text();
    QString pass = passwordEdit->text();
    QString cmd  = commandEdit->text();

    if (user.isEmpty() || pass.isEmpty() || cmd.isEmpty()) 
    {
        QMessageBox::warning(this, "Input Missing", "Please fill all fields.");
        return;
    }

    QString message = user + ":" + pass + ":" + cmd;
    socket->connectToHost(QHostAddress::LocalHost, 4242);
    if (!socket->waitForConnected(1000)) 
    {
        QMessageBox::critical(this, "Error", "Could not connect to daemon.");
        return;
    }

    socket->write(message.toUtf8());
}

void ClientWindow::onReadyRead() 
{
    QByteArray response = socket->readAll();
    responseBox->append("Daemon: " + QString::fromUtf8(response));
    socket->disconnectFromHost();
}

void ClientWindow::onError(QAbstractSocket::SocketError) 
{
    responseBox->append("Socket error: " + socket->errorString());
}