# ESP32-C3 / STM32F103C8T6 + ADXL345 + OLED peak pulse

Небольшой проект для ESP32-C3 или STM32F103C8T6: модуль читает ускорение ADXL345 по оси X, показывает текущее значение и максимум на OLED, а при новом максимуме выдает короткий импульс на светодиод.

Проект можно собирать как через PlatformIO, так и через Arduino IDE.

## Что делает

- читает данные ADXL345 по I2C;
- отображает текущее ускорение `X` и максимальное значение `Max` на OLED;
- мигает светодиодом при фиксации нового максимума;
- при проблеме с датчиком пишет подсказку в Serial и сканирует I2C-шину.

## Структура проекта

- `src/main.cpp` - основная версия для PlatformIO;
- `arduino/esp32c3_adxl345_oled/esp32c3_adxl345_oled.ino` - версия для Arduino IDE;
- `arduino/stm32_adxl345_oled_128x32/stm32_adxl345_oled_128x32.ino` - версия для Arduino IDE под STM32F103C8T6 и OLED 0.91" 128x32;
- `platformio.ini` - конфигурация PlatformIO.

## Железо

- ESP32-C3 или плата на STM32F103C8T6;
- ADXL345 по I2C;
- OLED I2C, адрес `0x3C`;
- для ESP32-C3: OLED `72x40`, `SDA=5`, `SCL=6`, смещение `X=28`, `Y=0`;
- для STM32F103C8T6: OLED LILUDIN 0.91" синий/белый `128x32`, `SDA=PB7`, `SCL=PB6`;
- светодиод на `LED_BUILTIN`; для популярных плат на STM32F103C8T6 обычно это `PC13` с обратной логикой.

## Подключение

| ESP32-C3 | OLED | ADXL345 |
| --- | --- | --- |
| 3V3 | VCC | VCC |
| GND | GND | GND |
| GPIO5 | SDA | SDA |
| GPIO6 | SCL | SCL |

Для STM32F103C8T6:

| STM32F103C8T6 | OLED 0.91" 128x32 | ADXL345 |
| --- | --- | --- |
| 3.3V | VCC | VCC |
| GND | GND | GND |
| PB7 | SDA | SDA |
| PB6 | SCL | SCL |

ADXL345 обычно работает по адресу `0x53`. Если вывод `SDO` подтянут к питанию, адрес может быть `0x1D`. В таком случае поменяйте `ADXL345_ADDR_7BIT` в коде.

## Сборка через PlatformIO

Среды уже описаны в `platformio.ini`:

- `esp32-c3-devkitm-1` - ESP32-C3 с маленьким OLED 72x40;
- `stm32-bluepill-f103c8-oled-128x32` - STM32F103C8T6 и OLED 0.91" 128x32, прошивка через USB-UART bootloader;
- `stm32-bluepill-f103c8-oled-128x32-stlink` - тот же STM32-вариант, но прошивка через ST-Link.

```bash
pio run
pio run --target upload
pio device monitor
```

Для сборки STM32-варианта:

```bash
pio run -e stm32-bluepill-f103c8-oled-128x32
pio device list
pio run -e stm32-bluepill-f103c8-oled-128x32 --target upload --upload-port /dev/cu.usbserial-XXXX
pio device monitor -b 115200
```

Перед прошивкой через USB-UART нужно поставить `BOOT0 = 1`, нажать `Reset`, после прошивки вернуть `BOOT0 = 0` и снова нажать `Reset`.

## Сборка через Arduino IDE

Для ESP32-C3 откройте файл `arduino/esp32c3_adxl345_oled/esp32c3_adxl345_oled.ino`.

Для STM32 откройте файл `arduino/stm32_adxl345_oled_128x32/stm32_adxl345_oled_128x32.ino`.

Перед загрузкой установите библиотеки через Library Manager:

- `Adafruit ADXL345`
- `Adafruit Unified Sensor`
- `U8g2`

Дальше:

1. Установите поддержку плат ESP32 от Espressif через Boards Manager.
2. Выберите плату ESP32-C3.
3. Укажите нужный COM/USB-порт.
4. Нажмите Upload.

Для STM32F103C8T6:

1. Установите поддержку плат STM32 MCU based boards через Boards Manager.
2. Выберите плату `Generic STM32F1 series` / `BluePill F103C8` или ближайший вариант для вашей платы на STM32F103C8T6.
3. Укажите способ загрузки: через ST-Link или USB bootloader, в зависимости от платы.
4. Укажите нужный порт или программатор.
5. Нажмите Upload.

## Настройки

Основные параметры находятся в начале `src/main.cpp` и дублируются в `.ino`-версии:

- `I2C_SDA_PIN`, `I2C_SCL_PIN` - пины I2C (`GPIO5/GPIO6` для ESP32-C3, `PB7/PB6` для STM32F103C8T6);
- `OLED_ADDR_7BIT = 0x3C` - адрес OLED;
- `ADXL345_ADDR_7BIT = 0x53` - адрес акселерометра;
- `DISPLAY_72X40 = 1` - экран `72x40` со смещением `X=28`; для OLED `128x32` используется `0`;
- `LED_ACTIVE_LOW = 1` - для плат на STM32F103C8T6, где встроенный светодиод на `PC13` обычно включается уровнем `LOW`;
- `TRIGGER_STEP = 0.25f` - насколько новый `X` должен превысить прошлый максимум, чтобы сработал импульс;
- `LED_PULSE_MS = 80` - длина импульса на светодиод;
- `RESET_DROP = 0.15f` - насколько значение должно опуститься, чтобы следующий пик снова считался новым.

## Что видно на экране

- `X` - текущее ускорение по оси X в `m/s^2`;
- `Max` - максимальное пойманное значение X.

Если ADXL345 не найден, в Serial Monitor появится сообщение с подсказкой и список найденных I2C-адресов.

## PlatformIO и Arduino IDE

По коду разница небольшая: проект уже подготовлен под оба варианта. Для Arduino IDE нужен файл `.ino`, для PlatformIO - `src/main.cpp` и `platformio.ini`.

На практике:

- `PlatformIO` удобнее для повторяемой сборки, зависимостей и нескольких окружений;
- `Arduino IDE` проще поставить и проще отдать другому человеку;
- для этого проекта результат будет почти одинаковый, если выбрана та же плата и те же библиотеки.

Если проект нужен отцу "открыть и прошить", Arduino IDE вполне нормальный вариант.
