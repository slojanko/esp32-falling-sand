#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "SD.h"
#include "FS.h"

#define PANEL_RES_X 64
#define PANEL_RES_Y 64
#define PANEL_CHAIN 1

#define SAND_COUNT 1024
#define SAND_SCENE_SCALE 256
#define SAND_SCENE_X (PANEL_RES_Y * SAND_SCENE_SCALE - 1) // Highest allowed x in sand scene
#define SAND_SCENE_Y (PANEL_RES_Y * SAND_SCENE_SCALE - 1) // Highest allowed y in sand scene

// Joystick
#define VRX_PIN 14
#define VRY_PIN 13

// microSD Card Reader connections
#define SD_CS         7
#define SPI_MOSI      15 
#define SPI_MISO      17
#define SPI_SCK       16

#define MAX_FPS 45
#define TIME_STEP (1000 / MAX_FPS)
// #define INCLUDE_vTaskDelay 1

struct Sand {
	int16_t x, y;
	int16_t vx, vy;
	uint16_t panelPixel;
	uint16_t colour;
}
sand[SAND_COUNT];

MatrixPanel_I2S_DMA * dma_display = nullptr;
uint16_t imagePtr[PANEL_RES_X * PANEL_RES_Y];

void setup() {
	Serial.begin(115200);

	HUB75_I2S_CFG mxconfig(
		PANEL_RES_X,
		PANEL_RES_Y,
		PANEL_CHAIN
	);
	mxconfig.gpio.e = 38;
	mxconfig.clkphase = false;

	dma_display = new MatrixPanel_I2S_DMA(mxconfig);
	dma_display -> begin();
	dma_display -> setBrightness8(90); //0-255
	dma_display -> clearScreen();
	dma_display -> setRotation(0);

	// Set microSD Card CS as OUTPUT and set HIGH
	pinMode(SD_CS, OUTPUT);      
	digitalWrite(SD_CS, HIGH); 

	// Initialize SPI bus for microSD Card
	SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

	// Start microSD Card
	if(!SD.begin(SD_CS))
	{
	Serial.println("Error accessing microSD card!");
		while(true); 
	}
	uint16_t* temp = (uint16_t*)malloc(64*64*2);
	File openFile = SD.open("/NeverGonnaGiveYouUp");
	openFile.read((uint8_t*)temp, 64*64*2);
	openFile.close();

	for (int i = 0; i < SAND_COUNT; i++) {
		uint16_t panelPixel = 0;
		// do {
		// 	sand[i].x = random(PANEL_RES_X * SAND_SCENE_SCALE);
		// 	sand[i].y = random(PANEL_RES_Y * SAND_SCENE_SCALE);

		// 	panelPixel = (sand[i].y / SAND_SCENE_SCALE) * PANEL_RES_X + (sand[i].x / SAND_SCENE_SCALE);

		// } while (imagePtr[panelPixel] != 0);

		sand[i].x = ((i * 4) % 64 + random(0, 2)) * 256;
		sand[i].y = ((i * 4) / 64 + random(0, 2)) * 256;

		panelPixel = (sand[i].y / SAND_SCENE_SCALE) * PANEL_RES_X + (sand[i].x / SAND_SCENE_SCALE);

		sand[i].panelPixel = panelPixel;
		sand[i].vx = 0;
		sand[i].vy = 0;
		sand[i].colour = temp[panelPixel];
		imagePtr[panelPixel] = sand[i].colour;
	}

	free(temp);

	// Watchdog is something that runs in background to prevent cpu core being hugged by a single task for too long
	// You can disable it per core, per task or temporarily put task to sleep
	// 2nd and 3rd ways are shown in pixelTask function but commented out
	disableCore0WDT();

	TaskHandle_t xHandle = NULL;
	xTaskCreatePinnedToCore(pixelTask, "PixelTask1", 5000, 0, (0 | tskIDLE_PRIORITY | INCLUDE_vTaskDelay), & xHandle, 0);
}

void loop() {
	dma_display -> drawRGBBitmap(0, 0, imagePtr, 64, 64);
	delay(10);
}

void pixelTask(void * param) {
	// Disable watchdog per task
	// esp_task_wdt_init(5, false);
	vTaskDelay(5000 / portTICK_PERIOD_MS);

	uint32_t prevTime = millis();
	//TickType_t xLastWakeTime = xTaskGetTickCount();

	while (true) {
		// TickType_t xDelay = (TIME_STEP - (millis() - prevTime)) / portTICK_PERIOD_MS;
		// if (xDelay > TIME_STEP) {
		//   xDelay = 1 / portTICK_PERIOD_MS;
		// }
		// vTaskDelay(xDelay);	
		TickType_t xDelay = (TIME_STEP - (millis() - prevTime)) / portTICK_PERIOD_MS;
		if (xDelay < TIME_STEP) {
			vTaskDelay(xDelay);
		}

		// This includes execution of function, so putting 100ms and logic takes 40ms, it will wait for only 60ms
		// vTaskDelayUntil(&xLastWakeTime, 100 / portTICK_PERIOD_MS);
		prevTime = millis();

		int16_t ax = ((int16_t) analogRead(VRX_PIN) - 2048) / (8 * 4);
		int16_t ay = ((int16_t) analogRead(VRY_PIN) - 2048) / (8 * 4);
		// Remove incorrect reading when joystick not in use - slight randomness in one direction
		// if (abs(ax) < 8) {
		//   ax = 0;
		// }
		// if (abs(ay) < 8) {
		//   ay = 0;
		// }

		// print data to Serial Monitor on Arduino IDE
		// Serial.print("x = ");
		// Serial.print(ax);
		// Serial.print(", y = ");
		// Serial.println(ay);

		int32_t v2;
		float v;
		// Limit max speed
		for (int i = 0; i < SAND_COUNT; i++) {
			sand[i].vx += ax + random(-2, 3);
			sand[i].vy += ay + random(-2, 3);

			v2 = (int32_t) sand[i].vx * sand[i].vx + (int32_t) sand[i].vy * sand[i].vy;
			if (v2 > 65536) {
				v = sqrt((float) v2); // Velocity vector magnitude
				sand[i].vx = (int)(256.0 * (float) sand[i].vx / v);
				sand[i].vy = (int)(256.0 * (float) sand[i].vy / v);
			}
		}

		uint16_t oldidx, newidx, delta;
		int16_t newx, newy;

		// Calculate next position, if any
		for (uint16_t i = 0; i < SAND_COUNT; i++) {
			newx = sand[i].x + sand[i].vx;
			newy = sand[i].y + sand[i].vy;

			// Clamp inside Scene
			if (newx > SAND_SCENE_X) {
				newx = SAND_SCENE_X;
				sand[i].vx /= -2;
			} else if (newx < 0) {
				newx = 0;
				sand[i].vx /= -2;
			}

			if (newy > SAND_SCENE_Y) {
				newy = SAND_SCENE_Y;
				sand[i].vy /= -2;
			} else if (newy < 0) {
				newy = 0;
				sand[i].vy /= -2;
			}

			oldidx = sand[i].panelPixel;
			newidx = (newy / SAND_SCENE_SCALE) * PANEL_RES_X + (newx / SAND_SCENE_SCALE);

			// If new pixel isn't free, try to find alternative
			if ((oldidx != newidx) && imagePtr[newidx]) {
				delta = abs(newidx - oldidx);

				// Sand tried to move only horizontally
				if (delta == 1) {
					newx = sand[i].x;
					sand[i].vx /= -2;
					sand[i].vy *= 3; // Try to topple columns
					newidx = oldidx;
				} // Sand tried to move only vertically 
				else if (delta == PANEL_RES_X) {
					newy = sand[i].y;
					sand[i].vx *= 3; // Try to topple columns
					sand[i].vy /= -2;
					newidx = oldidx;
				} // Sand tried to move diagonally, find axis with higher velocity
				else {
					// Faster horizontally
					if ((abs(sand[i].vx) - abs(sand[i].vy)) >= 0) {
						// Try to forcefully move on x
						newidx = (sand[i].y / SAND_SCENE_SCALE) * PANEL_RES_X + (newx / SAND_SCENE_SCALE);
						if (!imagePtr[newidx]) {
							newy = sand[i].y;
							sand[i].vy /= -2;
						} else {
							// Fallback to y
							newidx = (newy / SAND_SCENE_SCALE) * PANEL_RES_X + (sand[i].x / SAND_SCENE_SCALE);
							if (!imagePtr[newidx]) {
								newx = sand[i].x;
								sand[i].vx /= -2;
							} else {
								newx = sand[i].x;
								newy = sand[i].y;
								sand[i].vx /= -2;
								sand[i].vy /= -2;
								newidx = oldidx;
							}
						}
					} // Faster verticall 
					else {
						// Try to forcefully move on y
						newidx = (newy / SAND_SCENE_SCALE) * PANEL_RES_X + (sand[i].x / SAND_SCENE_SCALE);
						if (!imagePtr[newidx]) {
							newx = sand[i].x;
							sand[i].vx /= -2;
						} else {
							// Fallback to x
							newidx = (sand[i].y / SAND_SCENE_SCALE) * PANEL_RES_X + (newx / SAND_SCENE_SCALE);
							if (!imagePtr[newidx]) {
								newy = sand[i].y;
								sand[i].vy /= -2;
							} else {
								newx = sand[i].x;
								newy = sand[i].y;
								sand[i].vx /= -2;
								sand[i].vy /= -2;
								newidx = oldidx;
							}
						}
					}
				}
			}

			sand[i].x = newx;
			sand[i].y = newy;
			imagePtr[oldidx] = 0;
			imagePtr[newidx] = sand[i].colour;
			sand[i].panelPixel = newidx;
		}
	}
}