
// Example sketch which shows how to display some patterns
// on a 64x32 LED matrix
//

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>


#define PANEL_RES_X 64  // Number of pixels wide of each INDIVIDUAL panel module.
#define PANEL_RES_Y 64  // Number of pixels tall of each INDIVIDUAL panel module.
#define PANEL_CHAIN 1   // Total number of panels chained one to another

//MatrixPanel_I2S_DMA dma_display;
MatrixPanel_I2S_DMA *dma_display = nullptr;

void setup() {
  Serial.begin(9600);
  delay(500);
  Serial.println("AAA");

  // Module configuration
  HUB75_I2S_CFG mxconfig(
    PANEL_RES_X,  // module width
    PANEL_RES_Y,  // module height
    PANEL_CHAIN   // Chain length
  );
  mxconfig.gpio.e = 38;
  mxconfig.clkphase = false;

  // Display Setup
  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
      Serial.println("BBB");
  dma_display->begin();
  delay(500);
  dma_display->setBrightness8(100);  //0-255
  dma_display->clearScreen();
  // draw a blue circle
  dma_display->fillScreen(dma_display->color565(255, 255,255));
      Serial.println("CCC");
  delay(500);
      Serial.println("DDD");
}

int pos = 0;

void loop() {
  pos++;
  if (pos % 2 == 1) {
    dma_display->fillScreen(dma_display->color565(255, 0,0));
  } else {
    dma_display->fillScreen(dma_display->color565(255, 255,255));
  }
  // dma_display->drawPixel(pos % 64, pos / 64, dma_display->color444(14,15,15));
  // pos++;
  // if (pos >= 64*64) {
  //   pos = 0;
  // }
  // delay(1);
  delay(100);
}
