#include <pitches.h>
#include <IRremote.h>
#include <Adafruit_BusIO_Register.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_I2CRegister.h>
#include <Adafruit_SPIDevice.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>

#define PIN_SCLK 8
#define PIN_DIN 9
#define PIN_DC 10
#define PIN_CS 12
#define PIN_RST 11
#define PIN_BUZZER 2
#define PIN_IR 13

#define buttonUP 24
#define buttonDOWN 82
#define buttonLEFT 8
#define buttonRIGHT 90

Adafruit_PCD8544 myLCD(PIN_SCLK, PIN_DIN, PIN_DC, PIN_CS, PIN_RST);

const int xJoy = A0;
const int yJoy = A1;

uint8_t snakeX[21], snakeY[21];
int foodX, foodY;
int xOffset = 0, yOffset = 0;
bool GameOver = false;
int score;
int snakeSize = 1;
bool win = false;

unsigned long startTime = 0;
unsigned long endTime = 0;

const int loseMelody[] = {
  NOTE_E5, NOTE_D5, NOTE_FS4, NOTE_GS4, 
  NOTE_CS5, NOTE_B4, NOTE_D4, NOTE_E4, 
  NOTE_B4, NOTE_A4, NOTE_CS4, NOTE_E4,
  NOTE_A4
};

const int  durationsLose[] = {
  8, 8, 4, 4,
  8, 8, 4, 4,
  8, 8, 4, 4,
  2
};

const int winMelody[] = {
  NOTE_B4, NOTE_B5, NOTE_FS5, NOTE_DS5,
  NOTE_B5, NOTE_FS5, NOTE_DS5, NOTE_C5,
  NOTE_C6, NOTE_G6, NOTE_E6, NOTE_C6, NOTE_G6, NOTE_E6,
  
  NOTE_B4, NOTE_B5, NOTE_FS5, NOTE_DS5, NOTE_B5,
  NOTE_FS5, NOTE_DS5, NOTE_DS5, NOTE_E5, NOTE_F5,
  NOTE_F5, NOTE_FS5, NOTE_G5, NOTE_G5, NOTE_GS5, NOTE_A5, NOTE_B5
};

const int durationsWin[] = {
  16, 16, 16, 16,
  32, 16, 8, 16,
  16, 16, 16, 32, 16, 8,
  
  16, 16, 16, 16, 32,
  16, 8, 32, 32, 32,
  32, 32, 32, 32, 32, 16, 8
};

enum Direction {
  INITIAL = 0,
  DIR_UP = 1,
  DIR_RIGHT = 2,
  DIR_DOWN = 3,
  DIR_LEFT = 4
};

Direction direction = INITIAL;

void setup() {
  snakeX[0]=40; snakeY[0]=20;
  myLCD.begin();
  myLCD.setContrast(100);
  myLCD.clearDisplay();
  drawBorder();
  drawSnake();
  randomSeed(analogRead(0));
  randomizeFood();
  giveFood();
  myLCD.display();
  IrReceiver.begin(PIN_IR);
  pinMode(PIN_BUZZER, OUTPUT);
}

void loop() {
  if (!GameOver) {
    bool eaten = false;
    direction = getDirection(direction);
    switch (direction){
      case DIR_UP:    yOffset--; break;
      case DIR_DOWN:  yOffset++; break;
      case DIR_LEFT:  xOffset--; break;
      case DIR_RIGHT: xOffset++; break;
      default: break;
    }
    if (xOffset>=3 || xOffset<=(-3)){
      for (int i=snakeSize-1; i > 0; i--){
        snakeX[i] = snakeX[i-1];
        snakeY[i] = snakeY[i-1];
      }
      snakeX[0] += (xOffset > 0) ? 3 : -3;
      xOffset = 0;
    }
    if (yOffset>=3 || yOffset<=(-3)){
      for (int i=snakeSize-1; i > 0; i--){
        snakeX[i] = snakeX[i-1];
        snakeY[i] = snakeY[i-1];
      }
      snakeY[0] += (yOffset > 0) ? 3 : -3;
      yOffset = 0;
    }

    if (snakeX[0]<=0 || snakeX[0]>=80 || snakeY[0]<=1 || snakeY[0]>=45){
      GameOver = true;
    }
    snakeX[0] = constrain(snakeX[0], 1, 79);
    snakeY[0] = constrain(snakeY[0], 2, 44);
    eaten = checkFoodEaten();
    if (eaten){
      growSnake();
      randomizeFood();
      score += 5;
      if (score == 100){
        endTime = millis();
        GameOver = true;
        win = true;
      }
    }
    if (!eaten){
      for (int i = 1; i < snakeSize-1; i++) {
        if (snakeX[i] == snakeX[0] && snakeY[i] == snakeY[0]) {
          GameOver = true;
        }
      }
    }
    myLCD.clearDisplay();
    giveFood();
    drawBorder();
    drawSnake();
    myLCD.display();
    delay(50);
  } else {
    if (win){
      displayYouWin();
    } else {
      displayGameOver();  
    }
  }
}

Direction getDirection(Direction dir) {
  if (IrReceiver.decode()) {
    IrReceiver.resume();
    int command = IrReceiver.decodedIRData.command;
    if (dir == INITIAL) {
      if (command == buttonLEFT || command == buttonRIGHT || command == buttonUP || command == buttonDOWN) {
        startTime = millis();
      }
    }

    if (command == buttonLEFT && (snakeSize == 1 || dir != DIR_RIGHT)) return DIR_LEFT;
    if (command == buttonRIGHT && (snakeSize == 1 || dir != DIR_LEFT)) return DIR_RIGHT;
    if (command == buttonUP && (snakeSize == 1 || dir != DIR_DOWN)) return DIR_UP;
    if (command == buttonDOWN && (snakeSize == 1 || dir != DIR_UP)) return DIR_DOWN;
  }

  int x = analogRead(xJoy);
  int y = analogRead(yJoy);
  if (dir == INITIAL) {
    if (x < 200 || x > 850 || y < 200 || y > 850) {
      if (startTime == 0) { 
        startTime = millis();
      }
    }
  }

  if ((x < 200) && (snakeSize == 1 || dir != DIR_RIGHT)) return DIR_LEFT;
  if ((x > 850) && (snakeSize == 1 || dir != DIR_LEFT)) return DIR_RIGHT;
  if ((y < 200) && (snakeSize == 1 || dir != DIR_DOWN)) return DIR_UP;
  if ((y > 850) && (snakeSize == 1 || dir != DIR_UP)) return DIR_DOWN;
  return direction;
}

void drawBorder() {
  myLCD.drawLine(0, 1, 82, 1, BLACK);
  myLCD.drawLine(0, 47, 82, 47, BLACK);
  myLCD.drawLine(0, 1, 0, 47, BLACK);
  myLCD.drawLine(82, 1, 82, 47, BLACK);
  myLCD.setCursor(2, 3);
  myLCD.print(score);
  if (startTime == 0){
    myLCD.setCursor(3, 30);
    myLCD.print("MOVE TO START");
  }
}

void drawSnake() {
  for (int i = 0; i < snakeSize; i++){
    myLCD.fillRect(snakeX[i], snakeY[i], 3, 3, BLACK);
  }
}

void growSnake() {
  snakeX[snakeSize] = snakeX[snakeSize - 1];
  snakeY[snakeSize] = snakeY[snakeSize - 1];
  snakeSize++;
}

void randomizeFood() {
  foodX = 3*random(3,27)+1;
  foodY = 3*random(2,15)+2;
}

void giveFood() {
  myLCD.drawPixel(foodX+1, foodY, BLACK);
  myLCD.drawPixel(foodX, foodY+1, BLACK);
  myLCD.drawPixel(foodX+2, foodY+1, BLACK);
  myLCD.drawPixel(foodX+1, foodY+2, BLACK);
}

bool checkFoodEaten() {
  return (snakeX[0] == foodX - 1 || snakeX[0] == foodX || snakeX[0] == foodX + 1) &&
         (snakeY[0] == foodY - 1 || snakeY[0] == foodY || snakeY[0] == foodY + 1);
}

void displayGameOver() {
  myLCD.clearDisplay();
  myLCD.setTextSize(1);
  myLCD.setTextColor(BLACK);

  myLCD.setCursor(16, 10);
  myLCD.print("GAME OVER");
  myLCD.setCursor(16, 24);
  myLCD.print("Score:");
  myLCD.setCursor(52, 24);
  myLCD.print(score);

  myLCD.display();
  playLoseMelody();
}

void displayYouWin() {
  myLCD.clearDisplay();
  myLCD.setTextSize(1);
  myLCD.setTextColor(BLACK);

  myLCD.setCursor(16, 10);
  myLCD.print("YOU WIN!");
  myLCD.setCursor(16, 24);
  myLCD.print("Time:");
  myLCD.setCursor(46, 24);
  myLCD.print((endTime-startTime)/1000);

  myLCD.display();
  playWinMelody();
}

void playLoseMelody() {
  int size = sizeof(durationsLose) / sizeof(int);
  for (int note = 0; note < size; note++) {
    int duration = 1000 / durationsLose[note];
    tone(PIN_BUZZER, loseMelody[note], duration);
    int pauseBetweenNotes = duration * 1.30;
    delay(pauseBetweenNotes);
    noTone(PIN_BUZZER);
  }
}

void playWinMelody() {
  int size = sizeof(durationsWin) / sizeof(int);
  for (int note = 0; note < size; note++) {
    int duration = 1000 / durationsWin[note];
    tone(PIN_BUZZER, loseMelody[note], duration);
    int pauseBetweenNotes = duration * 1.30;
    delay(pauseBetweenNotes);
    noTone(PIN_BUZZER);
  }
}