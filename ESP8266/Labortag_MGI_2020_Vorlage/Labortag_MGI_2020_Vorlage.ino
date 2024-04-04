/******************************************************
 * 
 *  .-.        Labor für Mikrocomputertechnik und
 *  | | .-.      Embedded Systems i.d. Mechatronik
 *  | | '-'
 *  | |        Fachhochschule Südwestfalen
 *  '-'        Prof. Dr.-Ing. Tobias Ellermeyer
 *  .-.     
  *  '-'        *** MGI Labortag WS2019/20 ***
 *  
 *******************************************************
 * (c) 10.04.2017
 *
 * Mit Code-Teilen von:
 * ESP-Webserver Dateien im SPIFFS v0.5 MSoft 18.05.2016
 *     32MBit-Flash: 4M(1M SPIFFS), NodeMCU
 *
 * 
 * 01/2020: Update Labortag MGI
 * 01/2020: LED-Ansteuerung auf Serial1 (Hardware-UART) umgestellt; 
 *          ESP8266_ISR_Servo getestet, aber Winkel ungenau
 */

/* TODO:
 [?] Auto Disconnect entfernen
 [?] Flash bei nicht index.html entfernen
 - FH Logo einbauen
 - Beschriftung LED/Servo sortieren
 [?] ServoX gültige Werte verlassen beheben
 [X] Debugging standardmäßig aus
*/

#include <stdlib.h>
#include "hardware.h"
#include "webserver.h"    // Hier stehen weitere Includes und globalen Variablen
#include "autoplay.h"

//#define SHOW_MODE 
//#define AUTOPLAY

//#define DEBUG
#define FADE_SPEED  5   // Geschwindigkeit des FADE-Effekts
#define DISCO_SPEED  50   // Geschwindigkeit des FADE-Effekts
#define LISSA_SPEED  50   // Geschwindigkeit Kreis
#define CROSS_SPEED 800  // Geschwindigkeit Kreuz 
uint32_t cycle_next,mcycle_next;
uint32_t disconnect_timer=0;

#ifdef SHOW_MODE
uint8_t show_mode=1;
#else
uint8_t show_mode=0;
#endif

// Prototypen
void rainbowCycle_Disco(uint8_t);

// ###############################################################
// --------------------- setup/loop ------------------------------
// ###############################################################

void setup()
{
  uint32_t color;
  int i,j;              // Für Morsecode IP
  int div[]={100,10,1}; // Für Morsecode IP


  // Serielle Verbindung über USB starten, um Debugging-Ausgaben zu erhalten
  Serial.begin(115200);

  // Willkommens-Nachricht ausgeben
  Serial.println("\n\n");
  Serial.println("*** RESET***");
  Serial.println("\n\n******************\n* Smarte RGB-LED *\n******************\n\nEin Projekt der FH-Suedwestfalen zum MGI Labortag WS2019/20\n(c) Prof. Dr.-Ing. Tobias Ellermeyer\n");

  // Servos initialisieren
  Serial.println("... Servos initialisieren");
  ServosInit();
  
  // Pixie-LED initialisieren
  Serial.println("... Pixie LED initialisieren");
  PixieInit();
  // Ein Lebenszeichen der LED
  Serial.println("... Color-Cycle als Lebenszeichen");
  PixieRainbowCycle(1);
  PixieColor(0,0,0);
  delay(500);
  PixieRainbowCycle(1);
  PixieColor(0,0,0);
    
  // WIFI initialisieren
  Serial.println("... WIFI/WLAN initialisieren");
  WifiConfig();
  WifiInit();

  
  // LED jetzt hell schalten
  PixieBrightness(PIXIE_BRIGHTNESS_RUN);
  // Farbe je nach Verbindungsart setzen...
  for (i=0;i<3;i++)
  {
    PixieColor(0,50*(ap_mode==false),50*(ap_mode==true));
    delay(200);
    PixieColor(0,0,0);
    delay(200);    
  }
  if (ap_mode==false)
  {
    Serial.printf("... Letzte Stelle der IP-Adresse morsen %i\n",myAddr[3]);
  
    for(i=0;i<3;i++)
    {
      for ( j=(myAddr[3] / div[i])%10; j>0;j--)
      {
        Serial.print("* ");
        PixieColor(50,0,0);
        delay(200);
        PixieColor(0,0,0);
        delay(200);
      }
      if (i<2)
      {
        PixieColor(0,50,0);
        delay(600);
        PixieColor(0,0,0);
        delay(200);   
        Serial.print("| ");
      }
    }  
  }
  else
  {
    Serial.printf("Accesspoint Mode!\n");
  }
  // Farbe entsprechend setzen
  PixieColor(0,50*(ap_mode==false),50*(ap_mode==true));

  // Autoplay Init und laden
  autoplay_init();
  if (autoplay_load()==0)
  {
    Serial.println("Autoplay anwenden...");
    if (auto_mode == 1) // RGB
      {
        Serial.println("   ... Mode 1");
        PixieColor(auto_red, auto_green, auto_blue);
        effect=0;
      }
    if (auto_mode == 2) // Fade
    {
        Serial.println("   ... Mode 2");
      effect=1;
    }
    ServosXY(auto_posx, auto_posy);
  }
  Serial.println("\n... Initialisierung fertig !");
  cycle_next=0;
  mcycle_next=0;
}

void loop()
{
  static uint32_t now;
  static uint32_t blinkwait=0;
  static int blinkstate =0 ;
  static int sock_last = 1;
  static int dimit;
  static int effect_changed=0;

  if (!ap_mode)
  {
    WifiCheckRestart();
  }

  WifiDoEvents();
  //Serial.print(".");
  // Keepalivie
  now = millis();
#ifdef AUTOPLAY
   //Serial.println("AutoPlay Cross Blink");
   effect = 2;
   motion_effect = 2;
#else 
  #ifdef SHOW_MODE   // Kiosk / Schaukasten-Modus
    if (sock_connected == 0)
    {
      if (sock_last == 1)
      {
        ServosXY(45,60);    // Wandposition
        sock_last = 0;
        Serial.println("Ruheposition");
      }
      if (now>blinkwait)
      {
        switch(blinkstate)
        {
          case 1: PixieColor(255,255,255);
                  blinkwait = now + 100;
                  blinkstate++;
                  break;
          case 2: PixieColor(0,0,0);
                  blinkwait = now + 100;
                  blinkstate++;
                  break;
          case 3: PixieColor(255,255,255);
                  blinkwait = now + 100;
                  blinkstate++;
                  break;
          case 4: PixieColor(0,0,0);
                  blinkwait = now + 100;
                  blinkstate++;
                  break;
          case 5: PixieColor(255,255,255);
                  blinkwait = now + 100;
                  blinkstate++;
                  break;
          case 6: PixieColor(0,0,0);
                  blinkwait = now + 100;
                  blinkstate++;
                  break;
          case 7: PixieColor(255,255,255);
                  blinkwait = now + 2000;
                  blinkstate++;
                  dimit=255;
                  break;
          case 8: PixieColor(dimit,dimit,dimit);
                  blinkwait = now + 100;
                  dimit -=8;
                  if (dimit<64) blinkstate++;
                  break;
          case 9: blinkwait = now + 15000;
                  blinkstate++;
                  break;
          default: PixieColor(0,0,0);
                   blinkwait = now + 100;
                   blinkstate = 1;
                   break;                
        }
      }
    }
    //------------------------------------------
    // Jemand verbunden
    else
  #endif
#endif
  {
    if (sock_last==0)   // Erster Durchlauf
    {
      sock_last = 1;
      ServosXY(90,90);
      PixieColor(139,90,0);   // Dunkles orange
      effect = 0;
      motion_effect = 0;
      disconnect_timer = now + 1000*60*5;   // 5 minuten
    }
  
    //------------------------------------------
    // LED Effekt
    switch(effect)
    {
      case 0: if (effect_changed) 
                {
                  PixieRestore();  // Letzte Farbe wiederherstellen
                  effect_changed=0;
                }
              break;
      case 1: if (now>cycle_next) 
                {
                  PixieRainbowFade(1);
                  cycle_next = now+FADE_SPEED;
                }
              effect_changed=1;
              break;
      case 2: if (now>cycle_next) 
                {
                  PixieDisco();
                  cycle_next = now+DISCO_SPEED;
                }
              effect_changed=1;
              break;
    }
    //------------------------------------------
    // Motion Effekt
    switch(motion_effect)
    {
      case 0: break;    // Keine automatische Bewegung
      case 1: if (now>mcycle_next) 
                {
                  ServosLissajous();
                  mcycle_next = now+LISSA_SPEED;
                }
              break;
      case 2: if (now>mcycle_next) 
                {
                  ServosCross();
                  mcycle_next = now+CROSS_SPEED;
                }
              break;
    }
    
#ifdef SHOW_MODE    // Kiosk/Demo Mode im Schaukasten
    #ifndef AUTOPLAY
      // Disconnect Timer
      if (now > disconnect_timer) 
      {
        // Restart
        Serial.println("Max. Timer für eine Session erreicht!\nDISCONNECT\n");
        WifiDisconnect();
      }
    #endif
#endif
  }
  // Prüfen, ob Servos abgeschaltet werden können/sollen
  ServosCheckDetach(now);

  // Alle x ms ein Keepalive-Paket schicken
  PixieKeepAlive(now);
}
