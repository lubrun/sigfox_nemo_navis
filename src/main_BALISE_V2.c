/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   main_BALISE_V2.c                                 .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: lubrun <lubrun@student.le-101.fr>          +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2019/03/21 17:23:19 by lubrun       #+#   ##    ##    #+#       */
/*   Updated: 2019/04/12 18:24:31 by lubrun      ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */

/*!******************************************************************
 * \file main_BALISE.c
 * \brief Sens'it Discovery mode Temperature demonstration code
 * \author Sens'it Team
 * \copyright Copyright (c) 2018 Sigfox, All Rights Reserved.
 *
 * For more information on this firmware, see temperature.md.
 *******************************************************************/
/******* INCLUDES **************************************************/
#include "sensit_types.h"
#include "sensit_api.h"
#include "error.h"
#include "button.h"
#include "battery.h"
#include "radio_api.h"
#include "hts221.h"
#include "ltr329.h"
#include "discovery.h"


/******** DEFINES **************************************************/
/* Message sending period, in seconds */
#define SEND_MSG_FULL		86400	//1 day
#define SEND_MSG_SIMPLE		86400	//1 day
#define SEND_MSG_LOWBATT	172800	//2 days
#define SEND_MSG_TEST       1200    //20 minutes

/* Time between 2 data gathering, in seconds */
#define CALC_FULL		3600
#define CALC_SIMPLE		SEND_MSG_SIMPLE		//No calculations to do
#define CALC_LOWBATT	SEND_MSG_LOWBATT	//No calculations to do
#define CALC_TEST		1200

/* Beacon modes */
#define MODE_FULL		0b1000
#define MODE_SIMPLE		0b0100
#define MODE_LOWBATT	0b0010
#define MODE_TEST		0b0001
#define MODE_DEFAULT    MODE_FULL   //Default mode after first start and recharge

/* Types limits */
#define CHAR_MIN -128
#define CHAR_MAX 127
#define USHRT_MAX 65535


/******* CUSTOM STRUCTURES  ****************************************/
typedef struct t_data {
	//BASIC INFOS
	u16 batt;
	u8 mode;

	//TEMPERATURE DATA
	s8 cur_temp;
	s8 moy_temp;
	s8 max_temp;
	int max_temp_time;
	s8 min_temp;
	int min_temp_time;

	//LIGHT DATA
	u16 cur_light0;
	u16 moy_light0;
	u16 max_light0;
	int max_light_time0;
	u16 min_light0;
	int min_light_time0;

	//INFRARED LIGHT DATA
	u16 cur_light1;
	u16 moy_light1;
	u16 max_light1;
	int max_light_time1;
	u16 min_light1;
	int min_light_time1;
} t_data;


/******* GLOBAL VARIABLES ******************************************/
u8 firmware_version[] = "Balise V 1.0.0";


/*******************************************************************/

void send_data(t_data *data)
{
	error_t err;
	int msg_size = 0;
	u8 msg[12] = {0};

	//SENDING BASIC INFOS : 1 message, 3bytes
	msg[msg_size++] = data->mode;
	msg[msg_size++] = data->batt >> 8;
	msg[msg_size++] = data->batt;
	err = RADIO_API_send_message((data->mode & MODE_TEST) ? RGB_YELLOW : RGB_OFF, msg, msg_size, FALSE, NULL);
	ERROR_parser(err);

	if (data->mode & MODE_SIMPLE) {
		//SENDING BASIC DATA : 1 message, 5bytes
		msg_size = 0;

		msg[msg_size++] = data->cur_temp;
		msg[msg_size++] = data->cur_light0 >> 8;
		msg[msg_size++] = data->cur_light0;
		msg[msg_size++] = data->cur_light1 >> 8;
		msg[msg_size++] = data->cur_light1;
		err = RADIO_API_send_message((data->mode & MODE_TEST) ? RGB_YELLOW : RGB_OFF, msg, msg_size, FALSE, NULL);
		ERROR_parser(err);

		//Total : 2messages, 8bytes
	}
	else if (data->mode & MODE_FULL) {
		//SENDING ALL TEMPERATURE DATA : 1 message, 12bytes
		msg_size = 0;

		msg[msg_size++] = data->cur_temp;
		msg[msg_size++] = data->moy_temp;
		msg[msg_size++] = data->max_temp;
		msg[msg_size++] = data->max_temp_time >> 24;
		msg[msg_size++] = data->max_temp_time >> 16;
		msg[msg_size++] = data->max_temp_time >> 8;
		msg[msg_size++] = data->max_temp_time;
		msg[msg_size++] = data->min_temp;
		msg[msg_size++] = data->min_temp_time >> 24;
		msg[msg_size++] = data->min_temp_time >> 16;
		msg[msg_size++] = data->min_temp_time >> 8;
		msg[msg_size++] = data->min_temp_time;
		err = RADIO_API_send_message((data->mode & MODE_TEST) ? RGB_YELLOW : RGB_OFF, msg, msg_size, FALSE, NULL);
		ERROR_parser(err);

		//SENDING CURRENT AND MEDIUM LIGHT DATA (regular + infrared) : 1 message, 8bytes
		msg_size = 0;

		msg[msg_size++] = data->cur_light0 >> 8;
		msg[msg_size++] = data->cur_light0;
		msg[msg_size++] = data->cur_light1 >> 8;
		msg[msg_size++] = data->cur_light1;
		msg[msg_size++] = data->moy_light0 >> 8;
		msg[msg_size++] = data->moy_light0;
		msg[msg_size++] = data->moy_light1 >> 8;
		msg[msg_size++] = data->moy_light1;
		err = RADIO_API_send_message((data->mode & MODE_TEST) ? RGB_YELLOW : RGB_OFF, msg, msg_size, FALSE, NULL);
		ERROR_parser(err);


		//SENDING MAX LIGHT DATA (regular + infrared) : 1 message, 12bytes
		msg_size = 0;

		msg[msg_size++] = data->max_light0 >> 8;
		msg[msg_size++] = data->max_light0;
		msg[msg_size++] = data->max_light1 >> 8;
		msg[msg_size++] = data->max_light1;
		msg[msg_size++] = data->max_light_time0 >> 24;
		msg[msg_size++] = data->max_light_time0 >> 16;
		msg[msg_size++] = data->max_light_time0 >> 8;
		msg[msg_size++] = data->max_light_time0;
		msg[msg_size++] = data->max_light_time1 >> 24;
		msg[msg_size++] = data->max_light_time1 >> 16;
		msg[msg_size++] = data->max_light_time1 >> 8;
		msg[msg_size++] = data->max_light_time1;
		err = RADIO_API_send_message((data->mode & MODE_TEST) ? RGB_YELLOW : RGB_OFF, msg, msg_size, FALSE, NULL);
		ERROR_parser(err);


		//SENDING MIN LIGHT DATA (regular + infrared) : 1 message, 12bytes
		msg_size = 0;

		msg[msg_size++] = data->min_light0 >> 8;
		msg[msg_size++] = data->min_light0;
		msg[msg_size++] = data->min_light1 >> 8;
		msg[msg_size++] = data->min_light1;
		msg[msg_size++] = data->min_light_time0 >> 24;
		msg[msg_size++] = data->min_light_time0 >> 16;
		msg[msg_size++] = data->min_light_time0 >> 8;
		msg[msg_size++] = data->min_light_time0;
		msg[msg_size++] = data->min_light_time1 >> 24;
		msg[msg_size++] = data->min_light_time1 >> 16;
		msg[msg_size++] = data->min_light_time1 >> 8;
		msg[msg_size++] = data->min_light_time1;
		err = RADIO_API_send_message((data->mode & MODE_TEST) ? RGB_YELLOW : RGB_OFF, msg, msg_size, FALSE, NULL);
		ERROR_parser(err);

		//Total : 5messages, 47bytes
	}
}

void get_data(t_data *data, int elapsed_time)
{
	if (data->mode & MODE_SIMPLE) {
		s16 temperature;
		u16 humidity;
		HTS221_measure(&temperature, &humidity);
		data->cur_temp = (s8)temperature;
		LTR329_measure(&(data->cur_light0), &(data->cur_light1));
	}

	if (data->mode & MODE_FULL) {
		int tmp;
		int tmp_infra;
		s16 temperature;
		u16 humidity;

		tmp = data->cur_temp;
		HTS221_measure(&temperature, &humidity);
		data->cur_temp = (s8)temperature;
		data->moy_temp = (data->cur_temp + tmp) / 2; // >> 1 can't be used because value is signed
		if (data->cur_temp > data->max_temp)
			data->max_temp = data->cur_temp, data->max_temp_time = elapsed_time;
		if (data->cur_temp < data->min_temp)
			data->min_temp = data->cur_temp, data->min_temp_time = elapsed_time;
		
		tmp = data->cur_light0;
		tmp_infra = data->cur_light1;
		LTR329_measure(&(data->cur_light0), &(data->cur_light1));

		data->moy_light0 = (data->cur_light0 + tmp) >> 1; // same as : divided by 2 (for unsigned types)
		if (data->cur_light0 > data->max_light0)
			data->max_light0 = data->cur_light0, data->max_light_time0 = elapsed_time;
		if (data->cur_light0 < data->min_light0)
			data->min_light0 = data->cur_light0, data->min_light_time0 = elapsed_time;

		data->moy_light1 = (data->cur_light1 + tmp_infra) >> 1; // same as : divided by 2 (for unsigned types)
		if (data->cur_light1 > data->max_light1)
			data->max_light1 = data->cur_light1, data->max_light_time1 = elapsed_time;
		if (data->cur_light1 < data->min_light1)
			data->min_light1 = data->cur_light1, data->min_light_time1 = elapsed_time;
	}
}

int set_mode(button_e btn, t_data *data)
{
	if (data->mode != 2)
	{
		//COMPARAISON DU NOMBRE DE PRESSION SUR LE BOUTON
		switch ((u8)btn)
		{
			case BUTTON_ONE_PRESS:
				if (data->mode & MODE_TEST)
					send_data(data);
				SENSIT_API_set_rgb_led(RGB_BLUE);
				return (MODE_FULL ^ (data->mode & MODE_TEST));
			case BUTTON_TWO_PRESSES:
				SENSIT_API_set_rgb_led(RGB_MAGENTA);
				return (MODE_SIMPLE ^ (data->mode & MODE_TEST));
			case BUTTON_THREE_PRESSES:
				data->mode ^= MODE_TEST;
				SENSIT_API_set_rgb_led(data->mode & MODE_TEST ? RGB_GREEN : RGB_RED);
				return (data->mode);
		}
	}
	if (btn == BUTTON_FOUR_PRESSES)
		SENSIT_API_reset();
	return (data->mode);
}

int get_send_timer(t_data *data)
{
	if (data->mode & MODE_TEST)
		return (SEND_MSG_TEST); //TODO : Use activated mode calc timer instead of calc_test.
	else if (data->mode & MODE_LOWBATT)
		return (SEND_MSG_LOWBATT);
	else if (data->mode & MODE_FULL)
		return (SEND_MSG_FULL);
	return (SEND_MSG_SIMPLE);
}

int get_sleep_timer(t_data *data)
{
	if (data->mode & MODE_TEST)
		return (CALC_TEST); //TODO : Use activated mode calc timer instead of calc_test.
	else if (data->mode & MODE_LOWBATT)
		return (CALC_LOWBATT);
	else if (data->mode & MODE_FULL)
		return (CALC_FULL);
	return (CALC_SIMPLE);
}

void init_data(t_data *data)
{
	data->cur_temp = CHAR_MAX;
	data->moy_temp = CHAR_MAX;
	data->max_temp = CHAR_MIN;
	data->max_temp_time = -1;
	data->min_temp = CHAR_MAX;
	data->min_temp_time = -1;

	data->cur_light0 = USHRT_MAX;
	data->moy_light0 = USHRT_MAX;
	data->max_light0 = 0;
	data->max_light_time0 = -1;
	data->min_light0 = USHRT_MAX;
	data->min_light_time0 = -1;

	data->cur_light1 = USHRT_MAX;
	data->moy_light1 = USHRT_MAX;
	data->max_light1 = 0;
	data->max_light_time1 = -1;
	data->min_light1 = USHRT_MAX;
	data->min_light_time1 = -1;
}

void init()
{
	error_t	err;


	//Message system
	err = RADIO_API_init();
	ERROR_parser(err);

	//Temperature & humidity
	err = HTS221_init();
	ERROR_parser(err);

	//Light
	err = LTR329_init();
	ERROR_parser(err);

	//TODO : Add color code for successful init ?
}

int main()
{
	pending_interrupt = 0;
	int elapsed_time;
	t_data data;

	SENSIT_API_configure_button(INTERRUPT_BOTH_EGDE);
	//init();

	data.mode = MODE_DEFAULT;
	elapsed_time = get_send_timer(&data);

	while (TRUE)
	{
		//DEFINIG NEXT WAKE UP TIME
		SENSIT_API_set_rtc_alarm(get_sleep_timer(&data));

		//SLEEPING UNTIL ALARM
		if (!pending_interrupt)
			SENSIT_API_sleep(FALSE);

		//LOW BATTERY
		SENSIT_API_get_battery_level(&(data.batt));
		if (data.batt < BATTERY_LOW_LEVEL) {
			data.mode &= MODE_LOWBATT;
			elapsed_time = get_send_timer(&data);
		}
		else if (data.mode & MODE_LOWBATT) {
			elapsed_time = get_send_timer(&data);
			data.mode &= MODE_DEFAULT;
		}

		SENSIT_API_set_rgb_led(RGB_BLUE);
		SENSIT_API_set_rgb_led(RGB_CYAN);
		SENSIT_API_set_rgb_led(RGB_RED);
		SENSIT_API_set_rgb_led(RGB_YELLOW);
		SENSIT_API_set_rgb_led(RGB_GREEN);
		SENSIT_API_set_rgb_led(RGB_MAGENTA);
		SENSIT_API_set_rgb_led(RGB_WHITE);

		//INTERRUPT : ALARM
		if ((pending_interrupt & INTERRUPT_MASK_RTC) == INTERRUPT_MASK_RTC)
		{
			elapsed_time -= get_sleep_timer(&data);
			get_data(&data, elapsed_time);
			if (elapsed_time <= 0)
			{
				send_data(&data);
				init_data(&data);
				elapsed_time = get_send_timer(&data);
			}
			pending_interrupt &= ~INTERRUPT_MASK_RTC;
		}

		//INTERRUPT : BUTTON
		if ((pending_interrupt & INTERRUPT_MASK_BUTTON) == INTERRUPT_MASK_BUTTON)
		{
			elapsed_time = get_send_timer(&data);
			data.mode = set_mode(BUTTON_handler(), &data);
			if (BUTTON_handler() == BUTTON_FOUR_PRESSES)
				SENSIT_API_reset();
			pending_interrupt &= ~INTERRUPT_MASK_BUTTON;
		}
	}
}
