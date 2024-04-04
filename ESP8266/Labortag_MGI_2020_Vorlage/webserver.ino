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
 * ESP-Webserver Dateien im SPIFFS
 * v0.5 MSoft 18.05.2016
 * 32MBit-Flash: 4M(1M SPIFFS), NodeMCU
 * Websocket lib von: https://github.com/Links2004/arduinoWebSockets/tree/master/src
* example from here: https://gist.github.com/bbx10/667e3d4f5f2c0831d00b
*/

#include <base64.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>



#include "webserver.h"
#include "hardware.h"
#include "autoplay.h"

// Die nachfolgenden Settings werden
// im Normalfall aus dem Filesystem
// aktualisiert
String ssid = "";
String password = "none";
String bu_ssid = ""; 
String bu_password = "";
String ap_name = "SMARTLED";
String ap_password = "";
String ap_ip   = "192.168.4.1";
// -- Ende Settings --

String GetPicture(String filename);                        // codiert ein Bild aus dem SPIFFS in Base64 wenn kleiner als MAX_PIC_SIZE
String getContentType(String filename);                    // setzt Content-Typ nach Dateiendung
void notFound(void);                                       // schickt 404-Meldung 
void handleUnknown(void);                                  // alles, was auf SPIFFS gefunden wurde und keine callback-Funktion hat
void handleUpload(void);                                   // Datei-Upload holt File und schreibt ins SPIFFS
void handleConfig(void);
void parseCmd(char*);

MDNSResponder mdns;
ESP8266WiFiMulti WiFiMulti;

// Create an instance of the webserver on Port 80
ESP8266WebServer webserver(80);

WebSocketsServer webSocket = WebSocketsServer(81);

#define DEBUG

struct {
  char idx;
  int a,b,c;
} cmd_s;

extern uint8_t show_mode;

/********************************************************************************
 * 
 * WebSocket Event behandeln
 * 
 */
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
{
  //Serial.printf("[debug}  webSocketEvent(%d, %d, ...)\r\n", num, type);
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("    ... WS: [%u] Getrennt!\r\n", num);
      sock_connected = 0;
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("    ... WS: [%u] Verbunden von %d.%d.%d.%d url: %s\r\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        // Send the current LED status
      }
      sock_connected = 1;
      break;
    case WStype_TEXT:
      #ifdef DEBUG
        Serial.printf("    ... WS: [%u] Empfangen: %s\r\n", num, (char*)payload);
      #endif
      parseCmd((char*)payload);
      //Serial.printf("Cmd: %i p1: %i  p2: %i  p3: %i\n", cmd_s.idx,cmd_s.a, cmd_s.b, cmd_s.c);
      // send data to all connected clients
      webSocket.broadcastTXT(payload, length);
      break;
    case WStype_BIN:
      #ifdef DEBUG
         Serial.printf("    ... WS: [%u] get binary length: %u\r\n", num, length);
      #endif
      hexdump(payload, length);

      // echo data back to browser
      webSocket.sendBIN(num, payload, length);
      break;
    default:
      Serial.printf("    ... WS: Invalid WStype [%d]\r\n", type);
      break;
  }
}

/*********************************************************
 * 
 * Websocket Befehl auswerten
 * 
 */
void parseCmd(char* payload)
{
  int i=0,j=0,par=0;
  char buf[20];

  // Zunächst auf unbekanntes Setting
  cmd_s.idx=0;
  
  while(i<strlen(payload))
  {
    // Semikolon ist Trenner zwischen einzelnen Werten
    if (payload[i]==';')
    {
      buf[j]=0;   // String-Ende
      switch(par) // Der wievielte Parameter ist es
      {
        case 0: if (strcmp(buf,"RGB")==0)
                {
                  cmd_s.idx=1;
                }
                else if (strcmp(buf,"SERVO")==0)
                {
                  cmd_s.idx=2;
                }
                else if (strcmp(buf,"EFFECT")==0)
                {
                  cmd_s.idx=3;
                }
                else if (strcmp(buf,"MOVE")==0)
                {
                  cmd_s.idx=4;
                }
                else if (strcmp(buf,"APSAVE")==0)
                {
                  cmd_s.idx=100;
                }
                else {
                  Serial.printf("    ... WS: FEHLER Unbekannter Parameter: %s\n",buf);
                }
                break;
        case 1: cmd_s.a = atoi(buf);
                break;
        case 2: cmd_s.b = atoi(buf);
                break;
        case 3: cmd_s.c = atoi(buf);
                break;
        default: Serial.printf("    ... WS: FEHLER in Parameterliste\n");
                break;        
      }
      par++;  // Parameter hochz
      j=0;    // Buffer auf Beginn setzen
      i++;    // Semikolon überspringen
    }
    else
    {
      buf[j++]=payload[i++];
      if (j>19)
      {
        Serial.printf("    ... WS: Buffer overflow in parseCmd!\n");
        j=19;
      }
    }
  }
  // Befehl ausführen
  switch(cmd_s.idx)
  {
    case 1:  PixieColor(cmd_s.a, cmd_s.b, cmd_s.c);
             break;
    case 2:  ServosXY(cmd_s.a / 200. * 180., cmd_s.b / 200. * 180.);
             break;
    case 3: effect = cmd_s.a;
            if (effect==0)
              {
                auto_mode=1;
              }
            if (effect==1)
              {
                auto_mode=2;
              }
             break;
    case 4: motion_effect = cmd_s.a;
             break;
    case 100:
            autoplay_save();
            break;
  
    default: Serial.println("    ... WS: FEHLER switch cmd_s");
             break;
  }
}




// ###############################################################
// ---------------------- WiFi -----------------------------------
// ###############################################################

/************************************************************
 * config.txt aus SPIFFS lesen
 */
void WifiConfig()
{
  Serial.println("... Wifi config.txt lesen");
  
  // SPIFFS Dateisystem initialisieren
  if (!SPIFFS.begin())
  {
    Serial.println("    ... FEHLER: SPIFFS nicht initialisiert!");   
    while(1)                                        // ohne SPIFFS geht es sowieso nicht...
    {
      yield();
    }
  }
  Serial.println("    ... SPIFFS ok");

  // config.txt für WLAN-Zugriff holen
  File configfile = SPIFFS.open("/config.txt","r");         
  if (configfile)
  {
    while(String indaten = configfile.readStringUntil('\n'))
    {
      if (indaten.length() == 0)
      {
        break;
      }  
      // Standard-WLAN Netz Username / Password
      if (indaten.startsWith("ssid"))           // username WLAN-Client
      {
        indaten.replace("ssid","");
        indaten.trim();
        ssid = indaten;  
        Serial.print("    ... SSID 1: ");
        Serial.println(ssid);
      }
      else if (indaten.startsWith("pass"))           // username WLAN-Client
      {
        indaten.replace("pass","");
        indaten.trim();
        password = indaten;
      }
      // Backup-WLAN Netz Username / Password
      else if (indaten.startsWith("bssid"))           // username WLAN-Client
      {
        indaten.replace("bssid","");
        indaten.trim();
        bu_ssid = indaten;  
        Serial.print("    ... SSID 2: ");
        Serial.println(bu_ssid);
      }
      else if (indaten.startsWith("bpass"))           // username WLAN-Client
      {
        indaten.replace("bpass","");
        indaten.trim();
        bu_password = indaten;
      }
      // Accesspont Daten
      else if (indaten.startsWith("ap_name"))           // username WLAN-Client
      {
        indaten.replace("ap_name","");
        indaten.trim();
        ap_name = indaten;
        Serial.print("    ... AP Name: ");
        Serial.println(ap_name);
      }
      else if (indaten.startsWith("ap_pass"))           // username WLAN-Client
      {
        indaten.replace("ap_pass","");
        indaten.trim();
        ap_password = indaten;
      }
      else if (indaten.startsWith("ap_ip"))           // username WLAN-Client
      {
        indaten.replace("ap_ip","");
        indaten.trim();
        ap_ip = indaten;
      }
    }  
    configfile.close();
  }
}

IPAddress ip_to_int (const char * ip)
{
    /* The return value. */
    unsigned v = 0;
    unsigned char digits[4];
    /* The count of the number of bytes processed. */
    int i;
    /* A pointer to the next digit to process. */
    const char * start;

    start = ip;
    for (i = 0; i < 4; i++) {
        /* The digit being processed. */
        char c;
        /* The value of this byte. */
        int n = 0;
        while (1) {
            c = * start;
            start++;
            if (c >= '0' && c <= '9') {
                n *= 10;
                n += c - '0';
            }
            /* We insist on stopping at "." if we are still parsing
               the first, second, or third numbers. If we have reached
               the end of the numbers, we will allow any character. */
            else if ((i < 3 && c == '.') || i == 3) {
                break;
            }
            else {
                return 0;
            }
        }
        if (n >= 256) {
            return 0;
        }
        digits[i]=n;
    }
    return IPAddress(digits);
}
/**************************************************
 *  Wifi starten
 */
void WifiStart()
{
  char ssid_send[64];
  char password_send[64];
  char no[2];
  static int run=0;
  char wlan_force_ap_mode = 0;
  IPAddress my_apip;
  
  // Connect to WiFi network
  ssid.toCharArray(ssid_send,ssid.length() + 1);
  password.toCharArray(password_send,password.length() + 1);

  // Check if SSID is set to none -> dont check for existing WLANs
  if (strcasecmp(ssid_send,"none") !=0)
  {
      WiFi.begin(ssid_send, password_send);
      Serial.println("... Wifi initialisieren");
      // Standard-WLAN
      Serial.printf("    ... versuche mit %s zu verbinden ",ssid_send);
      for (int i=0; i<WIFI_TIMEOUT; i++)
      {
        if (WiFi.status() != WL_CONNECTED)
        {
          Serial.print(".");
          delay(500);
        }
      }  
      if (WiFi.status() == WL_CONNECTED)
      {
        Serial.print("  OK!");
        myAddr = WiFi.localIP();
      }
      else
      // check secondardy backup-WLAN
      {
        bu_ssid.toCharArray(ssid_send,bu_ssid.length() + 1);
        bu_password.toCharArray(password_send,bu_password.length() + 1);
    
        WiFi.begin(ssid_send, password_send);
        Serial.printf("\n    ... Backup: versuche mit %s zu verbinden ",ssid_send);
        for (int i=0; i<WIFI_TIMEOUT; i++)
        {
          if (WiFi.status() != WL_CONNECTED)
          {
            Serial.print(".");
            delay(500);
          }
          if (WiFi.status() == WL_CONNECTED)
          {
            Serial.print("OK!");
            myAddr = WiFi.localIP();
          }
        }
    }
  }  
  else  // Here we are when connection was disabled
  {
    Serial.println("Connect to WLAN disabled");
    wlan_force_ap_mode = 1;
    WiFi.begin();
  }
  if ( (WiFi.status() != WL_CONNECTED) || wlan_force_ap_mode)
  {  
    WiFi.mode(WIFI_AP_STA);

    ap_name.toCharArray(ssid_send,ap_name.length() + 1);
    ap_password.toCharArray(password_send,ap_password.length() + 1);
    no[0]='0'+run;
    no[1]='\0';
    run++;
    if(run>9) run=0;
    if (show_mode) {
      strcat(ssid_send,no);
    }
    if (ap_ip.length() > 0)
    {
      my_apip=ip_to_int(ap_ip.c_str());
      WiFi.softAPConfig(my_apip, my_apip, IPAddress(255,255,255,0));
    }
    
    WiFi.softAP(ssid_send, password_send);
    Serial.printf("\n    ... ACCESS-POINT Mode %s", ssid_send);
    myAddr = WiFi.softAPIP();
    ap_mode = true;
  }   
  Serial.print("\n    ... IP-Adresse: ");
  Serial.println(myAddr);
  mdns.begin("smartled");
}

// ###############################################################
// ---------------------- Web-Tools ------------------------------
// ###############################################################

void handleUnknown()
{
  String filename = webserver.uri();

#ifdef DEBUG
  Serial.print("URL: ");
  Serial.println(filename);
#endif

  File pageFile = SPIFFS.open(filename, "r"); 
  if (pageFile)
  {
    String contentTyp = getContentType(filename);

#ifdef DEBUG
    Serial.print("Content-Typ: ");
    Serial.println(contentTyp);
#endif

    webserver.streamFile(pageFile, contentTyp);
    pageFile.close(); 
  }
  else
  {
    notFound(); 
  }
}

//---------------------------------------------------------------

void notFound()
{
  String sHTML = F("<html><head><title>404 Not Found</title></head><body><h1>RGB-LED Server</h1><p>Die Seite wurde nicht gefunden!</p></body></html>");
  webserver.send(404, "text/html", sHTML);    
}

//---------------------------------------------------------------

void handleUpload()
{

#ifdef DEBUG
  Serial.println("Datei-Upload"); 
#endif

  static File fsUploadFile;                                         // aktueller Upload

  HTTPUpload& upload = webserver.upload();
  if(upload.status == UPLOAD_FILE_START)
  {
    String dateiname = upload.filename;
    if(!dateiname.startsWith("/"))
    {
      dateiname = "/" + dateiname;
    }
    
#ifdef DEBUG
    Serial.print("handleFileUpload Name: ");
    Serial.println(dateiname);
#endif

    fsUploadFile = SPIFFS.open(dateiname, "w");
    dateiname = String();
  }
  else if(upload.status == UPLOAD_FILE_WRITE)
  {
    if(fsUploadFile)
    {
      fsUploadFile.write(upload.buf, upload.currentSize);

#ifdef DEBUG
      Serial.println(upload.currentSize,DEC);
#endif

    }  
  }
  else if(upload.status == UPLOAD_FILE_END)
  {
    if(fsUploadFile)
    {
      fsUploadFile.close();
    }  

#ifdef DEBUG
    Serial.print("handleFileUpload Size: ");
    Serial.println(upload.totalSize);
#endif

  }
}

// ----------------------------------------------------------------------

String GetPicture(String filename)
{   
  String contentTyp = getContentType(filename);

  File tempFile = SPIFFS.open(filename,"r");     
  if (tempFile)
  {
    if (tempFile.size() < MAX_PIC_SIZE)
    {        
      filename = tempFile.readString(); 
      filename = "data:" + contentTyp + ";base64," + base64::encode((uint8_t *) filename.c_str(), tempFile.size());  
      Serial.println("Test");
      Serial.println(filename);
    }
    tempFile.close();
  }
  return filename;
}

// ----------------------------------------------------------------------

String getContentType(String filename)
{
  if(webserver.hasArg("download"))
    return "application/octet-stream";
  else if(filename.endsWith(".ico"))
    return "image/x-icon";
  else if(filename.endsWith(".htm"))
    return "text/html";
  else if(filename.endsWith(".html"))
    return "text/html";
  else if(filename.endsWith(".css"))
    return "text/css";
  else if(filename.endsWith(".js"))
    return "application/javascript";
  else if(filename.endsWith(".png"))
    return "image/png";
  else if(filename.endsWith(".gif"))
    return "image/gif";
  else if(filename.endsWith(".jpg"))
    return "image/jpeg";
  else if(filename.endsWith(".ico"))
    return "image/x-icon";
  else if(filename.endsWith(".xml"))
    return "text/xml";
  else if(filename.endsWith(".pdf"))
    return "application/x-pdf";
  else if(filename.endsWith(".zip"))
    return "application/x-zip";
  else if(filename.endsWith(".gz"))
    return "application/x-gzip";
  return "text/plain";
}

// ###############################################################
//---------------------- Webseiten -------------------------------
// ###############################################################

void handleIndex()
{
  String filename = webserver.uri();
  if (filename == "/") 
    {
      filename = "/index.html";
    }
    File pageFile = SPIFFS.open(filename,"r"); 
    if (pageFile)
    {
      String sHTML = pageFile.readString();
      pageFile.close();  
      
      //String bilddaten = "/logofh.gif";
      //bilddaten = GetPicture(bilddaten);
      //sHTML.replace("~logofh~",bilddaten);                              // Bild Webseite einfügen
      
      webserver.send(200, "text/html", sHTML);
    }
    else
    {
      notFound(); 
    }
  Serial.println("    ... Root done");
}



// ----------------------------------------------------------------------

void handleConfig()
{ 
  static File fsUploadFile;                                         // aktueller Upload

  String filename = webserver.uri();
  Serial.println("    ... Config call");
  File pageFile = SPIFFS.open(filename, "r"); 
  if (pageFile)
  {
    String sHTML = pageFile.readString();
    pageFile.close();

    //String bilddaten = "/logo.png";
    //bilddaten = GetPicture(bilddaten);
    //sHTML.replace("~logo~",bilddaten);                              // Bild Webseite einfügen
        
    if (webserver.hasArg("reboot"))                           
    {
      Serial.println("\r\n\r\n**** NEUSTART ****\r\n");
      ESP.restart();
    }
       
    // config.txt speichern
    File configfile;
    if (webserver.hasArg("config"))                            // neue config.txt
    {

#ifdef DEBUG
      Serial.println("\r\n--- config.txt begin ---");
      Serial.print(webserver.arg("config"));  
      Serial.println("--- config.txt end ---\r\n");
#endif

      configfile = SPIFFS.open("/config.txt","w");             // config.txt überschreiben
      if (configfile)
      {
        configfile.print(webserver.arg("config"));             // Daten schreiben    
        configfile.close();   
        sHTML.replace("~config~",webserver.arg("config"));     // und in Webseite einfügen
      }
    }
    else                                                       // keine configdaten zu schreiben
    {
      File configfile = SPIFFS.open("/config.txt","r");        // config.txt holen
      if (configfile)
      {     
        sHTML.replace("~config~",configfile.readString());     // und in Webseite einfügen
        configfile.close();
      }
      else
      {
        sHTML.replace("~config~","");                          // keine config.txt da
      }
    }

    // Datei download
    if (webserver.hasArg("filename"))                          // Download oder Delete
    {
      if (webserver.hasArg("download"))
      {
        Serial.print("Download Filename: ");
        File downfile = SPIFFS.open(webserver.arg("filename"),"r"); 
        String sendname = webserver.arg("filename");
        sendname.replace("/",""); 
        Serial.println(sendname);
        webserver.sendHeader("Content-Disposition"," attachment; filename=\"" + sendname + "\"");
        webserver.streamFile(downfile,"application/octet-stream");
        downfile.close();
      }
      if (webserver.hasArg("delete"))
      {
        Serial.print("Delete Filename: ");
        SPIFFS.remove(webserver.arg("filename"));
      }
      Serial.println(webserver.arg("filename"));
    }
    
#ifdef DEBUG
    Serial.println("\r\n--- Dir-Liste begin ---");
#endif

    // Dateiliste anzeigen
    Dir dir = SPIFFS.openDir("/");
    int anzahl = 0;
    while (dir.next())
    {
      String filename = dir.fileName(); 
      File dirFile = dir.openFile("r");
      size_t filelen =  dirFile.size(); 

#ifdef DEBUG
      Serial.print(filename);
      Serial.print(" - ");
      Serial.println(filelen);
#endif
      String filename_temp = filename;
      filename_temp.replace("/","");
      String dirzeile = "<form action=\"config.html\" method=\"POST\">\r\n<tr><td align=\"center\">";
      dirzeile += "<input type=\"hidden\" name = \"filename\" value=\"" + filename + "\">" + filename_temp;
      dirzeile +=  "</td>\r\n<td align=\"right\">" + String(filelen,DEC);
      dirzeile += "</td><td align=\"center\"><button type=\"submit\" name= \"download\">Download</button></td>\r\n";
      dirzeile += "<td align=\"center\" bgcolor=\"red\"><button type=\"submit\" name=\"delete\">&nbsp;&nbsp;&nbsp;Delete&nbsp;&nbsp;&nbsp;</button></td>";
      dirzeile += "</tr></form>\r\n~dirzeile~"; 
      sHTML.replace("~dirzeile~",dirzeile);
      anzahl++;
    }   
    sHTML.replace("~dirzeile~","");

    FSInfo fs_info;
    SPIFFS.info(fs_info);
    sHTML.replace("~anzahl~",String(anzahl,DEC));
    sHTML.replace("~total~",String(fs_info.totalBytes/1024,DEC));
    sHTML.replace("~used~",String(fs_info.usedBytes/1024,DEC));
    sHTML.replace("~free~",String((fs_info.totalBytes - fs_info.usedBytes)/1024,DEC));

#ifdef DEBUG
  Serial.println("--- Dir-Liste end ---\r\n");
#endif
   
    webserver.send(200, "text/html", sHTML);
  }
  else
  {
    notFound(); 
  }
}


void WifiInit()
{
  // inital connect
  WiFi.mode(WIFI_STA);
  WifiStart();

  // Start the server
  webserver.on("/", handleIndex);
  webserver.on("/index.html", handleIndex);
  webserver.on("/index.htm", handleIndex);

  webserver.on("/config.html", HTTP_GET, handleConfig);
  webserver.on("/config.html", HTTP_POST, handleConfig, handleUpload);
 
  webserver.onNotFound(handleUnknown);
  webserver.begin();
  mdns.addService("http", "tcp", 80);
  
  Serial.println("    ... HTTP server gestartet auf Port 80");
    
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("    ... WebSocket gestartet auf Port 81");
}

void WifiCheckRestart()
{
    // check if WLAN is connected
    if (WiFi.status() != WL_CONNECTED)
    {
      WifiStart();
    }
}

void WifiDoEvents()
{
  webSocket.loop();
  webserver.handleClient();
}

void WifiDisconnect()
{
  sock_connected = 0;
  WiFi.disconnect();
  delay(1000);
  WifiStart();
}
