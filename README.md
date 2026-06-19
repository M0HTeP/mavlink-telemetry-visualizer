
## Сборка
```bash
cd /mavlink-telemetry-visualizer
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

## Запуск
1. Запустите приложение:
   ```bash
   ./mav_visualizer
   ```

2. Запустите SITL:
   ```bash
   ardupilot/Tools/autotest/sim_vehicle.py -v ArduCopter --console --map --out=udp:127.0.0.1:14550
   ```

## Работа с окном графиков

Колесо мыши - зумит по оси Х

Левая кнопка мыши - движение по графику 

Правая кнопка мыши - при нажатии на нужный график в легенде - включает\отключает его показ


## API работы с видимыми сериями и историей данных

- `std::vector<SeriesKey> MainWindow::getAvailableSeries() const`
  - возвращает список доступных пар `(message_id, param_name)`.

- `void MainWindow::setVisibleSeries(const std::vector<SeriesKey>& series)`
  - меняет набор видимых серий на графике; остальные серии скрываются.

- `std::vector<SeriesKey> MainWindow::getVisibleSeries() const`
  - возвращает текущие видимые серии.

- `std::vector<TelemetryPoint> MainWindow::querySeriesData(const SeriesKey& key, TimePoint start, TimePoint end) const`
  - возвращает точки для серии за указанный интервал времени.



