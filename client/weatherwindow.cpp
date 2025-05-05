#include "weatherwindow.h"
#include "ui_weatherwindow.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

WeatherWindow::WeatherWindow(QTcpSocket *socket, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::WeatherWindow)
    , socket(socket)
{
    ui->setupUi(this);

    // Только один connect: когда придёт ответ — обработаем
    connect(socket, &QTcpSocket::readyRead, this, &WeatherWindow::readWeatherResponse);

    // Кнопка отправляет запрос
    connect(ui->getWeatherButton, &QPushButton::clicked, this, &WeatherWindow::requestWeather);
}

WeatherWindow::~WeatherWindow()
{
    delete ui;
}

void WeatherWindow::requestWeather()
{
    QString city = ui->cityEdit->text();

    QJsonObject request;
    request["type"] = "weather";
    request["city"] = city;

    QJsonDocument doc(request);
    socket->write(doc.toJson(QJsonDocument::Compact));
    socket->flush();

    qDebug() << "➡️ Запрос погоды для:" << city;

    // 🔄 Очистим старый результат
    ui->resultLabel->clear();
    ui->resultLabel->setText("⏳ Загружается погода...");
}

void WeatherWindow::readWeatherResponse()
{
    QByteArray data = socket->readAll();
    qDebug() << "⬅️ Ответ от сервера:" << data;

    if (data.isEmpty()) {
        qDebug() << "⚠️ Ответ пустой!";
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();

    if (obj["type"] == "weather" && obj["status"] == "success") {
        QString city = obj["city"].toString();
        double temp = obj["temp"].toDouble();
        QString desc = obj["desc"].toString();

        qDebug() << "🌤 Город:" << city << "Температура:" << temp << "Описание:" << desc;

        ui->resultLabel->setText(
            QString("🌤 %1\n🌡 %2°C\n📋 %3").arg(city).arg(temp).arg(desc)
            );

        // ✅ Очистка после успешного получения
        ui->cityEdit->clear();
        ui->cityEdit->setFocus();
    }
}

