
//Group Name: HUT Hokkey      (ID:e20180335)
//            HAK Menghour    (ID:e20180239)
//            CHHUN Pichpisal (ID:20180149)
//______________________________________________
//                TP2
//Task1: Handle LED1 blink rate(timer)
//Task2: Handle ADC read every 50ms
//Task3: Handle task delay
//Note: LED on/off(start/stop timer)
//      LED blink(Blink rate(ms) change period)
//      ADC start number of cycle
//_______________________________________________

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_log.h"
#include "task_stats.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include <esp_system.h>
#include "driver/adc.h"
#include "esp_adc_cal.h"

#define TAG "main"
#define TAG_UART_CONSOLE "UART Console"
#define TAG_READ_CMD "Read_CMD_Info"
#define TAG_ADC "ADC_Info"

#define CORE_PRO 0
#define CORE_APP 1
#define DEFAULT_VREF 1107
#define LED1_GPIO 14

TimerHandle_t BlinkLedTimer;
TimerHandle_t ADCTimer;


uint8_t count = 0;
bool led_state = 0;
uint16_t period_long = 0, period_short = 0;
uint16_t sample_time;
uint16_t num_of_cycle;
uint16_t iter;
uint32_t adc_reading = 0;
int16_t ch_id = -1;
uint32_t val_type;


static esp_adc_cal_characteristics_t *adc_chars;

#if CONFIG_IDF_TARGET_ESP32
static const adc_channel_t channel[8] = {
		ADC_CHANNEL_0,
		ADC_CHANNEL_1,
		ADC_CHANNEL_2,
		ADC_CHANNEL_3,
		ADC_CHANNEL_4,
		ADC_CHANNEL_5,
		ADC_CHANNEL_6,
		ADC_CHANNEL_7
};
static const adc_bits_width_t width = ADC_WIDTH_BIT_12;
#endif

static const adc_atten_t atten = ADC_ATTEN_DB_11;
static const adc_unit_t unit = ADC_UNIT_1;


void adc_init(adc_channel_t adc_cha) {

	/* ADC Config */
	adc1_config_width(width);
	adc1_config_channel_atten(adc_cha, atten);
	adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
	esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, atten, width, DEFAULT_VREF, adc_chars);
}

void vTaskReadADC(char *m) {  //Read cmd from UART
	char *arg0, *arg1;
	const char delim[2] = " ";
	char *cmd = strtok(m, delim);

	if (cmd != NULL){

		if (strcmp(cmd,"blink") == 0) {
			arg0 = strtok(NULL,delim);
			arg1 = strtok(NULL,delim);

			if (arg1 != NULL) {
				period_short = atoi(arg0); //atoi convert string to integer
				period_long = atoi(arg1);
				xTimerStart(BlinkLedTimer,0);
				ESP_LOGI(TAG_READ_CMD, "LED is blinked with BlinkRate is [%d] and Stop [%d]\n", period_short,period_long);
			}
			else if (strcmp(arg0,"on")==0) {

				if (period_short == 0 || period_long == 0) {
					period_short = 100;
					period_long = 1000;
				}
				xTimerStart(BlinkLedTimer,0);
				ESP_LOGI(TAG_READ_CMD, "LED is ON with default BlinkRate is [100] and stop [1000]\n");
			}
			else if (strcmp(arg0,"off")==0){
				xTimerStop(BlinkLedTimer,0);
				ESP_LOGI(TAG_READ_CMD, "LED is OFF\n");
			}
		}


		else if (strcmp(cmd, "adc")==0){
			arg0 = strtok(NULL,delim);
			arg1 = strtok(NULL,delim);
			sample_time = atoi(arg0);
			num_of_cycle = atoi(arg1);

			if (ch_id == -1){
				ch_id = 0;
				adc_init(channel[ch_id]);
			}
			xTimerStart(ADCTimer,0);
			ESP_LOGI(TAG_READ_CMD, "ADC Command recieved ADC Channel 0: sample_time: %d and num_of_cycle: %d", sample_time, num_of_cycle);
		}

		else {
			printf("Command is not matched!!\n\n");
		}
	}
}

void vTaskHandleUART (void *pvParameters){
	char input_cmd[128];
	uint8_t count = 0;

	while (1) {
		uint8_t ch;
		ch = getchar();

		if (ch != 0xff) {
			//echo character to the console
			putchar(ch);
			if (ch == '\n') {
				printf("\n");
				input_cmd[count] = '\0';
				count = 0;
				ESP_LOGI(TAG_UART_CONSOLE, "%s", input_cmd);
				vTaskReadADC((char*)&input_cmd);
			}
			else if (ch == '\b'){
				if (count > 0) {
					count--;
					input_cmd[count] = '\0';
					putchar(' ');
					putchar('\b');
				}
			}
			else {
				input_cmd[count] = ch;
				count++;
			}
		}

		vTaskDelay(pdMS_TO_TICKS(10));
	}
}



void vTaskBlinkLED (TimerHandle_t xTimer) {
	if (count == 0) {
		xTimerChangePeriod(xTimer,pdMS_TO_TICKS(period_short), 0);

	}
	else if (count == 8) {
		xTimerChangePeriod(xTimer,pdMS_TO_TICKS(period_long), 0);
		count = 0;
		led_state = 0;
		gpio_set_level(LED1_GPIO, led_state); //
		return;
	}

	count++;
	led_state = !led_state;
	gpio_set_level(LED1_GPIO, led_state);
}

void adc_callback (TimerHandle_t xTimer) {
	if (iter < num_of_cycle) {
		adc_reading += adc1_get_raw((adc1_channel_t)channel[ch_id]);
		xTimerChangePeriod(xTimer, pdMS_TO_TICKS(sample_time),0);
		iter++;
		printf("ADC Reading = %d and Num_of_Cycle = %d\n",adc_reading, iter);
	}
	else if (iter == num_of_cycle) {
		adc_reading /= num_of_cycle;
		uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
		xTimerStop(ADCTimer,0);
		ESP_LOGI(TAG_ADC, ">>> ADC Channel[%d]:  Raw: %d  Voltage: %dmV\n", ch_id,adc_reading, voltage);
		adc_reading = 0;
		iter = 0;
	}

}

void app_main(void)
{
	/* Create Task Here */
	xTaskCreatePinnedToCore(vTaskHandleUART, " vTaskHandleUART", 2* 1024,NULL , 10, NULL, CORE_APP);


	//	xTaskCreatePinnedToCore(vTaskHandleUART, "UART Task", 2 * 1024, NULL, 10, NULL, CORE_APP);
	//	xTaskCreatePinnedToCore(vTaskReadADC, "ADC Task", 2* 1024,NULL , 10, NULL, CORE_APP);


	/* Create Timer Here */
	/* Initialize GPIO for blink LED */
	gpio_config_t config;
	config.pin_bit_mask = (1ULL << LED1_GPIO);
	config.mode = GPIO_MODE_OUTPUT;
	config.pull_down_en = GPIO_PULLDOWN_DISABLE;
	config.pull_up_en = GPIO_PULLUP_DISABLE;
	config.intr_type = GPIO_INTR_DISABLE;
	gpio_config(&config);


	BlinkLedTimer = xTimerCreate("Blink LED Timer", pdMS_TO_TICKS(1000), pdTRUE,vTaskBlinkLED, vTaskBlinkLED);
	ADCTimer = xTimerCreate("ADC Timer", pdMS_TO_TICKS(10), pdTRUE, ADCTimer, adc_callback);

	/* Print Real Time stats */
	vTaskDelay(pdMS_TO_TICKS(1000));
	print_real_time_stats(pdMS_TO_TICKS(1000));
	vTaskDelete(NULL);
}
