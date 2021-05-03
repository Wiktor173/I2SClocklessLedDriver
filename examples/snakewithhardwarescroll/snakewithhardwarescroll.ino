#include "FastLED.h"
#include "I2SClocklessLedDriver.h"
#define ledsperstrip 256
#define numstrips 16
//here we have 3 colors per pixel
//uint8_t leds[numstrips*ledsperstrip*3];
//this one below is same as the one above
CRGB leds[numstrips*ledsperstrip];

int pins[16]={0,2,4,5,12,13,14,15,16,18,19,21,22,23,25,26};

I2SClocklessLedDriver driver;
void setup() {
    Serial.begin(115200);
    
    driver.initled((uint8_t*)leds,pins,numstrips,ledsperstrip,ORDER_GRB);
    driver.setBrightness(10);
    for(int j=0;j<numstrips;j++)
    {
      for (int i=0;i<j+1;i++)
      {
        leds[i+ledsperstrip*j]=CRGB::Red;
      }
    }
    
}

int off=0;
long time1,time2,time3;
void loop() {
//going forward
        time2=ESP.getCycleCount();
        driver.showPixels(-off+ledsperstrip); //driver.showPixels(off+ledsperstrip); //to go backwards
        time3=ESP.getCycleCount();
        Serial.printf("Calcul pixel fps:%.2f   showPixels fps:%.2f   Total fps:%.2f \n",(float)240000000/(time2-time1),(float)240000000/(time3-time2),(float)240000000/(time3-time1));
        off++;
        delay(50);
}
