#ifndef _CAMERA_H
#define _CAMERA_H

#define ESP32CAM
static const char *TAG0 = "Camera";
//#define BOARD_ESP32CAM_AITHINKER 1
// ESP32Cam (AiThinker) PIN Map
#ifdef BOARD_ESP32CAM_AITHINKER

#define CAM_PIN_PWDN -1
#define CAM_PIN_RESET -1 //software reset will be performed
#define CAM_PIN_XCLK 10
#define CAM_PIN_SIOD 21
#define CAM_PIN_SIOC 14

#define CAM_PIN_D7 11
#define CAM_PIN_D6 9
#define CAM_PIN_D5 8
#define CAM_PIN_D4 16
#define CAM_PIN_D3 06
#define CAM_PIN_D2 04
#define CAM_PIN_D1 05
#define CAM_PIN_D0 17
#define CAM_PIN_VSYNC 13
#define CAM_PIN_HREF 12
#define CAM_PIN_PCLK 18

#endif
#ifdef ESP32CAM
#define CAM_PIN_PWDN 32
#define CAM_PIN_RESET -1 //software reset will be performed
#define CAM_PIN_XCLK 0
#define CAM_PIN_SIOD 26
#define CAM_PIN_SIOC 27

#define CAM_PIN_D7 35
#define CAM_PIN_D6 34
#define CAM_PIN_D5 39
#define CAM_PIN_D4 36
#define CAM_PIN_D3 21
#define CAM_PIN_D2 19
#define CAM_PIN_D1 18
#define CAM_PIN_D0 5
#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF 23
#define CAM_PIN_PCLK 22

#endif
#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

#endif
