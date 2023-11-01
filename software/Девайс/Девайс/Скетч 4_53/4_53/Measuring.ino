void izmereniya() 
{ 
  if (ProtocolSTATUS==ANALOG) {if (DS18B20[VyhlopC].Temper>50 && webasto) startWebasto_OK = 1; else startWebasto_OK = 0; }  // контроль пламени при протоколе опроса статусов по аналогу

 
#ifdef  Signalka
 static bool last_trevoga = trevoga; 
 if (last_trevoga != trevoga) 
   {
      if (!last_trevoga &&  trevoga) {AlarmSMS();} 
      last_trevoga=trevoga;
   }

engine   = test.state_button(EngineButton);
ignition = test.state_button(IGNButton);
ohrana   = test.state_button(OhranaButton);
trevoga  = test.state_button(TrevogaButton);
 #endif
 
if (ProtocolSTART==IMPULSE) webasto = test.state_button(SostButton);


digitalWrite (StatusWebastoLED, webasto);   // индикация включенности котла
  

static uint32_t prevVpit = 0;            //переменная для таймера периодического измерения параметров

if (currmillis-prevVpit>7000)
    {

//измерение напряжения борт сети
if (ProtocolSTATUS==ANALOG)
{
   Vpit = (analogRead(Voltmeter_pin) * 4.01) / 1024;             
   Vpit = Vpit / (9800.0/(100000.0+10000.0));  // По формуле Vpit = vout / (R2/(R1+R2)) 
   if (Vpit<0.09)  Vpit=0.0;                   // Округление до нуля 
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
    ds.select(DS18B20[i].DSaddress);
    ds.write(0xBE); // чтение регистров датчиков
    for (byte j=0; j<9; j++) buff[j]=ds.read(); // читаем все 9 байт от датчика
    ds.reset();
    if (OneWire::crc8(buff, 8) == buff[8]){     // если контрольная сумма совпадает 
          Temper_ = buff[0]|(buff[1]<<8);       // читаем температуру из первых двух байт (остальные были нужны только для проверки CRC)
          Temper_ = Temper_ / 16;
          if (Temper_<150 && Temper_>-55) DS18B20[i].Temper = Temper_;
}
else DS18B20[i].Temper = -101;                     // если контрольная сумма не совпала, пусть t будет -101 градус. 
}}
prevVpit=currmillis;
     }
}

void waiting_NextMes(){Prev_PeriodW_BusStartStop = currmillis; waitingNextMes=1;}

void timers_and_buttons (){


if (TimerVklData && currmillis-timerVklData >10000) TimerVklData = 0;

// таймер задержки между посылкой сообщений на шину TTC 10400 кбит/с 
if (waitingNextMes && currmillis-Prev_PeriodW_BusStartStop >50) {
          if (w_bus_init == 10 && needAction>0) sendMessage (REQUEST_2A0101, sizeof(REQUEST_2A0101)); 
     else if (w_bus_init == 12) sendMessage (REQUEST_2A0102, sizeof(REQUEST_2A0102)); 
     else if (w_bus_init == 13) sendMessage (REQUEST_DTC, sizeof(REQUEST_DTC)), countDTC = 0;
     else if (w_bus_init == 15) sendMessage (CLEAR_DTC, sizeof(CLEAR_DTC));
     waitingNextMes = 0;}

// таймер импульса старт/стоп котла 
 
  if (GND_impulse_timer && currmillis - prevGND_impulse > 800) {digitalWrite (OutWebasto_GndImp, HIGH); GND_impulse_timer=false;}
  

//ниже для таймера работы помпы после выключения котла 
if (Pumpe_timer && currmillis - prevPumpe    > (uint32_t) PumpeTime * 1000UL)     {digitalWrite (PUMPE, 0); Pumpe_timer=0;}
if (!CentrLock && webasto) {digitalWrite (PUMPE, 1);Pumpe_timer=0;}


//ниже для таймера старта котла по шине и аналогу 
if (WorkCycleHeater_timer && currmillis - prevWorkCycleHeater > (uint32_t)TimeWebasto * 60000UL) 
 {
         if (smszapusk){startSMS(KTOreport);
          SIM800.println (F("Webasto OFF, workcycle is over."));
          SIM800.print(F("Battery: "));
          SIM800.print (Vpit, 2); SIM800.println(F(" V"));
          SIM800.print (DS18B20[EngineC].TempName);     SIM800.print (F(" "));
          SIM800.print (DS18B20[EngineC].Temper); grad ();
         EndSMS();}
         
         StopWebasto();
 }

 if (EngWorkTimer && currmillis -  prevWorkCycleEngine > (uint32_t)TimeEngine * 60000UL) {EngWorkTimer = 0 ;}
 if (!engine)EngWorkTimer = 0 ;

// таймер отправки отчета об успешности запуска котла (отчёт через 5 мин после старта)
   if(webasto && report && currmillis - prevReport > 300000UL) 
   { report = false; SMSzapros(KTOreport);  }

// таймер отправки отчета об успешности запуска ДВС  (отчёт через 90сек после старта)                       
   if(reportEngine && currmillis - prevReportEngine > 90000ul) 
   {reportEngine = false; SMSzapros(KTOreport); }

// таймер рестарта модема по смс                  
   if(smsModemReboot_timer && currmillis - prevModemReboot > 15000) 
   {smsModemReboot_timer = false;   MQTTrestart(); }

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
if (addrbuf[8]<=size_arrayTemp) {for (byte k=0; k<sizeof(DS18B20[addrbuf[8]].DSaddress); k++) DS18B20[addrbuf[8]].DSaddress[k] = addrbuf[k], EEPROM.write (DallasAddr_cell+addrbuf[8]*10+k, addrbuf[k]);}
                                        

return addrbuf[8];
}
else return 20; 
  }


  
