/*****************************************************************************
 *   Peripherals such as temp sensor, light sensor, accelerometer,
 *   and trim potentiometer are monitored and values are written to
 *   the OLED display.
 *
 *   Copyright(C) 2010, Embedded Artists AB
 *   All rights reserved.
 *
 ******************************************************************************/



#include "lpc17xx_pinsel.h"
#include "lpc17xx_i2c.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_ssp.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_timer.h"


#include "light.h"
#include "oled.h"
#include "temp.h"
#include "acc.h"
#include "rgb.h"


static uint32_t msTicks = 0;
static uint8_t buf[10];


void SysTick_Handler(void) {
    msTicks++;
}

static uint32_t getTicks(void)
{
    return msTicks;
}

static void init_ssp(void)
{
	SSP_CFG_Type SSP_ConfigStruct;
	PINSEL_CFG_Type PinCfg;

	/*
	 * Initialize SPI pin connect
	 * P0.7 - SCK;
	 * P0.8 - MISO
	 * P0.9 - MOSI
	 * P2.2 - SSEL - used as GPIO
	 */
	PinCfg.Funcnum = 2;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;
	PinCfg.Portnum = 0;
	PinCfg.Pinnum = 7;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 8;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 9;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Funcnum = 0;
	PinCfg.Portnum = 2;
	PinCfg.Pinnum = 2;
	PINSEL_ConfigPin(&PinCfg);

	SSP_ConfigStructInit(&SSP_ConfigStruct);

	// Initialize SSP peripheral with parameter given in structure above
	SSP_Init(LPC_SSP1, &SSP_ConfigStruct);

	// Enable SSP peripheral
	SSP_Cmd(LPC_SSP1, ENABLE);

}

static void init_i2c(void)
{
	PINSEL_CFG_Type PinCfg;

	/* Initialize I2C2 pin connect */
	PinCfg.Funcnum = 2;
	PinCfg.Pinnum = 10;
	PinCfg.Portnum = 0;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 11;
	PINSEL_ConfigPin(&PinCfg);

	// Initialize I2C peripheral
	I2C_Init(LPC_I2C2, 100000);

	/* Enable I2C1 operation */
	I2C_Cmd(LPC_I2C2, ENABLE);
}

static void init_adc(void)
{
	PINSEL_CFG_Type PinCfg;

	/*
	 * Init ADC pin connect
	 * AD0.0 on P0.23
	 */
	PinCfg.Funcnum = 1;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;
	PinCfg.Pinnum = 23;
	PinCfg.Portnum = 0;
	PINSEL_ConfigPin(&PinCfg);

	/* Configuration for ADC :
	 * 	Frequency at 0.2Mhz
	 *  ADC channel 0, no Interrupt
	 */
	ADC_Init(LPC_ADC, 200000);
	ADC_IntConfig(LPC_ADC,ADC_CHANNEL_0,DISABLE);
	ADC_ChannelCmd(LPC_ADC,ADC_CHANNEL_0,ENABLE);

}

void PWM_Init(){
	LPC_SC->PCONP |= (1<<6);
	LPC_PWM1->PR = 0x18;
	LPC_PWM1->MR0 = 0x3e8;
	LPC_PWM1->MR1 = 0x0;
	LPC_PINCON->PINSEL4 |= (1<<0);
	LPC_PWM1->PCR |= (1<<9);
	LPC_PWM1->TCR = 0x9;
	LPC_PWM1->PC = 0x1;
	LPC_PWM1->LER = 0x3;
}

int main (void)
{

    int32_t t = 0;
    int isManual = 0;
    uint32_t lux = 0;
    uint32_t trim = 0;


    init_i2c();
    init_ssp();
    init_adc();
    rgb_init();

    oled_init();
    light_init();
    acc_init();

    temp_init (&getTicks);
    PWM_Init();

	if (SysTick_Config(SystemCoreClock / 1000)) {
		    while (1);  // Capture error
	}

    /*
     * Assume base board in zero-g position when reading first value.
     */
	LPC_SC->PCONP |= (1<<15);
	LPC_GPIO2->FIODIR &= ~(1<<10);
	LPC_GPIO2->FIOPIN |=(1<<10);


    light_enable();
    light_setRange(LIGHT_RANGE_4000);

    oled_clearScreen(OLED_COLOR_WHITE);

    oled_putString(1,1,  (uint8_t*)"Temp : ", OLED_COLOR_BLACK, OLED_COLOR_WHITE);
    oled_putString(1, 20, (uint8_t*)"Swiat : ", OLED_COLOR_BLACK, OLED_COLOR_WHITE );



    while(1) {


        /* Temperature */
    	char str[10];
    	char str2[10];
    	t = temp_read();
    	sprintf(str,"%.1f", t/10.0);


        /* light */
        lux = light_read();
        sprintf(str2, "%3d", lux);

        /* trimpot */
		//ADC_StartCmd(LPC_ADC,ADC_START_NOW);
		//Wait conversion complete
		//while (!(ADC_ChannelGetStatus(LPC_ADC,ADC_CHANNEL_0,ADC_DATA_DONE)));
		//trim = ADC_ChannelGetData(LPC_ADC,ADC_CHANNEL_0);


       // intToString(t, buf, 10, 10);
        oled_fillRect((1+9*6),1, 80, 8, OLED_COLOR_WHITE);
        oled_putString((1+9*6),1, str, OLED_COLOR_BLACK, OLED_COLOR_WHITE);
        oled_putString((1+9*6),20, str2, OLED_COLOR_BLACK, OLED_COLOR_WHITE);

        uint16_t ledOn = 0;
        uint16_t ledOff = 0;


        if(isManual == 1){
        	 LPC_PWM1->MR1 = 0;
        	 LPC_PWM1->LER = 0x2;
        }

          // power level 3 -> niebieskie rgb
          if(t >= 265)
          {
        	  rgb_setLeds(0x06);
        	  LPC_PWM1->MR1 = 1000;
        	  LPC_PWM1->LER = 0x2;
          }

          // power level 2 -> zielone rgb
          if (t >= 245 && t < 265)
          {
        	  rgb_setLeds(0x04);
        	  LPC_PWM1->MR1 = 750;
        	  LPC_PWM1->LER = 0x2;
          }

          // power level 1 -> zolty rgb
          if(t<245)
          {
        	   rgb_setLeds(0x05);
        	   LPC_PWM1->MR1 = 400;
        	   LPC_PWM1->LER = 0x2;

          }

//          if((LPC_GPIO2->FIOPIN & (1<<10)) == 0 && isManual == 0)
//          {
//        	  isManual = 1;
//          }
//
//          if((LPC_GPIO2->FIOPIN & (1<<10)) == 0 && isManual == 1)
//          {
//        	  isManual = 0;
//          }

          if(lux < 10) {
        	  oled_inverse(0);
          }
          else{
        	  oled_inverse(1);
          }

        /* delay */
        Timer0_Wait(200);
    }

}

void check_failed(uint8_t *file, uint32_t line)
{
	/* User can add his own implementation to report the file name and line number,
	 ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

	/* Infinite loop */
	while(1);
}
