/*
 * Acceleration_STEPPER.c
 *
 *  Created on: Jan 19, 2023
 *      Author: HUT HOKKEY
 *      This Library is base on AVR446: Linear speed control of stepper motor
 *      Document link
 *      https://ww1.microchip.com/downloads/en/Appnotes/doc8017.pdf
 *      https://docs.espressif.com/projects/esp-idf/en/v4.3/esp32/api-reference/peripherals/gpio.html
 */
#include "acceleration_stepper.h"

//Acceleration_t Accel[sizeof(Stepper_t)];
/*
 * Set GPIO Pin for each STEPPER
 * Accel_stepper : Pointer struct handler of Acceleration_t
 * step_pin : GPIO pin number of step pin
 * dir_pin  : GPIO pin number of direction pin
 */
void Accel_Stepper_SetPin(Acceleration_t *Accel_stepper,gpio_num_t step_pin, gpio_num_t dir_pin){
	Accel_stepper->step_pin = step_pin;
	Accel_stepper->dir_pin = dir_pin;
}

/*
 * Set Timer for each motor
 * timer : pointer to timer typedef(Which timer is use for control speed)
 */
void Accel_Stepper_SetTimer(Acceleration_t *Accel_stepper,timer_group_t group, timer_idx_t timer) {
	Accel_stepper->group = group;
	Accel_stepper->timer = timer;
}
/*
 * Accel_Stepper_TIMIT_Handler
 * stepper : Num of which stepper use found @ Stepper_t
 */
void Accel_Stepper_ISR(Acceleration_t *Accel_stepper, bool toggle){

	switch(Accel_stepper->run_state) {
		case STOP:
			Accel_stepper->step_count = 0;
			Accel_stepper->rest = 0;
		     // Stop Timer/Counter 1.

			timer_pause(Accel_stepper->group,Accel_stepper->timer);
			timer_disable_intr(Accel_stepper->group,Accel_stepper->timer);

		   	Accel_stepper->run_status = 0;
		   	break;
	    case ACCEL:
	    	Accel_stepper->run_status = 1;

	    	gpio_set_level(Accel_stepper->step_pin, toggle);

	    	Accel_stepper->step_count++;
			Accel_stepper->accel_count++;
			Accel_stepper->new_step_delay = Accel_stepper->step_delay - (((2 * (long)Accel_stepper->step_delay) + Accel_stepper->rest)/(4 * Accel_stepper->accel_count + 1));
			Accel_stepper->rest = ((2 * (long)Accel_stepper->step_delay)+Accel_stepper->rest)%(4 * Accel_stepper->accel_count + 1);
	      // Check if we should start deceleration.
			if(Accel_stepper->step_count >= Accel_stepper->decel_start) {
				Accel_stepper->accel_count = Accel_stepper->decel_val;
				Accel_stepper->run_state = DECEL;
			}
		      // Chech if we hitted max speed.
			else if(Accel_stepper->new_step_delay <= Accel_stepper->min_step_delay) {
				Accel_stepper->last_accel_delay = Accel_stepper->new_step_delay;
				Accel_stepper->new_step_delay = Accel_stepper->min_step_delay;
				Accel_stepper->rest = 0;
				Accel_stepper->run_state = RUN;
			}
			break;

	    case RUN:
	    	Accel_stepper->run_status = 1;

	    	gpio_set_level(Accel_stepper->step_pin, toggle);
	    	Accel_stepper->step_count++;
	    	Accel_stepper->new_step_delay = Accel_stepper->min_step_delay;
//	         Check if we should start deceleration.
			 if(Accel_stepper->step_count >= Accel_stepper->decel_start) {
				 Accel_stepper->accel_count = Accel_stepper->decel_val;
//	         Start deceleration with same delay as accel ended with.
				 Accel_stepper->new_step_delay = Accel_stepper->last_accel_delay;
				 Accel_stepper->run_state = DECEL;
			 }
			 break;

	    case DECEL:
	    	Accel_stepper->run_status = 1;
	    	gpio_set_level(Accel_stepper->step_pin, toggle);
	    	 Accel_stepper->step_count++;
			 Accel_stepper->accel_count++;
			 Accel_stepper->new_step_delay = Accel_stepper->step_delay + (((2 * (long)Accel_stepper->step_delay) + Accel_stepper->rest)/(4 * abs(Accel_stepper->accel_count) + 1));
			 Accel_stepper->rest = ((2 * (long)Accel_stepper->step_delay)+Accel_stepper->rest)%(4 * (long) abs(Accel_stepper->accel_count) + 1);
//	         Check if we at last step
			 if(Accel_stepper->accel_count >= 0){
				 Accel_stepper->run_state = STOP;
			 }
			 break;
	  }
	if(Accel_stepper->new_step_delay<100) {
		Accel_stepper->step_delay = 100;
	}else if(Accel_stepper->new_step_delay>2000){
		Accel_stepper->step_delay = 2000;
	}
	else Accel_stepper->step_delay = Accel_stepper->new_step_delay;
	timer_pause(Accel_stepper->group, Accel_stepper->timer);
//	timer_group_intr_disable(Accel_stepper->group,Accel_stepper->timer);
	timer_set_alarm_value(Accel_stepper->group, Accel_stepper->timer, Accel_stepper->step_delay);
//	timer_enable_intr(Accel_stepper->group, Accel_stepper->timer);
	timer_start(Accel_stepper->group,Accel_stepper->timer);
//		  return rc;
}
/*
 * Accel_Stepper_Move
 * Accel_stepper : Pointer strut handler of Acceleration_t
 * TODO: must declare Acceleration_t handler in main.c (support multi-handler)
 * step : Number of step to run
 * accel : acceleration
 * decel : deceleration
 * rpm : speed at run state
 */
void Accel_Stepper_Move(Acceleration_t *Accel_stepper, signed int step, unsigned int accel, unsigned int decel, unsigned int rpm)//acc*100
{

	unsigned int max_step_lim; //! Number of steps before we hit max speed.
	unsigned int accel_lim;//! Number of steps before we must start deceleration (if accel does not hit max speed).
	unsigned int speed = 2 * 3.14159 * rpm/60;
	Accel_stepper->step_count = 0;
//   Set direction from sign on step value.
	if(step < 0){
//    srd.dir = CCW;
		gpio_set_level( Accel_stepper->dir_pin, 0);
		step = -2*step;
	}
	if(step>0){
		gpio_set_level(Accel_stepper->dir_pin, 1);
		step = 2*step;
//    srd.dir = CW;
	}

//  If moving only 1 step.
	if(step == 1){

//      Move one step...
		Accel_stepper->accel_count = -1;
//      ...in DECEL state.
		Accel_stepper->run_state = DECEL;
//      Just a short delay so main() can act on 'running'.
		Accel_stepper->step_delay = 1000;
//      status.running = TRUE;
		timer_start(Accel_stepper->group, Accel_stepper->timer);
	}
//  Only move if number of steps to move is not zero.
	else if(step != 0){
//	   Set max speed limit, by calc min_step_delay to use in timer.
//	   min_step_delay = (alpha / tt)/ w
//	   Accel[stepper].min_step_delay = alpha*t_freq / speed; //speed(rad/s)
//	   ARR=freq*t, where t=60/step/rpm; t here is in second unit (s) step = 6400(1/32)

		Accel_stepper->min_step_delay = 20;
//		Accel_stepper->min_step_delay = ALPHA*TIM_FREQ/speed;

//      Set acceleration by calc the first (c0) step delay .
//      step_delay = 1/tt * sqrt(2*alpha/accel)
//      step_delay = ( tfreq*0.676) * sqrt( (2*alpha*10000000000) / (accel*100) )/10000
//      SRD.step_delay = (TIM_FREQ_SCALE * sqrt(A_SQ / accel))/10000;
		Accel_stepper->step_delay = (TIM_FREQ_SCALE * sqrt(A_SQ / accel))/10000;;
//      Find out after how many steps does the speed hit the max speed limit.(step for accel)
//      max_step_lim = speed^2 / (2*alpha*accel)
		max_step_lim = (long)speed*speed*10000/(long)(((long)A_x20000*accel)/100);
//      If we hit max speed limit before 0,5 step it will round to 0.
//      But in practice we need to move at least 1 step to get any speed at all.
		if(max_step_lim == 0){
			max_step_lim = 1;
		}
//      find out after how many steps we must start deceleration.(step before start decel)
//      n1 = (n1+n2)decel / (accel + decel)
		accel_lim = ((long)step*decel) / (accel+decel);
//      We must accelerate at least 1 step before we can start deceleration.
		if(accel_lim == 0){
			accel_lim = 1;
		}
//     Use the limit we hit first to calculate decel.
		if(accel_lim <= max_step_lim){
			Accel_stepper->decel_val = accel_lim - step;//decel_val: step for decel)
		}
		else{
			Accel_stepper->decel_val = -(((long)(max_step_lim*accel))/decel);
		}
//     We must decelrate at least 1 step to stop.
		if(Accel_stepper->decel_val == 0){
			Accel_stepper->decel_val = -1;
		}
//     Find step to start deceleration.
		Accel_stepper->decel_start = step + Accel_stepper->decel_val;
//     If the maximum speed is too High that we don't need to go via acceleration state.
		if(Accel_stepper->step_delay <= Accel_stepper->min_step_delay){
			Accel_stepper->step_delay = Accel_stepper->min_step_delay;
			Accel_stepper->run_state = RUN;
		}
		else{
			Accel_stepper->run_state = ACCEL;
		}
//     Reset counter.
		Accel_stepper->accel_count = 0;
//    status.running = TRUE;
		timer_pause(Accel_stepper->group, Accel_stepper->timer);
		timer_set_alarm_value(Accel_stepper->group, Accel_stepper->timer, 100);
	    timer_enable_intr(Accel_stepper->group, Accel_stepper->timer);
		timer_start(Accel_stepper->group,Accel_stepper->timer);
	}
}




