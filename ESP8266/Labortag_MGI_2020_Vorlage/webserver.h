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
 *
 * Header-Datei mit allen Libraries
 */

#ifndef __WEBSERVER__
#define __WEBSERVER__

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsServer.h>
#include <Hash.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>

#define WIFI_TIMEOUT 20                 // Wartezeit für connect * 200ms 
#define MAX_PIC_SIZE  1 * 1024          // bis zu dieser Größe wird Base64 encoded


IPAddress myAddr;

bool ap_mode = false;                                      // true wenn AP aktiv  
int  sock_connected = 0;

void WifiConfig(void);                                     // holt die WLAN-Daten aus config.txt
void WifiStart(void);
void WifiInit(void);
void WifiCheckRestart(void);
void WifiDoEvents(void);
void WifiDisconnect(void);

#endif
