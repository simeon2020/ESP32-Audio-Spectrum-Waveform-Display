/* ESP32 Stereo Audio Spectrum Analyser on an SSD1306/SH1106 Display, 8-bands 125, 250, 500, 1k, 2k, 4k, 8k, 16k
  *  
 This software, the ideas and concepts is Copyright (c) David Bird 2018. All rights to this software are reserved.
 
 Any redistribution or reproduction of any part or all of the contents in any form is prohibited other than the following:
 1. You may print or download to a local hard disk extracts for your personal and non-commercial use only.
 2. You may copy the content to individual third parties for their personal use, but only if you acknowledge the author David Bird as the source of the material.
 3. You may not, except with my express written permission, distribute or commercially exploit the content.
 4. You may not transmit it or store it in any other website or other form of electronic retrieval system for commercial purposes.

 The above copyright ('as annotated') notice and this permission notice shall be included in all copies or substantial portions of the Software and where the
 software use is visible to an end-user.
 
 THE SOFTWARE IS PROVIDED "AS IS" FOR PRIVATE USE ONLY, IT IS NOT FOR COMMERCIAL USE IN WHOLE OR PART OR CONCEPT. FOR PERSONAL USE IT IS SUPPLIED WITHOUT WARRANTY 
 OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE AUTHOR OR COPYRIGHT HOLDER BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 See more at http://www.dsbird.org.uk
*/

//#include <Wire.h>
#include "arduinoFFT.h" // Standard Arduino FFT library https://github.com/kosme/arduinoFFT
#include <FastLED.h>

#include <LEDMatrix.h>
arduinoFFT FFT = arduinoFFT();
/////////////////////////////////////////////////////////////////////////
// Comment out the display your NOT using e.g. if you have a 1.3" display comment out the SSD1306 library and object
//#include "SH1106.h"     // https://github.com/squix78/esp8266-oled-ssd1306
//SH1106 display(0x3c, 17,16); // 1.3" OLED display object definition (address, SDA, SCL) Connect OLED SDA , SCL pins to ESP SDA, SCL pins

//#include "SSD1306.h"  // https://github.com/squix78/esp8266-oled-ssd1306
//SSD1306 display(0x3c, 16,17);  // 0.96" OLED display object definition (address, SDA, SCL) Connect OLED SDA , SCL pins to ESP SDA, SCL pins
/////////////////////////////////////////////////////////////////////////
//#include "font.h" // The font.h file must be in the same folder as this sketch
/////////////////////////////////////////////////////////////////////////
#define SAMPLES 1024             // Must be a power of 2
#define SAMPLING_FREQUENCY 40000 // Hz, must be 40000 or less due to ADC conversion time. Determines maximum frequency that can be analysed by the FFT Fmax=sampleF/2.
#define amplitude 1000           // Depending on your audio source level, you may need to increase this value
#define Left  true
#define Right false

#define LED_PIN        0
#define COLOR_ORDER    RGB
#define CHIPSET        WS2811

#define MATRIX_WIDTH   8  // Set this negative if physical led 0 is opposite to where you want logical 0
#define MATRIX_HEIGHT  6  // Set this negative if physical led 0 is opposite to where you want logical 0
#define MATRIX_TYPE    VERTICAL_ZIGZAG_MATRIX  // See top of LEDMatrix.h for matrix wiring types

cLEDMatrix<MATRIX_WIDTH, MATRIX_HEIGHT, MATRIX_TYPE> leds;


unsigned int sampling_period_us;
unsigned long microseconds;
byte Lpeak[] = {0,0,0,0,0,0,0,0};
byte Rpeak[] = {0,0,0,0,0,0,0,0};

double LvReal[SAMPLES];
double LvImag[SAMPLES];

double RvReal[SAMPLES];
double RvImag[SAMPLES];

unsigned long newTime, oldTime;

/////////////////////////////////////////////////////////////////////////
void setup() {
  Serial.begin(115200);
//  Wire.begin(17,16); // SDA, SCL
//  display.init();
//  display.setFont(Dialog_plain_8);
  //display.setFont(ArialMT_Plain_10);
//  display.flipScreenVertically(); // Adjust to suit or remove
  sampling_period_us = round(1000000 * (1.0 / SAMPLING_FREQUENCY));
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds[0], leds.Size());
  FastLED.setBrightness(127);
  FastLED.clear(true);
  delay(100);
  FastLED.showColor(CRGB::Red);
  delay(300);
  for (int band = 0; band <8; band ++){
    leds.DrawLine(band, 0, band, leds.Height(), CRGB(24*band, 24*band, 255-(band*25)));
     FastLED.show();
    delay(100);
   }
  FastLED.showColor(CRGB::Black);
  delay(100);
}

void loop() {
//  display.clear();
  FastLED.clear();        
//  display.drawString(0,0,"125 250 500 1K  2K 4K 8K 16K");
  for (int i = 0; i < SAMPLES; i++) {
    newTime = micros();
    //VP input = Left, VN = Right
    LvReal[i] = analogRead(36); // Using Arduino ADC nomenclature. A conversion takes about 1uS on an ESP32
    LvImag[i] = 0;
//    RvReal[i] = analogRead(39); // Using Arduino ADC nomenclature. A conversion takes about 1uS on an ESP32
//    RvImag[i] = 0;
    while ((micros() - newTime) < sampling_period_us) { /* do nothing to wait */ }
  }
  FFT.Windowing(LvReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.Compute(LvReal, LvImag, SAMPLES, FFT_FORWARD);
  FFT.ComplexToMagnitude(LvReal, LvImag, SAMPLES);
/*
  FFT.Windowing(RvReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.Compute(RvReal, RvImag, SAMPLES, FFT_FORWARD);
  FFT.ComplexToMagnitude(RvReal, RvImag, SAMPLES);
*/  
  for (int i = 2; i < (SAMPLES/2); i++){ // Don't use sample 0 and only the first SAMPLES/2 are usable.
    // Each array element represents a frequency and its value, is the amplitude. Note the frequencies are not discrete.
    if (LvReal[i] > 1500 /*|| RvReal[i] > 1500*/) { // Add a crude noise filter, 10 x amplitude or more
      Serial.println(LvReal[i]);
      if (i<=2 ) {
        displayBand(Left,0,(int)LvReal[i]);  // 125Hz
//        displayBand(Right,0,(int)RvReal[i]); // 125Hz
      }
      if (i >2   && i<=4 ) {
        displayBand(Left,1,(int)LvReal[i]);  // 250Hz
//        displayBand(Right,1,(int)RvReal[i]); // 250Hz
      }
      if (i >4   && i<=7 ) {
        displayBand(Left,2,(int)LvReal[i]);  // 500Hz
//        displayBand(Right,2,(int)RvReal[i]); // 500Hz
      }
      if (i >7   && i<=15 ) {
        displayBand(Left,3,(int)LvReal[i]);  // 1000Hz
//        displayBand(Right,3,(int)RvReal[i]); // 1000Hz
      }
      if (i >15  && i<=40 ) {
        displayBand(Left,4,(int)LvReal[i]);  // 2000Hz
//        displayBand(Right,4,(int)RvReal[i]); // 2000Hz
      }
      if (i >40  && i<=50 ) {
        displayBand(Left,5,(int)LvReal[i]);  // 4000Hz
//        displayBand(Right,5,(int)RvReal[i]); // 4000Hz
      }
      if (i >50  && i<=70 ) {
        displayBand(Left,6,(int)LvReal[i]);  // 8000Hz
//        displayBand(Right,6,(int)RvReal[i]); // 8000Hz
      }
      if (i >70           ) {
        displayBand(Left,7,(int)LvReal[i]);  // 16000Hz
//        displayBand(Right,7,(int)RvReal[i]); // 16000Hz
      }
      //Serial.println(i);
    }
  }
 //   for (byte band = 0; band <= 7; band++) display.drawHorizontalLine(1+16*band,64-Lpeak[band],14); // Only the Left change for peaks
      for (int band = 0; band <= 7; band++) {
        leds.DrawLine(band,Lpeak[band],band,Lpeak[band],CRGB(255, 0, 0)); // Only the Left change for peaks
      }
      
  if (millis()%4 == 0) {
    for (byte band = 0; band <= 7; band++) {
      if (Lpeak[band] > 0 || Rpeak[band] > 0) {
        Lpeak[band] -= 1; // Decay the peaks
 //       Rpeak[band] -= 1; // Decay the peaks
      }
    }
  }
//  display.display();
FastLED.show();
}

void displayBand(bool channel, int band, int dsize){
  int dmax = 5;
  // For now, display spectrum for Left then Right in quick succession
//  Serial.println("DSIZE");  Serial.println(dsize);
  dsize /= amplitude;
//  Serial.println(dsize);
  if (dsize > dmax) dsize = dmax;
 // for (int s = 0; s <= dsize; s=s+2){display.drawHorizontalLine(1+16*band,64-s, 14);}
  for (int s = 0; s <= dsize; s=s+1) {leds.DrawLine(band,s,band,s,CRGB(16+(16*band), 128-32*s, 0)); }
  if (dsize >Lpeak[band]) {Lpeak[band] = dsize;}
//  if (dsize >Rpeak[band]) {Rpeak[band] = dsize;}
//  if (channel == Left)  Serial.println("Left LEDs");  // turn on left lights at column = band and heigth = dsize
//  if (channel == Right) Serial.println("Right LEDs"); // turn on right lights at column = band and heigth = dsize
}
