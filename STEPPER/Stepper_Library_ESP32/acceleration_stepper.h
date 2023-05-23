/*
 * Acceleration_STEPPER.h
 *
 *  Created on: Jan 19, 2023
 *      Author: HUT HOKKEY x THAI PHANNY
 *      This Library is base on AVR446: Linear speed control of stepper motor
 *      Document link
 *      https://ww1.microchip.com/downloads/en/Appnotes/doc8017.pdf
 *      https://docs.espressif.com/projects/esp-idf/en/v4.3/esp32/api-reference/peripherals/gpio.html
 */

#ifndef MAIN_ACCELERATION_STEPPER_H_
#define MAIN_ACCELERATION_STEPPER_H_
#include <stdio.h>
#include "math.h"
#include <stdbool.h>
#include <stdint.h>
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "driver/timer.h"

#define SPR 800 //1/4
#define TIM_FREQ 1000000
#define ALPHA (2*3.14159/SPR)                    // 2*pi/spr   // Step per revolution
#define AxTIM_FREQ ((long)(ALPHA*TIM_FREQ))     // (ALPHA / TIM_FREQ)
#define TIM_FREQ_SCALE ((int)((TIM_FREQ*0.676))) //scaled by 0.676
#define A_SQ (long)(ALPHA*2*10000000000)         // ALPHA*2*10000000000
#define A_x20000 (int)(ALPHA*20000)              //2*@ : ALPHA*20000 must divide by 10000 after use

/*
 * Increase scale for faster calculate
 * SPR : step per round
 * TIM_FREQ : frequency of timer interrupt
 *
 */
typedef enum{
	STOP,
	ACCEL,
	DECEL,
	RUN
}ramp_state_t;
typedef enum{
	STEPPER1,
	STEPPER2,
	STEPPER3,
	STEPPER4,
	STEPPER5,
	STEPPER6
}Stepper_t;
typedef struct {
	uint8_t run_status;
//  speed ramp state we are in.
	ramp_state_t run_state;
//  Direction stepper motor should move.
	unsigned char dir;
//  Counter peroid of timer delay (ARR). At start this value set the acceleration rate.
	unsigned int step_delay;
//  step_pos to start deceleration
	unsigned int decel_start;
//  Number of stepp for deceleration(in negative).
	signed int decel_val;
//  Minimum time ARR (max speed)
	signed int min_step_delay;
//  Counter used when accelerating/decelerating to calculate step_delay.
	signed int accel_count;
//  Counting steps when moving.
	unsigned int step_count;
//  Keep track of remainder from new_step-delay calculation to increase accuracy
	unsigned int rest;
//  Reset Delta
	unsigned int new_step_delay;// Holds next delay period.
//  Remember the last step delay used when accelerating.
	unsigned int last_accel_delay;

	timer_group_t group;
	timer_idx_t timer;
	gpio_num_t step_pin;
	gpio_num_t dir_pin;
	gpio_pull_mode_t pull;
	uint8_t pin_state;
	uint32_t freq_hz;
	//	typedef intr_handle_t timer_isr_handle_t; //Handle Service timer
} Acceleration_t;

void Accel_Stepper_SetTimer(Acceleration_t *Accel_stepper,timer_group_t group, timer_idx_t timer);
void Accel_Stepper_SetPin(Acceleration_t *Accel_stepper, gpio_num_t step_pin, gpio_num_t dir_pin);
void Accel_Stepper_Move(Acceleration_t *Accel_stepper, signed int step, unsigned int accel, unsigned int aecel, unsigned int rpm);
void Accel_Stepper_ISR(Acceleration_t *Accel_stepper, bool toggle); // Interrupt Service Routine


#endif /* MAIN_ACCELERATION_STEPPER_H_ */
