#include <avr/io.h>
#include <util/twi.h> 
#include <avr/interrupt.h>

#define F_CPU 12000000UL					//Частота тактирования

/* настройка I2C- TWI */
#define TWI_SLAVE3_ADRESSE 0x73					//SLAVE ADRESSE 1...127: 125(7-bit)=250 (8-bit)
#define TWI_GENERAL_CALL_enable 0				//1=чтения устройства / 0=запись

volatile uint8_t TWI_SR_RECEIVED_BYTE[16];			//Буфер для принятых данных ( Master-Receiver-Mode)
volatile uint8_t received_bite_counter=0;
volatile uint8_t TWI_SR_MSG_Flag=0;
/*макросы*/
#define and  &&
#define or   ||
#define not  !=

void TWI_Init_Slave(uint8_t Slave_Adress)
{
  TWAR = (Slave_Adress << 1 | TWI_GENERAL_CALL_enable << TWGCE);
  TWCR = (1<<TWINT | 1<<TWEA | 1<<TWEN | 1<<TWIE);
}

void TWI_ACK()
{
  TWCR = (1<<TWINT | 1<<TWEA | 1<<TWEN | 1<<TWIE);
}

void TWI_ERROR ()
{
  TWCR = (1<<TWINT | 1<<TWEN | 1<<TWSTO);			//TWI STOP
  TWCR = 0x00;							//обнуление регистров
  TWI_Init_Slave(TWI_SLAVE3_ADRESSE);				//TWI инициализация как Slave
  TWI_ACK();
}


ISR(TWI_vect)
{
  switch (TWSR & 0xF8)						//получаем статус шины и выбираем дальнейшие дейсвия:
	{
	//SLAVE RECEIVER
	case TW_SR_SLA_ACK:
	  received_bite_counter=0;
	  TWI_ACK();
	  break;
	case TW_SR_ARB_LOST_SLA_ACK:
	  TWI_ACK();
	  break;
	case TW_SR_GCALL_ACK:
	  TWI_ACK();
	  break;
	case TW_SR_ARB_LOST_GCALL_ACK:
	  TWI_ACK();
	  break;
	case TW_SR_DATA_ACK:					// 0x80 Slave Receiver, получили байт данных
	  TWI_SR_RECEIVED_BYTE[received_bite_counter] = TWDR;
	  received_bite_counter++;
	  TWI_ACK();
	  break;
	case TW_SR_DATA_NACK:					// 0x88
	  TWI_ACK();
	  break;
	case TW_SR_GCALL_DATA_ACK:
	  TWI_ACK();
	  break;
	case TW_SR_GCALL_DATA_NACK:
	  TWI_ACK();
	  break;
	case TW_SR_STOP:						// 0xA0 STOP 
	  TWI_ACK();
	  TWI_SR_MSG_Flag=1;
	  break;
  //SLAVE TRANSMITTER
    case TW_ST_SLA_ACK:							//0xA8 
	  received_bite_counter=0;
	case TW_ST_DATA_ACK:						//0xB8 Slave Transmitter, мастер ждет данные
	  TWDR = TWI_SR_RECEIVED_BYTE[received_bite_counter];		//массив байтов данных для отправки
	  received_bite_counter++;
	  TWI_ACK();
	  break;
	case TW_ST_ARB_LOST_SLA_ACK:
	  TWI_ACK();
	  break;
	case TW_ST_DATA_NACK:						// 0xC0 
	  TWI_ACK();
	  break;
	case TW_ST_LAST_DATA:						// 0xC8 
	  TWI_ACK();
	  break;
	case TW_NO_INFO:
	  TWI_ACK();
	  break;
	case TW_BUS_ERROR:
	  TWI_ERROR();
	  TWI_ACK();
	  break;
	} 
}

int main(void)
{
	TWI_Init_Slave(TWI_SLAVE3_ADRESSE);					
	sei(); 
	TCCR1A|=(1<<COM1A1)|(1<<COM1B1)|(1<<WGM11);	//FAST-PWM - Mode 14
	TCCR1B|=(1<<WGM13)|(1<<WGM12)|(1<<CS11);	//PRESCALER=8 MODE 14(FAST PWM)
	ICR1=30000;					//TOP=30000 ->fPWM=50Hz, Period=20ms: TOP= (F_CPU / (PRESCALER * fPWM)) - 1
	DDRB|=(1<<PB1) | (1<<PB2);			//PWM Pins
	
	while(1)
	{
		if (TWI_SR_MSG_Flag==1) 		//если получили весь пакет
		{
			uint16_t SERVO1,SERVO2;				
			SERVO1=TWI_SR_RECEIVED_BYTE[0]|(TWI_SR_RECEIVED_BYTE[1]<<8); //собираем с 2 байт один uint16_t
			SERVO2=TWI_SR_RECEIVED_BYTE[2]|(TWI_SR_RECEIVED_BYTE[3]<<8); //сначала младший байт, потом старший
			OCR1A=SERVO1;								
			OCR1B=SERVO2;									
			TWI_SR_MSG_Flag=0;		//сбрасываем флаг получения пакета
			received_bite_counter=0;	//сбрасываем счетчик полученых байтов
		}				
	}
}
