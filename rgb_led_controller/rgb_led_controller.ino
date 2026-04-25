// RGB LED Controller
// Проект для управления RGB лентой через Arduino

// Пины для подключения RGB ленты
const int RED_PIN = 9;
const int GREEN_PIN = 10;
const int BLUE_PIN = 11;

// Текущие значения цветов (0-255)
int currentRed = 0;
int currentGreen = 0;
int currentBlue = 0;

// Параметры эффектов на основе времени
unsigned long previousMillis = 0;
float phase = 0.0;  // Фаза для синусоиды (0 - 2*PI)

void setup() {
  // Инициализация serial порта для отладки
  Serial.begin(9600);

  // Настройка пинов как выходы
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  // Начальное состояние - выключено
  setColor(0, 0, 0);

  Serial.println("RGB LED Controller initialized");
}

void loop() {
  // Основной цикл программы
  // Здесь можно добавить логику управления

  // Пример: эффект дыхания синим цветом (как у Алисы)
  // Раскомментируйте для демонстрации
  breatheEffect(0, 100, 255, 2000);  // Синий цвет, период 2 секунды

  // Другие примеры:
  // breatheEffect(255, 50, 150, 3000);  // Фиолетовый, медленнее
  // pulseEffect(255, 255, 255, 1500);   // Белый пульс
  // waveEffect(255, 0, 0, 2500);        // Красная волна
}

// Установить цвет RGB ленты
void setColor(int red, int green, int blue) {
  currentRed = constrain(red, 0, 255);
  currentGreen = constrain(green, 0, 255);
  currentBlue = constrain(blue, 0, 255);

  analogWrite(RED_PIN, currentRed);
  analogWrite(GREEN_PIN, currentGreen);
  analogWrite(BLUE_PIN, currentBlue);
}

// Плавное изменение цвета
void fadeToColor(int targetRed, int targetGreen, int targetBlue, int duration) {
  int steps = 50;
  int delayTime = duration / steps;

  for (int i = 0; i <= steps; i++) {
    int r = map(i, 0, steps, currentRed, targetRed);
    int g = map(i, 0, steps, currentGreen, targetGreen);
    int b = map(i, 0, steps, currentBlue, targetBlue);

    setColor(r, g, b);
    delay(delayTime);
  }
}

// Выключить ленту
void turnOff() {
  setColor(0, 0, 0);
}

// Предустановленные цвета
void setRed() { setColor(255, 0, 0); }
void setGreen() { setColor(0, 255, 0); }
void setBlue() { setColor(0, 0, 255); }
void setWhite() { setColor(255, 255, 255); }
void setYellow() { setColor(255, 255, 0); }
void setCyan() { setColor(0, 255, 255); }
void setMagenta() { setColor(255, 0, 255); }

// Эффект циклической смены цветов
void colorCycle() {
  fadeToColor(255, 0, 0, 500);    // Красный
  fadeToColor(0, 255, 0, 500);    // Зелёный
  fadeToColor(0, 0, 255, 500);    // Синий
  fadeToColor(255, 255, 0, 500);  // Жёлтый
  fadeToColor(0, 255, 255, 500);  // Циан
  fadeToColor(255, 0, 255, 500);  // Маджента
  fadeToColor(255, 255, 255, 500);// Белый
}

// ============================================
// ЭФФЕКТЫ НА ОСНОВЕ ВРЕМЕНИ (как у Алисы)
// ============================================

// Эффект "дыхания" - плавная пульсация с синусоидой
// Самый похожий на эффект Алисы от Яндекса
void breatheEffect(int red, int green, int blue, unsigned int period) {
  unsigned long currentMillis = millis();

  // Обновляем фазу на основе времени
  phase = (currentMillis % period) * 2.0 * PI / period;

  // Синусоида от 0 до 1 (используем sin^2 для более плавного эффекта)
  float brightness = (sin(phase) + 1.0) / 2.0;
  brightness = brightness * brightness;  // Квадрат для более выраженного затухания

  // Применяем яркость к цвету
  int r = (int)(red * brightness);
  int g = (int)(green * brightness);
  int b = (int)(blue * brightness);

  setColor(r, g, b);
}

// Эффект пульсации - более резкий переход (вкл/выкл)
void pulseEffect(int red, int green, int blue, unsigned int period) {
  unsigned long currentMillis = millis();

  phase = (currentMillis % period) * 2.0 * PI / period;

  // Используем abs(sin) для более четкой пульсации
  float brightness = abs(sin(phase));

  int r = (int)(red * brightness);
  int g = (int)(green * brightness);
  int b = (int)(blue * brightness);

  setColor(r, g, b);
}

// Эффект волны - плавное нарастание и спад
void waveEffect(int red, int green, int blue, unsigned int period) {
  unsigned long currentMillis = millis();

  phase = (currentMillis % period) * 2.0 * PI / period;

  // Только положительная часть синусоиды
  float brightness = max(0.0, sin(phase));

  int r = (int)(red * brightness);
  int g = (int)(green * brightness);
  int b = (int)(blue * brightness);

  setColor(r, g, b);
}

// Мерцание со случайными интервалами (эффект "мигания")
void flickerEffect(int red, int green, int blue, int minInterval, int maxInterval) {
  static unsigned long lastFlicker = 0;
  static unsigned long flickerInterval = 0;
  static bool isOn = false;

  unsigned long currentMillis = millis();

  if (currentMillis - lastFlicker >= flickerInterval) {
    isOn = !isOn;
    lastFlicker = currentMillis;
    flickerInterval = random(minInterval, maxInterval);

    if (isOn) {
      setColor(red, green, blue);
    } else {
      setColor(0, 0, 0);
    }
  }
}

// Радужный эффект - плавная смена цветов по спектру
void rainbowEffect(unsigned int period) {
  unsigned long currentMillis = millis();

  phase = (currentMillis % period) * 2.0 * PI / period;

  // Генерируем RGB из HSV (hue меняется со временем)
  float hue = phase / (2.0 * PI);  // 0.0 - 1.0

  int r, g, b;
  hsvToRgb(hue, 1.0, 1.0, r, g, b);

  setColor(r, g, b);
}

// Конвертация HSV в RGB для радужного эффекта
void hsvToRgb(float h, float s, float v, int &r, int &g, int &b) {
  float c = v * s;
  float x = c * (1.0 - abs(fmod(h * 6.0, 2.0) - 1.0));
  float m = v - c;

  float rPrime, gPrime, bPrime;

  if (h < 1.0/6.0) {
    rPrime = c; gPrime = x; bPrime = 0;
  } else if (h < 2.0/6.0) {
    rPrime = x; gPrime = c; bPrime = 0;
  } else if (h < 3.0/6.0) {
    rPrime = 0; gPrime = c; bPrime = x;
  } else if (h < 4.0/6.0) {
    rPrime = 0; gPrime = x; bPrime = c;
  } else if (h < 5.0/6.0) {
    rPrime = x; gPrime = 0; bPrime = c;
  } else {
    rPrime = c; gPrime = 0; bPrime = x;
  }

  r = (int)((rPrime + m) * 255);
  g = (int)((gPrime + m) * 255);
  b = (int)((bPrime + m) * 255);
}
