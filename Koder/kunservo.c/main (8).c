#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <util/delay.h>
#include <stdbool.h>

#define F_CPU 16000000UL

static uint8_t usart_transmitt_printf(char c, FILE *stream){
    usart_transmitt(c);
    return 0;
}

static FILE new_std_out = FDEV_SETUP_STREAM(usart_transmitt_printf, NULL, _FDEV_SETUP_WRITE);

void USART_Init() {
    PORTMUX.USARTROUTEA |= PORTMUX_USART2_ALT1_gc;
    PORTF.DIR |= 1<<4;
    PORTF.PIN5CTRL |= PORT_PULLUPEN_bm;
    USART2.BAUD = 1667; //9600
    USART2.CTRLB |= USART_RXEN_bm | USART_TXEN_bm | USART_RXMODE_NORMAL_gc; 
    USART2.CTRLC |= USART_SBMODE_bm | USART_CHSIZE_8BIT_gc; // 8 data bits, 1 stop bit, no parity
    stdout = &new_std_out; // address to struct
}

void usart_transmitt(char c){
    while(!(USART2.STATUS & USART_DREIF_bm)){
        ;
    }
    USART2.TXDATAL = c;
}

void adc_init() {
	VREF.ADC0REF |= VREF_REFSEL_VDD_gc;
	ADC0.MUXPOS |= ADC_MUXPOS_AIN3_gc; //PD3 37
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
	PORTMUX.TCAROUTEA = PORTMUX_TCA0_PORTF_gc;	//route strøm fra port a til c
	TCA0.SINGLE.CTRLB |= TCA_SINGLE_WGMODE_SINGLESLOPE_gc | PIN6_bm;
}

void servo(void)
{
	//servo 50 h< = 20ms som er det samme som 40000 i per verdi, utregnet i c.
	//servoen lytter kun når pulsbredden er 1-2 ms
	pwm_init();
	PORTF.DIR |= PIN2_bm;
	PORTF.PIN2CTRL |= PORT_INVEN_bm;
	//uint16_t pb = 1500;
	TCA0.SINGLE.CMP2 = 5000;
	_delay_ms(3000);
	TCA0.SINGLE.CMP2 = 1050;
	/*
	while (1)
	{
		/* while (pb <= 4500)
		{
			TCA0.SINGLE.CMP2BUF = pb;
			pb = (pb + 10);
			_delay_ms(2);
		}
		while (pb >= 1500)
		{
			TCA0.SINGLE.CMP2BUF = pb;
			pb = (pb - 10);
			_delay_ms(2);
		}
		*/
//	}
}

int main(void) {
    char received_data[20];
    uint8_t received_data_index = 0;
    USART_Init();
	adc_init();
	uint16_t distance;
    sei();
	PORTC.DIR |= (1 << 2);
	PORTC.DIR |= (1 << 3);
    while (1) {
		distance = adc_get_value();
		static bool flag = false;

		if (distance > 200 && !flag)
		{
			printf("%u\n", distance);
			//servo();
			flag = true;
		}
		else if (distance <= 260)
		{
			_delay_ms(300); //Når man fjerner eller forlenger dette delayet så påvirker det om AVReren mottar data eller ikke
			flag = false;
		}
		
		if (USART2.STATUS & USART_RXCIF_bm)
		{
			char c = USART2.RXDATAL;
			if (c == '\n') {
				received_data[received_data_index] = '\0';
				//printf("%s\n", received_data);
				received_data_index = 0;
				if (strcmp(received_data, "a") == 0) {
					//PORTC.OUTTGL = (1 << 2);
					//Problem: ved PWM_init funker ikke lenger LEDene.
					servo();
					_delay_ms(1500);
					servo();
				}
				else if (strcmp(received_data, "1") == 0) { //Må ha en kort streng med data sånn at han rekk å prosessere den.
					//PORTMUX.TCAROUTEA = !(PORTMUX_TCA0_PORTC_gc); //må skrives tilbake til 0.
					//PORTMUX.TCAROUTEA = !(PORTMUX_TCA0_PORTC_gc);
					//PORTC.OUTTGL = (1 << 3);
					servo();
				}
				} else {
				received_data[received_data_index] = c;
				received_data_index++;
			}
		}
    }
}
