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
 * Header-Datei hardware.h
 * 
 */

#ifndef __HARDWARE__
#define __HARDWARE__


#define NUMPIXELS 1       // Anzahl angeschlossener Pixie LEDs
#define PIXIEPIN  14      // Pin der Pixie-Led D5 -> GPIO 14
#define PIXIE_KEEPALIVE 500  // Abstand der Pakete, um LED aktiv zu halten
#define PIXIE_BRIGHTNESS_INIT 50 // Max. 255; Achtung, bei hohen Werten kann LED zu warm werden und abschalten
#define PIXIE_BRIGHTNESS_RUN 200 // Max. 255; Achtung, bei hohen Werten kann LED zu warm werden und abschalten
                             // Außerdem nicht direkt in die LED schauen
#define SERVOXPIN 5       // Servo X Pin  D1 -> GPIO 5 
#define SERVOYPIN 4       // Servo Y Pin  D2 -> GPIO 4

#define SERVO_ATTACHED 1  // Servo aktiv
#define SERVO_DETACHED 0  // Servo inaktiv
#define SERVO_ATTACHTIME 60000  // Wie lange in ms sollen die Servos aktiv bleiben (60s)
#define SERVO_DONTDETACH  // Nie deaktivieren...

#define DEG2RAD   3.1415927/180.0


int effect;                               // Gewählter Effekt
int motion_effect;


//*******************************************
// Servo-Funktionen
//
void ServosInit();                         // Servos initialisieren
void ServosXY(int, int);                   // Prototyp, um Servo an Position x,y zu fahren
void ServosAttach();                       // Servo-Pins aktivieren
void ServosDetach();                       // Servo-Pins deaktivieren
void ServosCheckDetach(uint32_t);          // Prüfen, ob Servo-Pins deaktiviert werden können und ggf. deaktivieren
void ServosLissajous();                    // Lissajous-Figur
void ServosCross();                        // Crossing-Effekt

//********************************************
// LED Funktionen
//
void PixieInit();
void PixieBrightness(uint8_t);
void PixieColor(int,int,int);               // Prototyp LED Farbe setzen
void PixieKeepalive(uint32_t);
void PixieRestore();
void PixieRainbowCycle();
void PixieRainbowFade(int);
void PixieDisco();

#endif
