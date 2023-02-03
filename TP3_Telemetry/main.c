
//Group Name: HUT Hokkey      (ID:e20180335)
//            HAK Menghour    (ID:e20180239)
//            CHHUN Pichpisal (ID:20180149)
//______________________________________________
//                TP3: Telemetry
//   a: Read ADC every 5s, then send to Thingboard.
//   b: Read Button state and send to ThingBoard on state charge
//_______________________________________________

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_tls.h"
#include "connect.h"
#include "cJSON.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

#define TAG_BTN "main"
#define TAG 	"ESP_HTTP_CLIENT"

#define PRO_CORE  0
#define APP_CORE  1

#define TB_ROOT_URI       "http://demo.thingsboard.io/api/v1/"
#define TB_ACCESS_TOKEN   "FKNrUVIa8v6VgchrPfnj"
#define TB_URL_TELEMETRY  TB_ROOT_URI TB_ACCESS_TOKEN "/telemetry"
#define TB_URL_RPC        TB_ROOT_URI TB_ACCESS_TOKEN "/rpc?timeout=500"

#define WIFI_SSID "TEKK:)"
#define WIFI_PASS "44445555"


uint8_t state;
#define LED_GPIO 2
QueueHandle_t QLED;

typedef enum {
	BUTTON_PRESSING,
	BUTTON_PRESSED,
	BUTTON_LONG_PRESSED,
	BUTTON_RELEASING,
	BUTTON_RELEASED
} eButtonState_t;


typedef struct {
	uint8_t gpio;
	uint8_t value;
	uint8_t longPressCounter;
	uint16_t longPressDuration;
	eButtonState_t nextState;
	eButtonState_t currentState;
} ButtonConfig_t;

ButtonConfig_t BUTTON_CONFIG_DEFAULT = {
		.gpio = GPIO_NUM_MAX,
		.value = 1,
		.longPressCounter = 0,
		.longPressDuration = 60, // duration * delay = 60 * 50 = 3000 millisecond
		.nextState = BUTTON_RELEASED,
		.currentState = BUTTON_RELEASED,
};

static esp_err_t _http_event_handler (esp_http_client_event_t *evt)
{
  static char *output_buffer; // Buffer to store response of http request from event handler
  static int output_len;       // Stores number of bytes read
  switch (evt->event_id)
  {
    case HTTP_EVENT_ERROR:
      ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
      break;
    case HTTP_EVENT_ON_CONNECTED:
      ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
      break;
    case HTTP_EVENT_HEADER_SENT:
      ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
      break;
    case HTTP_EVENT_ON_HEADER:
      ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key,
               evt->header_value);
      break;
    case HTTP_EVENT_ON_DATA:
      ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
      /*
       *  Check for chunked encoding is added as the URL for chunked encoding used,
       *  returns binary data.
       *  However, event handler can also be used in case chunked encoding is used.
       */
      if (!esp_http_client_is_chunked_response (evt->client))
      {
        // If user_data buffer is configured, copy the response into the buffer
        if (evt->user_data)
        {
          memcpy(evt->user_data + output_len, evt->data, evt->data_len);
        }
        else
        {
          if (output_buffer == NULL)
          {
            output_buffer = (char*) malloc(esp_http_client_get_content_length(evt->client));
            output_len = 0;
            if (output_buffer == NULL)
            {
              ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
              return ESP_FAIL;
            }
          }
          memcpy(output_buffer + output_len, evt->data, evt->data_len);
        }
        output_len += evt->data_len;
      }

      break;
    case HTTP_EVENT_ON_FINISH:
      ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
      if (output_buffer != NULL)
      {
        // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
        // ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
        free (output_buffer);
        output_buffer = NULL;
      }
      output_len = 0;
      break;
    case HTTP_EVENT_DISCONNECTED:
    {
//      ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
      int mbedtls_err = 0;
      esp_err_t err = esp_tls_get_and_clear_last_error(evt->data, &mbedtls_err,
                                                        NULL);
      if (err != 0)
      {
        ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
        ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
      }
      if (output_buffer != NULL)
      {
        free(output_buffer);
        output_buffer = NULL;
      }
      output_len = 0;
      break;
    }
  }
  return ESP_OK;
}

static void tb_send_telemetry(cJSON *json)
{
  esp_http_client_config_t esp_http_client_config = {
      .url = TB_URL_TELEMETRY,
      .method = HTTP_METHOD_POST,
      .event_handler = _http_event_handler
  };

  esp_http_client_handle_t client = esp_http_client_init(
      &esp_http_client_config);

  char *data = cJSON_PrintUnformatted(json);
  printf("Telemetry data: %s\n", data);

  esp_http_client_set_header(client, "Content-Type", "application/json");
  esp_http_client_set_post_field(client, data, strlen(data));
  esp_err_t err = esp_http_client_perform(client);
  if (err == ESP_OK)
  {
    ESP_LOGI(TAG, "HTTP POST Telemetry status = %d, content_length = %d",
             esp_http_client_get_status_code(client),
             esp_http_client_get_content_length(client));
  }
  else
  {
    ESP_LOGE(TAG, "HTTP POST Telemetry request failed: %s", esp_err_to_name(err));
  }
  esp_http_client_cleanup(client);
}

static esp_err_t tb_fetch_rpc(cJSON *json)
{
  esp_err_t ret_err = ESP_FAIL;

  char data[80] = {0};

  esp_http_client_config_t esp_http_client_config = {
      .url = TB_URL_RPC,
      .method = HTTP_METHOD_GET,
      .event_handler = _http_event_handler,
      .user_data = data
  };

  esp_http_client_handle_t client = esp_http_client_init(
      &esp_http_client_config);

  esp_http_client_set_header(client, "Content-Type", "application/json");
  esp_err_t err = esp_http_client_perform(client);
  if (err == ESP_OK)
  {
    ESP_LOGI(TAG, "HTTP GET RPC status = %d, content_length = %d",
             esp_http_client_get_status_code(client),
             esp_http_client_get_content_length(client));
    int status = esp_http_client_get_status_code(client);
    if (status == 200)
    {
      printf ("RPC info: %s\n", data);
      memcpy(json, cJSON_Parse(data), sizeof(cJSON));
      ret_err = ESP_OK;
    }
  }
  else
  {
    ESP_LOGE(TAG, "HTTP GET RPC request failed: %s", esp_err_to_name(err));
  }
  esp_http_client_cleanup(client);
  return ret_err;
}

static void buttonPressCallback(int btn_idx, eButtonState_t btn_state) {
	// handle button event here

	if (btn_idx == 1 && btn_state == BUTTON_PRESSED) {
		  cJSON *telemetry = cJSON_CreateObject();
		  cJSON_AddNumberToObject(telemetry, "Button_Pressed", 1);
		  tb_send_telemetry(telemetry);
		  cJSON_Delete(telemetry);

	}

	else if (btn_idx == 1 && btn_state == BUTTON_RELEASED) {

		  cJSON *telemetry = cJSON_CreateObject();
		  cJSON_AddNumberToObject(telemetry, "Button_Released", 0);
		  tb_send_telemetry(telemetry);
		  cJSON_Delete(telemetry);

	}
}

void vTaskReadADC (void *pvParameters) {
	/* Initialing ADC GPIO */
	adc1_config_width(ADC_WIDTH_12Bit);
	adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);

	while (1){
		int SoilMoistureValue = adc1_get_raw(ADC1_CHANNEL_0);
		printf("value from Moisture Sensor: %d\n",SoilMoistureValue);
		cJSON *telemetry = cJSON_CreateObject();
		cJSON_AddNumberToObject(telemetry, "ADC_Reader", SoilMoistureValue);;
		tb_send_telemetry(telemetry);
		vTaskDelay(pdMS_TO_TICKS(5000));
	}

	vTaskDelete(NULL);
}

void vButtonTask(void *pvParameters)
{
	ButtonConfig_t button[2];
	for (int i = 0; i < 2; i++)
	{
		button[i] = BUTTON_CONFIG_DEFAULT;
	}

	button[0].gpio = GPIO_NUM_18;
	button[1].gpio = GPIO_NUM_15;


	gpio_config_t config;
	for  (int i = 0; i < 2; i++){
	config.pin_bit_mask = (1ULL << button[i].gpio);
	config.mode = GPIO_MODE_INPUT;
	config.pull_down_en = 0;
	config.pull_up_en = 1;
	config.intr_type = GPIO_INTR_DISABLE;
	gpio_config(&config);}

	while (1) {

			//Handle button read and states

			for (int i = 0; i < 2; i++)
			{
				button[i].value = gpio_get_level(button[i].gpio);

				switch (button[i].currentState)
				{

					case BUTTON_RELEASED:
					{
						if (button[i].value == 0)
						{
							button[i].nextState = BUTTON_PRESSING;
						}
						break;
					}
					case BUTTON_PRESSING:
					{
						if (button[i].value == 0)
						{
							button[i].nextState = BUTTON_PRESSED;

							ESP_LOGI(TAG_BTN, "Button %d is pressed", i);

							buttonPressCallback(i, button[i].nextState);
						}
						else
						{
							button[i].nextState = BUTTON_RELEASED;
						}
						break;
					}

					case BUTTON_PRESSED:
					{
						if (button[i].value == 0)
						{
							button[i].longPressCounter++;
							if (button[i].longPressCounter >= button[i].longPressDuration)
							{
								button[i].nextState = BUTTON_LONG_PRESSED;
								button[i].longPressCounter = 0;

								ESP_LOGI(TAG_BTN, "button %d is long pressed", i);
								buttonPressCallback(i, button[i].nextState);
							}
						}
						else {
							button[i].nextState = BUTTON_RELEASING;
							button[i].longPressCounter = 0;
						}
					break;

					}

					case BUTTON_LONG_PRESSED:
					{
						if (button[i].value == 1)
						{
							button[i].nextState = BUTTON_RELEASING;
						}
						break;
					}

					case BUTTON_RELEASING:
					{
						if (button[i].value == 1)
						{
							button[i].nextState = BUTTON_RELEASED;

							ESP_LOGI(TAG_BTN, "button %d is released", i);
							buttonPressCallback(i, button[i].nextState);
						}
						break;
					}

			}
			button[i].currentState = button[i].nextState;
			vTaskDelay(pdMS_TO_TICKS(50));
		}

	}
	vTaskDelete(NULL);

}


void vTbRpcTask(void *pvParameters)
{
  cJSON *root = cJSON_CreateObject();
  TickType_t previousWakeTime = xTaskGetTickCount();
  while (1)
  {
    if (tb_fetch_rpc(root) == ESP_OK)
    {
      int id = cJSON_GetNumberValue(cJSON_GetObjectItem(root, "id"));
      char* method = cJSON_GetStringValue(cJSON_GetObjectItem(root, "method"));
      char* params = cJSON_Print(cJSON_GetObjectItem(root, "params"));

      if (strcmp(method, "Button") == 0)
      {
        if (strcmp(params, "true") == 0)
        {
        	state = 0;
        	xQueueSend(QLED, &state, 0);
        }
        else if (strcmp(params, "false") == 0)
         {
        	state = 1;
         	xQueueSend(QLED, &state, 0);
         }
      }

    }

    vTaskDelayUntil(&previousWakeTime, pdMS_TO_TICKS(1000));
  }

  cJSON_Delete(root);
  vTaskDelete(NULL);
}
void vTaskLED () {
	gpio_config_t config;
       	config.pin_bit_mask = (1ULL << LED_GPIO);
       	config.mode = GPIO_MODE_OUTPUT;
       	config.pull_down_en = GPIO_PULLDOWN_DISABLE;
       	config.pull_up_en = GPIO_PULLUP_DISABLE;
       	config.intr_type = GPIO_INTR_DISABLE;
       	gpio_config(&config);
       	while (1) {

       		xQueueReceive(QLED, &state, portMAX_DELAY);
	        state =! state;
	        gpio_set_level(LED_GPIO, state);
	        vTaskDelay(pdMS_TO_TICKS(50));
       	}
       	vTaskDelete(NULL);
}

void app_main(void)
{
  ESP_ERROR_CHECK(nvs_flash_init());
  wifi_init();
  ESP_ERROR_CHECK(wifi_connect_sta(WIFI_SSID, WIFI_PASS, 10000));

//  cJSON *telemetry = cJSON_CreateObject();
//  cJSON_AddNumberToObject(telemetry, "temperature", 107.5);
//  cJSON_AddStringToObject(telemetry, "device_name", "ESP32");
//  cJSON_AddBoolToObject(telemetry, "button_state", true);
//
//  tb_send_telemetry(telemetry);
//
//  cJSON_Delete(telemetry);

  QLED = xQueueCreate(10, sizeof(int));
  xTaskCreatePinnedToCore(vTbRpcTask, "TB RPC Task", 4*1024, NULL, 12, NULL, PRO_CORE);
  xTaskCreatePinnedToCore(vButtonTask, "Button Task", 4*1024, NULL ,11, NULL, PRO_CORE);
  xTaskCreatePinnedToCore(vTaskLED, "LED Task", 2*1024, NULL ,12, NULL, PRO_CORE);
  xTaskCreatePinnedToCore(vTaskReadADC, "Read Moisture", 2* 1024,NULL , 11, NULL, PRO_CORE);

  vTaskDelete(NULL);

}
