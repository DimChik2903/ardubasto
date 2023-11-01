void izmereniya() 
{ 

  if (ProtocolSTATUS==ANALOG) {  if (Temper(VyhlopC) - Temper(UlicaC) > delta) startWebasto_OK = 1;
                               else startWebasto_OK = 0;}  // контроль пламени при протоколе опроса статусов по аналогу

  if (ProtocolSTART==IMPULSE) {webasto = !digitalRead (Sost);}

   if (!ohrana) alarmSMS = false;
   if (trevoga && !alarmSMS) AlarmSMS ();

  
 engine =  !digitalRead (Eng);
 ignition= !digitalRead (IGN); 
 ohrana=   !digitalRead (Ohrana);  
 trevoga=  !digitalRead (Trevoga);

static uint32_t prevVpit = 0;            //переменная для таймера периодического измерения параметров

if (currmillis-prevVpit>7000)
    {

//измерение напряжения борт сети
if (ProtocolSTATUS==ANALOG)
{
   Vpit = (analogRead(Voltmeter_pin) * 4.13) / 1024;             
   Vpit = Vpit / (9700.0/(98930.0+9700.0));  // По формуле Vpit = vout / (R2/(R1+R2)) 
   if (Vpit<0.09)  Vpit=0.0;                  // Округление до нуля 
}  

// ниже измерение датчиков даллас
static bool y=0;        // флаг работы: запрос температуры или её чтение
y=!y;
if (y) {ds.reset();     // сброс шины
        ds.write(0xCC); // обращение ко всем датчикам
        ds.write(0x44); // начать преобразование (без паразитного питания)  
       }
else   {
  for (byte i=0; i<size_arrayTemp; i++){  
    int Temper_ = 20; byte buff[9];
    ds.reset();
    ds.select(DS18B20[i]);
    ds.write(0xBE); // чтение регистров датчиков
    for (byte j=0; j<9; j++) buff[j]=ds.read(); // читаем все 9 байт от датчика
    ds.reset();
    if (OneWire::crc8(buff, 8) == buff[8]){     // если контрольная сумма совпадает 
          Temper_ = buff[0]|(buff[1]<<8);       // читаем температуру из первых двух байт (остальные были нужны только для проверки CRC)
          Temper_ = Temper_ / 16;
          if (Temper_<150 && Temper_>-55) DS18B20[i][9] = Temper_;
}
else DS18B20[i][9] = -101;                     // если контрольная сумма не совпала, пусть t будет -101 градус. 
}}
prevVpit=currmillis;
     }
}


void timers_and_buttons (){

// опрос допканалов от сигнализации включения/выключение котла и таймер импульса старт/стоп котла 
 
  if (GND_impulse_timer && currmillis - prevGND_impulse > 800) {digitalWrite (OutWebasto_GndImp, HIGH); GND_impulse_timer=false;}
  
//ниже для таймера создания импульса на старт ДВС 
if (StartEng_timer && currmillis - prevStartEng > (uint32_t)EngimpTime * 1000ul ) {digitalWrite (StartEng, 0); StartEng_timer=0;}

//ниже для таймера работы помпы после выключения котла 
if (Pumpe_timer && currmillis - prevPumpe    > (uint32_t) PumpeTime * 1000ul)     {digitalWrite (PUMPE, 0); Pumpe_timer=0;}

//ниже для таймера старта котла по шине и аналогу 
if (WorkCycleHeater_timer && currmillis - prevWorkCycleHeater > (uint32_t)TimeWebasto * 60000UL) {StopWebasto();}

// таймер отправки отчета об успешности запуска котла (отчёт через 6 мин после старта)
   if(webasto && report && currmillis - prevReport > 360000UL) 
   { report = false; Queue(NEED_SMSZAPROS);  }

// таймер отправки отчета об успешности запуска ДВС  (отчёт через 90сек после старта)                       
   if(reportEngine && currmillis - prevReportEngine > 90000ul) 
   {reportEngine = false; Queue(NEED_SMSZAPROS);}

//если нажали на допканал - делаем соответствующее состояние котла 
if (test.event_click_Dn (DopOnButton) && !webasto) {StartWebasto(); KTOreport = 1;}  
if (test.event_click_Dn (DopOffButton) && webasto) {StopWebasto();}

//если нажали тактовую кнопку меняем состояние котла на противоположное 
if (test.event_press_short (StartButton)) 
    {
       if   (!webasto) {StartWebasto(); report = false;}
       else {StopWebasto();}
    }
}


int8_t Temper (const byte &addressTemp) {for(byte j=0; j<size_arrayTemp; j++){if(DS18B20[j][8]==addressTemp)return(int8_t)DS18B20[j][9];} return-99;}

byte ConvertAddr() {
  
  byte addrbuf[9];
byte Cs=0;
for (byte i = 0; i<9; i++) {
    char str[]="  ";
    for (byte k = 0; k<2; k++) str[k] = currStr[3*i+k];

 addrbuf[i] = strtol(str,NULL,HEX);
      if (i<7) Cs+= addrbuf[i] = strtol(str,NULL,HEX);
 
  }
if (OneWire::crc8(addrbuf, 7) == addrbuf[7] && addrbuf[8]>=0 && addrbuf[8]<=3) { 

for (byte i = 0; i<size_arrayTemp; i++) {
  if (addrbuf[8]==DS18B20[i][8]) {for (byte k=0; k<9; k++) DS18B20 [i][k] = addrbuf[k], EEPROM.write (DallasAddr_cell+addrbuf[8]*10+k, addrbuf[k]);}
                                        }

return addrbuf[8];
}
else return 20; 
  }
