#include <stdlib.h>
#include <math.h>
#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#define F_CPU 8000000UL
#define BAUD 9600
#define FOSC F_CPU
#define MYUBRR FOSC/16/BAUD-1
#define b 3169

//Variaveis GLobais
char buffer[5];
bool flag = false;
uint8_t digito0  = 0 ;
uint8_t digito1 = 0;
uint8_t estado = 0;
uint8_t digito_select0 = 0;
uint8_t digito_select1 = 0;
uint16_t val_int =0;
uint16_t temp_int = 0;
uint16_t temp_sel=0;
double dutyCycle = 0;
float Res_div=82;
float Vin;
float Vcc=5;
float Temp_Inicial=298.15;
float Res;
float Resc=0;
int Released_Level = 0;
int Pressed_Level = 0;
int Pressed =0;




//Funçao para Configurar Entradas e Saidas
void GPIO_Init(){
	DDRD &= ~(1 << PIND3)|(1 << PIND4);//Entrada Botoes PD3 e PD4(Botões escolher temp)
	DDRC &= ~(1 << PINC6);   //Entrada botao reset PC6
	DDRB |= (1<<PINB6)|(1<<PINB7); // Saida BJTs PB6 e PB7 (Controlar os displays)
	DDRD |= (1<<PIND2)| (1<<PIND6); //Saida Leds
	
	//Saida Display
	//A - PC5 //B - PC4 //C - PC3 //D - PC2	//E - PB0 //F - PD7 //G - PB2 //DT - PB1
	DDRB |= (1<<PINB2)|(1<<PINB1)|(1<<PINB0);
	DDRC |= (1<<PINC5)|(1<<PINC4)|(1<<PINC3)|(1<<PINC2);
	DDRD |= (1<<PIND7);

}

//Funcoes para Transmitir dados para a Sorta Serie
void USART_Init (unsigned int ubrr){
	UBRR0H =  (ubrr>>8);//(unsigned char)
	UBRR0L  = ubrr;//(unsigned char)
	UCSR0B = (1<<RXEN0) | (1<<TXEN0);
	UCSR0C = (1<<USBS0) | (3<< UCSZ00);
}
void USART_Transmit (unsigned char data){
	while (! (UCSR0A & (1<<UDRE0)));
	UDR0=data;
}
void USART_putstring(char* StringPtr){

	while(*StringPtr != 0x00){
		USART_Transmit(*StringPtr);
	StringPtr++;}

}

//Funçao para definir a temperatura que o utilizador deseja
void set_temp()
{
	if(((PIND&(1<<PIND4)) == 0) || ((PIND&(1<<PIND3)) == 0))
	{
		flag = true;
		if ((PIND&(1<<PIND4)) == 0)
		{
			Pressed_Level ++;
			if(Pressed_Level > 0)
			{
				if(Pressed == 1)
				{
					Pressed = 0;
					digito_select1 = digito_select1 +1;
					if (digito_select1 > 9)
					{
						digito_select1 = 0;
					}
				}
				Pressed_Level  = 0;
			}
		}
		else
		{
			Released_Level ++;
			if (Released_Level > 0 )
			{
				Pressed = 1;
				Released_Level =0;
			}
		}
		///////////////////////////////////
		if ((PIND&(1<<PIND3)) == 0)
		{
			Pressed_Level ++;
			if(Pressed_Level > 0)
			{
				if(Pressed == 1)
				{
					Pressed = 0;
					digito_select0 = digito_select0 +1;
					if (digito_select0 > 9)
					{
						digito_select0 = 0;
					}
				}
				Pressed_Level  = 0;
			}
		}
		else
		{
			Released_Level ++;
			if (Released_Level > 0 )
			{
				Pressed = 1;
				Released_Level =0;
			}
		}
		////////////////////////////////////////
		if (((PIND&(1<<PIND3)) == 0) && ((PIND&(1<<PIND4)) == 0) )
		{
			flag = false;
			temp_sel  = ((digito_select1*10)+ digito_select0);
		}
	}
} //Void

//Funçao de Inicializacao do ADC
void initADC_NTC_Interno()
{	
	ADCSRA  = (1 << ADEN) | (7 << ADPS0);
	ADCSRB = 0b00000000;
	ADMUX = (1 << REFS0) | (0 << REFS1) | (1 << MUX0); 
}

//Funçao de ler o ADC
uint16_t ler_adc(){
	ADCSRA |= (1 << ADSC);         // começa a conversao
	while (ADCSRA & (1 << ADSC)); // espera que a conversao acabe
	return ADCW;
}

//Funçao de Inicializacao do PWM
uint16_t PWM_Init(){
	DDRD |= (1<<PIND5);
	TCCR0A |= (1 << WGM01) | (1 << WGM00)| (1 << COM0B1);
	TIMSK0 |= (1 << TOIE1);
	OCR0B = 0;
	TCCR0B |= (1 << CS00);
}

//Funçao de ativar o PWM
void PWM_Duty_Set(dutyCycle){
	OCR0B= (dutyCycle/100)*255;
}

//Interrupçao
ISR (TIMER0_OVF_vect){
	switch(estado)
	{
		case 0:
		if (flag == true){
			num_digito_select0();
			} else {
			num_digito0();
		}
		estado = estado+1;
		break;

		case 1:
		PORTB  &= ~ (1<<PINB7);
		PORTB &= ~(1<<PINB6);
		PORTC &= ~ (1<<PINC5);
		PORTC  &= ~ (1<<PINC4);
		PORTC  &= ~ (1<<PINC3);
		PORTC &= ~ (1<<PINC2);
		PORTB &= ~ (1<<PINB2);
		PORTB  &= ~ (1<<PINB1);
		PORTB &= ~ (1<<PINB0);
		PORTD &= ~ (1<<PIND7);
		estado = estado+1;
		break;

		case 2:
		if(flag == true){
			num_digito_select1();
			} else {
			num_digito1();
		}
		estado = estado+1;
		break;
		case 3:
		PORTB  &= ~ (1<<PINB7);
		PORTB &= ~(1<<PINB6);
		PORTC &= ~ (1<<PINC5);
		PORTC  &= ~ (1<<PINC4);
		PORTC  &= ~ (1<<PINC3);
		PORTC &= ~ (1<<PINC2);
		PORTB &= ~ (1<<PINB2);
		PORTB  &= ~ (1<<PINB1);
		PORTB &= ~ (1<<PINB0);
		PORTD &= ~ (1<<PIND7);
		estado = 0;
		break;
	}
}

int main(void)
{

	GPIO_Init();            //Funçao definir entradas e saidas
	USART_Init(MYUBRR);	    //Funçao iniciar  porta serie
	PWM_Init();             //Funçao iniciar o PWM
	initADC_NTC_Interno();  //Funçao iniciar o ADC
	sei();                  //Ativar Interrupçoes Globais

	while (1)
	{

		//Botoes
		set_temp();

		//Calculo da Temperatura
		val_int= ler_adc();                         // Valor Retirado do ADC
		Vin=((val_int*(float)Vcc)/((float)1023));   //Tensao de entrada
		Res=(((Vin*Res_div)/(((float)Vcc)-Vin)));   //Resistencia NTC

		for (int i= 0; i<(600);i++)
		{
			Resc = (Temp_Inicial *(exp(b*((1/25)-(1/((float)i)*0.1))))); //Formula para saber qual a temp = res calculada
			if (Resc >= Res)
			{
				break;
			}
			temp_int = i*0.1; //Resoluçao
		}

		//Serial Print
		itoa((temp_sel), buffer, 10);
		USART_putstring(buffer);
		USART_Transmit('\n');

		//Separa os digitos para saber qual é qual

		digito1 = temp_int / 10;	      //Digito da Esquerda
		digito0 = ((int)temp_int % 10);	  //Digito da Direita
		
		//Ligar o PWM
		if(temp_int <= temp_sel) //Caso a temperatura Atual seja maior que a que queremos
		{
			PORTD &= ~(1<<PIND2);  // Desliga o Sinalizador de Temperatura OK
			PORTD |= (1<<PIND6);  // Liga o Sinalizador de Temperatura NOK
			PWM_Duty_Set(100);    //Liga o aquecedor
		}
		if(temp_int >= temp_sel)
		{
			PORTD &= ~(1<<PIND6); // Desliga o Sinalizador de Temperatura NOK
			PORTD |= (1<<PIND2);  // Liga o Sinalizador de Temperatura OK
			PWM_Duty_Set(0);      //Desliga o aquecedor
		}

	}//Loop
}//Main

void num_digito_select0(){

	switch(digito_select0){
		case 0: //Digito 0
		PORTB |= (1<<PINB7);//Direita
		PORTB &= ~ (1<<PINB6);//Esquerda
		PORTC |= (1<<PINC5);
		PORTC |= (1<<PINC4);
		PORTC |= (1<<PINC3);
		PORTC |= (1<<PINC2);
		PORTB &= ~ (1<<PINB2);
		PORTB |= (1<<PINB1);
		PORTB |= (1<<PINB0);
		PORTD |= (1<<PIND7);
		break;
		
		case 1: //Digito 1
		PORTB |= (1<<PINB7);
		PORTB &= ~(1<<PINB6);
		PORTC &= ~ (1<<PINC5);
		PORTC |= (1<<PINC4);
		PORTC |= (1<<PINC3);
		PORTC &= ~ (1<<PINC2);
		PORTB &= ~ (1<<PINB2);
		PORTB |= (1<<PINB1);
		PORTB &= ~ (1<<PINB0);
		PORTD &= ~ (1<<PIND7);
		break;
		
		case 2: //Digito 2
		PORTB |= (1<<PINB7);
		PORTB &= ~(1<<PINB6);
		PORTC |= (1<<PINC5);
		PORTC |= (1<<PINC4);
		PORTC &= ~ (1<<PINC3);
		PORTC |= (1<<PINC2);
		PORTB |= (1<<PINB2);
		PORTB |= (1<<PINB1);
		PORTB |= (1<<PINB0);
		PORTD &= ~ (1<<PIND7);
		break;
		
		case 3://Digito 3
		PORTB |= (1<<PINB7);
		PORTB &= ~(1<<PINB6);
		PORTC |= (1<<PINC5);
		PORTC |= (1<<PINC4);
		PORTC |= (1<<PINC3);
		PORTC |= (1<<PINC2);
		PORTB |= (1<<PINB2);
		PORTB |= (1<<PINB1);
		PORTB &= ~ (1<<PINB0);
		PORTD &= ~ (1<<PIND7);
		break;
		
		case 4://Digito 4
		PORTB |= (1<<PINB7);
		PORTB &= ~(1<<PINB6);
		PORTC &= ~ (1<<PINC5);
		PORTC |= (1<<PINC4);
		PORTC |= (1<<PINC3);
		PORTC &= ~ (1<<PINC2);
		PORTB |= (1<<PINB2);
		PORTB |= (1<<PINB1);
		PORTB &= ~ (1<<PINB0);
		PORTD |= (1<<PIND7);
		break;
		
		case 5://Digito 5
		PORTB |= (1<<PINB7);
		PORTB &= ~(1<<PINB6);
		PORTC |= (1<<PINC5);
		PORTC &= ~ (1<<PINC4);
		PORTC |= (1<<PINC3);
		PORTC |= (1<<PINC2);
		PORTB |= (1<<PINB2);
		PORTB |= (1<<PINB1);
		PORTB &= ~ (1<<PINB0);
		PORTD |= (1<<PIND7);
		break;
		
		case 6://Digito 6
		PORTB |= (1<<PINB7);
		PORTB &= ~(1<<PINB6);
		PORTC |= (1<<PINC5);
		PORTC &= ~ (1<<PINC4);
		PORTC |= (1<<PINC3);
		PORTC |= (1<<PINC2);
		PORTB |= (1<<PINB2);
		PORTB |= (1<<PINB1);
		PORTB |= (1<<PINB0);
		PORTD |= (1<<PIND7);
		break;
		
		case 7://Digito 7
		PORTB |= (1<<PINB7);
		PORTB &= ~(1<<PINB6);
		PORTC |= (1<<PINC5);
		PORTC |= (1<<PINC4);
		PORTC |= (1<<PINC3);
		PORTC &= ~ (1<<PINC2);
		PORTB &= ~ (1<<PINB2);
		PORTB |= (1<<PINB1);
		PORTB &= ~ (1<<PINB0);
		PORTD &= ~ (1<<PIND7);
		break;
		
		case 8: //Digito 8
		PORTB |= (1<<PINB7);
		PORTB &= ~(1<<PINB6);
		PORTC |= (1<<PINC5);
		PORTC |= (1<<PINC4);
		PORTC |= (1<<PINC3);
		PORTC |= (1<<PINC2);
		PORTB |= (1<<PINB2);
		PORTB |= (1<<PINB1);
		PORTB |= (1<<PINB0);
		PORTD |= (1<<PIND7);
		break;
		
		case 9://Digito 9
		PORTB |= (1<<PINB7);
		PORTB &= ~(1<<PINB6);
		PORTC |= (1<<PINC5);
		PORTC |= (1<<PINC4);
		PORTC |= (1<<PINC3);
		PORTC &= ~(1<<PINC2);
		PORTB |= (1<<PINB2);
		PORTB |= (1<<PINB1);
		PORTB &= ~(1<<PINB0);
		PORTD |= (1<<PIND7);
		break;
		
	}//Switch
}//Void
void num_digito0(){

	switch(digito0){
		case 0: //Digito 0
		PORTB |= (1<<PINB7);//Direita
		PORTB &= ~ (1<<PINB6);//Esquerda
		PORTC |= (1<<PINC5);
		PORTC |= (1<<PINC4);
		PORTC |= (1<<PINC3);
		PORTC |= (1<<PINC2);
		PORTB &= ~ (1<<PINB2);
		PORTB |= (1<<PINB1);
		PORTB |= (1<<PINB0);
		PORTD |= (1<<PIND7);
		break;
		
		case 1: //Digito 1
		PORTB |= (1<<PINB7);
		PORTB &= ~(1<<PINB6);
		PORTC &= ~ (1<<PINC5);
		PORTC |= (1<<PINC4);
		PORTC |= (1<<PINC3);
		PORTC &= ~ (1<<PINC2);
		PORTB &= ~ (1<<PINB2);
		PORTB |= (1<<PINB1);
		PORTB &= ~ (1<<PINB0);
		PORTD &= ~ (1<<PIND7);
		break;
		
		case 2: //Digito 2
		PORTB |= (1<<PINB7);
		PORTB &= ~(1<<PINB6);
		PORTC |= (1<<PINC5);
		PORTC |= (1<<PINC4);
		PORTC &= ~ (1<<PINC3);
		PORTC |= (1<<PINC2);
		PORTB |= (1<<PINB2);
		PORTB |= (1<<PINB1);
		PORTB |= (1<<PINB0);
		PORTD &= ~ (1<<PIND7);
		break;
		
		case 3://Digito 3
		PORTB |= (1<<PINB7);
		PORTB &= ~(1<<PINB6);
		PORTC |= (1<<PINC5);
		PORTC |= (1<<PINC4);
		PORTC |= (1<<PINC3);
		PORTC |= (1<<PINC2);
		PORTB |= (1<<PINB2);
		PORTB |= (1<<PINB1);
		PORTB &= ~ (1<<PINB0);
		PORTD &= ~ (1<<PIND7);
		break;
		
		case 4://Digito 4
		PORTB |= (1<<PINB7);
		PORTB &= ~(1<<PINB6);
		PORTC &= ~ (1<<PINC5);
		PORTC |= (1<<PINC4);
		PORTC |= (1<<PINC3);
		PORTC &= ~ (1<<PINC2);
		PORTB |= (1<<PINB2);
		PORTB |= (1<<PINB1);
		PORTB &= ~ (1<<PINB0);
		PORTD |= (1<<PIND7);
		break;
		
		case 5://Digito 5
		PORTB |= (1<<PINB7);
		PORTB &= ~(1<<PINB6);
		PORTC |= (1<<PINC5);
		PORTC &= ~ (1<<PINC4);
		PORTC |= (1<<PINC3);
		PORTC |= (1<<PINC2);
		PORTB |= (1<<PINB2);
		PORTB |= (1<<PINB1);
		PORTB &= ~ (1<<PINB0);
		PORTD |= (1<<PIND7);
		break;
		
		case 6://Digito 6
		PORTB |= (1<<PINB7);
		PORTB &= ~(1<<PINB6);
		PORTC |= (1<<PINC5);
		PORTC &= ~ (1<<PINC4);
		PORTC |= (1<<PINC3);
		PORTC |= (1<<PINC2);
		PORTB |= (1<<PINB2);
		PORTB |= (1<<PINB1);
		PORTB |= (1<<PINB0);
		PORTD |= (1<<PIND7);
		break;
		
		case 7://Digito 7
		PORTB |= (1<<PINB7);
		PORTB &= ~(1<<PINB6);
		PORTC |= (1<<PINC5);
		PORTC |= (1<<PINC4);
		PORTC |= (1<<PINC3);
		PORTC &= ~ (1<<PINC2);
		PORTB &= ~ (1<<PINB2);
		PORTB |= (1<<PINB1);
		PORTB &= ~ (1<<PINB0);
		PORTD &= ~ (1<<PIND7);
		break;
		
		case 8: //Digito 8
		PORTB |= (1<<PINB7);
		PORTB &= ~(1<<PINB6);
		PORTC |= (1<<PINC5);
		PORTC |= (1<<PINC4);
		PORTC |= (1<<PINC3);
		PORTC |= (1<<PINC2);
		PORTB |= (1<<PINB2);
		PORTB |= (1<<PINB1);
		PORTB |= (1<<PINB0);
		PORTD |= (1<<PIND7);
		break;
		
		case 9://Digito 9
		PORTB |= (1<<PINB7);
		PORTB &= ~(1<<PINB6);
		PORTC |= (1<<PINC5);
		PORTC |= (1<<PINC4);
		PORTC |= (1<<PINC3);
		PORTC &= ~(1<<PINC2);
		PORTB |= (1<<PINB2);
		PORTB |= (1<<PINB1);
		PORTB &= ~(1<<PINB0);
		PORTD |= (1<<PIND7);
		break;
		
	}//Switch
}//Void

void num_digito_select1(){

	switch(digito_select1){
		case 0: //Digito 0
		PORTB |= (1<<PINB6);
		PORTB &= ~(1<<PINB7);
		PORTC |= (1<<PINC5);
		PORTC |= (1<<PINC4);
		PORTC |= (1<<PINC3);
		PORTC |= (1<<PINC2);
		PORTB &= ~ (1<<PINB2);
		PORTB |= (1<<PINB1);
		PORTB |= (1<<PINB0);
		PORTD |= (1<<PIND7);
		break;
		
		case 1: //Digito 1
		PORTB |= (1<<PINB6);
		PORTB &= ~(1<<PINB7);
		PORTC &= ~ (1<<PINC5);
		PORTC |= (1<<PINC4);
		PORTC |= (1<<PINC3);
		PORTC &= ~ (1<<PINC2);
		PORTB &= ~ (1<<PINB2);
		PORTB |= (1<<PINB1);
		PORTB &= ~ (1<<PINB0);
		PORTD &= ~ (1<<PIND7);
		break;
		
		case 2: //Digito 2
		PORTB |= (1<<PINB6);
		PORTB &= ~(1<<PINB7);
		PORTC |= (1<<PINC5);
		PORTC |= (1<<PINC4);
		PORTC &= ~ (1<<PINC3);
		PORTC |= (1<<PINC2);
		PORTB |= (1<<PINB2);
		PORTB |= (1<<PINB1);
		PORTB |= (1<<PINB0);
		PORTD &= ~ (1<<PIND7);
		break;
		
		case 3://Digito 3
		PORTB &= ~(1<<PINB7);
		PORTB |= (1<<PINB6);
		PORTC |= (1<<PINC5);
		PORTC |= (1<<PINC4);
		PORTC |= (1<<PINC3);
		PORTC |= (1<<PINC2);
		PORTB |= (1<<PINB2);
		PORTB |= (1<<PINB1);
		PORTB &= ~ (1<<PINB0);
		PORTD &= ~ (1<<PIND7);
		break;
		
		case 4://Digito 4
		PORTB |= (1<<PINB6);
		PORTB &= ~(1<<PINB7);
		PORTC &= ~ (1<<PINC5);
		PORTC |= (1<<PINC4);
		PORTC |= (1<<PINC3);
		PORTC &= ~ (1<<PINC2);
		PORTB |= (1<<PINB2);
		PORTB |= (1<<PINB1);
		PORTB &= ~ (1<<PINB0);
		PORTD |= (1<<PIND7);
		break;
		
		case 5://Digito 5
		PORTB |= (1<<PINB6);
		PORTB &= ~(1<<PINB7);
		PORTC |= (1<<PINC5);
		PORTC &= ~ (1<<PINC4);
		PORTC |= (1<<PINC3);
		PORTC |= (1<<PINC2);
		PORTB |= (1<<PINB2);
		PORTB |= (1<<PINB1);
		PORTB &= ~ (1<<PINB0);
		PORTD |= (1<<PIND7);
		break;
		
		case 6://Digito 6
		PORTB |= (1<<PINB6);
		PORTB &= ~(1<<PINB7);
		PORTC |= (1<<PINC5);
		PORTC &= ~ (1<<PINC4);
		PORTC |= (1<<PINC3);
		PORTC |= (1<<PINC2);
		PORTB |= (1<<PINB2);
		PORTB |= (1<<PINB1);
		PORTB |= (1<<PINB0);
		PORTD |= (1<<PIND7);
		break;
		
		case 7://Digito 7
		PORTB |= (1<<PINB6);
		PORTB &= ~(1<<PINB7);
		PORTC |= (1<<PINC5);
		PORTC |= (1<<PINC4);
		PORTC |= (1<<PINC3);
		PORTC &= ~ (1<<PINC2);
		PORTB &= ~ (1<<PINB2);
		PORTB |= (1<<PINB1);
		PORTB &= ~ (1<<PINB0);
		PORTD &= ~ (1<<PIND7);
		break;
		
		case 8: //Digito 8
		PORTB |= (1<<PINB6);
		PORTB &= ~(1<<PINB7);
		PORTC |= (1<<PINC5);
		PORTC |= (1<<PINC4);
		PORTC |= (1<<PINC3);
		PORTC |= (1<<PINC2);
		PORTB |= (1<<PINB2);
		PORTB |= (1<<PINB1);
		PORTB |= (1<<PINB0);
		PORTD |= (1<<PIND7);
		break;
		
		case 9://Digito 9
		PORTB |= (1<<PINB6);
		PORTB &= ~(1<<PINB7);
		PORTC |= (1<<PINC5);
		PORTC |= (1<<PINC4);
		PORTC |= (1<<PINC3);
		PORTC &= ~(1<<PINC2);
		PORTB |= (1<<PINB2);
		PORTB |= (1<<PINB1);
		PORTB &= ~(1<<PINB0);
		PORTD |= (1<<PIND7);
		break;
		
	}//Switch
}//Void
void num_digito1()
{
	switch(digito1)
	{
		case 0: //Digito 0
		PORTB |= (1<<PINB6);
		PORTB &= ~(1<<PINB7);
		PORTC |= (1<<PINC5);
		PORTC |= (1<<PINC4);
		PORTC |= (1<<PINC3);
		PORTC |= (1<<PINC2);
		PORTB &= ~ (1<<PINB2);
		PORTB |= (1<<PINB1);
		PORTB |= (1<<PINB0);
		PORTD |= (1<<PIND7);
		break;
		
		case 1: //Digito 1
		PORTB |= (1<<PINB6);
		PORTB &= ~(1<<PINB7);
		PORTC &= ~ (1<<PINC5);
		PORTC |= (1<<PINC4);
		PORTC |= (1<<PINC3);
		PORTC &= ~ (1<<PINC2);
		PORTB &= ~ (1<<PINB2);
		PORTB |= (1<<PINB1);
		PORTB &= ~ (1<<PINB0);
		PORTD &= ~ (1<<PIND7);
		break;
		
		case 2: //Digito 2
		PORTB |= (1<<PINB6);
		PORTB &= ~(1<<PINB7);
		PORTC |= (1<<PINC5);
		PORTC |= (1<<PINC4);
		PORTC &= ~ (1<<PINC3);
		PORTC |= (1<<PINC2);
		PORTB |= (1<<PINB2);
		PORTB |= (1<<PINB1);
		PORTB |= (1<<PINB0);
		PORTD &= ~ (1<<PIND7);
		break;
		
		case 3://Digito 3
		PORTB &= ~(1<<PINB7);
		PORTB |= (1<<PINB6);
		PORTC |= (1<<PINC5);
		PORTC |= (1<<PINC4);
		PORTC |= (1<<PINC3);
		PORTC |= (1<<PINC2);
		PORTB |= (1<<PINB2);
		PORTB |= (1<<PINB1);
		PORTB &= ~ (1<<PINB0);
		PORTD &= ~ (1<<PIND7);
		break;
		
		case 4://Digito 4
		PORTB |= (1<<PINB6);
		PORTB &= ~(1<<PINB7);
		PORTC &= ~ (1<<PINC5);
		PORTC |= (1<<PINC4);
		PORTC |= (1<<PINC3);
		PORTC &= ~ (1<<PINC2);
		PORTB |= (1<<PINB2);
		PORTB |= (1<<PINB1);
		PORTB &= ~ (1<<PINB0);
		PORTD |= (1<<PIND7);
		break;
		
		case 5://Digito 5
		PORTB |= (1<<PINB6);
		PORTB &= ~(1<<PINB7);
		PORTC |= (1<<PINC5);
		PORTC &= ~ (1<<PINC4);
		PORTC |= (1<<PINC3);
		PORTC |= (1<<PINC2);
		PORTB |= (1<<PINB2);
		PORTB |= (1<<PINB1);
		PORTB &= ~ (1<<PINB0);
		PORTD |= (1<<PIND7);
		break;
		
		case 6://Digito 6
		PORTB |= (1<<PINB6);
		PORTB &= ~(1<<PINB7);
		PORTC |= (1<<PINC5);
		PORTC &= ~ (1<<PINC4);
		PORTC |= (1<<PINC3);
		PORTC |= (1<<PINC2);
		PORTB |= (1<<PINB2);
		PORTB |= (1<<PINB1);
		PORTB |= (1<<PINB0);
		PORTD |= (1<<PIND7);
		break;
		
		case 7://Digito 7
		PORTB |= (1<<PINB6);
		PORTB &= ~(1<<PINB7);
		PORTC |= (1<<PINC5);
		PORTC |= (1<<PINC4);
		PORTC |= (1<<PINC3);
		PORTC &= ~ (1<<PINC2);
		PORTB &= ~ (1<<PINB2);
		PORTB |= (1<<PINB1);
		PORTB &= ~ (1<<PINB0);
		PORTD &= ~ (1<<PIND7);
		break;
		
		case 8: //Digito 8
		PORTB |= (1<<PINB6);
		PORTB &= ~(1<<PINB7);
		PORTC |= (1<<PINC5);
		PORTC |= (1<<PINC4);
		PORTC |= (1<<PINC3);
		PORTC |= (1<<PINC2);
		PORTB |= (1<<PINB2);
		PORTB |= (1<<PINB1);
		PORTB |= (1<<PINB0);
		PORTD |= (1<<PIND7);
		break;
		
		case 9://Digito 9
		PORTB |= (1<<PINB6);
		PORTB &= ~(1<<PINB7);
		PORTC |= (1<<PINC5);
		PORTC |= (1<<PINC4);
		PORTC |= (1<<PINC3);
		PORTC &= ~(1<<PINC2);
		PORTB |= (1<<PINB2);
		PORTB |= (1<<PINB1);
		PORTB &= ~(1<<PINB0);
		PORTD |= (1<<PIND7);
		break;
		
	}//Switch
}//Void



