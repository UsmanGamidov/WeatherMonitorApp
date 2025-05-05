#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "weatherwindow.h"
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    socket = new QTcpSocket(this);

    // Когда подключились — отправляем логин
    connect(socket, &QTcpSocket::connected, this, [this]() {
        sendLogin();
    });

    // Когда пришёл ответ от сервера
    connect(socket, &QTcpSocket::readyRead, this, [this]() {
        QByteArray data = socket->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject obj = doc.object();

        if (obj["type"] == "login") {
            if (obj["status"] == "success") {
                ui->statusLabel->setText("✅ Авторизация успешна");

                // Отключаем старый обработчик readyRead
                disconnect(socket, &QTcpSocket::readyRead, nullptr, nullptr);

                // Показываем окно погоды
                WeatherWindow *weatherWin = new WeatherWindow(socket);
                weatherWin->show();
                this->hide();
            } else {
                ui->statusLabel->setText("❌ Неверный логин или пароль");
            }
        }
    });

    // Нажатие на кнопку "Войти"
    connect(ui->loginButton, &QPushButton::clicked, this, [this]() {
        if (socket->state() == QAbstractSocket::ConnectedState) {
            sendLogin();
        } else {
            socket->connectToHost("127.0.0.1", 12345);
        }
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::sendLogin()
{
    QJsonObject request;
    request["type"] = "login";
    request["username"] = ui->loginEdit->text();
    request["password"] = ui->passwordEdit->text();

    QJsonDocument doc(request);
    socket->write(doc.toJson(QJsonDocument::Compact));
    socket->flush();
}
