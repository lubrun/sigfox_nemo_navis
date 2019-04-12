#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <limits.h>


typedef char				s8;
typedef unsigned char		u8;
typedef short int			s16;
typedef unsigned short int	u16;


#define 	INTERRUPT_MASK_RTC   0b00000001
#define 	INTERRUPT_MASK_BUTTON   0b00000010
#define 	INTERRUPT_MASK_REED_SWITCH   0b00000100
#define 	INTERRUPT_MASK_FXOS8700   0b00001000


#define SEND_MSG_FULL       86400   //1 day
#define SEND_MSG_SIMPLE     86400   //1 day
#define SEND_MSG_LOWBATT    172800  //2 days
#define SEND_MSG_TEST       1200    //20 minutes

#define CALC_FULL       3600
#define CALC_SIMPLE     SEND_MSG_SIMPLE     //No calculations to do
#define CALC_LOWBATT    SEND_MSG_LOWBATT    //No calculations to do
#define CALC_TEST		120

#define MODE_FULL       0b1000
#define MODE_SIMPLE     0b0100
#define MODE_LOWBATT    0b0010
#define MODE_TEST       0b0001
#define MODE_DEFAULT    MODE_FULL   //Default mode after first start and recharge



//False defines, easier to debug
#define BUTTON_ONE_PRESS 1
#define BUTTON_TWO_PRESSES 2
#define BUTTON_THREE_PRESSES 3
#define BUTTON_FOUR_PRESSES 4


//reminder : max payload size = 12bytes, max 6msg/hour

//TODO : Add defines to allow easy payload modification ? (ex : LOWBATT_SEND_SIMPLEMODE or LOWBATT_ONLYINFOS or LOWBATT_SEND_FULLMODE)
//MESSAGE TO SEND :
// FULL MODE	: all
// SIMPLE MODE	: cur_temp + cur_light + infos (mixed in only 1 message)
// LOWBATT MODE : infos only ? (normally needs gps).

typedef struct t_payload {
	//TEMP PAYLOAD : 4*1bytes temperatures + 2*4bytes timers = 12bytes = 1message (0bytes empty)
	// Note : Timers can be shortened, and can be transformed to minutes or hours to take less space.
	s8 cur_temp;
	s8 moy_temp;
	s8 max_temp;
	int max_temp_time;
	s8 min_temp;
	int min_temp_time;

	//LIGHT PAYLOAD : 8*2bytes lights + 4*4bytes timers = 32bytes = 3messages (4bytes empty)
	// Msg1 : cur + moy (8bytes)
	// Msg2 : max (12bytes)
	// Msg3 : min (12bytes)
	// Note : Timers can be shortened, and can be transformed to minutes or hours to take less space.
	u16 cur_light0;
	u16 cur_light1; //infrared
	u16 moy_light0;
	u16 moy_light1; //infrared
	u16 max_light0;
	int max_light_time0;
	u16 max_light1; //infrared
	int max_light_time1;
	u16 min_light0;
	int min_light_time0;
	u16 min_light1; //infrared
	int min_light_time1;

	//INFOS PAYLOAD : 1*2bytes battery level + 1*1bytes beacon mode = 3bytes = 1message (9bytes empty)
	// Note : If nothing more needed here, can be mixed with LIGHT PAYLOAD
	// Things that can be added : humidity level (is the beacon drowning ?), time until next message (useless...), GPS (needs to be added), etc?
	short int batt; //TODO Check actual size
	u8 mode; //Only needs 4 bytes tho...
} t_payload;


int btn_press = 0; //nombre de clics sur bouton
u16 battery_level = 3000; //niveau de batterie


int set_mode(u8 balise_mode, int btn)
{
	if (balise_mode != 2)
	{
		switch ((u8)btn)
		{
			case BUTTON_ONE_PRESS:
				if (balise_mode & MODE_TEST)
					printf("** SENDING TEST PAYLOAD **\n");//send_payload();
				//SENSIT_API_set_rgb_led(RGB_BLUE);
				printf("Activated full mode\n");
				printf("Light up with color %s\n", "RGB_BLUE");
				return (MODE_FULL ^ (balise_mode & MODE_TEST)); //TODO : Check result keeps test bit
			case BUTTON_TWO_PRESSES:
				//SENSIT_API_set_rgb_led(RGB_MAGENTA);
				printf("Activated simple mode\n");
				printf("Light up with color %s\n", "RGB_MAGENTA");
				return (MODE_SIMPLE ^ (balise_mode & MODE_TEST)); //TODO : Check result keeps test bit
			case BUTTON_THREE_PRESSES:
				balise_mode ^= MODE_TEST;
				//SENSIT_API_set_rgb_led(balise_mode & MODE_TEST ? RGB_GREEN : RGB_RED);
				printf("Test mode is %s\n", balise_mode & MODE_TEST ? "ON" : "OFF");
				printf("Light up with color %s\n", balise_mode & MODE_TEST ? "RGB_GREEN" : "RGB_RED");
		}
	}
	else
		printf("Low battery mode, ignoring button presses except 4 presses\n");
	if (btn == BUTTON_FOUR_PRESSES)
		printf("Sensit reset\n"), exit(0);//SENSIT_API_reset();
	return (balise_mode);
}

int get_send_timer(u8 balise_mode)
{
	if (balise_mode & MODE_TEST)
		return (SEND_MSG_TEST); //TODO : Use activated mode calc timer instead of calc_test.
	if (balise_mode & MODE_FULL)
		return (SEND_MSG_FULL);
	if (balise_mode & MODE_SIMPLE)
		return (SEND_MSG_SIMPLE);
	//	if (balise_mode & MODE_LOWBATT)
	return (SEND_MSG_LOWBATT);
}

int get_sleep_timer(u8 balise_mode)
{
	if (balise_mode & MODE_TEST)
		return (CALC_TEST); //TODO : Use activated mode calc timer instead of calc_test.
	if (balise_mode & MODE_FULL)
		return (CALC_FULL);
	if (balise_mode & MODE_SIMPLE)
		return (CALC_SIMPLE);
	//	if (balise_mode & MODE_LOWBATT)
	return (CALC_LOWBATT);
}


//FAUSSE FONCTION SLEEP : Permet de renseigner des nouvelles donnees pour le prochain cycle
u16 sleep_sensit(int time, u16 pending_interrupt)
{
	printf("Slept for %d seconds\n", time);

	static int stop_asking = 0;
	if (stop_asking)
		stop_asking--;
	else
	{
		char c;
		printf("Enter wake up reason (1-4 : button press(es), b : change battery level, s : skip 1, t : skip 10) : ");
		scanf("%c", &c), fflush(stdin);
		switch (c)
		{
			case '1':
				printf("Button pressed 1 time\n");
				break;
			case '2':
				printf("Button pressed 2 times\n");
				break;
			case '3':
				printf("Button pressed 3 times\n");
				break;
			case '4':
				printf("Button pressed 4 times\n");
				break;
			case 's':
				printf("Skipping 1turn\n");
				break;
			case 't':
				stop_asking = 10;
				printf("Skipping 10 turns\n");
				break;
			case 'b':
				scanf("%c", &c); //garbage \n collected
				int newbatt;
				printf("New battery level : ");
				scanf("%d", &newbatt), fflush(stdin);
				battery_level = newbatt;
				break;
			default:
				printf("You suck at typing...\nSkipping once\n");
				break;
		}
		char garbage;
		scanf("%c", &garbage), fflush(stdin); //flushing \n from stdin, fflush does not work...
		if (c >= '1' && c <= '4')
		{
			btn_press = c - '0';
			pending_interrupt |= INTERRUPT_MASK_BUTTON;
		}
	}
	return (pending_interrupt | INTERRUPT_MASK_RTC);
}

void get_data(u8 balise_mode, u16 battery_level, t_payload *payload, int elapsed_time)
{
	//GENERAL INFOS NEEDED FOR EVERY MODE
	payload->batt = battery_level;
	payload->mode = balise_mode;

	if (balise_mode & MODE_SIMPLE) {
		payload->cur_temp = rand() % 60 - 20;
		payload->cur_light0 = rand() % 64000;
		payload->cur_light1 = rand() % 64000;
	}

	if (balise_mode & MODE_FULL) {
		int tmp;

		tmp = payload->cur_temp;
		payload->cur_temp = rand() % 60 - 20;
		payload->moy_temp = (payload->cur_temp + tmp) / 2; // >> 1 can't be used because value is signed
		if (payload->cur_temp > payload->max_temp)
			payload->max_temp = payload->cur_temp, payload->max_temp_time = elapsed_time;
		if (payload->cur_temp < payload->min_temp)
			payload->min_temp = payload->cur_temp, payload->min_temp_time = elapsed_time;

		tmp = payload->cur_light0;
		payload->cur_light0 = rand() % 64000;
		payload->moy_light0 = (payload->cur_light0 + tmp) >> 1; // same as : divided by 2 (for unsigned types)
		if (payload->cur_light0 > payload->max_light0)
			payload->max_light0 = payload->cur_light0, payload->max_light_time0 = elapsed_time;
		if (payload->cur_light0 < payload->min_light0)
			payload->min_light0 = payload->cur_light0, payload->min_light_time0 = elapsed_time;

		tmp = payload->cur_light1;
		payload->cur_light1 = rand() % 64000;
		payload->moy_light1 = (payload->cur_light1 + tmp) >> 1; // same as : divided by 2 (for unsigned types)
		if (payload->cur_light1 > payload->max_light1)
			payload->max_light1 = payload->cur_light1, payload->max_light_time1 = elapsed_time;
		if (payload->cur_light1 < payload->min_light1)
			payload->min_light1 = payload->cur_light1, payload->min_light_time1 = elapsed_time;
	}
}

void init_data(t_payload *payload)
{
	payload->cur_temp = CHAR_MAX;
	payload->moy_temp = CHAR_MAX;
	payload->max_temp = CHAR_MIN;
	payload->max_temp_time = -1;
	payload->min_temp = CHAR_MAX;
	payload->min_temp_time = -1;

	payload->cur_light0 = USHRT_MAX;
	payload->moy_light0 = USHRT_MAX;
	payload->max_light0 = 0;
	payload->max_light_time0 = -1;
	payload->min_light0 = USHRT_MAX;
	payload->min_light_time0 = -1;

	payload->cur_light1 = USHRT_MAX;
	payload->moy_light1 = USHRT_MAX;
	payload->max_light1 = 0;
	payload->max_light_time1 = -1;
	payload->min_light1 = USHRT_MAX;
	payload->min_light_time1 = -1;
}

void send_payload(t_payload *payload)
{
	printf("\033[0;32m\t*** SENDING DATA : ***\n");
	printf("\tMode : %d\n", payload->mode);
	printf("\tBattery level : %d\n", payload->batt);

	if (payload->mode & MODE_SIMPLE) {
		printf("\tCurrent Temperature : %d\n", payload->cur_temp);
		printf("\tCurrent Light : %d\n", payload->cur_light0);
		printf("\tCurrent Light (infrared) : %d\n", payload->cur_light1);
	}

	if (payload->mode & MODE_FULL) {
		printf("\tCurrent Temperature : %d\n", payload->cur_temp);
		printf("\tMedium Temperature : %d\n", payload->moy_temp);
		printf("\tMax Temperature : %d (%d seconds ago)\n", payload->max_temp, payload->max_temp_time);
		printf("\tMin Temperature : %d (%d seconds ago)\n", payload->min_temp, payload->min_temp_time);

		printf("\tCurrent Light : %d\n", payload->cur_light0);
		printf("\tMedium Light : %d\n", payload->moy_light0);
		printf("\tMax Light : %d (%d seconds ago)\n", payload->max_light0, payload->max_light_time0);
		printf("\tMin Light : %d (%d seconds ago)\n", payload->min_light0, payload->min_light_time0);

		printf("\tCurrent Light (infrared) : %d\n", payload->cur_light1);
		printf("\tMedium Light (infrared) : %d\n", payload->moy_light1);
		printf("\tMax Light (infrared) : %d (%d seconds ago)\n", payload->max_light1, payload->max_light_time1);
		printf("\tMin Light (infrared) : %d (%d seconds ago)\n", payload->min_light1, payload->min_light_time1);
	}
	printf("\033[0m");
}

int main()
{
	srand(time(NULL)); //random seed to generate bogus debug data
	printf("\033[0m");
	printf("\n");

	t_payload payload = {0};
	u16 pending_interrupt = 0;
	u8 balise_mode = MODE_DEFAULT;
	int elapsed_time = get_send_timer(balise_mode);
	//	u16 battery_level = 3000;

	pending_interrupt = 0;
	while (1)
	{
		printf("*** NEW LOOP TURN (Battery : %dmV) ***\n", battery_level);
		printf("*** MODE %d (8:full, 4:simple, 2:lowbatt) - MODE TEST IS %s ***\n", balise_mode & MODE_TEST ? balise_mode-1 : balise_mode, balise_mode & MODE_TEST ? "ON" : "OFF");

		pending_interrupt = sleep_sensit(get_sleep_timer(balise_mode), pending_interrupt);

		if (battery_level < 500) //TODO : Exact value is to be found...
		{
			printf("Entering low battery mode\n");
			balise_mode = 2;
		}
		else if (balise_mode == 2)
		{
			printf("Exiting low battery mode, returning to default mode\n");
			balise_mode = MODE_DEFAULT;
		}

		if ((pending_interrupt & INTERRUPT_MASK_RTC) == INTERRUPT_MASK_RTC)
		{
			elapsed_time -= get_sleep_timer(balise_mode);
			printf("Entering alarm function (timer = %d out of %d)\n", elapsed_time, get_send_timer(balise_mode));
			get_data(balise_mode, battery_level, &payload, elapsed_time);
			if (elapsed_time <= 0)
			{
				printf("Entering send message function\n");
				send_payload(&payload);
				init_data(&payload);
				elapsed_time = get_send_timer(balise_mode);
			}
			pending_interrupt &= ~INTERRUPT_MASK_RTC;
		}

		if ((pending_interrupt & INTERRUPT_MASK_BUTTON) == INTERRUPT_MASK_BUTTON)
		{
			elapsed_time = get_send_timer(balise_mode);
			printf("Entering button handling function (pressed %d time(s))\n", btn_press);
			balise_mode = set_mode(balise_mode, btn_press);
			pending_interrupt &= ~INTERRUPT_MASK_BUTTON;
		}

		printf("\n");
	}
	return 0;
}
