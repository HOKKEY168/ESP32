#include <stdint.h>
#include <driver/i2c.h>
#include <math.h>
#include "esp_err.h"
#include <errno.h>
#include "esp_log.h"
#include "esp_system.h"
#include <freertos/FreeRTOS.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "pca9685.h"

//#define I2C_MASTER_SCL_IO 22    // GPIO number for I2C master clock
//#define I2C_MASTER_SDA_IO 21    // GPIO number for I2C master data
//#define I2C_MASTER_NUM I2C_NUM_0 // I2C port number for master dev
//#define I2C_MASTER_FREQ_HZ 100000 // I2C master clock frequency
#define CORE_PRO 0
#define CORE_APP 1

#define Servo1_Channel 1
#define Servo2_Channel 2
#define Servo3_Channel 3
#define Servo4_Channel 4
#define Servo5_Channel 5
#define Servo6_Channel 6
#define Servo7_Channel 7
#define Servo8_Channel 8
#define Servo9_Channel 9
#define Servo10_Channel 10
#define Servo11_Channel 11
#define Servo12_Channel 12

void i2c_scan()
{
    uint8_t cnt = 0;
    printf("Scanning I2C bus...\n");
    for (uint8_t addr = 1; addr <= 127; addr++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, addr << 1 | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);
        esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_RATE_MS);
        i2c_cmd_link_delete(cmd);
        if (ret == ESP_OK) {
            printf("Device found at address 0x%02X\n", addr);
            cnt++;
        }
    }
    printf("%d devices found\n", cnt);
}
void SERVO_Init(void)
{
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = 21;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = 22;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = 100000;
    i2c_param_config(I2C_NUM_0, &conf);
    i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);
}
void vSERVOTask(void *arg)
{
	i2c_scan();
	PCA9685_Init();
    uint8_t data[20]={1,0};
    while (1) {
    	for(int i=1;i<90;i++){
    		PCA9685_SetServoAngle(Servo1_Channel, i);
    		PCA9685_SetServoAngle(Servo2_Channel, i);
    		PCA9685_SetServoAngle(Servo3_Channel, i);
    		PCA9685_SetServoAngle(Servo4_Channel, i);
    		PCA9685_SetServoAngle(Servo5_Channel, i);
    		PCA9685_SetServoAngle(Servo6_Channel, i);
    		PCA9685_SetServoAngle(Servo7_Channel, i);
    		PCA9685_SetServoAngle(Servo8_Channel, i);
    		PCA9685_SetServoAngle(Servo9_Channel, i);
    		PCA9685_SetServoAngle(Servo10_Channel, i);
    		PCA9685_SetServoAngle(Servo11_Channel, i);
    		PCA9685_SetServoAngle(Servo12_Channel, i);

    		printf("%d\n",i );
    		vTaskDelay(pdMS_TO_TICKS(1));
    	}
    	for(int i=90;i>=1;i--){

    		PCA9685_SetServoAngle(Servo1_Channel, i);
    		PCA9685_SetServoAngle(Servo2_Channel, i);
    		PCA9685_SetServoAngle(Servo3_Channel, i);
    		PCA9685_SetServoAngle(Servo4_Channel, i);
    		PCA9685_SetServoAngle(Servo5_Channel, i);
    		PCA9685_SetServoAngle(Servo6_Channel, i);
    		PCA9685_SetServoAngle(Servo7_Channel, i);
    		PCA9685_SetServoAngle(Servo8_Channel, i);
    		PCA9685_SetServoAngle(Servo9_Channel, i);
    		PCA9685_SetServoAngle(Servo10_Channel, i);
    		PCA9685_SetServoAngle(Servo11_Channel, i);
    		PCA9685_SetServoAngle(Servo12_Channel, i);
    		 vTaskDelay(pdMS_TO_TICKS(1));
    		 printf("Data: %d\n",i);
    	}
    	vTaskDelay(pdMS_TO_TICKS(10));
    }
}
void app_main()
{
	SERVO_Init();
    xTaskCreate(vSERVOTask, "vSERVOTask", 2048, NULL, 5, 0);

}
