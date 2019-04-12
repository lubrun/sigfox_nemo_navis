/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   main_BALISE.c                                    .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: lubrun <lubrun@student.le-101.fr>          +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2019/03/21 17:23:19 by lubrun       #+#   ##    ##    #+#       */
/*   Updated: 2019/04/01 15:33:18 by lubrun      ###    #+. /#+    ###.fr     */
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
#include "discovery.h"

/******** DEFINES **************************************************/
#define MEASUREMENT_PERIOD                 3600 /* Measurement & Message sending period, in second */


/******* GLOBAL VARIABLES ******************************************/
u8 firmware_version[] = "Balise V 1.0.0";
u8 balise_mode = 0; /*ValueSet { 0 : Mode Complet | 1 : Mode Simple | 2 : Mode Batterie Faible}*/
s16 temp_max = 35;
s16 temp_min = -12;
u16 hum_max = 80;
u16 hum_min = 60;
bool test_mode = FALSE;

/*******************************************************************/

int send_payload0()
{
	u8	payload[2] = {0, 0};
	error_t		err;
	
	payload[0] = (u8)temp_max >> 8;
	payload[1] = (u8)temp_max;
	err = RADIO_API_send_message(RGB_MAGENTA, payload, 2, FALSE, NULL);
	ERROR_parser(err);
	return (1);
}

int send_payload1()
{
	u8	payload[2] = {0, 0};
	error_t		err;
	
	payload[0] = (u8)temp_max >> 8;
	payload[1] = (u8)temp_max;
	err = RADIO_API_send_message(RGB_YELLOW, payload, 2, FALSE, NULL);
	ERROR_parser(err);
	return (1);
}

int send_payload2()
{
	u8	payload[2] = {0, 0};
	error_t		err;
	
	payload[0] = (u8)temp_max >> 8;
	payload[1] = (u8)temp_max;
	err = RADIO_API_send_message(RGB_YELLOW, payload, 2, FALSE, NULL);
	ERROR_parser(err);
	return (1);
}

int	send_payload()
{
	switch (balise_mode)
	{
		case 0:
			send_payload0();
			break;
		case 1:
			send_payload1();
			break;
		case 2:
			send_payload2();
			break;
	}
	return (1);
}

int set_mode(button_e btn)
{
	if (balise_mode != 2)
	{
		//COMPARAISON DU NOMBRE DE PRESSION SUR LE BOUTON
		switch ((u8)btn)
		{
			case BUTTON_ONE_PRESS:
				/*if (test_mode == TRUE)
				{
					send_payload0();
					break;
				}*/
				SENSIT_API_set_rgb_led(RGB_BLUE);
				send_payload0();
				return (0);
			/*case BUTTON_TWO_PRESSES :
				SENSIT_API_set_rgb_led(RGB_MAGENTA);
				return (1);
			case BUTTON_THREE_PRESSES:
				test_mode = test_mode == TRUE  ? FALSE : TRUE ;
				test_mode == TRUE ? SENSIT_API_set_rgb_led(RGB_GREEN) : SENSIT_API_set_rgb_led(RGB_RED);
				break;*/
			case BUTTON_FOUR_PRESSES:
				SENSIT_API_reset();
				break;
		}
	}
	else
		if (btn == BUTTON_FOUR_PRESSES)
			SENSIT_API_reset();
	return (balise_mode);
}

int main()
{
	error_t		err;
	button_e	btn;
	u16			battery_level;

	//INITIALISTION DES DIFFERENTS CAPTEURS ET DE L'API
	SENSIT_API_configure_button(INTERRUPT_BOTH_EGDE);
	err = RADIO_API_init();
	ERROR_parser(err);
	err = HTS221_init();
	ERROR_parser(err);
	SENSIT_API_set_rtc_alarm(MEASUREMENT_PERIOD);
	pending_interrupt = 0;
	while (TRUE)
	{
		BATTERY_handler(&battery_level);
		if ((pending_interrupt & INTERRUPT_MASK_RTC) == INTERRUPT_MASK_RTC)
		{
			//INTERRUPTION DE MESURE (LIE A LA VAR "MEASUREMENT_PERIOD")
			send_payload();
			pending_interrupt &= ~INTERRUPT_MASK_RTC;//VIRE L'INTERRUPTION
		}
		if ((pending_interrupt & INTERRUPT_MASK_BUTTON) == INTERRUPT_MASK_BUTTON)
		{
			btn = BUTTON_handler();//INTERRUPTION BOUTON
			//balise_mode = set_mode(btn);//SET LE MODE DE LA BALISE
			if (btn == BUTTON_TWO_PRESSES)
            {
				send_payload1();
            }
            else if (btn == BUTTON_FOUR_PRESSES)
            {
                SENSIT_API_reset();
            }
			pending_interrupt &= ~INTERRUPT_MASK_BUTTON;
		}
		if ((pending_interrupt & INTERRUPT_MASK_REED_SWITCH) == INTERRUPT_MASK_REED_SWITCH)
		{	//INTERRUPTION AIMANT
			pending_interrupt &= ~INTERRUPT_MASK_REED_SWITCH;
			SENSIT_API_set_rgb_led(RGB_RED);
			send_payload2();
		}
		if ((pending_interrupt & INTERRUPT_MASK_FXOS8700) == INTERRUPT_MASK_FXOS8700)
		{	//INTERRUPTION ACCELEROMETRE
			pending_interrupt &= ~INTERRUPT_MASK_FXOS8700;
		}if (pending_interrupt == 0)
		{
			SENSIT_API_sleep(FALSE);
			continue;
		}
	}
}
