/*
 * task_stats.h
 *
 *  Created on: 6 déc. 2022
 *      Author: sovichea
 */

#ifndef MAIN_TASK_STATS_H_
#define MAIN_TASK_STATS_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

esp_err_t print_real_time_stats(TickType_t xTicksToWait);

#endif /* MAIN_TASK_STATS_H_ */
