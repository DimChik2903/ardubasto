const char ver[] = "v4.072";    // версия прошивки

//#define WBUS_heaters  // тут раскомментируем одну из строчек 
#define TTE_C_heaters   // с нужным типом котла

//#define Signalka      // раскомментировать , если к девайсу подключена сигнализация

bool mqtt = 1;          // работа по MQTT (интернету) 1 - включено, 0 - выключено.  


//---------------------------Настройки MQTT-----------------------------------------------------------------------------

const char ACCESSPOINT[]= "\"internet.mts.by\"";     // точка доступа оператора связи симкарты
const char MQTTUSER[]    =  "vahfyite";              // имя пользователя (логин)  (смотрим в аккаунте MQTT)
const char MQTTPASSWORD[] = "UytGtehYjd";            // пароль                    (смотрим в аккаунте MQTT)
const char SERVERNAME_PORT[] = "\"farmer.cloudmqtt.com\", \"14689\"";  // адрес и порт    (смотрим в аккаунте MQTT)
                                                                       // на cloudmqtt  смотрим просто "Port" (без SSL)
                                                                       // на clusterfly смотрим Port  tcp     (без SSL)
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
EngineTime_cell,      //9
PumpeTime_cell,       //10
sizeNumber_cell,      //11
TelNumber1_cell =20,  //20
TelNumber2_cell =40,  //40
DallasAddr_cell =60   //60
};

//------------------- распиновка ног ардуино (плата весий 8.8-8.91)--------------------------------------------------------

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

//#include <SoftwareSerial.h>
//      SoftwareSerial SIM800 (GSM_RX, GSM_TX);//Rx, Tx   //UART для соединения с GSM модулем
#include <CustomSoftwareSerial.h>

CustomSoftwareSerial* SIM800;               // Declare serial

//#define DEBUG_MODEM        // раскомментировать , если нужно отладить модем (k-line при этом отключать)
const char PROTOCOLIUS[] =  "MQIsdp";                 // протокол MQTT
const char MQTTNAME[]    =  "b";                      // инстанс MQTT
#define TIMEBROKER 20   // таймаут бездействия клиента брокера, минут. Если превысить - с сервера выкинет 

#define BUF_MODEM_SIZE 60
char currStr[BUF_MODEM_SIZE];   
char TelNumber[][16] = {"", "", "", ""};
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
bool TimerMQTTreconnect = 0;       // таймер переподключения к брокеру если были 4 неудачных попытки
uint32_t prevModemReboot = 0; bool smsModemReboot_timer = 0; // таймер ребута модема по смс
byte signalLevelstatus = 3;        // состояние уровня gsm сети
byte last_signalLevelstatus = 3;   // прошлое состояние уровня gsm сети
byte signalLevel = 0;              // уровень сигнала gsm сети 

enum gsmstatus_  
{
  WaitGSM, echoOFF, EnergySave, Head, setText, setProgctrl, closeIncoming, newMessage, delSMS, // настройки для СМС
  setGPRS, setAccPoint, setGPRSconnect, setBrokerconnect, setAuthPack, setSubPack, checkLevel,  setPubPack  // настройки для MQTT
};

byte gsmstatus = WaitGSM;

char buffeR[16] = {0};   // буффер для работы функций вытаскивания строк из прогмем 
char buffVAR[10] = {0};  // буффер для работы функций конвертации переменных в char 

#define ENDSTR 1

const char Zapros []        PROGMEM = "Zapros"; 
const char Service_info []  PROGMEM = "Service-info"; 
const char Erase_DTC []     PROGMEM = "Erase DTC";
const char Zaprostel []     PROGMEM = "Zaprostel";
const char GSMResets_0[]    PROGMEM = "GSMResets";
const char Version[]        PROGMEM = "Version";
const char MQTT_reset[]     PROGMEM = "MQTT-reset";
const char Webasto_ON[]     PROGMEM = "Heater-ON";
const char Webasto_OFF[]    PROGMEM = "Heater-OFF";
const char Engine_ON[]      PROGMEM = "Engine-ON";
const char Engine_OFF[]     PROGMEM = "Engine-OFF";
const char Impulse[]        PROGMEM = "Impulse";
const char Startbus[]       PROGMEM = "Startbus";
const char Potencial[]      PROGMEM = "Potencial";
const char DallasAddr[]     PROGMEM = "DallasAddr";
const char Status[]         PROGMEM = "Status";
const char HeaterBusType[]  PROGMEM = "HeaterBusType";
const char ADDr[]           PROGMEM = "address";
const char Delta[]          PROGMEM = "Delta";
const char Min[]            PROGMEM = "min";
const char StartBytE[]      PROGMEM = "StartByte";
const char ResetNumbers[]   PROGMEM = "ResetNumbers";
const char WriteNumber2[]   PROGMEM = "WriteNumber2";
const char Balance[]        PROGMEM = "Balance";
const char WriteNumber1[]   PROGMEM = "WriteNumber1";
const char cmt[]            PROGMEM = "+CMT: \"";
const char cusd[]           PROGMEM = "+CUSD: 0,";
const char csq[]            PROGMEM = "+CSQ:";
const char Closed[]         PROGMEM = "CLOSED";
const char okey[]           PROGMEM = "OK";
const char Connect[]        PROGMEM = "CONNECT";
const char SendOk[]         PROGMEM = "SEND OK";
const char Space[]          PROGMEM = " ";
const char ErroR[]          PROGMEM = "ERROR";
const char Bus[]            PROGMEM = "BUS";
const char Wbus[]           PROGMEM = "W-BUS";
const char ttce[]           PROGMEM = "TTC_E";
const char hydronic[]       PROGMEM = "Hydronic";
const char analog[]         PROGMEM = "Analog";

const char WAIT[] PROGMEM = "wait";
const char FAIL[] PROGMEM = "fail";
const char VYKL[] PROGMEM = "выкл";

//---------------------------Топики MQTT--------------------------------------------------------------------------------
const char CTRL_HEAT_TOP[]    PROGMEM = "c/hea";
const char CTRL_TIME_TOP[]    PROGMEM = "c/tim";
const char CTRL_TIMEENG_TOP[] PROGMEM = "c/Eti";
const char CTRL_REFRESH_TOP[] PROGMEM = "c/ref";
const char CTRL_RESRES_TOP[]  PROGMEM = "c/Res";
const char CTRL_LOCK_TOP[]    PROGMEM = "c/loc";
const char CTRL_DVS_TOP[]     PROGMEM = "c/dvs";
const char CTRL_DTC_CL[]      PROGMEM = "c/ecl";

const char STAT_HEAT_TOP[]    PROGMEM = "s/hea";
const char STAT_TIME_TOP[]    PROGMEM = "s/tim";
const char STAT_RESETS_TOP[]  PROGMEM = "s/res";
const char STAT_FLAME_TOP[]   PROGMEM = "s/fla";
const char STAT_POWER_TOP[]   PROGMEM = "s/pow";
const char STAT_DTS_TOP[]     PROGMEM = "s/dtc";
const char STAT_LEVEL_TOP[]   PROGMEM = "s/lev";
const char STAT_NAPR_TOP[]    PROGMEM = "u";
const char STAT_REMTIME_TOP[] PROGMEM = "s/rti";
const char STAT_ENGTIME_TOP[] PROGMEM = "s/eti";
const char tHEAT_TOP[]        PROGMEM = "s/t/h";
const char tDVS_TOP[]         PROGMEM = "s/t/d";
const char tUL_TOP[]          PROGMEM = "s/t/u";
const char tSAL_TOP[]         PROGMEM = "s/t/s";
const char tVYH_TOP[]         PROGMEM = "s/t/v";
const char STAT_ENGINE_TOP[]  PROGMEM = "s/dvs";
const char STAT_IGN_TOP[]     PROGMEM = "s/ign";
const char STAT_OHR_TOP[]     PROGMEM = "s/ohr";
const char STAT_TREV_TOP[]    PROGMEM = "s/tre";

const char SUBSCRIBE_TOP[]    PROGMEM = "/c/#";
//------------------------------------------------------------------------------------------------------------------


const char* const PGMtable[] PROGMEM = 
{
Zapros,
Service_info,
Erase_DTC,
Zaprostel,
GSMResets_0,
Version,
MQTT_reset,
Webasto_ON,
Webasto_OFF,
Engine_ON,
Engine_OFF,
Impulse,
Startbus,
Potencial,
DallasAddr,
Status,
HeaterBusType,
ADDr,
Delta,
Min,
StartBytE,
ResetNumbers,
WriteNumber2,
Balance,
WriteNumber1,
cmt,
cusd,
csq,
Closed,
okey,
Connect,
SendOk,
Space,
ErroR,
Bus,
Wbus,
ttce,
hydronic,
analog,

WAIT,
FAIL,
VYKL,

CTRL_HEAT_TOP,
CTRL_TIME_TOP,
CTRL_TIMEENG_TOP,
CTRL_REFRESH_TOP,
CTRL_RESRES_TOP,
CTRL_LOCK_TOP,
CTRL_DVS_TOP,
CTRL_DTC_CL,

STAT_HEAT_TOP,
STAT_TIME_TOP,
STAT_RESETS_TOP,
STAT_FLAME_TOP,
STAT_POWER_TOP,
STAT_DTS_TOP,
STAT_LEVEL_TOP,
STAT_NAPR_TOP,
STAT_REMTIME_TOP,
STAT_ENGTIME_TOP,
tHEAT_TOP,
tDVS_TOP,
tUL_TOP,
tSAL_TOP,
tVYH_TOP,
STAT_ENGINE_TOP,
STAT_IGN_TOP,
STAT_OHR_TOP,
STAT_TREV_TOP,

SUBSCRIBE_TOP

};

enum PGMtable_
{
_ZAPROS,
_SERVICE_INFO,
_ERASE_DTC,
_ZAPROSTEL,
_GSM_RESETS,
_VERSION,
_MQTT_RESET,
_HEATER_ON,
_HEATER_OFF,
_ENGINE_ON,
_ENGINE_OFF,
_IMPULSE,
_STARTBUS,
_POTENCIAL,
_DALLASADDR,
_STATUS,
_HEATERBUSTYPE,
_ADDRESSA,
_DELTA,
_MIN,
_STARTBYTE,
_RESETNUMBERS,
_WRITENUMBER2,
_BALANCE,
_WRITENUMBER1,
_CMT,
_CUSD,
_CSQ,
_CLOSED,
_OK,
_CONNECT,
_SEND_OK,
_SPACE,
_ERROR,
_BUS,
_WBUS,
_TTCE,
_HYDRONIC,
_ANALOG,

_WAIT,
_FAIL,
_VYKL,

_CTRL_HEAT_TOP,
_CTRL_TIME_TOP,
_CTRL_TIMEENG_TOP,
_CTRL_REFRESH_TOP,
_CTRL_RESRES_TOP,
_CTRL_LOCK_TOP,
_CTRL_DVS_TOP,
_CTRL_DTC_CL,

_STAT_HEAT_TOP,
_STAT_TIME_TOP,
_STAT_RESETS_TOP,
_STAT_FLAME_TOP,
_STAT_POWER_TOP,
_STAT_DTS_TOP,
_STAT_LEVEL_TOP,
_STAT_NAPR_TOP,
_STAT_REMTIME_TOP,
_STAT_ENGTIME_TOP,
_tHEAT_TOP,
_tDVS_TOP,
_tUL_TOP,
_tSAL_TOP,
_tVYH_TOP,
_STAT_ENGINE_TOP,
_STAT_IGN_TOP,
_STAT_OHR_TOP,
_STAT_TREV_TOP,
_SUBSCRIBE_TOP
};


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
const byte START_SESSION[]       {0x81};
const byte REQUEST_2A0101[]      {0x2A, 0x01, 0x01};
const byte REQUEST_2A0102[]      {0x2A, 0x01, 0x02};
const byte REQUEST_2A0105[]      {0x2A, 0x01, 0x05};
const byte REQUEST_DTC[]         {0xA1};
const byte CLEAR_DTC[]           {0x14};
const byte START_TTC[]           {0x31, 0x22, 0xFF};
const byte START_PRESENCE[]      {0x31, 0x22, 0x01};
const byte STOP_TTC[]            {0x31, 0x22, 0x00};


enum needAction_ {NO_ACTION, NEED_SMSZAPROS, NEED_SERVICEINFO, NEED_MQTTZAPROS, NEED_DTCCLEAR};// возможные действия, стоящие в очереди
byte needAction = NO_ACTION;                                                  // переменная действия, стоящего в очереди

enum ProtocolSTATUS_ {STATUSBUS=1, ANALOG};                 // возможные протоколы чтения статуса котла
enum ProtocolSTART_  {STARTBUS=1, IMPULSE, POTENCIAL};      // возможные протоколы запуска котла
enum HeaterBUSTYPE_         {TTC_E=1, WBUS, HYDRONIC};      // тип шины котла


byte  ProtocolSTATUS = STATUSBUS; 
byte  ProtocolSTART  = STARTBUS;
byte  HeaterBUSTYPE = WBUS;
byte  WBUS_VER = 0x40;   

bool noData  = 0;                               // флаг пришли ли данные от котла после запроса. 
byte w_bus_init = 0;                            //состояние инициализация шины w-bus (25мс LOW, 25мс HIGH для  ЭВО
                                                //                                            либо 300ms LOW, 50ms HIGH, 25ms LOW, 3025ms HIGH для TTC 

byte StartMessageRepeat = 0;                    //количество отправленных сообщений на старт котла
byte StopMessageRepeat =  4;                    //количество отправленных сообщений на остановку котла

byte  TimeWebasto = 30;                         //время работы котла, = 30мин

uint32_t currmillis = 0;                        // снимок системного времени
uint32_t Prev_PeriodW_BusMessage = 0;           //переменная для таймера периодической отправки сообщений состояния котла в шину W-Bus 
uint32_t Prev_PeriodW_BusStartStop = 0;         //переменная для таймера периодической отправки сообщений старта/стопа котла в шину W-Bus 
uint32_t prevdelSMS = 0;                        //переменная для таймера периодического удаления СМС 
uint32_t prevInitreset = 0;                     //переменная для таймера сброса инита шины
bool Initreset = 0;                             //переменная для таймера сброса инита шины
uint32_t timerInit = 0;                         //для таймера инита шины W-BUS

bool TimerVklData = 0;                             //
uint32_t timerVklData = 0;                         //



uint32_t prevNeedTimer = 0; bool NeedTimer = 0; //для таймера задержки функций SMSzapros() и ServiceINFO() на время обновления параметров по шине
bool waitingNextMes = 0;                        //для таймера задержки между сообщениями на шине 10400 TTC
                   
uint32_t prevReportEngine = 0; bool reportEngine = false;    //таймер задержки на отправку отчёта о запуске двигателя
uint32_t prevReport = 0;bool report = false;                 //таймер задержки на отправку отчёта о запуске котла
uint32_t last_Flame = 0;                                     //для таймера сброса флага пламени, если нет ответов от котла
uint32_t prevGND_impulse = 0; bool GND_impulse_timer = 0;    //для таймера создания импульса GND - для протокола запуска котла импульсом GND 

uint32_t prevStartEng=0; bool StartEng_timer=0;              //для таймера  - старт двигателя: минусовой импульс на вход событий сигналки для запуска ДВС
byte EngineTime = 1;                                         //сек, время импульса на старт двигателя 

uint32_t prevPumpe=0; bool Pumpe_timer=0;                    //для таймера  - работы помпы после выключения котла
byte PumpeTime = 1;                                          //сек, время работы помпы после выключения котла
bool CentrLock = 0;
uint32_t prevWorkCycleHeater; bool WorkCycleHeater_timer=0;  //для таймера отсчёта цикла работы котла
uint32_t prevWorkCycleEngine = 0;                            //для таймера отсчёта цикла работы двигателя
byte TimeEngine = 30;                                        //время работы двигателя


//---------------------------------Основные переменные--------------------------------------------------------------------------------  

bool webasto = 0;             // флаг команды на работу Webasto. 0 - котел выключен, 1 - котел включен
bool startWebasto_OK = 0;     // флаг успешного запуска котла

float  Vpit = 0.0;            // Измеряемое напряжение бортсети 
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
byte DTC[13] ={0};            // коды неисправностей котла
byte countDTC = 0; 
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
  
//  SIM800.begin(19200);           // сериал соединение для gsm модуля
SIM800 = new CustomSoftwareSerial(10, 11); // rx, tx

SIM800->begin(9600, CSERIAL_8N1);         // Baud rate: 9600, configuration: CSERIAL_8N1   

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
EngineTime     = EEPROM.read (EngineTime_cell);
PumpeTime      = EEPROM.read (PumpeTime_cell);
for (int i=0; i<SizeTelNumber; i++) 
    {
      char bu[2] = "0";
      strcat (TelNumber[0],bu);
      strcat (TelNumber[3],bu);
      
      bu[0] = (char)EEPROM.read (i+TelNumber1_cell);
      strcat (TelNumber[1],bu);
      bu[0] = (char)EEPROM.read (i+TelNumber2_cell);
      strcat (TelNumber[2],bu);
    }


// ниже читаем из еепром адреса датчиков температуры даллас 
for (byte i = 0; i<size_arrayTemp; i++) 
    {
    for (byte k=0; k<9; k++) DS18B20 [i][k] = EEPROM.read(DallasAddr_cell+i*10+k);
    }
        
        if (HeaterBUSTYPE == WBUS)  K_LINE.begin(2400, SERIAL_8E1);
   else if (HeaterBUSTYPE == TTC_E) K_LINE.begin(10400);

#ifdef DEBUG_MODEM
Serial.begin (9600);
#endif
//ниже контроль рестарта АТмеги по миганию втроенного LED 
//for (byte i = 0; i<20; i++) {digitalWrite (13, !digitalRead(13)); delay (60); }

fails = 3;  // задаем сразу условия для рестарта модема
if (!mqtt) failresets = 5; // если MQTT отключено, то и нефиг его настраивать
}

//--------------------------------------------------------------ЛУП---------------------------------------------------------------
void loop() 
{
  currmillis = millis();
  test.read();
  
  Check_responseModem ();
  readModem();   
  Heater_BUS();
  timers_and_buttons ();
  izmereniya();
}

//-----------------------------------------------------------конец луп-------------------------------------------------------------


