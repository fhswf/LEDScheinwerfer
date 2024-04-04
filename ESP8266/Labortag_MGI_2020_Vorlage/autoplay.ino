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
 * autoplay.ino - einladen der letzen Einstellung
 * 
 */
 
 #include <FS.h>

void autoplay_init()
{
    auto_mode = 1;    // 0 - kein Autoplay ; 1 - Feste RGB   ; 2 - Fade
    auto_red=0;
    auto_green=0;
    auto_blue=0;
    auto_posx=100;
    auto_posy=100;
}


uint8_t autoplay_load()
{
    Serial.println("... Autoplay autoplay.txt lesen");
  
  // SPIFFS Dateisystem initialisieren
  if (!SPIFFS.begin())
  {
    Serial.println("    ... FEHLER (Autoplay): SPIFFS nicht initialisiert!");  
    return 1; 
  }
  Serial.println("    ... SPIFFS ok");

  File configfile = SPIFFS.open("/autoplay.txt","r");         
  if (configfile)
  {
    while(String indaten = configfile.readStringUntil('\n'))
    {
      if (indaten.length() == 0)
      {
        break;
      }  
      // Standard-WLAN Netz Username / Password
      if (indaten.startsWith("mode"))           // autoplay mode
      {
        indaten.replace("mode","");
        indaten.trim();
        auto_mode = indaten.toInt(); 
        Serial.print("    ... mode: ");
        Serial.println(auto_mode);
      }
      else if (indaten.startsWith("red"))       // autoplay red   
      {
        indaten.replace("red","");
        indaten.trim();
        auto_red = indaten.toInt();
      }
      else if (indaten.startsWith("green"))       // autoplay green
      {
        indaten.replace("green","");
        indaten.trim();
        auto_green = indaten.toInt();
      }
      else if (indaten.startsWith("blue"))       // autoplay blue
      {
        indaten.replace("blue","");
        indaten.trim();
        auto_blue = indaten.toInt();
      }
      else if (indaten.startsWith("posx"))       // autoplay pos x
      {
        indaten.replace("posx","");
        indaten.trim();
        auto_posx = indaten.toInt();
        Serial.print("    ... posx: ");
        Serial.println(auto_posx);
      }
      else if (indaten.startsWith("posy"))       // autoplay pos y
      {
        indaten.replace("posy","");
        indaten.trim();
        auto_posy = indaten.toInt();
        Serial.print("    ... posy: ");
        Serial.println(auto_posy);
      }
    }  
    configfile.close();
    return 0;
  }
}

void autoplay_save()
{
  File ofile;
  Serial.println("Autoplay -> Save");
  ofile = SPIFFS.open("/autoplay.txt","w");
  Serial.print("   ... auto_mode: ");
  Serial.println(auto_mode);
  ofile.print("mode ");
  ofile.println(auto_mode);
  ofile.print("red ");
  ofile.println(auto_red);
  ofile.print("green ");
  ofile.println(auto_green);
  ofile.print("blue ");
  Serial.print("   ... auto_rgb: ");
  Serial.print(auto_red);
  Serial.print("; ");
  Serial.print(auto_green);
  Serial.print("; ");
  Serial.print(auto_blue);
  Serial.println("; ");
  ofile.println(auto_blue);
  ofile.print("posx ");
  ofile.println(auto_posx);
  ofile.print("posy ");
  ofile.println(auto_posy);
  Serial.print("   ... auto_posxy: ");
  Serial.print(auto_posx);
  Serial.print("; ");
  Serial.print(auto_posy);
  Serial.println("; ");

  ofile.close();
  
}
