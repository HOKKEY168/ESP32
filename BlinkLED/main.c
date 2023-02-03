#include <stdio.h>
#include <stdbool.h>
#include "driver/ledc.h"
#include "rom/ets_sys.h"

bool state = false;
#define LED 46
#define time 1000

void LED_init(void){
	gpio_config_t io_config = {
			.mode = GPIO_MODE_OUTPUT,
			.pin_bit_mask = (1ULL<<LED),
			.pull_down_en = false,
			.pull_up_en = false,
			.intr_type = GPIO_INTR_DISABLE,
	};
	gpio_config(&io_config);
}
void app_main(void)
{
	LED_init();
    while (true) {
    	state ^= true;
    	gpio_set_level(LED,state);
    	ets_delay_us(time);
    }
}

