# Инструкция по прошивке Arduino

## Скомпилированные файлы

Проект успешно скомпилирован. Файлы находятся в папке `build/`.

**Главный файл для прошивки:**
- `rgb_led_controller.ino.hex` - HEX файл для загрузки в Arduino (10 KB)

**Статистика:**
- Использовано памяти программ: 3788 байт (11%) из 32256 байт
- Использовано оперативной памяти: 222 байта (10%) из 2048 байт

## Способы прошивки

### Способ 1: Через Arduino CLI (рекомендуется)

1. Подключите Arduino к компьютеру через USB

2. Узнайте порт Arduino:
```bash
arduino-cli board list
```

3. Перейдите в корень проекта и прошейте Arduino (замените `/dev/cu.usbserial-XXX` на ваш порт):
```bash
arduino-cli upload -p /dev/cu.usbserial-XXX --fqbn arduino:avr:uno rgb_led_controller
```

**Для Arduino Uno через USB обычно порт:**
- macOS: `/dev/cu.usbserial-*` или `/dev/cu.usbmodem*`
- Linux: `/dev/ttyUSB0` или `/dev/ttyACM0`
- Windows: `COM3`, `COM4` и т.д.

### Способ 2: Через Arduino IDE

1. Откройте Arduino IDE
2. Откройте файл `rgb_led_controller/rgb_led_controller.ino`
3. Выберите плату: Tools → Board → Arduino AVR Boards → Arduino Uno
4. Выберите порт: Tools → Port → (ваш порт)
5. Нажмите кнопку Upload (→)

### Способ 3: Прямая прошивка HEX файла через avrdude

Если у вас установлен avrdude:
```bash
avrdude -C/etc/avrdude.conf -v -patmega328p -carduino -P/dev/cu.usbserial-XXX -b115200 -D -Uflash:w:build/rgb_led_controller.ino.hex:i
```

## Быстрая команда для прошивки

Скопируйте и выполните (после подключения Arduino):

```bash
# Показать доступные порты
arduino-cli board list

# Перейти в папку проекта
cd /path/to/arduino

# Прошить (замените порт на свой!)
arduino-cli upload -p /dev/cu.usbmodem14101 --fqbn arduino:avr:uno rgb_led_controller
```

## После прошивки

1. Подключите RGB ленту к пинам 9, 10, 11 (см. README.md)
2. Откройте Serial Monitor (115200 baud) чтобы увидеть отладочные сообщения
3. Лента должна начать плавно "дышать" синим цветом

## Изменение эффекта

Чтобы изменить эффект, отредактируйте функцию `loop()` в файле:
`rgb_led_controller/rgb_led_controller.ino`

Затем снова скомпилируйте и прошейте:
```bash
arduino-cli compile --fqbn arduino:avr:uno --output-dir build rgb_led_controller
arduino-cli upload -p /dev/cu.usbmodem14101 --fqbn arduino:avr:uno rgb_led_controller
```

## Поддерживаемые платы

Текущая компиляция для: **Arduino Uno** (atmega328p)

Для других плат измените `--fqbn`:
- Arduino Nano: `arduino:avr:nano`
- Arduino Mega: `arduino:avr:mega`
- Arduino Leonardo: `arduino:avr:leonardo`

## Troubleshooting

### Ошибка "Permission denied"
```bash
sudo chmod 666 /dev/cu.usbserial-XXX
```

### Не находит плату
- Проверьте USB кабель (должен поддерживать передачу данных, не только зарядку)
- Установите драйвера CH340/CP2102 если используете клон Arduino
- Попробуйте другой USB порт

### Ошибка при прошивке
- Убедитесь что Serial Monitor закрыт
- Нажмите кнопку Reset на Arduino перед прошивкой
- Проверьте правильность выбора платы
