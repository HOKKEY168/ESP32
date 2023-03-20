#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "conection.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "esp_http_client.h"
#include <esp_http_server.h>

//#include "lwip/err.h"
//#include "lwip/sys.h"
#include "esp_camera.h"
#include "Camera.h"

static camera_config_t camera_config = {
	    .pin_pwdn  = CAM_PIN_PWDN,
	    .pin_reset = CAM_PIN_RESET,
	    .pin_xclk = CAM_PIN_XCLK,
	    .pin_sscb_sda = CAM_PIN_SIOD,
	    .pin_sscb_scl = CAM_PIN_SIOC,

	    .pin_d7 = CAM_PIN_D7,
	    .pin_d6 = CAM_PIN_D6,
	    .pin_d5 = CAM_PIN_D5,
	    .pin_d4 = CAM_PIN_D4,
	    .pin_d3 = CAM_PIN_D3,
	    .pin_d2 = CAM_PIN_D2,
	    .pin_d1 = CAM_PIN_D1,
	    .pin_d0 = CAM_PIN_D0,
	    .pin_vsync = CAM_PIN_VSYNC,
	    .pin_href = CAM_PIN_HREF,
	    .pin_pclk = CAM_PIN_PCLK,

    //XCLK 20MHz or 10MHz
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG, //YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_HVGA,   //QQVGA-UXGA Do not use sizes above QVGA when not JPEG
	.fb_location = CAMERA_FB_IN_DRAM,
	.grab_mode = CAMERA_GRAB_WHEN_EMPTY,
    .jpeg_quality = 10, //0-63 lower number means higher quality
    .fb_count = 1       //if more than one, i2s runs in continuous mode. Use only with JPEG
};

static esp_err_t init_camera()
{
    //initialize the camera
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG0, "Camera Init Failed");
        return err;
    }

    return ESP_OK;
}

esp_err_t camera_capture(){
    //acquire a frame
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
        ESP_LOGE(TAG0, "Camera Capture Failed");
        return ESP_FAIL;
    }
    //replace this with your own function
//    process_image(fb->width, fb->height, fb->format, fb->buf, fb->len);

    //return the frame buffer back to the driver for reuse
    esp_camera_fb_return(fb);
    return ESP_OK;
}
esp_err_t jpg_stream_httpd_handler(httpd_req_t *req){
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len;
    uint8_t * _jpg_buf;
    char * part_buf[64];
    static int64_t last_frame = 0;
    if(!last_frame) {
        last_frame = esp_timer_get_time();
    }

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if(res != ESP_OK){
        return res;
    }

    while(true){
        fb = esp_camera_fb_get();
        if (!fb) {
            ESP_LOGE(TAG0, "Camera capture failed");
            res = ESP_FAIL;
            break;
        }
        if(fb->format != PIXFORMAT_JPEG){
            bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
//            frame2jpg(fb, quality, out, out_len))
            if(!jpeg_converted){
                ESP_LOGE(TAG0, "JPEG compression failed");
                esp_camera_fb_return(fb);
                res = ESP_FAIL;
            }
        } else {
            _jpg_buf_len = fb->len;
            _jpg_buf = fb->buf;
        }

        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }
        if(res == ESP_OK){
            size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);

            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        }
        if(fb->format != PIXFORMAT_JPEG){
            free(_jpg_buf);
        }
        esp_camera_fb_return(fb);
        if(res != ESP_OK){
            break;
        }
        int64_t fr_end = esp_timer_get_time();
        int64_t frame_time = fr_end - last_frame;
        last_frame = fr_end;
        frame_time /= 1000;
        ESP_LOGI(TAG0, "MJPG: %uKB %ums (%.1ffps)",
            (uint32_t)(_jpg_buf_len/1024),
            (uint32_t)frame_time, 1000.0 / (uint32_t)frame_time);
    }

    last_frame = 0;
    return res;
}
static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.lru_purge_enable = true;

    httpd_uri_t index_uri = {
      .uri       = "/",
      .method    = HTTP_GET,
      .handler   = jpg_stream_httpd_handler,
      .user_ctx  = NULL
    };
    // Start the httpd server
    ESP_LOGI(TAG0, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG0, "Registering URI handlers");
        httpd_register_uri_handler(server, &index_uri);
        return server;
    }

    ESP_LOGI(TAG0, "Error starting server!");
    return NULL;
}

static void stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    httpd_stop(server);
}

//static void Jpeg_Stream_Server(void *pvParameters) {
//
//
//	vTaskDelay(pdMS_TO_TICKS(10));
//	vTaskDelete(NULL);
//}
static httpd_handle_t server = NULL;


void app_main(void)
{
	esp_timer_init();
	init_camera();
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG1, "ESP_WIFI_MODE_STA");
//    wifi_init_sta();
    wifi_init_softap();

    server = start_webserver();

}
