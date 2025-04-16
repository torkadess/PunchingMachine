/*
 * button.h
 *
 *  Created on: Mai 2023
 *      Author: DIACONESCO
 */

#ifndef BUTTON_H_
#define BUTTON_H_

typedef enum
{
	BUTTON_EVENT_NONE,
	BUTTON_EVENT_SHORT_PRESS,
	BUTTON_EVENT_LONG_PRESS
}button_event_e;

void BUTTON_init();

button_event_e BUTTON1_state_machine();
button_event_e BUTTON2_state_machine();


#endif /* BUTTON_H_ */
