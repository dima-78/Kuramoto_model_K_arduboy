#include <Arduboy2.h>
#include <stdlib.h>
#include <math.h>
#include "Tinyfont.h"

// #define K 0.1  
// #define DT 0.05 
#define CIRCLE_RADIUS 20  
#define CIRCLE_CENTER_X 32 
#define CIRCLE_CENTER_Y 32  
#define PENDULUM_ORIGIN_X 96 
#define PENDULUM_ORIGIN_Y 20  
#define PENDULUM_LENGTH 30  
#define MAX_ANGLE 0.523599  
#define MAX_N 10
#define MIN_N 1

Arduboy2 arduboy;
BeepPin2 beep;
Tinyfont tinyfont = Tinyfont(arduboy.sBuffer, Arduboy2::width(), Arduboy2::height());

char allLetters[101];

int N = 3;  // количество осцилляторов
double DT = 0.05;
double K = 0.1;
double *phases = NULL;     // указатель фаз
double *frequencies = NULL; // указатель частот
bool playSound = false;  // флаг звука
bool soundPlayed = false; // флаг звука в крайних положениях маятника

int tactCount = 0; // количество тактов маятника
bool phasePassedZero = false; // флаг отслеживания перехода через 0

unsigned long lastTactTime = 0; // время последнего такта
unsigned long tactInterval = 0; // интервал между тактами
double bpm = 0;                // количество ударов в минуту
bool wasAtExtreme = false;     // флаг отслеживания крайних положений

double smoothedBPM = 0; // для сглаживания

void initializeArrays();
void initializePhases();
void updatePhases();
void calculateBPM();
void handleInput();
void drawSimulation();
void cleanup();

void setup() {
  arduboy.begin();
  arduboy.clear();
  arduboy.print(F("   Kuramoto_model_K\n"
                  "  LEFT: + freq\n"
                  " RIGHT: - freq\n"
                  "    UP: + N(oscill)\n"
                  "  DOWN: - N(oscill)\n"
                  "     A: Off/On sound\n"
                  "     B: DT + 0.01\n"
                  "B+UP/D: K +/-0.01\n"
                  " A + B: Exit\n"
                  "  Press A to start"));
  arduboy.display();

  while (!arduboy.pressed(A_BUTTON)) {
    arduboy.idle();
  }
  
  // create all ascii letters from 32-126
  size_t newLineCounter = 0;
  for (size_t i = 0; i < 100; i++) {
    if ((i % 26) == 0) {
      allLetters[i] = '\n';
      newLineCounter++;
    }
    else{
      allLetters[i] = (char) (i+32) - newLineCounter;
    }
  }
  allLetters[100] = '\0';
  
    initializeArrays();  // инитим массивы и фазы
    initializePhases(); 
}

void loop() {
    if (!(arduboy.nextFrame())) return;
    beep.timer();

    /*
    double R = 0;
    double sumX = 0;
    double sumY = 0;
    for (int i = 0; i < N; i++) {
    sumX += cos(phases[i]);
    sumY += sin(phases[i]);
    }
    R = sqrt(sumX * sumX + sumY * sumY) / N;
    
    drawOrderParameter(R);

    drawDynamicSine(R);
    */
    // drawPhaseDistribution();
    
    handleInput();
    updatePhases();
    double normalizedPhase = fmod(phases[0], 2 * PI);
    if (normalizedPhase < 0) normalizedPhase += 2 * PI;
    double angle = MAX_ANGLE * sin(normalizedPhase);  
      
    calculateBPM(normalizedPhase);
    smoothBPM();
    
    drawSimulation();    
    arduboy.display();
}

void initializeArrays() {
    // чистим память, если массивы уже существуют
    if (phases != NULL) {
        free(phases);
        free(frequencies);
    }

    // выделяем память под массивы
    phases = (double *)malloc(N * sizeof(double));
    frequencies = (double *)malloc(N * sizeof(double));

    // инитим частоты
    for (int i = 0; i < N; i++) {
        frequencies[i] = 1.0 + i * 0.1; 
    }
}

void initializePhases() {
    for (int i = 0; i < N; i++) {
        phases[i] = random(0, 628) / 100.0;  // инитим случайные фазы от 0 до 2*PI
    }
}

void updatePhases() {
    double *newPhases = (double *)malloc(N * sizeof(double));  // временный массив для новых фаз

    for (int i = 0; i < N; i++) {
        double coupling = 0.0;
        for (int j = 0; j < N; j++) {
            if (i != j) {
                coupling += sin(phases[j] - phases[i]);
            }
        }
        newPhases[i] = phases[i] + (frequencies[i] + K * coupling) * DT;
    }

    // Обновляем фазы
    for (int i = 0; i < N; i++) {
        phases[i] = newPhases[i];
    }

    free(newPhases);  // чистим временный массив
}

void handleInput() {
    arduboy.pollButtons();
    
    if (arduboy.justPressed(A_BUTTON)) {
        playSound = !playSound;
    }
    if (arduboy.justPressed(B_BUTTON)) {
        DT += 0.01;
        if (DT > 0.1) {
            DT = 0.01; 
        }
    }

    // изменение K при удержании B_BUTTON
    if (arduboy.pressed(B_BUTTON)) {
        if (arduboy.justPressed(UP_BUTTON)) {
            K += 0.01;
            if (K > 1.0) K = 1.0;
        }
        if (arduboy.justPressed(DOWN_BUTTON)) {
            K -= 0.01;
            if (K < 0.0) K = 0.0; 
        }
    } else { // логика изменения N, если B_BUTTON не удерживается
        if (arduboy.justPressed(UP_BUTTON) && N < MAX_N) {
            N++;
            initializeArrays();  
        }
        if (arduboy.justPressed(DOWN_BUTTON) && N > MIN_N) {
            N--;
            initializeArrays();  
        }
    }

    if (arduboy.pressed(RIGHT_BUTTON)) {
        frequencies[0] += 0.1;  
    }
    if (arduboy.pressed(LEFT_BUTTON)) {
        frequencies[0] -= 0.1;
    }
}


void drawOrderParameter(double R) {
    int barWidth = 80;  // ширина прогресс-бара
    int barHeight = 4;  // высота прогресс-бара
    int barX = 24;      // по X
    int barY = 56;      // по Y

    int filledWidth = (int)(R * barWidth);  // ширина заполненной части
    arduboy.drawRect(barX, barY, barWidth, barHeight);  // рамка
    arduboy.fillRect(barX, barY, filledWidth, barHeight);  // заполняем прогресс-бар
}


void drawPhaseDistribution() {
    for (int i = 0; i < N; i++) {
        int x = CIRCLE_CENTER_X + CIRCLE_RADIUS * cos(phases[i]);
        int y = CIRCLE_CENTER_Y + CIRCLE_RADIUS * sin(phases[i]);
        arduboy.drawPixel(x, y);  //рисуем фазу как точку
    }
}

void drawDynamicSine(double R) {
    int graphXStart = 10;
    int graphYCenter = 32;
    int graphWidth = 108;
    int graphHeight = 20;

    for (int x = 0; x < graphWidth; x += 2) {
        int pointX = graphXStart + x;
        int pointY = graphYCenter - (int)(R * graphHeight * sin((2 * PI * x) / graphWidth));
        arduboy.drawPixel(pointX, pointY);  // точка на графике
    }
}

void checkAndPlaySound(double angle) {
    if (playSound) {
        if (abs(angle - MAX_ANGLE) < 0.01 || abs(angle + MAX_ANGLE) < 0.01) {
            if (!soundPlayed) {
                int toneFreq = 880;
                beep.tone(beep.freq(toneFreq));
                arduboy.delayShort(10);
                beep.noTone();
                soundPlayed = true;
            }
        } else {
            soundPlayed = false;
        }
    }
}


// DEBUG
void smoothBPM() {
    const double alpha = 0.1; // коэффициент сглаживания (0 < alpha < 1)
    smoothedBPM = alpha * bpm + (1 - alpha) * smoothedBPM;
}

void calculateBPM(double normalizedPhase) {
    const double threshold = 0.05; // порог отклонения
    bool atExtreme = (abs(normalizedPhase - PI / 2) < threshold || abs(normalizedPhase - 3 * PI / 2) < threshold);

    if (atExtreme && !wasAtExtreme) { //фиксация крайнего положения
        unsigned long currentTime = millis();
        if (lastTactTime > 0) {
            tactInterval = currentTime - lastTactTime;
            bpm = 60000.0 / tactInterval;
        }
        lastTactTime = currentTime;
        wasAtExtreme = true;
    } else if (!atExtreme) {
        wasAtExtreme = false;
    }
}

void drawSimulation() {
    arduboy.clear();

    // tinyfont.setCursor(0, 0);
    // tinyfont.print("(K/N)*sum(sin(Theta_j-Theta_i))");
    
    double meanFrequency = 0;
    for (int i = 0; i < N; i++) {
      meanFrequency += frequencies[i];
    }
    meanFrequency /= N;

    tinyfont.setCursor(0, 0);
    tinyfont.print("Freq=");
    tinyfont.print(meanFrequency, 2);
    
    tinyfont.setCursor(56, 0);
    tinyfont.print("N= ");
    tinyfont.print(N);

    tinyfont.setCursor(85, 0);
    tinyfont.print("DT= ");
    tinyfont.print(DT);

    tinyfont.setCursor(2, 6);
    tinyfont.print("K= ");
    tinyfont.print(K);
    
    // рассчитываем угол маятника, синхронизированный с движением точки по окружности
    double normalizedPhase = fmod(phases[0], 2 * PI);  // ограничиваем фазу от 0 до 2*PI
    double angle = MAX_ANGLE * cos(normalizedPhase);   // отклонение маятника

    // координаты корпуса метронома
    int topX1 = PENDULUM_ORIGIN_X - 5;         // левая точка вершины (уже)
    int topX2 = PENDULUM_ORIGIN_X + 5;         // правая точка вершины
    int topY = PENDULUM_ORIGIN_Y;              // верхний край усечённого треугольника
    int bottomX1 = PENDULUM_ORIGIN_X - 20;     // левая нижняя точка (шире основания)
    int bottomX2 = PENDULUM_ORIGIN_X + 20;     // правая нижняя точка
    int bottomY = PENDULUM_ORIGIN_Y + 40;      // нижний край треугольника (поднято)

    // корпус метронома (усечённый треугольник)
    arduboy.drawLine(bottomX1, bottomY, topX1, topY);  // левая сторона
    arduboy.drawLine(bottomX2, bottomY, topX2, topY);  // правая сторона
    arduboy.drawLine(topX1, topY, topX2, topY);        // усечённая вершина
    arduboy.drawLine(bottomX1, bottomY, bottomX2, bottomY); // основание

    // точка крепления маятника (центр основания корпуса)
    int pivotX = (bottomX1 + bottomX2) / 2;  // центр основания
    int pivotY = bottomY;

    // маятник
    int pendulumLength = 35;  // длина маятника
    int pendulumEndX = pivotX + pendulumLength * sin(angle);  // горизонтальное смещение
    int pendulumEndY = pivotY - pendulumLength * cos(angle);  // вертикальное направление

    // рисуем маятник
    arduboy.drawLine(pivotX, pivotY, pendulumEndX, pendulumEndY);  // линия маятника
    arduboy.fillCircle(pendulumEndX, pendulumEndY, 2);            // груз маятника

    // синусоидальный график
    int graphXStart = 10;   // начало графика
    int graphYCenter = 32;  // центр графика по вертикали
    int graphWidth = 108;   // ширина графика
    int graphHeight = 20;   // высота графика
    int graphStep = 2;      // шаг по x для прорисовки графика

    /*
    // Рисуем ось графика
    arduboy.drawLine(graphXStart, graphYCenter, graphXStart + graphWidth, graphYCenter);

    // Прорисовка синусоиды
    for (int x = 0; x < graphWidth; x += graphStep) {
        int x1 = graphXStart + x;
        int y1 = graphYCenter - (int)(graphHeight * sin((2 * PI * x) / graphWidth));
        int x2 = graphXStart + x + graphStep;
        int y2 = graphYCenter - (int)(graphHeight * sin((2 * PI * (x + graphStep)) / graphWidth));
        arduboy.drawLine(x1, y1, x2, y2);
    }
    */
    
    // ось графика
    for (int x = 0; x <= graphWidth; x += graphStep) {
        int pointX = graphXStart + x;
        arduboy.drawPixel(pointX, graphYCenter);  // точка на оси
    }

    // синусоида
    for (int x = 0; x < graphWidth; x += graphStep) {
        int pointX = graphXStart + x;
        int pointY = graphYCenter - (int)(graphHeight * sin((2 * PI * x) / graphWidth));
        arduboy.drawPixel(pointX, pointY);  // точка на графике
    }
    
    // точку перемещающаем по синусоиде
    int pointX = graphXStart + (int)(normalizedPhase * (graphWidth / (2 * PI)));
    int pointY = graphYCenter - (int)(graphHeight * sin(normalizedPhase));
    arduboy.fillCircle(pointX, pointY, 1);
    
    checkAndPlaySound(angle);
    
    // метки на основании метронома
    for (int i = -5; i <= 5; i++) {
        int tickX = PENDULUM_ORIGIN_X + i * 4;
        int tickY1 = bottomY - 2;
        int tickY2 = bottomY;
        arduboy.drawLine(tickX, tickY1, tickX, tickY2);  // вертикальные метки
    }

    // окружность с точкой
    arduboy.drawCircle(CIRCLE_CENTER_X, CIRCLE_CENTER_Y, CIRCLE_RADIUS);
    int circleX = CIRCLE_CENTER_X + CIRCLE_RADIUS * cos(normalizedPhase);
    int circleY = CIRCLE_CENTER_Y + CIRCLE_RADIUS * sin(normalizedPhase);
    arduboy.fillCircle(circleX, circleY, 2);  // точка на окружности

    // углы
    int angleDegrees = (int)(normalizedPhase * (180.0 / PI)) % 360;
    if (angleDegrees < 0) angleDegrees += 360;

    tinyfont.setCursor(12, 56);
    tinyfont.print("Angle:");
    tinyfont.print(angleDegrees);
    tinyfont.setCursor(68, 6);
    tinyfont.print("Pendulum:");
    tinyfont.print(angle * (abs(180.0 / PI)), 0);

    //double bpm = (1.0 / (DT * 2 * PI)) * 60;

    
    // подсчет тактов
    /*
    if (normalizedPhase < PI && !phasePassedZero) {
        tactCount++;
        phasePassedZero = true;
    } 
    if (normalizedPhase >= PI) {
        phasePassedZero = false;
    }
    */
    
    // подсчет BPM (ударов в минуту)
    /*
    static bool wasAtExtreme = false;
    if (abs(normalizedPhase - PI / 2) < 0.01 || abs(normalizedPhase - 3 * PI / 2) < 0.01) {
        if (!wasAtExtreme) {
            unsigned long currentTime = millis();
            if (lastTactTime > 0) {
                unsigned long deltaTime = currentTime - lastTactTime;
                bpm = 60000.0 / deltaTime;
            }
            lastTactTime = currentTime;
            wasAtExtreme = true;
        }
    } else {
        wasAtExtreme = false;
    }
    */

    //calculateBPM(normalizedPhase);
    
    tinyfont.setCursor(76, 14);
    tinyfont.print("BPM:");
    //tinyfont.print(smoothedBPM, 0);
    tinyfont.print(bpm, 0);
}

void cleanup() {
    free(phases);
    free(frequencies);
}
