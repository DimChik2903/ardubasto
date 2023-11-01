#include <EEPROM.h>


enum Cells {
ResetNumber_cell,      //0
TimeWebasto_cell,      //1
ProtocolSTART_cell,    //2
StartByte_cell,        //3
ProtocolSTATUS_cell,   //4
HeaterBUS_cell,        //5
WBUS_VER_cell,         //6
delta_cell,            //7
CentrLock_cell,        //8
EngimpTime_cell,       //9
PumpeTime_cell,        //10
sizeNumber_cell,       //11
sizeCells,
TelNumber1_cell =20,  //20
TelNumber2_cell =40,  //40
DallasAddr_cell =60   //60
};


// названия температур котла: 
enum TempC {VyhlopC, EngineC, UlicaC, SalonC, size_arrayTemp};

// возможные протоколы чтения статуса котла:

enum ProtocolSTATUS_ {
   STATUSBUS=1,                 // статусы котла читаются по цифровой шине
   ANALOG                     // статусы котла считываются силами ардуино - по датчикам 
};                   


// возможные протоколы запуска котла:

enum ProtocolSTART_  {
   STARTBUS=1,                  // запуск котла происходит по цифровой шине
   IMPULSE,                   // запуск котла происходит импульсом GND (для подпайки к кнопке пуск на таймере котла)
   POTENCIAL                  // запуск котла происходит подачей потенциала +12В (пока плюс висит - котёл работает)
};        

// возможные типы котла: 

enum Heater_  { TTC_E=1, WBUS, HYDRONIC};                 


//--------------------ниже здесь устанавливаем какие настройки у девайса будут по умолчанию

byte ProtocolSTATUS = STATUSBUS;  // в данном случае статусы читаются по цифровой шине                 
byte ProtocolSTART  = STARTBUS;   // в данном случае запуск котла происходит по цифровой шине                 
byte Heater         = WBUS;       // в данном случае тип шины WBUS
byte StartByte      = 0x20;       // в данном случае байт на старт котла 0x20 (для шины w-bus)
byte worktime       = 30;         // в данном случае время цикла работы котла 30 минут 
byte deltaT         = 45;         // разница температур улицы и выхлопа, выше которой считается, что котёл успешно стартанул

byte sizeTelNumber  = 12;         // количество символов в номере телефона, включая +. Т.е. +79121234567 это будет 12 символов

byte WBUStype  = 0x12;            // версия шины w-bus 

bool CentrLock  = 1;              // управление ЦЗ вместо помпы и старта ДВС (1 - активно, 0 - неактивно) кому нужно помпу ставим 0!!!
byte PumpeTime  = 1;              // время работы помпы      (ЦЗ открыть), сек
byte EngimpTime = 1;              // время импульса стартДВС (ЦЗ закрыть), сек

//сюда пишем адреса датчиков даллас (левые 8 байт): 

byte DS18B20 [size_arrayTemp][10] = {
{0x28, 0xFF, 0xB2, 0xB5, 0xC1, 0x17, 0x05, 0xD1,    VyhlopC,  -100}, 
{0x28, 0xFF, 0xD3, 0xE2, 0xC1, 0x17, 0x04, 0x0D,    EngineC,  -100}, 
{0x28, 0xFF, 0xF8, 0xBC, 0xC1, 0x17, 0x04, 0x48,    UlicaC,   -100},  
{0x28, 0xFF, 0x3F, 0xB7, 0xC1, 0x17, 0x05, 0xF1,    SalonC,   -100}
};

//----------------------


void setup() {
 
  pinMode(13, OUTPUT);
  digitalWrite(13, 0);
  
  EEPROM.write(ResetNumber_cell, 0);
  EEPROM.write(TimeWebasto_cell, worktime); 
  EEPROM.write(ProtocolSTART_cell, ProtocolSTART);
  EEPROM.write(StartByte_cell, StartByte);   
  EEPROM.write(ProtocolSTATUS_cell, ProtocolSTATUS);
  EEPROM.write(HeaterBUS_cell, Heater);
//EEPROM.write(WBUS_VER_cell, WBUS_VER);
  EEPROM.write(delta_cell, deltaT);   
  EEPROM.write(CentrLock_cell, CentrLock);
  EEPROM.write(EngimpTime_cell,EngimpTime);
  EEPROM.write(PumpeTime_cell, PumpeTime);
  EEPROM.write(sizeNumber_cell, sizeTelNumber);   
 
  
  for (int i = sizeCells ; i < DallasAddr_cell ; i++) EEPROM.write(i, '0');    // стираем ячейки под номера телефонов


// ниже записываем в еепром адреса датчиков даллас: 

for (byte i=0; i<sizeof(DS18B20)/10; i++) {
             for (byte k=0; k<10; k++) EEPROM.write (DallasAddr_cell+i*10+k, DS18B20 [i][k]);
                                          }


  
  digitalWrite(13, HIGH);
}

void loop() {}
