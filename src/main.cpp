#include <QApplication>
#include <QMessageBox>
#include "common/config.h"
#include "ui/main_window.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Устанавливаем стиль и имя приложения
    app.setApplicationName("Mavlink Telemetry Visualizer");
    app.setStyle("Fusion");

    // Загружаем конфиг (используем глобальную инстанцию)
    gAppConfig = loadConfig();
    const auto& cfg = getAppConfig();
    QString host = QString::fromStdString(cfg.udp.host);
    quint16 port = static_cast<quint16>(cfg.udp.port);

    MainWindow w;
    w.resize(1400, 900);
    w.show();

    // Запуск транспорта (UDP подключение к SITL)
    auto transport = w.getTransport();
    if (!transport->start(host, port)) {
        QMessageBox::critical(&w, "Ошибка UDP", 
            QString("Не удалось запустить UDP транспорт.\n"
                    "Хост: %1\nПорт: %2").arg(host).arg(port));
    }

    return app.exec();
}