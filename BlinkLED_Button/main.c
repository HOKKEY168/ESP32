
//Group Name: CHHUN Pichpisal (ID:20180149)
//            HAK Menghour    (ID:e20180239)
//            HUT Hokkey      (ID:e20180335)
//______________________________________________

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <freertos/queue.h>
#include "driver/gpio.h"
#include "task_stats.h"

#define TAG "main"

#define CORE_PRO 0
#define CORE_APP 1


#define LED1_GPIO 6
#define LED2_GPIO 7

TaskHandle_t TaskHandle_L1;
TaskHandle_t TaskHandle_L2;

int old_delay = 500;
int new_delay;

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

void vL1Task(void *pvParameters); //prototype function when use with callback function
static void buttonPressCallback(int btn_idx, eButtonState_t btn_state) {
	// handle button event here

	if (btn_idx == 0 && btn_state == BUTTON_LONG_PRESSED) {

		if (TaskHandle_L1 != NULL){
			vTaskSuspend(TaskHandle_L1);
			printf("Suspended");
			vTaskDelete(TaskHandle_L1);
		}
		else {
			vTaskResume(TaskHandle_L1);
			printf("Resumed");
		}


	}

	else if (btn_idx == 1 && btn_state == BUTTON_PRESSED) {

		if ( old_delay == 500) {
			new_delay = old_delay - 100;
			old_delay = new_delay;
			printf("blink rate is 400ms");
		}

		else if (old_delay == 400) {
			new_delay = old_delay - 100;
			old_delay = new_delay;
			printf("Blink rate is 300ms");
		}

		else if (old_delay == 300) {
				new_delay = old_delay - 100;
				old_delay = new_delay;
				printf("Blink rate is 200ms");
		}

		else if (old_delay == 200) {
				new_delay = old_delay - 100;
				old_delay = new_delay;
				printf("Blink rate is 100ms");
		}

		else if (old_delay == 100) {
				new_delay = old_delay - 100; //new_delay = 0 so blink rate is 0 too
				old_delay = 500;
				printf("Reset to 500ms");
		}

	}
}
void vButtonTask(void *pvParameters)
{
	ButtonConfig_t button[2];
	for (int i = 0; i < 2; i++)
	{
		button[i] = BUTTON_CONFIG_DEFAULT;
	}

	button[0].gpio = GPIO_NUM_15;
	button[1].gpio = GPIO_NUM_18;


	gpio_config_t config;
	for  (int i = 0; i < 2; i++){
	config.pin_bit_mask = (1ULL << button[i].gpio);
	config.mode = GPIO_MODE_INPUT;
	config.pull_down_en = 0;
	config.pull_up_en = 1;
	config.intr_type = GPIO_INTR_DISABLE;
	gpio_config(&config);}

//	gpio_set_direction(BTN_GPIO, GPIO_MODE_INPUT);
//		//Disable pull up and down
//	gpio_set_pull_mode(BTN_GPIO, GPIO_FLOATING);
//	// Initialize pull-up input for B1 and B2
//	ESP_LOGI("vButtonTask", "Initializing GPIO....");


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

							ESP_LOGI(TAG, "Button %d is pressed", i);

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

								ESP_LOGI(TAG, "button %d is long pressed", i);
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

							ESP_LOGI(TAG, "button %d is released", i);
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

void vL1Task(void *pvParameters) {
	uint8_t state = 0;

	//Initializing LED1 GPIO as output push-pull
	gpio_config_t config;
	//For configure GPIO Pin
	//config.pin_bit_mask = (1ULL << LED1_GPIO) | (1ULL << 4)) | (1ULL << 5);
	config.pin_bit_mask = (1ULL << LED1_GPIO);
	config.mode = GPIO_MODE_OUTPUT;
	config.pull_down_en = GPIO_PULLDOWN_DISABLE;
	config.pull_up_en = GPIO_PULLUP_DISABLE;
	config.intr_type = GPIO_INTR_DISABLE;
	gpio_config(&config);

//	//simple GPIO configuration
//	gpio_set_direction(LED1_GPIO, GPIO_MODE_INPUT);
//		//Disable pull up and down
//	gpio_set_pull_mode(LED1_GPIO, GPIO_FLOATING);

	while (1) {
		gpio_set_level(LED1_GPIO, state);
		state = !state;
		vTaskDelay(pdMS_TO_TICKS(100));
	}

	vTaskDelete(NULL);

}


void vL2Task(void *pvParameters) {


	uint8_t state = 0;

//	//simple GPIO configuration
//	gpio_set_direction(LED2_GPIO, GPIO_MODE_OUTPUT);
//	//Disable pull up and down
//	gpio_set_pull_mode(LED2_GPIO, GPIO_FLOATING);

	gpio_config_t config;
	config.pin_bit_mask = (1ULL << LED2_GPIO);
	config.mode = GPIO_MODE_OUTPUT;
	config.pull_down_en = GPIO_PULLDOWN_DISABLE;
	config.pull_up_en = GPIO_PULLUP_DISABLE;
	config.intr_type = GPIO_INTR_DISABLE;
	gpio_config(&config);

	while (1) {
		gpio_set_level(LED2_GPIO, state);
		state = !state;
		vTaskDelay(pdMS_TO_TICKS(old_delay));
	}

	vTaskDelete(NULL);
}


void app_main(void)

{
//	//Just the same printf in C to display string
//	ESP_LOGI("app_main", "Display simple info");
//	ESP_LOGW("app_main", "Display warning");
//	ESP_LOGE("app_main", "Display error");

	/* Create Tasks */

	xTaskCreatePinnedToCore(vButtonTask, "Button Task", 2 * 1024,NULL ,2 ,NULL ,CORE_APP);
	xTaskCreatePinnedToCore(vL1Task, "LED1 Task", 2* 1024, NULL,1 ,&TaskHandle_L1 ,CORE_APP);
	xTaskCreatePinnedToCore(vL2Task, "LED2 Task", 2* 1024, NULL,1 ,&TaskHandle_L2 ,CORE_APP);


	vTaskDelay(pdMS_TO_TICKS(1000));
	print_real_time_stats(pdMS_TO_TICKS(1000));
	vTaskDelete(NULL);
}

