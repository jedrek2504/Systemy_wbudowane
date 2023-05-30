#include "lpc17xx_pinsel.h"
#include "lpc17xx_i2c.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_timer.h"
#include "light.h"
#include "oled.h"
#include "temp.h"
#include "rgb.h"

static uint32_t msTicks = 0;


void SysTick_Handler(void) {
    msTicks++;
}

static uint32_t getTicks(void)
{
    return msTicks;
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

void changePwmBasedOnTemp(int32_t t)
{
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
}

void inverseColorsBasedOnLux(uint32_t l)
{
	if(l < 10)
	{
		oled_inverse(0);
	}
	else
	{
		oled_inverse(1);
	}
}

int main (void)
{

    int32_t temp = 0;
    uint32_t lux = 0;

    init_i2c();
    rgb_init();
    oled_init();
    light_init();
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

    oled_putString(1, 1 , (uint8_t*)"Temp   : ", OLED_COLOR_BLACK, OLED_COLOR_WHITE);
    oled_putString(1, 20, (uint8_t*)"Swiatlo: ", OLED_COLOR_BLACK, OLED_COLOR_WHITE );

    char str[10];
    char str2[10];


    while(1) {
        /* Temperature */
    	temp = temp_read();
    	sprintf(str,"%.1f", temp/10.0);

        /* light */
        lux = light_read();
        sprintf(str2, "%3d", lux);

        oled_fillRect((1+9*6),1, 80, 8, OLED_COLOR_WHITE);
        oled_putString((1+9*6),1, str, OLED_COLOR_BLACK, OLED_COLOR_WHITE);
        oled_putString((1+9*6),20, str2, OLED_COLOR_BLACK, OLED_COLOR_WHITE);

        uint16_t ledOn = 0;
        uint16_t ledOff = 0;

        changePwmBasedOnTemp(temp);

        inverseColorsBasedOnLux(lux);

        /* delay */
        Timer0_Wait(200);
    }

}
