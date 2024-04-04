/******************************************************
 * 
 *  .-.        Labor für Mikrocomputertechnik und
 *  | | .-.      Embedded Systems i.d. Mechatronik
 *  | | '-'
 *  | |        Fachhochschule Südwestfalen
 *  '-'        Prof. Dr.-Ing. Tobias Ellermeyer
 *  .-.     
  *  '-'        *** MGI Labortag WS2018/19 ***
 *  
 *******************************************************
 *
 * hardware.ino - Steuerung der LED und der Servos
 * 
 */

// Für die LED
//#include <SoftwareSerial.h>
#include <Adafruit_Pixie.h>
// Servos
#include <Servo.h>

#include "autoplay.h"
Servo servox, servoy;           // Servo-Objekte
uint8_t  servo_status;          // Status der Servos
uint32_t millis_servo;          // Counter, um Zeitpunkt des Servo-Einschaltens zu merken

int red,green,blue;             // Merker für Farbwerte der LED; Wertebereich 0...255
uint32_t millis_pixie;          // Zeitpunkt der nächsten LED Keepalive-Aktualisierung

// Serielle Verbindung zur LED
//SoftwareSerial pixieSerial(-1, PIXIEPIN);

//Serial1 pixieSerial;

// LED-Objekt
Adafruit_Pixie strip = Adafruit_Pixie(NUMPIXELS, &Serial1);

uint32_t ColorWheel(byte); // Interner Prototyp Farbrad

/***********************************************
 * Servos initialisieren
 */
void ServosInit()
{
  // Servo-Pins einschalten
  ServosAttach();
  
  // Servos auf Mittelstellung
  servox.write(90);
  servoy.write(90);
  
  // 0.5 Sekunden warten
  delay(500);

  // Servos wieder abschalten
  ServosDetach();
  motion_effect=0;
}

/***********************************************
 * Servos an X/Y-Position fahren
 * 
 * Wertebereich X bzw Y 0...180
 */
void ServosXY(int x, int y)
{
  // Müssen die Servo-Pins wieder aktiviert werden?
  if (servo_status == SERVO_DETACHED)
  {
    servox.attach(SERVOXPIN);
    servoy.attach(SERVOYPIN);
    servo_status = SERVO_ATTACHED;
  }


  // Wertebereich prüfen
  if ( (x<10) || (x>170) )
  {
    Serial.println("FEHLER: Servo X Wertebereich verlassen");
    x = -1;
  }
  if ( (y<10) || (y>170) )
  {
    Serial.println("FEHLER: Servo Y Wertebereich verlassen");
    y=-1;
  }

  // Servos auf Position fahren
  if (x>0) servox.write(x);
  if (y>0) servoy.write(y);
  auto_posx = x;
  auto_posy = y;
  // Zeitpunkt der Positionierung für Detach merken
  millis_servo = millis()+SERVO_ATTACHTIME;
}

/********************************************************
 * 
 * Das folgende Prozedere mit dem An-/Abschalten der
 * Servo-PWM ist nötig, da die PWM des ESP8266 einen
 * starken Zeitjitter aufweist und die Servos ansonsten
 * in Ruheposition ständig zucken...
 * 
 */

/**********************************************
 * 
 * Servo-Pin aktivieren
 * 
 */
void ServosAttach()
{
   servox.attach(SERVOXPIN);
   servoy.attach(SERVOYPIN);
   servo_status = SERVO_ATTACHED;
   millis_servo = millis()+SERVO_ATTACHTIME; // Zeitpunkt, wann die Servos wieder deaktiviert werden sollten/können
}

/**********************************************
 * 
 * Servo-Pin deaktivieren
 * 
 */
void ServosDetach()
{
  #ifdef SERVO_DONTDETACH
   servox.detach();
   servoy.detach();
   servo_status= SERVO_DETACHED;
  #endif
}


/***********************************************
 * Zeit prüfen und Servo ggf. deaktivieren
 * 
 */
void ServosCheckDetach(uint32_t acttime)
{
  #ifdef SERVO_DONTDETACH
    if ( (acttime > millis_servo) && (servo_status == SERVO_ATTACHED) )
    {
      ServosDetach();
    }
  #endif
}

/***********************************************
 * Servo-Effekte Lissajous und Kreuz
 * 
 */
void ServosLissajous()
{
  static int t=0;
  float x,y;
  
  x = 90.+45.*cos(t*DEG2RAD);
  y = 90.+45.*sin(2.*t*DEG2RAD);
  t+=4;
  ServosXY(x,y);
}

void ServosCross()
{
  static int state=0;
  int xpos[]={ 90,135, 90, 45, 90,135, 90, 45};
  int ypos[]={ 90, 45, 90,135, 90,135, 90, 45};
  state = (state+1)%8;
  ServosXY(xpos[state],ypos[state]);
}

/**************************************************************************************************************************************
 *  LED LED LED  LED LED LED  LED LED LED  LED LED LED  LED LED LED  LED LED LED
 */

/*********************************************
 * Pixie LED initialisieren
 */
void PixieInit()
{
  Serial1.begin(115200); // Pixie LED benötigt genau diese Baudrate
  strip.setBrightness(PIXIE_BRIGHTNESS_INIT);  // Helligkeit etwas zurück drehen, um Blendung zu vermeiden und Stromverbrauch einzugrenzen
  effect=0;
}

/*********************************************
 * Pixie LED Helligkeit
 */
void PixieBrightness(uint8_t bright)
{
  strip.setBrightness(bright);  // Helligkeit etwas zurück drehen, um Blendung zu vermeiden und Stromverbrauch einzugrenzen 
  strip.show();
  millis_pixie = millis()+PIXIE_KEEPALIVE;
}
/*********************************************
 * Farbe setzen und merken
 */
void PixieColor(int r, int g, int b)
{
  strip.setPixelColor(0,strip.Color(r,g,b));
  strip.show();  // Daten senden
  // Letzte Aktualisierung merken
  millis_pixie = millis()+PIXIE_KEEPALIVE;
  red = r;
  green = g;
  blue = b;
  auto_red = r;
  auto_green = g;
  auto_blue = b;
}

/***********************************************
 * Einmal durch alle Farben laufen
 */
void PixieRainbowCycle(uint8_t wait) 
{
  uint16_t j;

  for(j=0; j<256; j++) 
  { 
    strip.setPixelColor(0, ColorWheel(((256 / strip.numPixels()) + j) & 255));
    strip.show(); // Daten senden
    delay(wait);
  }
  // Letzte Aktualisierung merken
  millis_pixie = millis()+PIXIE_KEEPALIVE;
}

/*****************************************************
 * Die LED schaltet sich ab, wenn nicht 
 * mindestens 1x pro Sekunde Daten empfangen werden
 */
void PixieKeepAlive(uint32_t now)
{ 
    if ((now > millis_pixie) || (now==0))
    {
      strip.show();  // Daten senden
      // Letzte Aktualisierung merken
      millis_pixie = millis()+PIXIE_KEEPALIVE;
    }

}

/*************************************************************
 * Regenbogeneffekt, bei jede, Aufruf <inc> weioter schalten
 */
void PixieRainbowFade(int inc) {
  static uint8_t j=0;

  strip.setPixelColor(0, ColorWheel(((256 / strip.numPixels()) + j) & 255));
  j+=inc;
  strip.show();
  millis_pixie = millis()+PIXIE_KEEPALIVE;
}

/*************************************************************
 * Disco-Blinker
 */
void PixieDisco()
{
  static uint8_t s=0;
  uint32_t colors[]={
    strip.Color(0,0,0),
    strip.Color(255,0,0),
    strip.Color(0,0,0),
    strip.Color(255,0,0),

    strip.Color(0,0,0),
    strip.Color(0,0,0),
    strip.Color(0,0,0),
    strip.Color(0,0,0),
    
    strip.Color(0,0,0),
    strip.Color(0,255,0),
    strip.Color(0,0,0),
    strip.Color(0,255,0),
    
    strip.Color(0,0,0),
    strip.Color(0,0,0),
    strip.Color(0,0,0),
    strip.Color(0,0,0),

    strip.Color(0,0,0),
    strip.Color(255,0,255),
    strip.Color(0,0,0),
    strip.Color(255,0,255),
    strip.Color(0,0,0),
    strip.Color(0,0,0),
    strip.Color(0,0,0),
    strip.Color(0,0,0)
    };
  strip.setPixelColor(0,colors[s]);
  strip.show();  // Daten senden
  // Letzte Aktualisierung merken
  millis_pixie = millis()+PIXIE_KEEPALIVE;  
  // Weiter in Farbtabelle

  if ( (++s) > sizeof(colors)/sizeof(uint32_t) )
  {
    s=0;
  }
  
}

/*****************************************
 * Die letzte über PixieColor gesetzte
 * Farbe wiederherstellen
 */
void PixieRestore()
{
  strip.setPixelColor(0,strip.Color(red,green,blue));
  strip.show();  // Daten senden
  // Letzte Aktualisierung merken
  millis_pixie = millis()+PIXIE_KEEPALIVE;  
}

/*****************************************
 * Regenbogen-Effekt
 * 
 * Wert 0...255 gibt Farbe aus Farbrad an
 */
uint32_t ColorWheel(byte WheelPos) 
{
  if(WheelPos < 85) 
  {
    return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } 
  else if(WheelPos < 170) 
  {
    WheelPos -= 85;
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } 
  else 
  {
    WheelPos -= 170;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}
