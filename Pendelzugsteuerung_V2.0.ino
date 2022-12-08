// LGB Pendelzugsteuerung, 26.11.2022
// 18_Arduino_Projects_eBook_Rui_Santos
// https://www.etechnophiles.com/change-frequency-pwm-pins-arduino-uno/
// Arduino UNO PWM frequency for D5 & D6: 976.56 Hz (The DEFAULT)
// - Motor läuft bei ca. 60 an (~ 3V)
// PWM 31,3 kHz: Motor läuft erst bei ca. 160 an (~ 7V)

// 05.12.2022   V2.1 Zugstatus-Methoden verfeinert
// 07.12.2022   V2.2  Timeout-Methoden ok
// 08.12.2022   V2.3  Datentyp millis() long in ulong

const char* PROGVER = "=== Pendelzugsteuerung V2.3 ===";

// Arduino UNO
const int enb = 10;  // PWM-Pin an Motormodul, pin 6 nutzt Timer 0, 10 nutzt Timer1
const int in1 = 7;   // Output-Pin zu input 1 am Motormoduls
const int in2 = 8;   // Output-Pin zu input 2 am Motormodul

// 10 UND 13 NICHT GEEIGNET für Magnetschalter, evtl. kein Pullup-R
const int SwitchL = 12;  // ex 14 ; // Analog Pin 0 Haltepunkt A
const int SwitchR = 11;  // ex 15; // Analog Pin 1 Haltepunkt B
const int sensorC = 2;   // Digital Pin 2 sonstiges

// Parameter für Richtung(), wie enums...
const int links = 0;
const int rechts = 1;
const int stopp = 2;

/*
enum Richtungen
{
  richtung_links,
  richtung_rechts,
  richtung_stopp
};
*/

// Fahrkonstanten
const int halt_links_s = 10;   // Stillstandszeit links in Sekunden
const int halt_rechts_s = 10;  // Stillstandszeit rechts in Sekunden
const int TimeoutFahren_s = 20;   // Timeout beim Fahren (wenn kein Sensor erkannt wird)
const int minspeed = 140;      // enb = 9, 10: 140; enb = 5, 6 : 64
const int fullspeed = 220;
// Anfahren und bremsen
const int FahrstufenAnzahl = 10;  // Anzahl Geschwindigkeitsstufen beim Anfahren und Bremsen
const int accel_zeit_s = 10;      // Beschleunigungszeit in Sekunden
const int Bremszeit_s = 3;        // Bremszeit in Sekunden
unsigned long stepzeit_ms = 0;    // Dauer einer Geschwindigkeitsstufe
unsigned long stepstart_ms = 0;   // Anfangszeit einer Geschwindigkeitsstufe

int zugstatus = 0;
int zugstatus_alt = -1;
int sensorstatus = 0;             // aktueller Sensorstatus
int sensorstatus_alt = -1;        // alter Sensorstatus
int richtung = links;             // aktuelle Fahrtrichtung
int speed;                        // interner Wert für PWM analog out
int fahrstufe;                    // aktuelle Fahrstufe
unsigned long now_ms = 0;         // aktuelle Zeit in Millisekunden
unsigned long wartezeitstart_ms;  // Beginn der Wartezeit in Millisekunden
unsigned long wartezeit_ms;       // Wartezeit in Millisekunden
unsigned long watchstart_ms;      // Beginn der Timeout-Zeit in Millisekunden
unsigned long timeout_ms;         // Timeout-Zeit in Sekunden


// Einstellen der Geschwindigkeit entsprechend der Fahrstufe
void Schnell() {
  if (fahrstufe < 0) fahrstufe = 0;
  if (fahrstufe > FahrstufenAnzahl) fahrstufe = FahrstufenAnzahl;

  speed = GetSpeed(fahrstufe);
  analogWrite(enb, speed);

  Serial.print("Fahrstufe: ");
  Serial.print(fahrstufe);
  Serial.print(", Speed = ");
  Serial.println(speed);
}

void Richtung(int r) {
  //Serial.print("Richtung: "); Serial.println(r);
  richtung = r;
  if (r == links) {
    digitalWrite(in1, LOW);   // set pin 2 on L293D low
    digitalWrite(in2, HIGH);  // set pin 7 on L293D high
    Serial.println("  fahre nach links");
  } else if (r == rechts) {
    digitalWrite(in1, HIGH);  // set pin 2 on L293D high
    digitalWrite(in2, LOW);   // set pin 7 on L293D low
    Serial.println("  fahre nach rechts");
  } else {
    // Motor ausschalten
    digitalWrite(in1, LOW);  // set pin 2 on L293D low
    digitalWrite(in2, LOW);  // set pin 7 on L293D low
    Serial.println("  Motor aus");
  }
}

// Umrechnung Fahrstufe -> PWM-Ausgabeweet
int GetSpeed(int fahrstufe) {
  // map(value, fromLow, fromHigh, toLow, toHigh)
  return map(fahrstufe, 0, FahrstufenAnzahl, minspeed, fullspeed);
}

// Zum Anfahren die Fahrstufendauer berechnen 
void StartBeschleunigen() {
  Serial.println("StartBeschleunigen");
  stepzeit_ms = accel_zeit_s * 1000 / FahrstufenAnzahl;  // Beschleunigungszeit in Millisekunden / speedsteps
  Serial.print(" - Beschl.-zeit / Fahrstufe [ms]: ");
  Serial.println(stepzeit_ms);
  stepstart_ms = now_ms;
}

// Beschleunigung ist zu Ende, wenn höchste Fahrstufe beginnt
bool BeschleunigenEnde() 
{
  //Serial.println("BeschleunigenEnde");
  bool full = false;
  if (now_ms > stepstart_ms + stepzeit_ms)  // Stufenzeit abgelaufen?
  {
    if (fahrstufe < FahrstufenAnzahl) {  
      fahrstufe++;
      Schnell();
      stepstart_ms = now_ms;
    }
    else {
      full = true;
    }
  }
  return full;
}

void StartBremsen() {
  Serial.println("StartBremsen");
  //TODO Bremszeit und Bremsschritte berechnen, Anfangswerte setzen, Zustand setzen
  stepzeit_ms = Bremszeit_s * 1000 / FahrstufenAnzahl;  // Beschleunigungszeit in Millisekunden je Fahrstufe
  Serial.print(" - Bremszeit / Fahrstufe [ms]: ");
  Serial.println(stepzeit_ms);
  stepstart_ms = now_ms;
}

bool BremsenEnde() {
  bool standstill = false;
  if (now_ms > stepstart_ms + stepzeit_ms) 
  {  
    // Stufenzeit abgelaufen
    if (fahrstufe > 0) {
      // solange speed verringern, bis auf startspeed
      fahrstufe--;
      Schnell();
      stepstart_ms = now_ms;
    }
    else {
      standstill = true;
    }
  }
  return standstill;
}

void StartWartezeit(int wartesekunden) {
  Serial.print("StartWartezeit ");
  Serial.print(wartesekunden);
  Serial.println(" Sekunden");
  wartezeitstart_ms = millis();
  wartezeit_ms = wartesekunden * 1000;
}

// true, wenn Wartezeit zu Ende ist
bool WartezeitEnde() {
  bool ende = (now_ms - wartezeitstart_ms > wartezeit_ms);
  if (ende) Serial.println("Wartezeit Ende");
  return ende;
}

// Watchdog einstellen und starten, Timeout in Sekunden
void ResetWatchdog(int to_s) {
  watchstart_ms = millis();
  timeout_ms = to_s * 1000;
  Serial.print("Reset watchdog, Timeout[s] = "); Serial.println(to_s);
}

// Zyklisch aufzurufende Watchdog-Funktion
void Watchdog() {
  if (watchstart_ms + timeout_ms > now_ms) {
    zugstatus = 10;
    Serial.println("Timeout!");
  }
}

bool IsTimeout()
{
  bool to = (now_ms > watchstart_ms + timeout_ms); 
  //if (to) Serial.println(" TO "); else Serial.print("w");
  return to;
}

// Code nach Programmstart
void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println(PROGVER);

  // Setzt Timer0 (Pin 5 und 6), beeinflusst delay(), millis(), micros()
  //TCCR0B = TCCR0B & B11111000 | 0x01; // Teiler 1, for PWM frequency of 62,5 kHz
  //TCCR0B = TCCR0B & B11111000 | 0x02; // Teiler 8, for PWM frequency of 7812.50 Hz
  //TCCR0B = TCCR0B & B11111000 | 0x03; // Teiler 64, for PWM frequency of 976.56 Hz (The DEFAULT)
  //TCCR0B = TCCR0B & B11111000 | 0x04; // Teiler 256, for PWM frequency of 244.14 Hz
  //TCCR0B = TCCR0B & B11111000 | 0x05; // Teiler 1024, for PWM frequency of 61.04 Hz

  TCCR1B = TCCR1B & 0b11111000 | 0x01;  // Setzt Timer1 (Pin 9 und 10) auf 31,3 kHz
  //TCCR2B = TCCR2B & 0b11111000 | 0x01; // Setzt Timer2 (Pin 3 und 11) auf 31,3 kHz

  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(enb, OUTPUT);
  pinMode(SwitchL, INPUT_PULLUP);
  pinMode(SwitchR, INPUT_PULLUP);
  pinMode(sensorC, INPUT_PULLUP);

  Serial.println("Starte Langsamfahrt nach links");
  fahrstufe = FahrstufenAnzahl / 2;
  Schnell();
  Richtung(links);
  ResetWatchdog(15);
  zugstatus = 1;
}


void loop() {
  // put your main code here, to run repeatedly:

  // Zeit des Schleifendurchlaufbeginns merken
  now_ms = millis();
  //Watchdog();

  //=============================================
  // Abfrage der Sensoren und Schalter
  //=============================================

  //Serial.println("Wait for sensor");
  while (1)  //Loop bis SwitchX aktiviert ist
  {
    if (digitalRead(SwitchL) == 0)      sensorstatus = 1;
    else if (digitalRead(SwitchR) == 0) sensorstatus = 2;
    else if (digitalRead(sensorC) == 0) sensorstatus = 3;

    if (sensorstatus != sensorstatus_alt) {
      switch (sensorstatus) {
        case 1:
          Serial.println("+++ Sensor L +++");
          if (richtung == links) {
            zugstatus = 2;
            StartBremsen();
            ResetWatchdog(5);
          }
          break;
        case 2:
          Serial.println("+++ Sensor R +++");
          if (richtung == rechts) {
            zugstatus = 6;
            StartBremsen();
            ResetWatchdog(5);
          }
          break;  //SwitchR ist gegen 0 geschaltet

        case 3:
          Serial.println("+++ switch C +++");
          zugstatus = 10;
          break;  //SensorC ist gegen 0 geschaltet
      }
      sensorstatus_alt = sensorstatus;
    }
    break;  // immer loop beenden, weiter im Programm
  }

  // Statusmaschine 
  switch (zugstatus) {
    case 1:  // Startzustand, Bewegung nach links, relativ langsam
      zugstatus = 9;
      break;
    case 2:  // FL, SWL erkannt: Bewegung nach links, Bremsen, Stop
      if (BremsenEnde()) {
        Richtung(stopp);
        StartWartezeit(halt_links_s);
        zugstatus = 3;
      }
      break;
    case 3:  // Warten links: Haltezeit abwarten
      //ResetWatchdog(1);
      if (WartezeitEnde()) {
        Richtung(rechts);
        StartBeschleunigen();
        zugstatus = 4;
      }
      break;

    case 4:  // FR: Bewegung nach rechts, Beschleunigen
      if (BeschleunigenEnde()) {
        ResetWatchdog(TimeoutFahren_s);
        zugstatus = 5;
      }
      break;

    case 5:  // FR schnell: Bewegung nach rechts, volle Geschw. bis Sensor aktiv oder Timeout
      if (IsTimeout())
      {
        StartBremsen();
        zugstatus = 6;
      }
      break;

    case 6:  // FR, SWR erkannt - Bewegung nach rechts, Bremsen, Stop
      if (BremsenEnde()) {
        Richtung(stopp);
        StartWartezeit(halt_rechts_s);
        zugstatus = 7;
      }
      break;

    case 7:  // Stop rechts - Haltezeit rechts abwarten
      if (WartezeitEnde()) {
        StartBeschleunigen();
        Richtung(links);
        zugstatus = 8;
      }
      break;

    case 8:  // FL - Bewegung nach links, Beschleunigen
      if (BeschleunigenEnde()) {
        zugstatus = 9;
        ResetWatchdog(TimeoutFahren_s);
      }
      break;

    case 9:  // FL schnell - Bewegung nach links, volle Geschw. bis SwitchR aktiv oder Timeout
      if (IsTimeout())
      {
        StartBremsen();
        zugstatus = 2;
      }
      break;

    case 10:  // Fehler - sofortiger Stop
      Serial.print("!!! FEHLERSTATUS !!!");
      ResetWatchdog(10);
      Richtung(stopp);
      break;
  }

  if (zugstatus != zugstatus_alt) {
    Serial.print("Zugstatus: ");
    Serial.println(zugstatus);
    zugstatus_alt = zugstatus;
  }
  delay(50);
}
