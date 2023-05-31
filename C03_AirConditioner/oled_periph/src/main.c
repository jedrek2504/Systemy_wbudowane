#include "lpc17xx_pinsel.h"
#include "lpc17xx_i2c.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_ssp.h"
#include "light.h"
#include "oled.h"
#include "temp.h"
#include "rgb.h"

static uint32_t msTicks = 0;

/*!

@brief SysTick interrupt handler.
This function is the interrupt handler for the SysTick timer. It increments the value of the system tick counter.
*/
void SysTick_Handler(void) {
    msTicks++;
}

/*!

@brief Returns the value of the system tick counter.
This function returns the current value of the system tick counter, which represents the elapsed time in milliseconds since the system started.
@return The value of the system tick counter.
*/
static uint32_t getTicks(void)
{
    return msTicks;
}

/*!

@brief Initializes the SSP (Synchronous Serial Port) module.
This function initializes the SSP peripheral by configuring the necessary pins, setting up the SSP configuration structure, and enabling the SSP peripheral.
@param None
@return None
@side effects None
*/
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

/*!

@brief Initializes the I2C (Inter-Integrated Circuit) module.
This function initializes the I2C peripheral by configuring the necessary pins and enabling the I2C peripheral.
@param None
@return None
@side effects None
*/
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

/*!

@brief Initializes the PWM (Pulse Width Modulation) module.
This function initializes the PWM peripheral by setting up the necessary configuration registers and enabling the PWM peripheral.
@param None
@return None
@side effects None
*/
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

/*!

@brief Changes the PWM output and LED color based on the temperature value.
This function adjusts the PWM output and LED color based on the temperature value. It sets different power levels and corresponding PWM duty cycles, as well as LED colors, based on temperature ranges.
@param t The temperature value.
@return None
@side effects Changes the PWM output and LED colors.
*/
void changePwmBasedOnTemp(int32_t t)
{
	// power level 3 -> blue rgb
	if(t >= 265)
	{
		rgb_setLeds(0x06);
	    LPC_PWM1->MR1 = 1000;
	    LPC_PWM1->LER = 0x2;
	}

	// power level 2 -> green rgb
	if (t >= 245 && t < 265)
	{
	    rgb_setLeds(0x04);
	    LPC_PWM1->MR1 = 750;
	    LPC_PWM1->LER = 0x2;
	}

	// power level 1 -> yellow rgb
	if(t<245)
	{
	    rgb_setLeds(0x05);
	    LPC_PWM1->MR1 = 400;
	    LPC_PWM1->LER = 0x2;
	}
}

/*!

@brief Inverses the colors on the OLED display based on the lux value.
This function inverses the colors on the OLED display based on the lux value. If the lux value is less than 10, it sets the OLED display to non-inverse mode. Otherwise, it sets the OLED display to inverse mode.
@param l The lux value.
@return None
@side effects Changes the display color inversion on the OLED display.
*/
void inverseColorsBasedOnLux(uint32_t l)
{
	if(l < 10)
	{
		oled_inverse(0); /*text - white, background - black */
	}
	else
	{
		oled_inverse(1); /*text - black, background - white */
	}
}

int main (void)
{

    int32_t temp = 0;
    uint32_t lux = 0;

    init_i2c();
	init_ssp();
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
