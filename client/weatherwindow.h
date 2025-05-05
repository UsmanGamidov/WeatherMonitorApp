#ifndef WEATHERWINDOW_H
#define WEATHERWINDOW_H

#include <QDialog>
#include <QTcpSocket>

namespace Ui {
class WeatherWindow;
}

class WeatherWindow : public QDialog
{
    Q_OBJECT

public:
    explicit WeatherWindow(QTcpSocket *socket, QWidget *parent = nullptr);
    ~WeatherWindow();

private:
    Ui::WeatherWindow *ui;
    QTcpSocket *socket;

private slots:
    void requestWeather();
    void readWeatherResponse();
};

#endif // WEATHERWINDOW_H
