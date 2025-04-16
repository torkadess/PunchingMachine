/**
  ******************************************************************************
  * @file    main.c
  * @author  Nirgal
  * @date    03-July-2019
  * @brief   Default main function.
  ******************************************************************************
*/
#include "stm32f1xx_hal.h"
#include "stm32f1_uart.h"
#include "stm32f1_sys.h"
#include "stm32f1_gpio.h"
#include "macro_types.h"
#include "systick.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "tft_ili9341/stm32f1_ili9341.h"
#include "hx711\hx711.h"


//############################################
#include "button.h"
//############################################

static void state_machine(void);

hx711_t hx711;
float weight = 0;
char maChaine[32];

void writeLED(bool_e b)
{
	HAL_GPIO_WritePin(LED_GREEN_GPIO, LED_GREEN_PIN, b);
}

bool_e readButton(void)
{
	return !HAL_GPIO_ReadPin(BUTTON_1_GPIO, BUTTON_1_PIN);
}

static volatile uint32_t t = 0;
void process_ms(void)
{
	if(t)
		t--;
}



int main(void)
{
	//Initialisation de la couche logicielle HAL (Hardware Abstraction Layer)
	//Cette ligne doit rester la première étape de la fonction main().
 	HAL_Init();

	//Initialisation de l'UART2 à la vitesse de 115200 bauds/secondes (92kbits/s) PA2 : Tx  | PA3 : Rx.
		//Attention, les pins PA2 et PA3 ne sont pas reliées jusqu'au connecteur de la Nucleo.
		//Ces broches sont redirigées vers la sonde de débogage, la liaison UART étant ensuite encapsulée sur l'USB vers le PC de développement.
	UART_init(UART2_ID,115200);

	//"Indique que les printf sortent vers le périphérique UART2."
	SYS_set_std_usart(UART2_ID, UART2_ID, UART2_ID);

	//Initialisation du port de la led Verte (carte Nucleo)
	BSP_GPIO_PinCfg(LED_GREEN_GPIO, LED_GREEN_PIN, GPIO_MODE_OUTPUT_PP,GPIO_NOPULL,GPIO_SPEED_FREQ_HIGH);

	//Initialisation du port du bouton 1 et 2 (carte Nucleo)
	BSP_GPIO_PinCfg(BUTTON_1_GPIO, BUTTON_1_PIN, GPIO_MODE_INPUT,GPIO_PULLUP,GPIO_SPEED_FREQ_HIGH);
	BSP_GPIO_PinCfg(BUTTON_2_GPIO, BUTTON_2_PIN, GPIO_MODE_INPUT,GPIO_PULLUP,GPIO_SPEED_FREQ_HIGH);

	//On ajoute la fonction process_ms à la liste des fonctions appelées automatiquement chaque ms par la routine d'interruption du périphérique SYSTICK
	Systick_add_callback_function(&process_ms);

	while(1)
	{
		state_machine();
//		fonction_test();
	}
}


static void state_machine(void)
{
	typedef enum
	{
		INIT = 0,
		MODE_VITESSE,
		MODE_PUISSANCE
	} state_e;
	static state_e state = INIT;
	static state_e previous_state;
	static bool_e exoStart = FALSE;

	static bool_e entrance = TRUE; //  = (state != previous_state) ? TRUE : FALSE;
	previous_state = state;


	static button_event_e button1_event;
	static button_event_e button2_event;
	button1_event = BUTTON1_state_machine(); // chaque passage ici, on scrute un eventuel evenement sur le bouton
	button2_event = BUTTON2_state_machine();
	switch (state)
	{
		case INIT:
			/*     INIT ECRAN TFT    */
			ILI9341_Init();
			ILI9341_Fill(ILI9341_COLOR_BROWN);

			/*   AFFICHAGE DU MODE INIT */
			ILI9341_Fill(ILI9341_COLOR_BROWN);
			ILI9341_Puts(10,20, "Initialisation de la machine", &Font_11x18, ILI9341_COLOR_BLACK, ILI9341_COLOR_BROWN);

			/*     INIT BOUTONS    */
			BUTTON_init(1);
			BUTTON_init(2);


			/*     INIT HX711   */
			hx711_init(&hx711, HX711_CLOCK_GPIO, HX711_CLOCK_PIN, HX711_DATA_GPIO, HX711_DATA_PIN);
			hx711_calibration(&hx711, 7235140, 0, 100);
			hx711_coef_set(&hx711, 345); // read after calibration
			hx711_tare(&hx711, 100);

			Systick_add_callback_function(&process_ms);
			state = MODE_PUISSANCE;
			break;


		case MODE_PUISSANCE:
////			weight = 0;
//			weight = hx711_weight(&hx711, 10);
//
//	//		printf("weight = %f",weight);
//			sprintf(maChaine,"%f",weight);
//			ILI9341_Puts(75,70, &maChaine, &Font_11x18, ILI9341_COLOR_BLACK, ILI9341_COLOR_BROWN);

			/* Si on entre on ecrit une seul fois le texte */
			if (entrance) {
				ILI9341_Fill(ILI9341_COLOR_BROWN);
				ILI9341_Puts(75,20, "MODE PUISSANCE", &Font_11x18, ILI9341_COLOR_BLACK, ILI9341_COLOR_BROWN);
				entrance = FALSE;
			}
			/*  spécifie le début de l'execrice  */
			if (button2_event == BUTTON_EVENT_SHORT_PRESS) {
				ILI9341_Puts(75,40, "EXERCICE START", &Font_11x18, ILI9341_COLOR_BLACK, ILI9341_COLOR_BROWN);
				exoStart = TRUE;
			}

			/*   On lane l'exercice   */
			if (exoStart) {
				mode_jeu_puissance();
				exoStart= FALSE;
				entrance = TRUE;
			}
			/*  Changement de mode de jeu --> vers mode Vitesse  */
			if (button1_event == BUTTON_EVENT_LONG_PRESS)
			{
				previous_state = state;
				state = MODE_VITESSE;
				entrance = TRUE;
			}

			break;


		case MODE_VITESSE:
			/*  On ecrit qu'une seule fois le texte */
			if (entrance) {
				ILI9341_Fill(ILI9341_COLOR_BROWN);
				ILI9341_Puts(75,20, "MODE VITESSE", &Font_11x18, ILI9341_COLOR_BLACK, ILI9341_COLOR_BROWN);
				entrance = FALSE;
			}

			/*   lancement de l'exercice */
			if (button2_event == BUTTON_EVENT_SHORT_PRESS) {
				ILI9341_Puts(75,40, "EXERCICE START", &Font_11x18, ILI9341_COLOR_BLACK, ILI9341_COLOR_BROWN);
				exoStart = TRUE;
			}

			if (exoStart) {
				mode_jeu_vitesse();
				exoStart= FALSE;
				entrance = TRUE;
			}
			/* changement de mode --> vers mode Puissance */
			if (button1_event == BUTTON_EVENT_LONG_PRESS)
			{
				previous_state = state;
				state = MODE_PUISSANCE;
				entrance = TRUE;
			}
			break;
		default:
			break;
	}
}


void mode_jeu_puissance(void) {
	bool_e state = TRUE;
	weight = 0;
	while (state) {
		weight = hx711_custom_max_weight(&hx711, 10, weight); // on récupère la nouvelle valeur du poid
		sprintf(maChaine,"%.2f",weight); // converir le float en chaine de caractère
		ILI9341_Puts(75,70, &maChaine, &Font_11x18, ILI9341_COLOR_BLACK, ILI9341_COLOR_BROWN);
		/* Si on souhaite quitter l'exercice */
		if (BUTTON1_state_machine() == BUTTON_EVENT_LONG_PRESS) {
			state = FALSE;
		}
	}
	//rénitialiser la valeur du poid
	weight = 0;

}

void mode_jeu_vitesse(void) {
	bool_e state = TRUE;
	t = 100000;
	int32_t nombre_coup = 0 ;
	sprintf(maChaine,"Nombre coups : %d",nombre_coup);
	ILI9341_Puts(75,70, &maChaine, &Font_11x18, ILI9341_COLOR_BLACK, ILI9341_COLOR_BROWN);
	while (state && t!=0) {
		weight = hx711_weight(&hx711, 10 )  ;
		if (weight > 300) {
			nombre_coup += 1;
			sprintf(maChaine,"Nombre coups : %d",nombre_coup);
			ILI9341_Puts(75,70, &maChaine, &Font_11x18, ILI9341_COLOR_BLACK, ILI9341_COLOR_BROWN);
		}

		/* Si on souhaite quitter l'exercice */
		if (BUTTON1_state_machine() == BUTTON_EVENT_LONG_PRESS) {
			state = FALSE;
		}
	}
	sprintf(maChaine,"Resultat : %d      ",nombre_coup);
	// affichage du résultat
	ILI9341_Puts(75,70, &maChaine, &Font_11x18, ILI9341_COLOR_BLACK, ILI9341_COLOR_BROWN);
	state = TRUE;
	while (state) {
		if (BUTTON1_state_machine() == BUTTON_EVENT_LONG_PRESS) {
			state = FALSE;
		}
	}

}


void fonction_test(void) {
	state_machine() ;
	while (1) {
		weight = hx711_weight(&hx711, 10);
		sprintf(maChaine,"%f",weight);
		ILI9341_Puts(75,70, &maChaine, &Font_11x18, ILI9341_COLOR_BLACK, ILI9341_COLOR_BROWN);
	}

}
