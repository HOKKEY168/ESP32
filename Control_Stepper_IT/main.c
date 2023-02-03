
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"
#include "driver/timer.h"
#include "esp_timer.h"


/* Use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define TIMER_GROUP TIMER_GROUP_0
#define TIMER_ID TIMER_0
#define TIMER_INTERVAL_SEC 1
#define TIMER_DIVIDER         (80)  //  Hardware timer clock divider
#define TIMER_SCALE           (APB_CLK_FREQ  / TIMER_DIVIDER)  // convert counter value to seconds

#define STEP 2

typedef struct {
    int timer_group;
    int timer_idx;
    int alarm_interval;
    bool auto_reload;
} timer_info_t;

void STEP_Init(void){ // initial value
	         gpio_config_t io_config = {
			.mode = GPIO_MODE_OUTPUT,
			.pin_bit_mask = (1ULL<<STEP),
			.pull_down_en = false,
			.pull_up_en = false,
			.intr_type = GPIO_INTR_DISABLE,
	};
	gpio_config(&io_config);
}
static void IRAM_ATTR timer_isr(void *para)   // callback fountion -> base on GPT document
{
    /* Interrupt code here */

  gpio_set_level(STEP, 1);

    uint32_t timer_intr = timer_group_get_intr_status_in_isr(TIMER_GROUP_0);// gets interrupt flag
      if (timer_intr & TIMER_INTR_T0) {// if interrupt status is bit 0, timer 0 interrupt triggered, TIMER_INTR_T0 defined in API at bottom
//         printf("Interrupt triggered!\n");;// how many times interrupt has occured
           timer_group_clr_intr_status_in_isr(TIMER_GROUP_0, TIMER_0);//clear interrupt flag for timer_group0, timer_0
           //timer_counter_value += (uint64_t) (TIMER_INTERVAL0_SEC * TIMER_SCALE);
           //timer_group_set_alarm_value_in_isr(TIMER_GROUP_0, timer_idx, timer_counter_value);// probably dont need to set this if auto reload is enabled.
       } else if (timer_intr & TIMER_INTR_T1) {// if interrupt status is bit 1, timer 1 interrupr triggered
           timer_group_clr_intr_status_in_isr(TIMER_GROUP_0, TIMER_1);
       }
       timer_group_enable_alarm_in_isr(TIMER_GROUP_0, TIMER_ID);// might have to reset alarm enable everytime interrupt triggers?( to be confirmed)

}
void TIM_Init(void){
    timer_config_t config = {
        .alarm_en = TIMER_ALARM_EN,
        .counter_en = TIMER_PAUSE,
        .intr_type = TIMER_INTR_LEVEL,
        .counter_dir = TIMER_COUNT_UP,
        .auto_reload = TIMER_AUTORELOAD_EN,
        .divider = TIMER_DIVIDER,
    };
    timer_init(TIMER_GROUP, TIMER_ID, &config);
    /* Timer's counter will initially start from value below.
       Also, if auto_reload is set, this value will be automatically reload on alarm */
    timer_set_counter_value(TIMER_GROUP, TIMER_ID, 0x00000000ULL);
    timer_set_alarm_value(TIMER_GROUP, TIMER_ID, TIMER_SCALE);
    timer_enable_intr(TIMER_GROUP, TIMER_ID);
//
//    timer_isr_callback_add(TIMER_GROUP, TIMER_ID, timer_isr, NULL, 0);
    timer_isr_register(TIMER_GROUP, TIMER_ID, timer_isr, NULL, ESP_INTR_FLAG_IRAM, NULL);
    timer_start(TIMER_GROUP, TIMER_ID); //start timer
}


void set_timer_frequency(int frequency)
{
    timer_pause(TIMER_GROUP, TIMER_ID);
//    timer_set_divider(TIMER_GROUP, TIMER_ID, 80);
    timer_set_alarm_value(TIMER_GROUP, TIMER_ID,(int64_t) TIMER_SCALE/frequency);
    timer_enable_intr(TIMER_GROUP, TIMER_ID);
    timer_start(TIMER_GROUP, TIMER_ID);
}


void app_main(void)
{
//  esp_timer_init();
    /* Configure the peripheral according to the LED type */
//    configure_led();
    TIM_Init();
    set_timer_frequency(100);
    STEP_Init();

//    vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
//    while (1) {
//        ESP_LOGI(TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");
//        blink_led();
//        /* Toggle the LED state */
//        s_led_state = !s_led_state;
//        vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
    }
//}
