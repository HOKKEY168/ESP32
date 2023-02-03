
//Group Name: HUT Hokkey      (ID:e20180335)
//            HAK Menghour    (ID:e20180239)
//            CHHUN Pichpisal (ID:20180149)
//______________________________________________
//                TP4: Remote Pracedure Call (RPC)
//   a: Read button state on Thingboard, update LED in ESP32
//_______________________________________________

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "nvs_flash.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_tls.h"
#include "connect.h"
#include "cJSON.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

#define TAG "TB_MQTT_EXAMPLE"

#define PRO_CORE  0
#define APP_CORE  1

#define TB_ROOT_URI         "mqtt://demo.thingsboard.io"
#define TB_ACCESS_TOKEN     "9lQfz4Rg47Y5nXqM4qGU"
#define TB_TELEMETRY_TOPIC  "v1/devices/me/telemetry"
#define TB_RPC_SUBCRIBE_TO_REQUEST_TOPIC        "v1/devices/me/rpc/request"

#define WIFI_SSID           "TEKK"
#define WIFI_PASS           "44445555"
#define TAG_BTN "main"


esp_mqtt_client_handle_t mqtt_client;
char mqtt_msg[128];


#define LED_GPIO 5

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


static void tb_send_telemetry(cJSON *telemetry)
{
  char *telemetry_string = cJSON_PrintUnformatted(telemetry);
  ESP_LOGI(TAG, "Telemetry data: %s", telemetry_string);
  esp_mqtt_client_publish(mqtt_client, TB_TELEMETRY_TOPIC, telemetry_string,
                          strlen(telemetry_string), 1, 0);
}

static void tb_handle_rpc_command(cJSON *rpc)
{
	uint8_t state;
  char *method_s = cJSON_GetStringValue(cJSON_GetObjectItem(rpc, "method"));
  printf("method: %s\n", method_s);
  if (strcmp(method_s, "LED_switch") == 0)
  {
    char *params_s = cJSON_Print(cJSON_GetObjectItem(rpc, "params"));
    printf("params: %s\n", params_s);
    if (strcmp(params_s, "true") == 0)
    {
    	gpio_set_level(LED_GPIO, 1);
    }
    else if (strcmp(params_s, "false") == 0){
    	gpio_set_level(LED_GPIO, 0);
    }
  }

}

static void tb_handle_mqtt_data(const char * topic)
{
  /* Check whether RPC topic exists in the received topic. The reason being
   * the received RPC topic contains id field, i.e "v1/devices/me/rpc/request/123".
   * Since we don't care about the id field, we just find the if substring
   * "v1/devices/me/rpc/request" is included */
  if (strstr(topic, TB_RPC_SUBCRIBE_TO_REQUEST_TOPIC) != NULL)
  {
    cJSON *rpc = cJSON_Parse(mqtt_msg);

    if (rpc != NULL)
    {
      tb_handle_rpc_command(rpc);
    }
    cJSON_Delete(rpc);
  }
}

static esp_err_t _mqtt_event_handler(esp_mqtt_event_handle_t event) {
  assert(event != NULL);

  switch (event->event_id) {
  case MQTT_EVENT_CONNECTED:
    ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

    ESP_LOGI(TAG, "Subscribing to " TB_RPC_SUBCRIBE_TO_REQUEST_TOPIC "/+");
    esp_mqtt_client_subscribe(mqtt_client, TB_RPC_SUBCRIBE_TO_REQUEST_TOPIC "/+", 1);
    break;
  case MQTT_EVENT_DISCONNECTED:
    ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
    break;
  case MQTT_EVENT_SUBSCRIBED:
    ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
    break;
  case MQTT_EVENT_UNSUBSCRIBED:
    ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
    break;
  case MQTT_EVENT_PUBLISHED:
    ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
    break;
  case MQTT_EVENT_DATA:
    ESP_LOGI(TAG, "MQTT_EVENT_DATA, topic=%s, data=%.*s", event->topic,
             event->data_len, event->data);
    if (event->data_len >= (sizeof(mqtt_msg) - 1)) {
      ESP_LOGE(TAG,
          "Received MQTT message size [%d] more than expected [%d]",
          event->data_len, (sizeof(mqtt_msg) - 1));
      return ESP_FAIL;
    }

    memcpy(mqtt_msg, event->data, event->data_len);
    mqtt_msg[event->data_len] = 0; // add null terminator at the end of string

    tb_handle_mqtt_data(event->topic);

    break;
  case MQTT_EVENT_ERROR:
    ESP_LOGD(TAG, "MQTT_EVENT_ERROR");
    break;
  case MQTT_EVENT_BEFORE_CONNECT:
    ESP_LOGD(TAG, "MQTT_EVENT_BEFORE_CONNECT");
    break;
  default:
    break;
  }
  return ESP_OK;
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
		int potentiometerValue = adc1_get_raw(ADC1_CHANNEL_0);
		printf("value from potentiometer: %d\n",potentiometerValue);
		cJSON *telemetry = cJSON_CreateObject();
		cJSON_AddNumberToObject(telemetry, "Potentiometer", potentiometerValue);;
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





void app_main(void)
{
  ESP_ERROR_CHECK(nvs_flash_init());
  wifi_init();
  ESP_ERROR_CHECK(wifi_connect_sta(WIFI_SSID, WIFI_PASS, 10000));

  gpio_config_t config;
         	config.pin_bit_mask = (1ULL << LED_GPIO);
         	config.mode = GPIO_MODE_OUTPUT;
         	config.pull_down_en = GPIO_PULLDOWN_DISABLE;
         	config.pull_up_en = GPIO_PULLUP_DISABLE;
         	config.intr_type = GPIO_INTR_DISABLE;
         	gpio_config(&config);

  /* Start MQTT client */
  esp_mqtt_client_config_t mqtt_config = {
      .uri = TB_ROOT_URI,
      .port = 1883,
      .event_handle = _mqtt_event_handler,
      .username = TB_ACCESS_TOKEN,
      .keepalive = 10 /* keep the MQTT connection alive only 10 seconds after
                      Internet disconnected. Reconnect to MQTT broker when Internet is back.
                      It's a good practice to set this value between 10 - 60 seconds */
  };

  mqtt_client = esp_mqtt_client_init(&mqtt_config);
  esp_mqtt_client_start(mqtt_client);

  xTaskCreatePinnedToCore(vTaskReadADC, "Potentiometer", 2* 1024,NULL , 11, NULL, PRO_CORE);
  xTaskCreatePinnedToCore(vButtonTask, "Button Task", 4*1024, NULL ,11, NULL, PRO_CORE);
}
