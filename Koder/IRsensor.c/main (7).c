#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdbool.h>


static int USART_printChar (char c, FILE *stream)
{
	usart_send(c);
	return 0;
}

static FILE new_std_out = FDEV_SETUP_STREAM(USART_printChar,NULL, _FDEV_SETUP_WRITE);
void usart_init(){
	USART3.BAUD = 1667;
	USART3.CTRLB |= USART_RXEN_bm | USART_TXEN_bm | USART_RXMODE_NORMAL_gc;
	PORTB.PIN1CTRL |= PORT_PULLUPEN_bm;
	PORTB_DIR |= (1<<0);
	stdout = & new_std_out;
}

void usart_send(char c){
	while(!(USART3.STATUS & USART_DREIF_bm))
	{
		;
	}
	USART3.TXDATAL = c;
}

void adc_init() {
	VREF.ADC0REF |= VREF_REFSEL_VDD_gc;
	ADC0.MUXPOS |= ADC_MUXPOS_AIN11_gc;							//PD3=ADC_MUXPOS_AIN3_gc Kretskortet vårt bruker PE3=ADC_MUXPOS_AIN11_gc; 
	ADC0.CTRLA |= ADC_ENABLE_bm | ADC_RESSEL_10BIT_gc;
	ADC0.CTRLC |= ADC_PRESC_DIV16_gc;
	ADC0.COMMAND = ADC_STCONV_bm;
	ADC0.CTRLA |= ADC_FREERUN_bm;
}

uint16_t adc_get_value() {
	uint16_t adc_reading = ADC0.RES;
	return adc_reading;
}

void pwm_init()
{
	TCA0.SINGLE.PER = 40000UL;
	TCA0.SINGLE.CTRLA = (1<<0) | (1<<1);
	//TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV2_gc | TCA_SINGLE_ENABLE_bm;
	PORTMUX.TCAROUTEA = PORTMUX_TCA0_PORTF_gc;
	TCA0.SINGLE.CTRLB |= TCA_SINGLE_WGMODE_SINGLESLOPE_gc | PIN6_bm;
}

void servo(void)
{
	pwm_init();
	PORTF.DIR |= PIN2_bm;
	PORTF.PIN2CTRL |= PORT_INVEN_bm;
	TCA0.SINGLE.CMP2 = 3500;
	_delay_ms(1000);
	TCA0.SINGLE.CMP2 = 1100;
}

int main(void) {
	adc_init();
	usart_init();
	uint16_t distance;
	while (1) {
		distance = adc_get_value();
		static bool flag = false;

		if (distance > 400 && !flag)
		{
			printf("%u\n", distance);
			servo();
			//PORTC.OUTTGL |= (1 <<0);
			flag = true;
			_delay_ms(1000);
		}
		else if (distance <= 400)
		{
			
			flag = false;
			_delay_ms(1000);
			//PORTC_OUTTGL |= (1 <<0);
		}
	}
}

