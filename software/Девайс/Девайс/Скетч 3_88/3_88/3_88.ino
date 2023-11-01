const char ver[] = "Firmware 3.88";    // версия прошивки

//---------------------------Настройки MQTT-----------------------------------------------------------------------------

const char ACCESSPOINT[]= "\"internet.mts.by\""; // точка доступа оператора связи симкарты
const char PROTOCOLIUS[] =  "MQIsdp";                // это и оставляем
const char MQTTNAME[]    =  "M    V";                // это смотрим на сервере MQTT
char MQTTUSER[]    =  "iu      w";              // это смотрим на сервере MQTT
const char MQTTPASSWORD[] = "1Q        W";          // это смотрим на сервере MQTT
const char SERVERNAME_PORT[] = "\"m16.cloudmqtt.com\", \"13457\"";       // это смотрим на сервере MQTT
#define TIMEBROKER 80   // таймаут бездействия клиента брокера, минут. Если превысить - с сервера выкинет

//---------------------------Топики MQTT--------------------------------------------------------------------------------
const char ENDSTR[] = "&$";
const char WAIT[] = "wait";
const char FAIL[] = "fail";

const char D_HEAT_TOP[]    = "d/heat";
const char D_TIME_TOP[]    = "d/time";
const char D_REFRESH_TOP[] = "d/refr";
const char D_RESRES_TOP[]  = "d/resres";
const char D_LOCK_TOP[]    = "d/lock";

const char R_HEAT_TOP[]    = "r/heat";
const char R_TIME_TOP[]    = "r/time";
const char R_RESETS_TOP[]  = "r/resets";
const char R_FLAME_TOP[]   = "r/flame";
const char R_POWER_TOP[]   = "r/power";
const char R_DTS_TOP[]     = "r/dtc";
const char R_LEVEL_TOP[]   = "r/level";
const char R_NAPR_TOP[]    = "r/napr";
const char R_REMTIME_TOP[] = "r/remtime";
const char tHEAT_TOP[]     = "r/temp/heat";
const char tDVS_TOP[]      = "r/temp/dvs";
const char tUL_TOP[]       = "r/temp/ul";
const char tSAL_TOP[]      = "r/temp/sal";
const char tVYH_TOP[]      = "r/temp/vyh";

//----------------------------------название ячеек еепром----------------------------------------------------------------
#include <EEPROM.h>
enum Cells 
{
ResetNumber_cell,     //0
TimeWebasto_cell,     //1
ProtocolSTART_cell,   //2
StartByte_cell,       //3
ProtocolSTATUS_cell,  //4
HeaterBUS_cell,       //5
WBUS_VER_cell,        //6
delta_cell,           //7
CentrLock_cell,       //8
EngimpTime_cell,      //9
PumpeTime_cell,       //10
sizeNumber_cell,      //11
TelNumber1_cell =20,  //20
TelNumber2_cell =40,  //40
DallasAddr_cell =60   //60
};

//------------------- распиновка ног ардуино (плата весий 8.5-8.8)--------------------------------------------------------

#define OutWebasto_12V      2  // это +12В выход потенциала вкл/выкл вебасто (напрямую к котлу без таймера). 
#define Dallas_pin          3  // пин шины OneWire для датчиков даллас
#define DopOn               4  // сюда доп канал от сигналки на включение вебасто
#define DopOff              5  // сюда доп канал от сигналки на выключение вебасто
#define Ohrana              6  // Сюда состояние охраны сигналки
#define Trevoga             7  // Сюда состояние тревоги
#define IGN                 8  // Сюда состояние зажигания
#define Sost                9  // Сюда состояние вебасто (+12В когда работает)
#define ResetGSM           12  // пин ресета GSM подключен к реле, разрывающее питание модуля. 
#define PUMPE              13  // пин управления помпой
#define Eng                14  // (А0) Сюда состояние работы ДВС
#define StatusWebastoLED   15  // (А1) пин LED  индикация включенности котла
#define StartButtonpin     16  // (А2) пин тактовой кнопки вкл/выкл котла 
#define DTR                17  // пин (А3), управляющий энергосберегающим режимом GSM модуля
#define StartEng           18  // (A4) это импульсный минусовой выход вкл/выкл ДВС. подключать на вход событий сиги.
#define OutWebasto_GndImp  19  // (A5) это импульсный минусовой выход вкл/выкл вебасто (к впайке к кнопке таймера).  
#define Voltmeter_pin      A7  // пин, которым измеряем напряжение питания
#define StartButton         0  // программный номер тактовой кнопки вкл/выкл котла 
#define DopOnButton         1  // программный номер тактовой кнопки (допканала) вкл котла 
#define DopOffButton        2  // программный номер тактовой кнопки (допканала) выкл котла 
const bool RelayON =        1; // логика управления реле ресета GSM, в данном случае включается высоким уровнем на пине
#define GSM_RX             10  // пин софт RX Arduino для соединения с TX модуля SIM800
#define GSM_TX             11  // пин софт TX Arduino для соединения с RX модуля SIM800

//------------------------------------для GSM модуля----------------------------------------------------------------------

#include <SoftwareSerial.h>
      SoftwareSerial SIM800 (GSM_RX, GSM_TX);//Rx, Tx   //UART для соединения с GSM модулем

String currStr = "";
String TelNumber[] = {"", "", "", ""};
byte isStringMessage = 0; 
byte KTOreport = 1;           // флаг кто запросил отчет о запуске котла или ДВС
byte KTOzapros = 0;           // флаг кто запросил баланс или запрос параметров 
byte ResetNumber = 0;         // количество ресетов GSM модуля для статистики (хранится в еепром)
byte SizeTelNumber = 12;      // количество символов в номере телефона

const uint16_t Refresh_interval_heaterON = 30000; // интервал обновления параметров по MQTT при включенном котле, миллисек    
uint32_t prev_refreshMQTT = 0;
uint32_t prevGSMnastr = 0;
uint32_t prevTestModem = 0;
const byte interval_doprosGSM = 1; // интервал проверки модема на активность, мин  
uint16_t delayATcommand = 5000;
byte fails = 0;
byte failresets = 0;

bool settingGSM = 1;

byte signalLevelstatus = 3;        // состояние уровня gsm сети
byte last_signalLevelstatus = 3;   // прошлое состояние уровня gsm сети
byte signalLevel = 0;              // уровень сигнала gsm сети 

enum gsmstatus_  
{
  WaitGSM, echoOFF, EnergySave, Head, setText, setProgctrl, closeIncoming, newMessage, delSMS, // настройки для СМС
  setGPRS, setAccPoint, setGPRSconnect, checkLevel, setBrokerconnect, setAuthPack, setSubPack, setPubPack  // настройки для MQTT
};

byte gsmstatus = WaitGSM;

//------------------- для шины 1-wire и датчиков DS18B20---------------------------------------------------------------------

#include <OneWire.h>    // библиотека для DS18B20
OneWire ds(Dallas_pin); // датчики DS18B20 на нужный пин

enum TempC {VyhlopC, EngineC, UlicaC, SalonC, size_arrayTemp}; // перечисление нужных температур (в конце размер массива температур)

// ниже соответствие адресов датчиков различным температурам 
byte DS18B20 [size_arrayTemp][10] = {
{0x28, 0xFF, 0xB2, 0xB5, 0xC1, 0x17, 0x05, 0xD1, VyhlopC,  -100}, 
{0x28, 0xFF, 0xD3, 0xE2, 0xC1, 0x17, 0x04, 0x0D, EngineC,  -100}, 
{0x28, 0xFF, 0xF8, 0xBC, 0xC1, 0x17, 0x04, 0x48, UlicaC,   -100},  
{0x28, 0xFF, 0x3F, 0xB7, 0xC1, 0x17, 0x05, 0xF1, SalonC,   -100}
};
byte delta = 50;  // разница температур выхлопа и улицы, выше которой считается, что пламя в котле есть (для протокола статусов по аналогу)
int8_t HeaterC = -50;

//---------------------------для организации взаимодействия с котлом по цифровой шине и различные таймеры----------------------------

#include <Button.h>
Button test;

#define K_LINE Serial      //UART для соединения с шиной котла
#define TX 1    
#define NEED 1
#define READY 10

// команды для котлов с шиной W-BUS
byte StartByte = 0x20;
const byte HEATER_BEGIN[]         {0x51, 0x0A};
      byte HEATER_START[]         {StartByte, 0x3B};
      byte HEATER_PRESENCE[]      {0x44, StartByte, 0x00};
const byte HEATER_STOP[]          {0x10};
const byte HEATER_STATUS_VEVO[]   {0x50, 0x05};
const byte HEATER_STATUS_VEVO07[] {0x50, 0x07};
const byte HEATER_STATUS_EVO[]    {0x50, 0x30, 0x0A, 0x0C, 0x0E, 0x10, 0x12, 0x1E, 0x32, 0x6F};
const byte HEATER_DTC_REQUEST[]   {0x56, 0x01};
const byte HEATER_DTC_ERASE[]     {0x56, 0x03};

// команды для котлов ТТС/TTE с шиной К-line
const byte START_SESSION[]        {0x81};
const byte REQUEST_2A10101[]      {0x2A, 0x01, 0x01};
const byte REQUEST_2A10102[]      {0x2A, 0x01, 0x02};
const byte REQUEST_2A10105[]      {0x2A, 0x01, 0x05};
const byte REQUEST_DTC[]          {0xA1};
const byte START_TTC[]            {0x31, 0x22, 0xFF};
const byte STOP_TTC[]             {0x31, 0x22, 0x00};


enum needAction_ {NO_ACTION, NEED_SMSZAPROS, NEED_SERVICEINFO, NEED_MQTTZAPROS, NEED_DTCCLEAR};// возможные действия, стоящие в очереди
byte needAction = NO_ACTION;                                                  // переменная действия, стоящего в очереди

enum ProtocolSTATUS_ {STATUSBUS, ANALOG};                 // возможные протоколы чтения статуса котла
enum ProtocolSTART_  {STARTBUS, IMPULSE, POTENCIAL};      // возможные протоколы запуска котла
enum HeaterBUSTYPE_         {TTC_E, WBUS, HYDRONIC};      // тип шины котла


byte  ProtocolSTATUS = STATUSBUS; 
byte  ProtocolSTART  = STARTBUS;
byte  HeaterBUSTYPE = WBUS;
byte  WBUS_VER = 0x40;   

bool noData  = 0;                               // флаг пришли ли данные от котла после запроса. 
byte w_bus_init = 0;                            //состояние инициализация шины w-bus (25мс LOW, 25мс HIGH для  ЭВО
                                                //                                            либо 300ms LOW, 50ms HIGH, 25ms LOW, 3025ms HIGH для TTC 
byte requiredmessage =  1;                      //флаг, что отправляем в данный момент поддержание старта, запрос параметров или запрос ошибок
byte StartMessageRepeat = 0;                    //количество отправленных сообщений на старт котла
byte StopMessageRepeat =  4;                    //количество отправленных сообщений на остановку котла

byte  TimeWebasto = 30;                         //время работы котла, = 30мин

uint32_t currmillis = 0;                        // снимок системного времени
uint32_t Prev_PeriodW_BusMessage = 0;           //переменная для таймера периодической отправки сообщений состояния котла в шину W-Bus 
uint32_t Prev_PeriodW_BusStartStop = 0;         //переменная для таймера периодической отправки сообщений старта/стопа котла в шину W-Bus 
uint32_t prevdelSMS = 0;                        //переменная для таймера периодического удаления СМС 
uint32_t prevInitreset = 0;                     //переменная для таймера сброса инита шины
bool Initreset = 0;                             //переменная для таймера сброса инита шины
uint32_t timerInit = 0; bool timerInitflag = 0; //для таймера инита шины W-BUS
uint32_t prevNeedTimer = 0; bool NeedTimer = 0; //для таймера задержки функций SMSzapros() и ServiceINFO() на время обновления параметров по шине

                   
uint32_t prevReportEngine = 0; bool reportEngine = false;    //таймер задержки на отправку отчёта о запуске двигателя
uint32_t prevReport = 0;bool report = false;                 //таймер задержки на отправку отчёта о запуске котла
uint32_t last_Flame = 0;                                     //для таймера сброса флага пламени, если нет ответов от котла
uint32_t prevGND_impulse = 0; bool GND_impulse_timer = 0;    //для таймера создания импульса GND - для протокола запуска котла импульсом GND 

uint32_t prevStartEng=0; bool StartEng_timer=0;              //для таймера  - старт двигателя: минусовой импульс на вход событий сигналки для запуска ДВС
byte EngimpTime = 1;                                         //сек, время импульса на старт двигателя 

uint32_t prevPumpe=0; bool Pumpe_timer=0;                    //для таймера  - работы помпы после выключения котла
byte PumpeTime = 1;                                          //сек, время работы помпы после выключения котла
bool CentrLock = 0;
uint32_t prevWorkCycleHeater; bool WorkCycleHeater_timer=0;  //для таймера отсчёта цикла работы котла




//---------------------------------Основные переменные--------------------------------------------------------------------------------  

bool webasto = 0;             // флаг команды на работу Webasto. 0 - котел выключен, 1 - котел включен
bool startWebasto_OK = 0;     // флаг успешного запуска котла

float Vpit = 0.0;             // Измеряемое напряжение на выходе ИБП
bool engine =0;               // флаг работает ли ДВС или нет
bool ignition=0;              // флаг включено ли зажигание или нет
bool ohrana=0;                // флаг включена ли охрана или нет
bool trevoga=0;               // флаг включена ли тревога или нет
bool alarmSMS = 0;            // флаг отправлена ли смс о тревоге или нет

int PowerHeater= 0;           // мощность работы котла
bool waterpump = 0;           // флаг работы циркуляционного насоса
bool plug      = 0;           // флаг работы штифта накаливания
bool airfan    = 0;           // флаг работы нагнетателя воздуха
bool fuelpump  = 0;           // флаг работы топливного насоса
bool blowerfan = 0;           // флаг работы вентилятора печки автомобиля
byte flameout  = 0;           // количество срывов пламени
byte DTC[7] ={0};             // коды неисправностей котла

//---------------------------СТАРТОВЫЙ ЦИКЛ--------------------------------------------------------------------------------------------

void setup() 
{
delay (4500);
test.NO(); 
test.pullUp();
test.duration_bounce       (  50);
test.duration_click_Db     ( 250);
test.duration_inactivity_Up(5000);
test.duration_inactivity_Dn(1000);
test.duration_press        ( 500);
test.button(StartButtonpin, DopOn, DopOff);
 
pinMode (DopOn,   INPUT_PULLUP); 
pinMode (DopOff,  INPUT_PULLUP); 
pinMode (Sost,    INPUT_PULLUP); 
pinMode (Ohrana,  INPUT_PULLUP); 
pinMode (Trevoga, INPUT_PULLUP); 
pinMode (IGN,     INPUT_PULLUP); 
pinMode (Eng,     INPUT_PULLUP); 
pinMode (OutWebasto_12V,     OUTPUT);  digitalWrite (OutWebasto_12V,      LOW);
pinMode (StartEng,           OUTPUT);  digitalWrite (StartEng,            LOW);
pinMode (PUMPE,              OUTPUT);  digitalWrite (PUMPE,               LOW);
pinMode (StatusWebastoLED,   OUTPUT);  digitalWrite (StatusWebastoLED,    LOW);
pinMode (OutWebasto_GndImp,  OUTPUT);  digitalWrite (OutWebasto_GndImp,   HIGH);
pinMode (DTR,                OUTPUT);  digitalWrite (DTR,                 LOW );  // делаем низкий для вывода GSM из "спячки"
pinMode (ResetGSM,           OUTPUT);  digitalWrite (ResetGSM,        !RelayON);  // реле ресет на данный момент делаем "неактивно"
  
  SIM800.begin(19200);           // сериал соединение для gsm модуля
   

SizeTelNumber =  EEPROM.read(sizeNumber_cell);
if (SizeTelNumber>25)while(1);
TimeWebasto =    EEPROM.read(TimeWebasto_cell);
ProtocolSTART  = EEPROM.read(ProtocolSTART_cell);
ProtocolSTATUS = EEPROM.read(ProtocolSTATUS_cell);
ResetNumber =    EEPROM.read(ResetNumber_cell);
StartByte =      EEPROM.read(StartByte_cell);
HEATER_START[0] = StartByte;
HEATER_PRESENCE[1] = StartByte;
HeaterBUSTYPE =  EEPROM.read(HeaterBUS_cell);
WBUS_VER      =  EEPROM.read(WBUS_VER_cell);
delta  =         EEPROM.read(delta_cell);
CentrLock  =     EEPROM.read(CentrLock_cell);
EngimpTime     = EEPROM.read (EngimpTime_cell);
PumpeTime      = EEPROM.read (PumpeTime_cell);
for (int i=0; i<SizeTelNumber; i++) 
    {
      TelNumber[0]+= '0';
      TelNumber[3]+= '0';
      TelNumber[1]+= (char)EEPROM.read (i+TelNumber1_cell);
      TelNumber[2]+= (char)EEPROM.read (i+TelNumber2_cell);
    }


// ниже читаем из еепром адреса датчиков температуры даллас 
for (byte i = 0; i<size_arrayTemp; i++) 
    {
    for (byte k=0; k<9; k++) DS18B20 [i][k] = EEPROM.read(DallasAddr_cell+i*10+k);
    }
        
        if (HeaterBUSTYPE == WBUS) K_LINE.begin(2400, SERIAL_8E1);
   else if (HeaterBUSTYPE == TTC_E) K_LINE.begin(10400);

//for (byte i=0; i<20; i++) {digitalWrite (13, !digitalRead(13)); delay (80);}
//digitalWrite (13,0);

fails = 3;
}

//--------------------------------------------------------------ЛУП---------------------------------------------------------------
void loop() 
{
  currmillis = millis();
  test.read();

  digitalWrite (StatusWebastoLED, webasto);
  //digitalWrite (13, startWebasto_OK);
  //digitalWrite (13, webasto);

  Check_responseModem ();
  readModem();   
  Heater_BUS();
  timers_and_buttons ();
  izmereniya();
}

//-----------------------------------------------------------конец луп-------------------------------------------------------------


