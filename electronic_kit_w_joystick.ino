/* 05.10.2016
   przesuw xyz + wyswietlacz + sterowanie padem ps3 po usb
   przesuwanie jako predkosc + obsluga wyjatkow
   UWAGA - zmiana pinow EN i RS od wyswietlacza!
   sterowanie polozeniem soczewki
   poprawione kierunki przesuwania na zgodne z pilotem
   zdebugowane nagle zawiszanie, robienie tysiecy krokow i ich gubienie
   ----
   nowo dodane:
   zmiana println na print i napisanie wprost \n.
   zmiana home z '0' na '9', bo byly problemy z andoro basic'iem
   
   Maciej Sciesiek
*/

#include <LiquidCrystal.h> // do obslugi wyswietlacza lcd
#include <PS3USB.h>

// Satisfy the IDE, which needs to see the include statment in the ino too.
#ifdef dobogusinclude
#include <spi4teensy3.h>
#include <SPI.h>
#endif

USB Usb;
PS3USB PS3(&Usb, 0x00, 0x1B, 0x10, 0x00, 0x2A, 0xEC); // This will also store the bluetooth address - this can be obtained from the dongle when running the sketch

bool printAngle;
uint8_t state = 0;

volatile long absolutePositionX = 0L;//pozycja absolutna wzgledem Home
volatile long absolutePositionY = 0L;
volatile long absolutePositionZ = 0L;
volatile long absolutePositionS = 0L;
volatile long newPosition = 0L; // zmienna w petli
volatile long movement = 0L; // zmienna w petli
const int cycleTime = 250; // szybkosc wykonywania kroku
volatile int speed_BT = 1;
//%%%%% xyz - silniki
#define X_STEP_PIN 54
#define X_DIR_PIN 55
#define X_ENABLE_PIN 38
#define X_MIN_PIN 3
#define X_MAX_PIN 2

#define Y_STEP_PIN 60
#define Y_DIR_PIN 61
#define Y_ENABLE_PIN 56
#define Y_MIN_PIN 14
#define Y_MAX_PIN 15

#define Z_STEP_PIN 46
#define Z_DIR_PIN 48
#define Z_ENABLE_PIN 62
#define Z_MIN_PIN 18
#define Z_MAX_PIN 19

#define S_STEP_PIN 36
#define S_DIR_PIN 34
#define S_ENABLE_PIN 30

#define LED_PIN 13
//%%%%% wyswietlacz
#define DISP_D4 23
#define DISP_D5 25
#define DISP_D6 27
#define DISP_D7 29
#define DISP_RS 31
#define DISP_EN 33
//%%%%% modul bluetooth
// TX -> D16
// RX -> D17
// czyli standardowe "Serial2."


//%%%%% funkcje
inline void Home() { //ustawia pozycję, jako pozycję początkową
  absolutePositionX = 0L; // Y
  absolutePositionY = 0L; // X
  absolutePositionZ = 0L;
  absolutePositionS = 0L;
}

void Position() { //wyswietla pozycje absolutna
  Serial.print("x:   ");
  Serial.print(absolutePositionX);
  Serial.print("   ; y:   ");
  Serial.print(absolutePositionY);
  Serial.print("   ; z:   ");
  Serial.print(absolutePositionZ);
  Serial.print("   ; s:   ");
  Serial.println(absolutePositionS);
}

void MoveX(long stepsX, int cycleTime) {
  for (stepsX; stepsX > 0; stepsX--) {
    digitalWrite(X_STEP_PIN, HIGH);
    delayMicroseconds(2);
    digitalWrite(X_STEP_PIN, LOW);
    delayMicroseconds(2);
    delayMicroseconds(cycleTime);
  }
}

void MoveY(long stepsY, int cycleTime) {
  for (stepsY; stepsY > 0; stepsY--) {
    digitalWrite(Y_STEP_PIN, HIGH);
    delayMicroseconds(2);
    digitalWrite(Y_STEP_PIN, LOW);
    delayMicroseconds(2);
    delayMicroseconds(cycleTime);
  }
}

void MoveZ(long stepsZ, int cycleTime) {
  for (stepsZ; stepsZ > 0; stepsZ--) {
    digitalWrite(Z_STEP_PIN, HIGH);
    delayMicroseconds(2);
    digitalWrite(Z_STEP_PIN, LOW);
    delayMicroseconds(2);
    delayMicroseconds(cycleTime);
  }
}

void MoveS(long stepsS, int cycleTime) {
  for (stepsS; stepsS > 0; stepsS--) {
    digitalWrite(S_STEP_PIN, HIGH);
    delayMicroseconds(2);
    digitalWrite(S_STEP_PIN, LOW);
    delayMicroseconds(2);
    delayMicroseconds(cycleTime);
  }
}

void MoveAbsX(long newPositionX, int cycleTime) {
  long stepsX = newPositionX - absolutePositionX;

  if (stepsX < 0) {
    digitalWrite(X_DIR_PIN, HIGH);
  } else {
    digitalWrite(X_DIR_PIN, LOW);
  }
  MoveX(abs(stepsX), cycleTime);
  absolutePositionX = newPositionX;
}

void MoveAbsY(long newPositionY, int cycleTime) {
  long stepsY = newPositionY - absolutePositionY;

  if (stepsY < 0) {
    digitalWrite(Y_DIR_PIN, HIGH);
  } else {
    digitalWrite(Y_DIR_PIN, LOW);
  }
  MoveY(abs(stepsY), cycleTime);
  absolutePositionY = newPositionY;
}

void MoveAbsZ(long newPositionZ, int cycleTime) {
  long stepsZ = newPositionZ - absolutePositionZ;

  if (stepsZ < 0) {
    digitalWrite(Z_DIR_PIN, LOW);
  } else {
    digitalWrite(Z_DIR_PIN, HIGH);
  }
  MoveZ(abs(stepsZ), cycleTime);
  absolutePositionZ = newPositionZ;
}

void MoveAbsS(long newPositionS, int cycleTime) {
  long stepsS = newPositionS - absolutePositionS;

  if (stepsS < 0) {
    digitalWrite(S_DIR_PIN, HIGH);
  } else {
    digitalWrite(S_DIR_PIN, LOW);
  }
  MoveS(abs(stepsS), cycleTime);
  absolutePositionS = newPositionS;
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
ISR(TIMER3_COMPA_vect) { //X, dotad wysoki
  digitalWrite(X_STEP_PIN, LOW);
};
ISR(TIMER3_COMPB_vect) { //X, dotad niski, zerowanie
  TCNT3 = 0; // zerowanie licznika
  digitalWrite(X_STEP_PIN, HIGH);
  if (digitalRead(X_DIR_PIN))
    absolutePositionX--;
  else
    absolutePositionX++;
};

ISR(TIMER4_COMPA_vect) { //Y, dotad wysoki
  digitalWrite(Y_STEP_PIN, LOW);
};
ISR(TIMER4_COMPB_vect) { //Y, dotad niski, zerowanie
  TCNT4 = 0; // zerowanie licznika
  digitalWrite(Y_STEP_PIN, HIGH);
  if (digitalRead(Y_DIR_PIN))
    absolutePositionY--;
  else
    absolutePositionY++;
};

ISR(TIMER5_COMPA_vect) { //Z, dotad wysoki
  digitalWrite(Z_STEP_PIN, LOW);
};
ISR(TIMER5_COMPB_vect) { //Z, dotad niski, zerowanie
  TCNT5 = 0; // zerowanie licznika
  digitalWrite(Z_STEP_PIN, HIGH);
  if (digitalRead(Z_DIR_PIN))
    absolutePositionZ++;
  else
    absolutePositionZ--;
};

ISR(TIMER1_COMPA_vect) { //S, dotad wysoki
  digitalWrite(S_STEP_PIN, LOW);
};
ISR(TIMER1_COMPB_vect) { //S, dotad niski, zerowanie
  TCNT1 = 0; // zerowanie licznika
  digitalWrite(S_STEP_PIN, HIGH);
  if (digitalRead(S_DIR_PIN))
    absolutePositionS--;
  else
    absolutePositionS++;
};
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
LiquidCrystal lcd(DISP_RS, DISP_EN, DISP_D4, DISP_D5, DISP_D6, DISP_D7);

void setup() {
  Serial.begin(115200); // rs232 po usb do komputera
  if (Usb.Init() == -1) {
    Serial.print(F("\r\nOSC did not start"));
    //while (1); //halt
  }
//  Serial.println(F("\r\nPS3 USB Library Started"));

  lcd.begin(16, 2);
  lcd.print("przesuw xyz v03");
  // Serial2.begin(9600); // bluetooth, teraz nie wykorzystywany

  pinMode(LED_PIN , OUTPUT);

  pinMode(X_STEP_PIN , OUTPUT);
  pinMode(X_DIR_PIN , OUTPUT);
  pinMode(X_ENABLE_PIN , OUTPUT);

  pinMode(Y_STEP_PIN , OUTPUT);
  pinMode(Y_DIR_PIN , OUTPUT);
  pinMode(Y_ENABLE_PIN , OUTPUT);

  pinMode(Z_STEP_PIN , OUTPUT);
  pinMode(Z_DIR_PIN , OUTPUT);
  pinMode(Z_ENABLE_PIN , OUTPUT);

  pinMode(S_STEP_PIN , OUTPUT);
  pinMode(S_DIR_PIN , OUTPUT);
  pinMode(S_ENABLE_PIN , OUTPUT);

  digitalWrite(X_ENABLE_PIN , LOW);
  digitalWrite(Y_ENABLE_PIN , LOW);
  digitalWrite(Z_ENABLE_PIN , LOW);
  digitalWrite(S_ENABLE_PIN , LOW);
  
  delay(100);
  Home();
  // setup timerow 3,4,5 (x,y,z)
  TCCR3B = 0x00; // disable kiedy ustawiamy
  TCCR4B = 0x00;
  TCCR5B = 0x00;
  TCCR1B = 0x00;
  TCNT3 = 0;  //wartosci na zero
  TCNT4 = 0;
  TCNT5 = 0;
  TCNT1 = 0;
  TCCR3A = 0x00; // normal operation
  TCCR4A = 0x00;
  TCCR5A = 0x00;
  TCCR1A = 0x00;

  OCR3A = 32;  // dlugosc impulsu high w step x
  OCR4A = 32;  // y
  OCR5A = 32;  // z
  OCR1A = 32;  // s
  OCR3B = 25000; // poczatkowa wartosc. potem zmieniana w programie
  OCR4B = 25000;
  OCR5B = 25000;
  OCR1B = 25000;
  TIMSK3 = 0x06; // compare A,B interrupt enable
  TIMSK4 = 0x06;
  TIMSK5 = 0x06;
  TIMSK1 = 0x06;
  lcd.clear();
}

void loop() {
  Usb.Task();
  //digitalWrite(LED_PIN, HIGH); // w tej obudowie nie widac
  if (PS3.PS3Connected) { // sterowanie z pada
    int X = PS3.getAnalogHat(LeftHatX);
    if (X > 128) { //os 'x' do gory
      digitalWrite(X_DIR_PIN, LOW);
      noInterrupts();
      OCR3B = 2500 + 500 * (255 - X); // wartosc TOP -> okres jednego kroku
      TCCR3B = 0x01; // start, no prescaler
      interrupts();
    } else if (X < 126) { //os 'x' w dol
      digitalWrite(X_DIR_PIN, HIGH);
      noInterrupts();
      OCR3B = 2500 + 500 * X; // wartosc TOP -> okres jednego kroku
      TCCR3B = 0x01; // start, no prescaler
      interrupts();
    } else { // nie ruszamy sie
      noInterrupts();
      TCCR3B = 0x00; // no clock source, timer stopped
      TCNT3 = 0; // zerowanie
      interrupts();
    }
    int Y = PS3.getAnalogHat(LeftHatY);
    if (Y > 128) { //os 'y' do gory
      digitalWrite(Y_DIR_PIN, HIGH);
      noInterrupts();
      OCR4B = 2500 + 500 * (255 - Y); // wartosc TOP -> okres jednego kroku
      TCCR4B = 0x01; // start, no prescaler
      interrupts();
    } else if (Y < 126) { //os 'y' w dol
      digitalWrite(Y_DIR_PIN, LOW);
      noInterrupts();
      OCR4B = 2500 + 500 * Y; // wartosc TOP -> okres jednego kroku
      TCCR4B = 0x01; // start, no prescaler
      interrupts();
    } else { // nie ruszamy sie
      noInterrupts();
      TCCR4B = 0x00; // no clock source, timer stopped
      TCNT4 = 0; // zerowanie
      interrupts();
    }
    int Z = PS3.getAnalogHat(RightHatY);
    if (Z > 128) { //os 'z' do gory
      digitalWrite(Z_DIR_PIN, LOW);
      noInterrupts();
      OCR5B = 2500 + 500 * (255 - Z); // wartosc TOP -> okres jednego kroku
      TCCR5B = 0x01; // start, no prescaler
      interrupts();
    } else if (Z < 126) { //os 'z' w dol
      digitalWrite(Z_DIR_PIN, HIGH);
      noInterrupts();
      OCR5B = 2500 + 500 * Z; // wartosc TOP -> okres jednego kroku
      TCCR5B = 0x01; // start, no prescaler
      interrupts();
    } else { // nie ruszamy sie
      noInterrupts();
      TCCR5B = 0x00; // no clock source, timer stopped
      TCNT5 = 0; // zerowanie
      interrupts();
    }

    int S = PS3.getAnalogHat(RightHatX);
    if (S > 128) { //os 's' w prawo
      digitalWrite(S_DIR_PIN, LOW);
      noInterrupts();
      OCR1B = 2500 + 500 * (255 - S); // wartosc TOP -> okres jednego kroku
      TCCR1B = 0x01; // start, no prescaler
      interrupts();
    } else if (S < 126) { //os 's' w lewo
      digitalWrite(S_DIR_PIN, HIGH);
      noInterrupts();
      OCR1B = 2500 + 500 * S; // wartosc TOP -> okres jednego kroku
      TCCR1B = 0x01; // start, no prescaler
      interrupts();
    } else { // nie ruszamy sie
      noInterrupts();
      TCCR1B = 0x00; // no clock source, timer stopped
      TCNT1 = 0; // zerowanie
      interrupts();
    }

    int wielkosc_kroku = 1;
    if (PS3.getAnalogButton(R1)) {
      wielkosc_kroku = 10;
      if (PS3.getAnalogButton(L1)) {
        wielkosc_kroku = 1000;
      }
    } else if (PS3.getAnalogButton(L1)) {
      wielkosc_kroku = 100;
    }

    if (PS3.getButtonClick(TRIANGLE)) {
      newPosition = absolutePositionZ + wielkosc_kroku;
      MoveAbsZ(newPosition, 250 );
    }
    if (PS3.getButtonClick(CROSS)) {
      newPosition = absolutePositionZ - wielkosc_kroku;
      MoveAbsZ(newPosition, 250 );
    }
    if (PS3.getButtonClick(CIRCLE)) {
      newPosition = absolutePositionS + wielkosc_kroku;
      MoveAbsS(newPosition, 250 );
    }
    if (PS3.getButtonClick(SQUARE)) {
      newPosition = absolutePositionS - wielkosc_kroku;
      MoveAbsS(newPosition, 250 );
    }

    if (PS3.getButtonClick(UP)) {
      newPosition = absolutePositionY + wielkosc_kroku;
      MoveAbsY(newPosition, 250 );
    }
    if (PS3.getButtonClick(RIGHT)) {
      newPosition = absolutePositionX + wielkosc_kroku;
      MoveAbsX(newPosition, 250 );
    }
    if (PS3.getButtonClick(DOWN)) {
      newPosition = absolutePositionY - wielkosc_kroku;
      MoveAbsY(newPosition, 250 );
    }
    if (PS3.getButtonClick(LEFT)) {
      newPosition = absolutePositionX - wielkosc_kroku;
      MoveAbsX(newPosition, 250 );
    }

    if ((PS3.getButtonClick(START)) and (PS3.getButtonClick(SELECT))){
      Home();
    }
    lcd.setCursor(1, 1);
    long val = abs(absolutePositionX);
    if (val < 100000) lcd.print(" ");
    if (val < 10000) lcd.print(" ");
    if (val <  1000) lcd.print(" ");
    if (val <   100) lcd.print(" ");
    if (val <    10) lcd.print(" ");
    if (absolutePositionX >= 0) lcd.print(" ");
    lcd.print(absolutePositionX);
    lcd.print(" ");

    lcd.setCursor(9, 1);
    val = abs(absolutePositionY);
    if (val < 100000) lcd.print(" ");
    if (val < 10000) lcd.print(" ");
    if (val <  1000) lcd.print(" ");
    if (val <   100) lcd.print(" ");
    if (val <    10) lcd.print(" ");
    if (absolutePositionY >= 0) lcd.print(" ");
    lcd.print(absolutePositionY);
    lcd.print(" ");

    lcd.setCursor(9, 0);
    val = abs(absolutePositionZ);
    if (val < 100000) lcd.print(" ");
    if (val < 10000) lcd.print(" ");
    if (val <  1000) lcd.print(" ");
    if (val <   100) lcd.print(" ");
    if (val <    10) lcd.print(" ");
    if (absolutePositionZ >= 0) lcd.print(" ");
    lcd.print(absolutePositionZ);
    lcd.print(" ");

    lcd.setCursor(1, 0);
    val = abs(absolutePositionS);
    if (val < 100000) lcd.print(" ");
    if (val < 10000) lcd.print(" ");
    if (val <  1000) lcd.print(" ");
    if (val <   100) lcd.print(" ");
    if (val <    10) lcd.print(" ");
    if (absolutePositionS >= 0) lcd.print(" ");
    lcd.print(absolutePositionS);
    lcd.print(" ");

  } else if (Serial.available() > 0) { //sterowanie z komputera
    digitalWrite(LED_PIN, LOW);
    int wybor = Serial.parseInt();
    long kroki = 0L;

    if (wybor == 1) { //x relative
      movement = Serial.parseInt();
      newPosition = absolutePositionX + movement;
      MoveAbsX(newPosition, cycleTime);
      Position();
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("x steps:");
      lcd.setCursor(9, 0);
      lcd.print(absolutePositionX);
      lcd.setCursor(0, 1);
      lcd.print("x (um):");
      lcd.setCursor(8, 1);
      lcd.print(round(0.625 * absolutePositionX));
    } else if (wybor == 2) { //y relative
      movement = Serial.parseInt();
      newPosition = absolutePositionY + movement;
      MoveAbsY(newPosition, cycleTime);
      Position();
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("y steps:");
      lcd.setCursor(9, 0);
      lcd.print(absolutePositionY);
      lcd.setCursor(0, 1);
      lcd.print("y (um):");
      lcd.setCursor(8, 1);
      lcd.print(round(0.625 * absolutePositionY));
    } else if (wybor == 3) { //z relative
      movement = Serial.parseInt();
      newPosition = absolutePositionZ + movement;
      MoveAbsZ(newPosition, cycleTime);
      Position();
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("z steps:");
      lcd.setCursor(9, 0);
      lcd.print(absolutePositionZ);
      lcd.setCursor(0, 1);
      lcd.print("z (nm):");
      lcd.setCursor(8, 1);
      lcd.print(round(31.25 * absolutePositionZ));
    } else if (wybor == 4) { //s relative
      movement = Serial.parseInt();
      newPosition = absolutePositionS + movement;
      MoveAbsS(newPosition, cycleTime);
      Position();
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("s steps:");
      lcd.setCursor(9, 0);
      lcd.print(absolutePositionS);
      lcd.setCursor(0, 1);
      lcd.print("s (nm):");
      lcd.setCursor(8, 1);
      lcd.print(round(31.25 * absolutePositionS));
    } else if (wybor == 5) { //x absolute
      newPosition = Serial.parseInt();
      MoveAbsX(newPosition, cycleTime);
      Position();
    } else if (wybor == 6) { //idz do pozycji absolutnej na Y
      newPosition = Serial.parseInt();
      MoveAbsY(newPosition, cycleTime);
      Position();
    } else if (wybor == 7) { //idz do pozycji absolutnej na Z
      newPosition = Serial.parseInt();
      MoveAbsZ(newPosition, cycleTime);
      Position();
    } else if (wybor == 8) { //idz do pozycji absolutnej na S
      newPosition = Serial.parseInt();
      MoveAbsS(newPosition, cycleTime);
      Position();
    } else if (wybor == 9) {
      Home();
      Serial.print("new home position. \n");
      Position();
    } else if (wybor == 99){
      Serial.println("Polaczony z przesuwem XYZ");
    }
  }
}
