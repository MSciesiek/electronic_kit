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

uint8_t state = 0;

volatile long absolutePositionX = 0L;//pozycja absolutna wzgledem Home
volatile long absolutePositionY = 0L;
volatile long absolutePositionZ = 0L;
volatile long absolutePositionS = 0L;
volatile long newPosition = 0L; // zmienna w petli
volatile long movement = 0L; // zmienna w petli
const int cycleTime = 250; // szybkosc wykonywania kroku
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

void setup() {
  Serial.begin(115200); // rs232 po usb do komputera

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
}

void loop() {
  //digitalWrite(LED_PIN, HIGH); // w tej obudowie nie widac
  if (Serial.available() > 0) { //sterowanie z komputera
    digitalWrite(LED_PIN, LOW);
    int wybor = Serial.parseInt();
    long kroki = 0L;

    if (wybor == 1) { //x relative
      movement = Serial.parseInt();
      newPosition = absolutePositionX + movement;
      MoveAbsX(newPosition, cycleTime);
      Position();
    } else if (wybor == 2) { //y relative
      movement = Serial.parseInt();
      newPosition = absolutePositionY + movement;
      MoveAbsY(newPosition, cycleTime);
      Position();
    } else if (wybor == 3) { //z relative
      movement = Serial.parseInt();
      newPosition = absolutePositionZ + movement;
      MoveAbsZ(newPosition, cycleTime);
      Position();
    } else if (wybor == 4) { //s relative
      movement = Serial.parseInt();
      newPosition = absolutePositionS + movement;
      MoveAbsS(newPosition, cycleTime);
      Position();
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
