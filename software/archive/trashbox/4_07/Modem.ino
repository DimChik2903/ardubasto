void Check_responseModem ()
{
 
 //---------------- ниже оперативное обновление параметров MQTT которые поменяли своё значение-----------
 
 if (failresets <= 4)   //если MQTT активен 
    {
    static bool last_MQTTwebasto = 0;
    static bool last_startwebasto_OK = 0;
    static bool last_engine = 0;
    if (last_engine != engine || last_MQTTwebasto != webasto || last_startwebasto_OK != startWebasto_OK && (w_bus_init<1 || w_bus_init>6))
         {
          MQTTsendDatastream(); last_engine = engine; last_MQTTwebasto = webasto; last_startwebasto_OK = startWebasto_OK;
         }
    if ((webasto||engine) && currmillis - prev_refreshMQTT > Refresh_interval_heaterON) {MQTTsendDatastream(); prev_refreshMQTT = currmillis;}
    }
 
//------------------ниже опроверка модема на предмет жив он или мертв------------------------------------- 

     if (settingGSM) NastroykaGSM ();
else if (fails >= 3) settingGSM = 1;
 
 static uint32_t prevTestModem = 0;

if (!settingGSM && currmillis - prevTestModem > (uint32_t)interval_doprosGSM*60000UL)
    {
      static bool chto = 0;
      if (chto)  
      {
         digitalWrite (DTR, LOW);
         delay (150);
         SIM800.println (F("AT+CSQ"));
         delay (50);
         digitalWrite (DTR, HIGH);
      }
      else if (failresets <= 4) 
        {
        cipsend_begin ();
        PUBsend (PGM_Text(_STAT_HEAT_TOP),    Var_to_text(webasto,ENDSTR),     1);
        PUBsend (PGM_Text(_STAT_LEVEL_TOP),   Var_to_text(signalLevel,0), 1);
        SIM800.write (0x1A);
        digitalWrite (DTR, HIGH);
        }  //если MQTT активен 
     fails++; chto = !chto;
     prevTestModem = currmillis;
    }  
}


void NastroykaGSM () {

  if (fails >= 3) Reset();

  else if (currmillis - prevGSMnastr > delayATcommand)
  {

    fails ++;
         if (gsmstatus == WaitGSM)          SIM800.println (F("AT"));
    else if (gsmstatus == EnergySave)       SIM800.println (F("AT+CSCLK=1"));
    else if (gsmstatus == Head)             SIM800.println (F("AT+CIPHEAD=1"));
    else if (gsmstatus == echoOFF)          SIM800.println (F("ATE0")); //SIM800.println (F("ATE0")); 
    else if (gsmstatus == setText)          SIM800.println (F("AT+CMGF=1"));
    else if (gsmstatus == setProgctrl)      SIM800.println (F("AT+IFC=0, 0"));
    else if (gsmstatus == closeIncoming)    SIM800.println (F("AT+GSMBUSY=1"));
    else if (gsmstatus == newMessage)       SIM800.println (F("AT+CNMI=1,2,2,1,0"));
    else if (gsmstatus == delSMS)           SIM800.println (F("AT")); //SIM800.println (F("AT+CMGDA=\"DEL ALL\""));  

    else if (gsmstatus == setGPRS)          SIM800.println (F("AT+SAPBR=3,1, \"Contype\",\"GPRS\""));
    else if (gsmstatus == setAccPoint)      SIM800.print (F("AT+SAPBR=3,1, \"APN\",")), SIM800.println (ACCESSPOINT);
    else if (gsmstatus == setGPRSconnect)   SIM800.println (F("AT+SAPBR=1,1"));
    else if (gsmstatus == setBrokerconnect) SIM800.print (F("AT+CIPSTART=\"TCP\",")),  SIM800.println (SERVERNAME_PORT);
    else if (gsmstatus == setAuthPack)      AUTHsend ();
    else if (gsmstatus == setSubPack)       {char sub[30]={0}; strcpy (sub, MQTTUSER); strcat (sub, PGM_Text (_SUBSCRIBE_TOP) ) ; SUBsend(sub);} 
    else if (gsmstatus == checkLevel)       SIM800.println (F("AT+CSQ")); 
    else if (gsmstatus == setPubPack)       Queue(NEED_MQTTZAPROS), MQTTsendDatastream();
    
    delayATcommand = 6000;
    prevGSMnastr = currmillis;
  }
}


void Reset() {

static bool resettimer = 0;             // для таймера удерживания реле в режиме сброс питания
static uint32_t  resetTimer = 0;        // для таймера удерживания реле в режиме сброс питания
  
  if (!resettimer) {
    digitalWrite (ResetGSM, RelayON);
    resettimer = 1;
    resetTimer = currmillis;
  }
  else if (currmillis - resetTimer > 5000 ) {
    resettimer = 0;

    digitalWrite (ResetGSM, !RelayON);
    digitalWrite (DTR, LOW);
    delay (150);

    prevGSMnastr = currmillis;
    delayATcommand = 150;
    fails = 0;
    failresets ++; if (failresets > 4) failresets = 5;
    gsmstatus = WaitGSM;
    settingGSM = 1; 
    ResetNumber++;  EEPROM.write (ResetNumber_cell, ResetNumber);
  }
}






void startSMS(byte stat) //__________________Цикл подготовки модуля к отправке СМС-сообщений по первому номеру
{
    
  
    if (stat==0) stat = KTOreport;
     digitalWrite (DTR, LOW); // выводим из спячки GSM модуль
         delay (150);
     SIM800.print(F("AT+CMGF=1\r"));
         delay(200);
     SIM800.print(F("AT+CMGS=\"")); SIM800.print(TelNumber[stat]); SIM800.println("\""); 
         delay(200);
}



void EndSMS ()
{
   SIM800.println((char)26);                       // Команда отправки СМС
   delay(200);
   digitalWrite (DTR, HIGH); // вводим в спячку SIM800 модуль 
}



void SMSDallasAddr()
{
SIM800.println (F("Dallas Addresses:"));
  for (byte i = 0; i<size_arrayTemp; i++) 
  {
       TempName(DS18B20[i][8]); SIM800.println(); 
       for (byte k = 0; k<8; k++) {if (DS18B20[i][k]<=0x0F)SIM800.print(F("0")); SIM800.print(DS18B20[i][k],HEX); SIM800.print(F(" "));}
       SIM800.println(); 
  }

}


void SMSzaprosTEL()
{
  startSMS(isStringMessage);
  PrintNumbers (); 
  EndSMS();                                 
}


void SMSbalance() 
{
  digitalWrite (DTR, LOW); // выводим из спячки SIM800 модуль
      delay (150);
  SIM800.print(F("AT+CMGF=1\r"));
      delay(200);
  SIM800.println (F("AT+CUSD=1,\"#100#\""));    // команда на замену на транслит *111*6*2# у МТС 
      delay(1500);  
  digitalWrite (DTR, HIGH); // вводим в спячку SIM800 модуль 
      delay (150);  
}


void AlarmSMS() 
{
  for (byte i = 0; i<2; i++) {startSMS(i+1); SIM800.println (F("Vnimanie!!! Trevoga!!! Sirena Vkl!")); EndSMS(); delay (3500);} 
  alarmSMS = true;
}
 


void PrintNumbers () 
{
  for (byte i=0; i<2; i++) {SIM800.print(F("Tel#")); SIM800.print (i+1); SIM800.print(F(" ")); SIM800.println(TelNumber[i+1]);}
}


void TempName (const byte &_address_) 
{       
        if (_address_== VyhlopC) SIM800.print(F("Vyhlop:  "));
   else if (_address_== EngineC) SIM800.print(F("Engine:  "));
   else if (_address_== UlicaC)  SIM800.print(F("Ulica:     "));
   else if (_address_== SalonC)  SIM800.print(F("Salon:    "));
}

void SMSzapros(const byte &telef)
{
  startSMS(telef);
  if (telef == 3)     // если номер #1 не записан в память
  {
    SIM800.println (F("Tel.number#1 not has been save in memory"));
    SIM800.println (F("For save Tel#1 send SMS command \"WriteNumber1\""));
    PrintNumbers ();
  }
  else                        // если номер #1 записан в память
  {
    SIM800.print (F("Webasto ")); on_off (webasto);
    if (webasto) 
      { 
        SIM800.print (F("StartWebasto "));
        if (startWebasto_OK) SIM800.println (F("OK"));
        else SIM800.println (F("FAIL"));
      }
       #ifdef Signalka 
    SIM800.print (F("Engine    "));  on_off (engine);
    SIM800.print (F("IGN        ")); on_off (ignition);
    SIM800.print (F("Ohrana  "));    on_off (ohrana);
    if (trevoga)  SIM800.println (F("Vnimanie!!! Trevoga!!! Sirena Vkl!"));
      #endif
    SIM800.print(F("Battery: "));
    if ((needAction>0 && !noData) || needAction == 0 || ProtocolSTATUS == ANALOG) 
      {
        SIM800.print (Vpit,1); SIM800.println(F(" V"));
      }
    else if (needAction>0 && noData && ProtocolSTATUS != ANALOG) SIM800.println(F(" No Data"));
    SIM800.println(F("Temperatures:"));
    if (ProtocolSTATUS != ANALOG)
    {
      SIM800.print(F("Heater:  ")); 
      if ((needAction>0 && !noData) || needAction == 0) {SIM800.print (HeaterC);  grad ();}
      else if (needAction>0 && noData) SIM800.println(F(" No Data"));
    }
 
 // ниже распечатаем все температуры   
    for (byte i = 0; i<size_arrayTemp; i++) 
    {
      if (DS18B20[i][8]==VyhlopC && ProtocolSTATUS != ANALOG) {}
      else {TempName(DS18B20[i][8]); SIM800.print (Temper(DS18B20[i][8])); grad ();}
    }
  

    if (ProtocolSTATUS == STATUSBUS)
    {
      SIM800.print(F("Errors:   ")); 
      if ((needAction>0 && !noData) || needAction == 0) SIM800.println (DTC[0]);
      else if (needAction>0 && noData) SIM800.println(F(" No Data"));
    }
  }  // end of если номер #1 записан в память
   EndSMS();    
}

void on_off (const bool &stat) {if (stat) SIM800.println (F("ON")); else SIM800.println (F("OFF")); }

void grad () {SIM800.println (F("*C")); }


void ServiceINFO(const byte &telef)
{
  startSMS(telef);
  SIM800.print(F("Heater:  ")); 
       if (HeaterBUSTYPE==TTC_E)     SIM800.println(F("TTC/E"));
  else if (HeaterBUSTYPE==WBUS && WBUS_VER>=0x30 && WBUS_VER<0x40)      SIM800.println(F("VEVO"));
  else if (HeaterBUSTYPE==WBUS && WBUS_VER>=0x40)      SIM800.println(F("EVO"));
  else if (HeaterBUSTYPE==HYDRONIC)  SIM800.println(F("HYDRONIC"));
  SIM800.print(F("Start:    "));
       if (ProtocolSTART==IMPULSE)   SIM800.println(F("GND Imp"));
  else if (ProtocolSTART==STARTBUS)  {SIM800.print(F("BUS"));
             if (HeaterBUSTYPE==WBUS) {SIM800.print(F(" 0x")); SIM800.println (StartByte, HEX);}
             else SIM800.println();}
  else if (ProtocolSTART==POTENCIAL) SIM800.println(F("Potencial+12V"));
  
  SIM800.print(F("Status:  "));
       if (ProtocolSTATUS==ANALOG)    SIM800.println(F("ANALOG"));
  else if (ProtocolSTATUS==STATUSBUS) SIM800.println(F("BUS"));
  
   
  if (ProtocolSTART!=IMPULSE) {SIM800.print(F("Webasto Time: ")); SIM800.print (TimeWebasto); SIM800.println(F("min"));}
  SIM800.print(F("Modem Resets: ")); SIM800.println (ResetNumber); 

  if (ProtocolSTATUS==ANALOG) {SIM800.print(F("DeltaT:  ")); SIM800.print(delta); grad ();}

  if (ProtocolSTATUS==STATUSBUS){
    if (!noData) {
    SIM800.print(F("   BurnFAN       ")); on_off (airfan);
    SIM800.print(F("   WaterPUMP ")); on_off (waterpump);
    SIM800.print(F("   PLUG            ")); on_off (plug);
    SIM800.print(F("   FuelPUMP    ")); on_off (fuelpump);
    SIM800.print(F("   Blower          ")); on_off (blowerfan);
    
   SIM800.print (F("Errors:  ")); SIM800.println (DTC[0]);
  if (DTC[0] >0){
     #ifdef WBUS_heaters
     if (HeaterBUSTYPE==WBUS){
     for (byte i=0; i<DTC[0]; i++) {
     if (DTC[i+1]<=0x0F) SIM800.print(F("0"));
     SIM800.print (DTC[i+1], HEX); 
     if (bitRead(DTC[6], i+1)) SIM800.println (F(" Active")); 
     else SIM800.println (F(" Passive"));      }
                              }
     #endif
     #ifdef TTE_C_heaters
     if (HeaterBUSTYPE==TTC_E){
      for (byte i=0; i<DTC[0]; i++) {
        SIM800.print  (DTC[i*4+1], HEX);
        if (DTC[i*4+2]<10) SIM800.print(F("0"));  SIM800.print  (DTC[i*4+2], HEX);
        SIM800.print  ("/");
        SIM800.print  (DTC[i*4+3], HEX);
        SIM800.print  (F("  "));
        SIM800.println(DTC[i*4+4]);
                                    }
      
      }
      #endif
                }
                  }
 else SIM800.println(F("Heater not answer. No Data"));}
   EndSMS();    
}


void MQTTrestart() 
{
 fails = 3; settingGSM = 1; failresets =0; TimerMQTTreconnect = 0;
}

void Is_not_configured() {SIM800.println(F("Is not configured")); EndSMS();}

void DTCareCleared() 
{
  if (KTOzapros==5) PUBsend(PGM_Text(_STAT_DTS_TOP),  PGM_TextVAR(_OK) ,  0);
  else{
  startSMS (KTOzapros); SIM800.println (F("DTC are cleared!")); EndSMS();  
      }
 needAction=0; w_bus_init = 9; NeedTimer = 0; 
}

