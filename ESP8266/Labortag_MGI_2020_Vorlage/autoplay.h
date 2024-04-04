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
 * Header-Datei autoplay.h
 * 
 */
#ifndef AUTOPLAY_H

#define AUTOPLAY_H
uint8_t auto_mode;    // 0 - kein Autoplay ; 1 - Feste RGB   ; 2 - Fade
uint8_t auto_red;
uint8_t auto_green;
uint8_t auto_blue;
uint8_t auto_posx;
uint8_t auto_posy;

void autoplay_init();
uint8_t autoplay_load();
void autoplay_save();

#endif
