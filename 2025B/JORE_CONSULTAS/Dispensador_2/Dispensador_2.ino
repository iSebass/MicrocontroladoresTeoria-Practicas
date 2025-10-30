 #include <EEPROM.h>
#include  <LiquidCrystal_I2C.h>
#include  <Wire.h>
#include  <DS1307RTC.h>
#include  <TimeLib.h>

// ================== LCD ==================
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ================== Variables de UI / Dígitos grandes ==================
int x = 0, numeros;
int minutoanterior1 = -1, minutoanterior2 = -1, horaanterior1 = -1, horaanterior2 = -1;

// ================== Tiempo actual ==================
int hora;
int minuto;
int segundo;

// ================== Botones ==================
const int btndown = A0;
const int btnup   = A1;
const int btnprog = A2;

// ================== Antirrebote (debounce) ==================
const unsigned long DEBOUNCE_MS      = 60;
const unsigned long REPEAT_DELAY_MS  = 500;
const unsigned long REPEAT_RATE_MS   = 120;

enum {IDX_DOWN=0, IDX_UP=1, IDX_PROG=2};
const uint8_t BTN_PINS[3] = {btndown, btnup, btnprog};
int lastReadingBtn[3]     = {HIGH, HIGH, HIGH};
int stableStateBtn[3]     = {HIGH, HIGH, HIGH};
unsigned long lastDebounceBtn[3] = {0,0,0};
bool pressedEventBtn[3]   = {false,false,false};
unsigned long pressedAtBtn[3] = {0,0,0};
unsigned long lastRepeatAt[3]  = {0,0,0};

inline void updateButtons(){
  unsigned long now = millis();
  for (int i=0;i<3;i++){
    int r = digitalRead(BTN_PINS[i]);
    if (r != lastReadingBtn[i]){
      lastReadingBtn[i] = r;
      lastDebounceBtn[i] = now;
    }
    if ((now - lastDebounceBtn[i]) >= DEBOUNCE_MS){
      if (stableStateBtn[i] != r){
        stableStateBtn[i] = r;
        if (r == LOW){
          pressedEventBtn[i] = true;
          pressedAtBtn[i] = now;
          lastRepeatAt[i] = now;
        }
      }
    }
  }
}

inline bool buttonPressed(int idx){
  if (pressedEventBtn[idx]){ pressedEventBtn[idx] = false; return true; }
  return false;
}

inline bool buttonIsDown(int idx){
  return stableStateBtn[idx] == LOW;
}

inline bool buttonRepeat(int idx){
  unsigned long now = millis();
  if (stableStateBtn[idx] == LOW){
    if (now - pressedAtBtn[idx] >= REPEAT_DELAY_MS){
      if (now - lastRepeatAt[idx] >= REPEAT_RATE_MS){
        lastRepeatAt[idx] = now;
        return true;
      }
    }
  }
  return false;
}

// ================== Salidas ==================
const int Pinled1 = 2;
const int Pinled2 = 3;
const int Pinled3 = 4;
const int Pinled4 = 5;
const int Pinled5 = 6;
const int Pinled6 = 7;
const int Pinled7 = 8;
const int buzzer  = 9;

// ================== Estado general ==================
int cont = 1;
unsigned long antes, antes1;
bool estado = false;

// ================== RTC / cache ==================
int Hour = 0, Min = 0, Sec = 0;
int hora_actual1 = 0, minuto_actual1 = 0;
tmElements_t tm;

// ================== Dígitos personalizados ==================
byte LT[8]  = {B01111, B11111, B11111, B11111, B11111, B11111, B11111, B11111};
byte UB[8]  = {B11111, B11111, B11111, B00000, B00000, B00000, B00000, B00000};
byte RT[8]  = {B11110, B11111, B11111, B11111, B11111, B11111, B11111, B11111};
byte LL[8]  = {B11111, B11111, B11111, B11111, B11111, B11111, B11111, B01111};
byte LB[8]  = {B00000, B00000, B00000, B00000, B00000, B11111, B11111, B11111};
byte LR[8]  = {B11111, B11111, B11111, B11111, B11111, B11111, B11111, B11110};
byte UMB[8] = {B11111, B11111, B11111, B00000, B00000, B00000, B11111, B11111};
byte LMB[8] = {B11111, B11111, B00000, B00000, B00000, B11111, B11111, B11111};

// ================== Alarmas (7) ==================
int alarmaHora1 = 24, alarmaMinuto1 = 0;
int alarmaHora2 = 24, alarmaMinuto2 = 0;
int alarmaHora3 = 24, alarmaMinuto3 = 0;
int alarmaHora4 = 24, alarmaMinuto4 = 0;
int alarmaHora5 = 24, alarmaMinuto5 = 0;
int alarmaHora6 = 24, alarmaMinuto6 = 0;
int alarmaHora7 = 24, alarmaMinuto7 = 0;

int* AH[7] = {&alarmaHora1, &alarmaHora2, &alarmaHora3, &alarmaHora4, &alarmaHora5, &alarmaHora6, &alarmaHora7};
int* AM[7] = {&alarmaMinuto1, &alarmaMinuto2, &alarmaMinuto3, &alarmaMinuto4, &alarmaMinuto5, &alarmaMinuto6, &alarmaMinuto7};
const int LEDS[7] = {Pinled1, Pinled2, Pinled3, Pinled4, Pinled5, Pinled6, Pinled7};

bool alarmaSonando[7]   = {false,false,false,false,false,false,false};
bool alarmaEjecutada[7] = {false,false,false,false,false,false,false};
bool ledActivo[7]       = {false,false,false,false,false,false,false};
unsigned long alarmaInicio[7] = {0,0,0,0,0,0,0};
unsigned long ledInicio[7]    = {0,0,0,0,0,0,0};

// ======= CONFIGURACIÓN INICIAL =======
// Duración (en segundos) que el LED permanece encendido al activarse una alarma
const unsigned int DURACION_LED_S = 20;    // <-- AJUSTA AQUÍ (LED) 
const unsigned int DURACION_ALARMA_S = 10; // <-- AJUSTA AQUÍ (Buzzer/alarma) 
unsigned long duracionLed    = (unsigned long)DURACION_LED_S * 1000UL;
unsigned long duracionAlarma = (unsigned long)DURACION_ALARMA_S * 1000UL;

// ================== EEPROM ==================
struct Persist {
  int hora[7];
  int minuto[7];
};
const int EEPROM_ADDR = 0;

// ---------- Utilidades de dibujo ----------
inline void clearDigitAt(int col){
  lcd.setCursor(col,0); lcd.print("   ");
  lcd.setCursor(col,1); lcd.print("   ");
}

// Dibuja SIN limpiar (usado por drawDigitChanged)
void custom0(){ lcd.setCursor(x,0); lcd.write((byte)0); lcd.write(1); lcd.write(2);
                lcd.setCursor(x,1); lcd.write(3);        lcd.write(4); lcd.write(5); }
// ** CENTRADO del número 1 **
void custom1(){ lcd.setCursor(x+1,0); lcd.write(2);   // columna central arriba
                lcd.setCursor(x+1,1); lcd.write(5); } // columna central abajo
void custom2(){ lcd.setCursor(x,0); lcd.write(6); lcd.write(6); lcd.write(2);
                lcd.setCursor(x,1); lcd.write(3); lcd.write(7); lcd.write(7); }
void custom3(){ lcd.setCursor(x,0); lcd.write(6); lcd.write(6); lcd.write(2);
                lcd.setCursor(x,1); lcd.write(7); lcd.write(7); lcd.write(5); }
void custom4(){ lcd.setCursor(x,0); lcd.write(3); lcd.write(4); lcd.write(2);
                lcd.setCursor(x,1); lcd.print("  "); lcd.write(5); }
void custom5(){ lcd.setCursor(x,0); lcd.write((byte)0); lcd.write(6); lcd.write(6);
                lcd.setCursor(x,1); lcd.write(7);        lcd.write(7); lcd.write(5); }
void custom6(){ lcd.setCursor(x,0); lcd.write((byte)0); lcd.write(6); lcd.write(6);
                lcd.setCursor(x,1); lcd.write(3);        lcd.write(7); lcd.write(5); }
void custom7(){ lcd.setCursor(x,0); lcd.write((byte)0); lcd.write(1); lcd.write(2);
                lcd.setCursor(x,1); lcd.print("  "); lcd.write(5); }
void custom8(){ lcd.setCursor(x,0); lcd.write((byte)0); lcd.write(6); lcd.write(2);
                lcd.setCursor(x,1); lcd.write(3);        lcd.write(7); lcd.write(5); }
void custom9(){ lcd.setCursor(x,0); lcd.write((byte)0); lcd.write(6); lcd.write(2);
                lcd.setCursor(x,1); lcd.print("  ");     lcd.write(5); }

void mostranumero(){ // no limpia: se usa junto a drawDigitChanged
  switch(numeros){
    case 0: custom0(); break;
    case 1: custom1(); break;
    case 2: custom2(); break;
    case 3: custom3(); break;
    case 4: custom4(); break;
    case 5: custom5(); break;
    case 6: custom6(); break;
    case 7: custom7(); break;
    case 8: custom8(); break;
    case 9: custom9(); break;
  }
}

// Dibuja el dígito en 'col' sólo si cambia respecto a *last.
inline void drawDigitChanged(int col, int value, int* last){
  if (*last != value){
    clearDigitAt(col);
    x = col; numeros = value;
    mostranumero();
    *last = value;
  }
}

// DIBUJO FORZADO: limpia y dibuja sin chequear caches (para reentrada)
inline void drawDigitForced(int col, int value){
  clearDigitAt(col);
  x = col; numeros = value;
  mostranumero();
}

// ---------- Caches para pantallas de programación ----------
int lastProgHourT = -1, lastProgHourO = -1;
int lastProgMinT  = -1, lastProgMinO  = -1;
int lastAlarmHourT[7]; int lastAlarmHourO[7];
int lastAlarmMinT[7];  int lastAlarmMinO[7];

inline void resetProgCaches(){
  lastProgHourT = lastProgHourO = -1;
  lastProgMinT  = lastProgMinO  = -1;
  for (int i=0;i<7;i++){
    lastAlarmHourT[i] = lastAlarmHourO[i] = -1;
    lastAlarmMinT[i]  = lastAlarmMinO[i]  = -1;
  }
}

int lastSecDisplayed = -1; // cache de segundos mostrados

// ---------- Prototipos ----------
void guardarEEPROM();
void cargarEEPROM();
void Reloj();
void progHora();
void redrawClockForced();

// ================== SETUP ==================
void setup() {
  Serial.begin(9600);
  pinMode(btndown, INPUT_PULLUP);
  pinMode(btnup,   INPUT_PULLUP);
  pinMode(btnprog, INPUT_PULLUP);
  for (int i=0;i<7;i++) pinMode(LEDS[i], OUTPUT);
  pinMode(buzzer, OUTPUT);

  lcd.begin(16,2);
  lcd.backlight();
  delay(100);
  lcd.createChar(0, LT); lcd.createChar(1, UB); lcd.createChar(2, RT);
  lcd.createChar(3, LL); lcd.createChar(4, LB); lcd.createChar(5, LR);
  lcd.createChar(6, UMB); lcd.createChar(7, LMB);
  lcd.clear();
  lastSecDisplayed = -1;

  resetProgCaches();
  cargarEEPROM();
  Reloj();
}

// ================== LOOP ==================
void loop() {
  updateButtons();

  if (buttonPressed(IDX_PROG)) {
    tone(buzzer, 1000, 50);
    progHora();
  }

  if (millis() - antes >= 1000) {
    Reloj();
    antes = millis();
  }

  /*  // Puntos titilando (desactivado a favor de mostrar segundos)
  static bool puntos = false;
  if (millis() - antes1 >= 500 && !buttonIsDown(IDX_PROG)) {
    puntos = !puntos;
    lcd.setCursor(8, 0); lcd.print(puntos ? "." : " ");
    lcd.setCursor(8, 1); lcd.print(puntos ? "." : " ");
    antes1 = millis();
  }
*/for (int i = 0; i < 7; i++) {
    if ((*AH[i] >= 0 && *AH[i] <= 23) &&
        (hora_actual1 == *AH[i]) &&
        (minuto_actual1 == *AM[i]) &&
        !alarmaEjecutada[i]) {

      digitalWrite(LEDS[i], HIGH);
      tone(buzzer, 1000);
      alarmaInicio[i] = millis();
      ledInicio[i]    = millis();
      ledActivo[i]     = true;
      alarmaSonando[i] = true;
      alarmaEjecutada[i] = true;
    }

    // Apagar solo el LED cuando se cumpla su propia duración
    if (ledActivo[i] && (millis() - ledInicio[i] >= duracionLed)) {
      digitalWrite(LEDS[i], LOW);
      ledActivo[i] = false;
    }

    // Finalizar la alarma (buzzer) cuando llegue el tiempo de alarma
    if (alarmaSonando[i] && (millis() - alarmaInicio[i] >= duracionAlarma)) {
      alarmaSonando[i] = false;
      bool alguien = false;
      for (int k=0;k<7;k++) if (alarmaSonando[k]) {alguien=true; break;}
      if (!alguien) noTone(buzzer);
    }

    if (alarmaSonando[i] && (buttonIsDown(IDX_UP) || buttonIsDown(IDX_DOWN))) {
      digitalWrite(LEDS[i], LOW);
      ledActivo[i] = false;
      alarmaSonando[i] = false;
      bool alguien = false;
      for (int k=0;k<7;k++) if (alarmaSonando[k]) {alguien=true; break;}
      if (!alguien) noTone(buzzer);
    }
  }

  static int minutoAnteriorReset = -1;
  if (minuto_actual1 != minutoAnteriorReset) {
    for (int i=0;i<7;i++) alarmaEjecutada[i] = false;
    minutoAnteriorReset = minuto_actual1;
  }
}

// ================== EEPROM ==================
void guardarEEPROM() {
  Persist p;
  for (int i=0;i<7;i++) { p.hora[i] = *AH[i]; p.minuto[i] = *AM[i]; }
  EEPROM.put(EEPROM_ADDR, p);
}

void cargarEEPROM() {
  Persist p;
  EEPROM.get(EEPROM_ADDR, p);
  for (int i=0;i<7;i++) {
    int h = p.hora[i];
    int m = p.minuto[i];
    if (h < 0 || h > 24) h = 24;
    if (m < 0 || m > 59) m = 0;
    *AH[i] = h;
    *AM[i] = m;
  }
}

// ================== RELOJ (lectura + dibujo HH:MM) ==================
void Reloj(){
  tmElements_t tmm;
  if (RTC.read(tmm)){
    Hour = tmm.Hour;
    Min  = tmm.Minute;
    Sec  = tmm.Second;

    hora_actual1   = Hour;
    minuto_actual1 = Min;

    int hT = Hour / 10, hO = Hour % 10;
    int mT = Min  / 10, mO = Min  % 10;

    drawDigitChanged(0,  hT, &horaanterior1);
    drawDigitChanged(4,  hO, &horaanterior2);
    drawDigitChanged(9,  mT, &minutoanterior1);
    drawDigitChanged(13, mO, &minutoanterior2);
    // ===== Segundos en el centro (col 7-8, fila 1) =====
    if (!estado) {
      if (lastSecDisplayed != Sec) {
        lcd.setCursor(7, 1);
        if (Sec < 10) lcd.print("0");
        lcd.print(Sec);
        lastSecDisplayed = Sec;
      }
    }

  }
}

// Dibujo forzado del reloj: útil al salir de programación
void redrawClockForced(){
  // Intentamos leer del RTC para coherencia
  tmElements_t tmm;
  if (RTC.read(tmm)){
    Hour = tmm.Hour;
    Min  = tmm.Minute;
  }
  lcd.clear();
  drawDigitForced(0,  Hour / 10);
  drawDigitForced(4,  Hour % 10);
  drawDigitForced(9,  Min  / 10);
  drawDigitForced(13, Min  % 10);
  // Actualizamos caches para que Reloj() siga a partir de aquí
  horaanterior1 = Hour / 10;
  horaanterior2 = Hour % 10;
  minutoanterior1 = Min / 10;
  minutoanterior2 = Min % 10;
}

// ================== PROGRAMACIÓN (cont) ==================
void progHora(){
  estado = true;
  lcd.clear();
  resetProgCaches(); // prepara caches fresh para esta sesión

  while (estado == true){
    updateButtons();

    if (buttonPressed(IDX_PROG)){
      tone(buzzer, 1000, 50);
      cont++;
      if (cont > 16) {
        cont = 1;
        estado = false;
        // Escribimos nueva hora al RTC
        tm.Hour = Hour; tm.Minute = Min; tm.Second = Sec;
        RTC.write(tm);
        // Forzamos un repintado completo del reloj
        redrawClockForced();
        lastSecDisplayed = -1;
        break;
      }
      lcd.clear();
    }

    // 1: Hora actual
    if (cont == 1){
      lcd.setCursor(9,1); lcd.print("Horas");
      drawDigitChanged(0,  Hour / 10, &lastProgHourT);
      drawDigitChanged(4,  Hour % 10, &lastProgHourO);

      bool stepUp = buttonPressed(IDX_UP) || buttonRepeat(IDX_UP);
      bool stepDown = buttonPressed(IDX_DOWN) || buttonRepeat(IDX_DOWN);
      if (stepUp){ tone(buzzer, 1000, 10); Hour++; if (Hour >= 24) Hour = 0; lastProgHourT = -1; lastProgHourO = -1; }
      if (stepDown){ tone(buzzer, 1000, 10); Hour--; if (Hour < 0) Hour = 23; lastProgHourT = -1; lastProgHourO = -1; }
    }

    // 2: Minutos actuales
    if (cont == 2){
      lcd.setCursor(0,1); lcd.print("Minutos");
      drawDigitChanged(9,  Min / 10, &lastProgMinT);
      drawDigitChanged(13, Min % 10, &lastProgMinO);

      bool stepUp = buttonPressed(IDX_UP) || buttonRepeat(IDX_UP);
      bool stepDown = buttonPressed(IDX_DOWN) || buttonRepeat(IDX_DOWN);
      if (stepUp){ tone(buzzer, 1000, 10); Min++; if (Min >= 60) Min = 0; lastProgMinT = -1; lastProgMinO = -1; }
      if (stepDown){ tone(buzzer, 1000, 10); Min--; if (Min < 0) Min = 59; lastProgMinT = -1; lastProgMinO = -1; }
    }

    // 3..16: 7 alarmas (hora/min)
    for (int i=0;i<7;i++){
      int base = 3 + 2*i; // base: hora, base+1: minuto

      if (cont == base){
        // HORA: "Alarma n" a la DERECHA
        lcd.setCursor(8,0); lcd.print("Alarma "); lcd.print(i+1);
        lcd.setCursor(9,1); lcd.print("Horas");

        drawDigitChanged(0,  (*AH[i]) / 10, &lastAlarmHourT[i]);
        drawDigitChanged(4,  (*AH[i]) % 10, &lastAlarmHourO[i]);

        bool stepUp = buttonPressed(IDX_UP) || buttonRepeat(IDX_UP);
        bool stepDown = buttonPressed(IDX_DOWN) || buttonRepeat(IDX_DOWN);
        if (stepUp){
          tone(buzzer, 1000, 10);
          if (*AH[i] == 24) *AH[i] = 0; else (*AH[i])++;
          if (*AH[i] >= 24) *AH[i] = 0;
          guardarEEPROM();
          lastAlarmHourT[i] = -1; lastAlarmHourO[i] = -1;
        }
        if (stepDown){
          tone(buzzer, 1000, 10);
          if (*AH[i] == 24) *AH[i] = 23; else (*AH[i])--;
          if (*AH[i] < 0) *AH[i] = 23;
          guardarEEPROM();
          lastAlarmHourT[i] = -1; lastAlarmHourO[i] = -1;
        }
      }

      if (cont == base+1){
        // MINUTOS: "Alarma n" a la IZQUIERDA
        lcd.setCursor(0,0); lcd.print("Alarma "); lcd.print(i+1);
        lcd.setCursor(0,1); lcd.print("Minutos");

        drawDigitChanged(9,  (*AM[i]) / 10, &lastAlarmMinT[i]);
        drawDigitChanged(13, (*AM[i]) % 10, &lastAlarmMinO[i]);

        bool stepUp = buttonPressed(IDX_UP) || buttonRepeat(IDX_UP);
        bool stepDown = buttonPressed(IDX_DOWN) || buttonRepeat(IDX_DOWN);
        if (stepUp){ tone(buzzer, 1000, 10); (*AM[i])++; if (*AM[i] >= 60) *AM[i] = 0; guardarEEPROM(); lastAlarmMinT[i] = -1; lastAlarmMinO[i] = -1; }
        if (stepDown){ tone(buzzer, 1000, 10); (*AM[i])--; if (*AM[i] < 0) *AM[i] = 59; guardarEEPROM(); lastAlarmMinT[i] = -1; lastAlarmMinO[i] = -1; }
      }
    }

    tm.Hour = Hour; tm.Minute = Min; tm.Second = Sec;
    RTC.write(tm);
  }
  // aseguramos pantalla limpia al salir
  // (redrawClockForced ya se invoca al cerrar el ciclo)
}
