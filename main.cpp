///////////////////////////////////////////////////////////////////////
//
//
//                  Fresh-Perspective (c) 2018
//
//							by Terminator
///////////////////////////////////////////////////////////////////////
//
//
//	 ,,,
//	 	Если ты это читаешь, и ты не программист - тебе не повезло!
//		Немедленно это закрой!!!!!!!!!!!!!!
//										,,,
//
///////////////////////////////////////////////////////////////////////
//
//
//	Обратная ситуация: ты - программист...
//---------------------------------------------------------
// 				#include <iostream>
//
// 				using namespace std;
//
// 				int main(){
// 					cout << "Hello, world!" << endl;
// 					return 0;
// 				}
//---------------------------------------------------------
//	Если вкратце, то "Добро пожаловать!".
//
//
///////////////////////////////////////////////////////////////////////
//
//					Немного о железе
//
//	Плата:							Arduino Uno
//	Датчик температуры:				DS18B20(RI002)
//	Bluetooth-модуль:				HC-05
//
///////////////////////////////////////////////////////////////////////	
//
//					Исходящие данные
//
//	temp=7 							Температура = 7
//	temp=-7 						Температура = -7
//	tempMIN=7 						Тревога! Температура = 7 (ниже нормы)
//	tempMIN=-7 						Тревога! Температура = -7 (ниже нормы)
//	tempMAX=7 						Тревога! Температура = 7 (выше нормы)
//	tempMAX=-7 						Тревога! Температура = -7 (выше нормы)
//
////////////////////////////////////////////////////////////////////////
//
//					Входящие команды
//
//	temp 							Отправь свежие показания датчика температуры
//	tempMIN? 						Отправь значение пользовательского минимума
//	tempMAX?						Отправь значение пользовательского максимума
//	tempMIN=7 						Установи пользовательский минимум = 7
//	tempMIN=-7 						Установи пользовательский минимум = -7
//	tempMAX=7 						Установи пользовательский максимум = 7
//	tempMAX=-7 						Установи пользовательский максимум = -7
//
////////////////////////////////////////////////////////////////////////

////////////////////////////////Programm////////////////////////////////

/////////////////////////////// Подключение библиотек //////////////////
#include <String.h>		// Для работы со строками
#include <OneWire.h>	// Для работы с датчиком температуры
#include <EEPROM.h>		// Для работы с энергонезависимой памятью Arduino


/////////////////////////////// Интервал измерений, мс /////////////////
#define INTERVAL_GET_DATA 5000 		// Arduino считывает температуру каждые 5 секунд 
#define INTERVAL_SEND_DATA 30000	// Arduino отправляет температуру серверу 30 секунд
/////////////////////////////// Номера PIN`ов //////////////////////////
#define DS18B20PIN 9	// PIN датчика температуры


///////////////////////////////	Создание объектов //////////////////////
OneWire ds(DS18B20PIN);	// OneWire (для датчика температуры) (указываем PIN датчика)


/////////////////////////////// Переменная для интервала... ////////////
unsigned long millis_int_getTemperature = 0;	// ...измерения температуры
unsigned long millis_int_sendBluetoth = 0;		// ...отправки температуры

////////////////////////////// Переменные для хранения пользовательских настроек
int minValueTemp = 0;	// Пользовательский минимум 					(адрес в EEPROM = 1)
int maxValueTemp = 0;	// Пользовательский максимум 					(адрес в EEPROM = 2)
int minCharTemp = 0;	// Знак пользовательского минумума 	(1+ || 2-) 	(адрес в EEPROM = 3)
int maxCharTemp = 0;	// Знак пользовательского максимума (1+ || 2-) 	(адрес в EEPROM = 4)


float temperature = 0;


void setup(void)
{
   	Serial.begin(9600);	// Запуск последовательного порта
   	//Serial.println("Enter AT commands:");
  	//BTSerial.begin(38400); // HC-05 default speed in AT command more

   	int a = EEPROM.read(0);
   	if(a != 255) {
   		minCharTemp = EEPROM.read(3);

   		if(minCharTemp == 1)
   			minValueTemp = EEPROM.read(1);
   		else if(minCharTemp == 2)
   			minValueTemp = -(EEPROM.read(1));

   		maxCharTemp = EEPROM.read(4);

   		if(maxCharTemp == 1)
   			maxValueTemp = EEPROM.read(2);
   		else if(maxCharTemp == 2)
   			maxValueTemp = -(EEPROM.read(2));
   	}
}


void loop(void)
{
	// Проверка времени для снятия показаний с датчика температуры
	if(millis()-millis_int_getTemperature >= INTERVAL_GET_DATA) {

		temperature = get_data_ds18b20(); // Снятие показаний с датчика температуры

		monitor();// 

		millis_int_getTemperature = millis(); // Старт интервала отсчета
   }

	// Проверка времени для отправки показаний с датчика температуры
   	if(millis()-millis_int_sendBluetoth >= INTERVAL_SEND_DATA) {
		
		sendData_Bluetooth();
		
		millis_int_sendBluetoth = millis(); // Старт интервала отсчета
   }

	if (Serial.available())
    	decoder(Serial.read());
}


///////////////////////////////Bluetooth/////////////////////////////////
void sendData_Bluetooth()
{
	Serial.print("temp=");Serial.println(temperature); // Вывод в монитор последовательного порта
}
/////////////////////////////////////////////////////////////////////////
void sendDataAlarmMin_Bluetooth()
{
	Serial.print("tempMIN=");Serial.println(temperature); // Вывод в монитор последовательного порта
}
/////////////////////////////////////////////////////////////////////////
void sendDataAlarmMax_Bluetooth()
{
	Serial.print("tempMAX=");Serial.println(temperature); // Вывод в монитор последовательного порта
}
/////////////////////////////////////////////////////////////////////////
void sendMin_Bluetooth()
{
	if(minCharTemp != 0)
		Serial.print("MIN=");Serial.println(minValueTemp); // Вывод в монитор последовательного порта
	else
		Serial.print("MIN=");Serial.println("NONE"); // Вывод в монитор последовательного порта
}
/////////////////////////////////////////////////////////////////////////
void sendMax_Bluetooth()
{
	if(minCharTemp != 0)
		Serial.print("MAX=");Serial.println(maxValueTemp); // Вывод в монитор последовательного порта
	else
		Serial.print("MAX=");Serial.println("NONE"); // Вывод в монитор последовательного порта
}
/////////////////////////////////////////////////////////////////////////


void decoder(String msg)
{
	if(msg == "temp")
		sendTemperature();
	else if(msg == "tempMIN?")
		sendMin_Bluetooth();
	else if(msg == "tempMAX?")
		sendMax_Bluetooth();
	else
		String command;

		for(int c = 0; c<10; c++) {
			command[c] = msg[c];

			if(msg[c] == '=')
				break;
		}

		if(command == "tempMIN=")
		{

		}
		else if(command == "tempMAX=")
		{

		}

}


// Выполнение команды сервера 'temp'
void sendTemperature()
{
	temperature = get_data_ds18b20();

	if(temperature < minValueTemp)
		sendDataAlarmMin_Bluetooth();
	else if(temperature > maxValueTemp)
		sendDataAlarmMax_Bluetooth();
	else
		sendData_Bluetooth();
}


// Проверка температуры на выход за пределы нормы 
void monitor()
{
	if(temperature < minValueTemp)
		sendDataAlarmMin_Bluetooth();
	if(temperature > maxValueTemp)
		sendDataAlarmMax_Bluetooth();
}


// получение данных с датчика DS18B20
float get_data_ds18b20()
{
	byte i;
	byte present = 0;
	byte data[12];
	byte address[8];
	int Temp;
	float fTemp=0.0;

	if ( !ds.search(address)) {
		Serial.print("No more addresses.\n");
		ds.reset_search();
		return 999;
	}
	// вывод в монитор уникального адреса 1-Wire устройства
	for( i = 0; i < 8; i++)  {
		Serial.print(address[i], HEX);
		    Serial.print(" ");
	}
	if ( OneWire::crc8( address, 7) != address[7]) {
	    Serial.print("CRC is not valid!\n");
	    return 999;
	}
	if ( address[0] != 0x28) {
	    Serial.print("Device is not a DS18S20 family device.\n");
	    return 999;
	}
	ds.reset();
	ds.select(address);
	// запустить конвертацию температуры датчиком
	ds.write(0x44,1);
	delay(750); // ждем 750 мс
	present = ds.reset();
	ds.select(address);
	ds.write(0xBE); 

	// считываем ОЗУ датчика
	for ( i = 0; i < 9; i++)
		data[i] = ds.read();

	Temp = ((data[1]<<8)+data[0]);
	// перевод в значение float
	fTemp=1.0*Temp/16+(float(Temp%16))*1.0/16;

	return fTemp;
}
