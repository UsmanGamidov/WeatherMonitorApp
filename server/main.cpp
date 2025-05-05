#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

class WeatherServer : public QTcpServer {
    Q_OBJECT

private:
    QNetworkAccessManager *manager;

public:
    WeatherServer(QObject *parent = nullptr) : QTcpServer(parent) {
        manager = new QNetworkAccessManager(this);
    }

    bool startServer(quint16 port = 12345) {
        if (listen(QHostAddress::Any, port)) {
            qDebug() << "🚀 Сервер запущен на порту" << port;
            return true;
        } else {
            qDebug() << "❌ Ошибка запуска сервера:" << errorString();
            return false;
        }
    }

protected:
    void incomingConnection(qintptr socketDescriptor) override {
        QTcpSocket *clientSocket = new QTcpSocket(this);
        clientSocket->setSocketDescriptor(socketDescriptor);

        qDebug() << "🔌 Клиент подключился:" << clientSocket->peerAddress().toString();

        connect(clientSocket, &QTcpSocket::readyRead, this, [this, clientSocket]() {
            QByteArray data = clientSocket->readAll();
            qDebug() << "📥 Получено от клиента:" << QString::fromUtf8(data);

            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
            if (parseError.error != QJsonParseError::NoError) {
                qDebug() << "❌ Ошибка парсинга JSON:" << parseError.errorString();
                return;
            }

            QJsonObject obj = doc.object();

            if (obj["type"] == "login") {
                QString username = obj["username"].toString();
                QString password = obj["password"].toString();

                QJsonObject response;
                response["type"] = "login";
                response["status"] = (username == "admin" && password == "12345") ? "success" : "failed";

                QJsonDocument responseDoc(response);
                clientSocket->write(responseDoc.toJson(QJsonDocument::Compact));
                clientSocket->flush();
            }

            else if (obj["type"] == "weather") {
                QString city = obj["city"].toString();
                QString apiKey = "eb4bfe4c0895bd7ae14f3afcda6ab01a"; // ← твой API ключ

                qDebug() << "🌍 Запрос погоды для:" << city;

                QString url = QString("http://api.openweathermap.org/data/2.5/weather?q=%1&units=metric&appid=%2")
                                  .arg(city, apiKey);
                QNetworkRequest request(url);
                QNetworkReply *reply = manager->get(request);

                connect(reply, &QNetworkReply::finished, this, [reply, clientSocket]() {
                    if (reply->error() != QNetworkReply::NoError) {
                        QJsonObject errorResponse;
                        errorResponse["type"] = "weather";
                        errorResponse["status"] = "error";
                        errorResponse["message"] = reply->errorString();
                        clientSocket->write(QJsonDocument(errorResponse).toJson(QJsonDocument::Compact));
                        clientSocket->flush();
                        reply->deleteLater();
                        return;
                    }

                    QByteArray responseData = reply->readAll();
                    QJsonDocument doc = QJsonDocument::fromJson(responseData);
                    QJsonObject root = doc.object();

                    QJsonObject weatherResponse;
                    weatherResponse["type"] = "weather";
                    weatherResponse["status"] = "success";
                    weatherResponse["city"] = root["name"].toString();
                    weatherResponse["temp"] = root["main"].toObject()["temp"];
                    weatherResponse["desc"] = root["weather"].toArray()[0].toObject()["description"];

                    QJsonDocument responseDoc(weatherResponse);
                    clientSocket->write(responseDoc.toJson(QJsonDocument::Compact));
                    clientSocket->flush();

                    qDebug() << "📤 Отправляем клиенту:" << responseDoc.toJson(QJsonDocument::Compact);
                    reply->deleteLater();
                });
            }
        });

        connect(clientSocket, &QTcpSocket::disconnected, this, [clientSocket]() {
            qDebug() << "❌ Клиент отключился:" << clientSocket->peerAddress().toString();
        });
    }
};

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);

    WeatherServer server;
    if (!server.startServer()) {
        return -1;
    }

    return a.exec();
}

#include "main.moc"
