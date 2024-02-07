#include <stdio.h>
#include "driver/i2c.h"

#define I2C_SLAVE_ADDR 0x04
#define I2C_MASTER_SCL_IO 22    // GPIO number for I2C master clock
#define I2C_MASTER_SDA_IO 21    // GPIO number for I2C master data
#define I2C_MASTER_NUM I2C_NUM_0 // I2C port number for master dev
#define I2C_MASTER_FREQ_HZ 100000 // I2C master clock frequency

esp_err_t i2c_master_transmit(uint16_t DevADD, uint8_t* data, size_t size)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, DevADD << 1 | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, data, size, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

void i2c_master_init()
{
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    i2c_param_config(I2C_MASTER_NUM, &conf);
{
    // Configure I2C master
	i2c_master_init();
	uint8_t data[3]={1,2,3};
    // Create I2C slave device object
	while(1){

		i2c_master_transmit(I2C_SLAVE_ADDR, &data, 3);
		vTaskDelay(pdMS_TO_TICKS(1000));
	}

}
