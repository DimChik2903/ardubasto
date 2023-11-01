


void Queue (const byte &actioN) {needAction = actioN;
             if (ProtocolSTATUS==STATUSBUS){noData = 1; NeedTimer = 1; prevNeedTimer = currmillis; if (!webasto) w_bus_init = 1;}
        else if (ProtocolSTATUS==ANALOG) { if (needAction==NEED_SMSZAPROS) SMSzapros(isStringMessage); if (needAction==NEED_SERVICEINFO) ServiceINFO(isStringMessage);}
                                 } 


void readModem() //_____Цикл чтения входящих СМС-сообщений______________     
{

  static uint32_t timerMQTTreconnect = 0;
  

// если было неудачное подключение к MQTT, ждём час и снова пробуем
if (mqtt && TimerMQTTreconnect && currmillis - timerMQTTreconnect > 3600000UL) 
    {
      TimerMQTTreconnect = 0; 
      MQTTrestart();
    }


   


    if (!SIM800.available()) return;
    static bool SaveNumber2 = 0;      // флаг когда необходима запись номера#2, он true
    char currSymb[2] = {0};
    currSymb[0] = SIM800.read();
    static bool stringEnd = 0;
#ifdef DEBUG_MODEM
if (currSymb[0]!='&' && currSymb[0]!='$') Serial.write(currSymb[0]);
else if (currSymb[0]=='&') Serial.println();
#endif
    if (currSymb[0] == '\r' || currSymb[0] == '&' || stringEnd == 1)
       {
                
         if (isStringMessage>0&&isStringMessage<3) //если текущая строка - SMS-сообщение, отреагируем на него соответствующим образом
                {
     if (strcmp(currStr, PGM_Text(_ZAPROS))==0)       {if (Initreset) SMSzapros(isStringMessage);   else {KTOzapros = isStringMessage; Queue(NEED_SMSZAPROS);}}       // Передача параметров по СМС
else if (strcmp(currStr, PGM_Text(_SERVICE_INFO))==0) {if (Initreset) ServiceINFO(isStringMessage); else {KTOzapros = isStringMessage; Queue(NEED_SERVICEINFO);}}   // Передача сервисной информации  по СМС
else if (strcmp(currStr, PGM_Text(_ERASE_DTC))==0)    {if (!webasto) Queue(NEED_DTCCLEAR);}   // Запрос на стриание ошибок
else if (strcmp(currStr, PGM_Text(_ZAPROSTEL))==0)    {SMSzaprosTEL();}                    // Передача номеров телефонов пользователей по СМС
else if (strcmp(currStr, PGM_Text(_GSM_RESETS))==0)   {ResetNumber=0; EEPROM.write (ResetNumber_cell, ResetNumber); if (Initreset) ServiceINFO(isStringMessage); else {KTOzapros = isStringMessage; Queue(NEED_SERVICEINFO);}}     //сброс счетчика ресетов GSM модуля
else if (strcmp(currStr, PGM_Text(_VERSION))==0)      {startSMS(isStringMessage);  SIM800.println (ver); EndSMS ();}  //запрос версии ПО
else if (strcmp(currStr, PGM_Text(_MQTT_RESET))==0)   {prevModemReboot = currmillis; if (mqtt)smsModemReboot_timer = 1;}       // рестарт модема и MQTT 

              
else if (strcmp(currStr, PGM_Text(_HEATER_ON))==0)  { startSMS(isStringMessage); SIM800.println(F("Webasto ")); 
  
           if (!webasto)  {StartWebasto (); KTOreport = isStringMessage;}
           else SIM800.println(F("uzhe ")); SIM800.println (F("vkluchena")); EndSMS();}

                                                            
else if (strcmp(currStr, PGM_Text(_HEATER_OFF))==0)   {startSMS(isStringMessage); SIM800.println(F("Webasto "));  
           if (webasto)StopWebasto ();  // если получили команду на выключение и вебаста в настоящий момент включена - выключаем
           else SIM800.println(F("uzhe "));  SIM800.println(F("otkluchena"));EndSMS();}          

// если получили команду на включение ДВС и он в настоящий момент выключен - включаем
else if (strcmp(currStr, PGM_Text(_ENGINE_ON))==0)  {startSMS(isStringMessage); if (!CentrLock) {SIM800.println(F("Dvigatel "));  
           if (!engine)  { StartStop_Engine(); reportEngine = true; prevReportEngine = currmillis; KTOreport = isStringMessage;} 
           else SIM800.println(F("uzhe ")); SIM800.println(F("start")); } else Is_not_configured();}
                                                                                 
else if (strcmp(currStr, PGM_Text(_ENGINE_OFF))==0)   {startSMS(isStringMessage); if (!CentrLock) { SIM800.println(F("Dvigatel "));  
           if (engine){ StartStop_Engine(); reportEngine = false;} // если получили команду на выключение ДВС и он в настоящий момент работает - выключаем
           else SIM800.println(F("uzhe ")); SIM800.println(F("ostanovlen"));} else Is_not_configured();}   
              
else if (strcmp(currStr, PGM_Text(_IMPULSE))==0)   {if (!webasto) {ProtocolSTART = IMPULSE;  EEPROM.write(ProtocolSTART_cell,ProtocolSTART);     
                                            startSMS(isStringMessage); SIM800.println(F("zapusk GND_impulse")); EndSMS();}}

else if (strcmp(currStr, PGM_Text(_STARTBUS))==0)  {if (!webasto) {ProtocolSTART = STARTBUS; EEPROM.write(ProtocolSTART_cell,ProtocolSTART);  webasto = 0;  
                                            startSMS(isStringMessage); SIM800.println(F("zapusk BUS")); EndSMS();}}

else if (strcmp(currStr, PGM_Text(_POTENCIAL))==0) {if (!webasto) {ProtocolSTART = POTENCIAL; EEPROM.write(ProtocolSTART_cell,ProtocolSTART);   
                                            startSMS(isStringMessage); SIM800.println(F("zapusk +12V Potencial")); EndSMS();}}

else if (strcmp(currStr, PGM_Text(_DALLASADDR))==0) {startSMS(isStringMessage); SMSDallasAddr(); EndSMS();}

else if (strstr(currStr, PGM_Text(_STATUS))>0)   {if (!webasto) {
                                                            
                                            startSMS(isStringMessage); SIM800.print (PGM_Text(_STATUS)); SIM800.print (PGM_Text(_SPACE));
                                            bool dadada=0;
                                                    if (strstr(currStr, PGM_Text(_BUS))>0)   {ProtocolSTATUS = STATUSBUS; SIM800.println(PGM_Text(_BUS)),  dadada=1;}
                                               else if (strstr(currStr, PGM_Text(_ANALOG))>0){ProtocolSTATUS = ANALOG;  SIM800.println(PGM_Text(_ANALOG)), dadada=1;}
                                               else SIM800.println (PGM_Text(_ERROR));
                                               EndSMS(); if (dadada) EEPROM.write(ProtocolSTATUS_cell,ProtocolSTATUS);}}    


else if (strstr(currStr, PGM_Text(_HEATERBUSTYPE))>0) {if (!webasto) {
                                            bool dadada=0; 
                                            startSMS(isStringMessage); SIM800.print (PGM_Text(_HEATERBUSTYPE)); SIM800.print (PGM_Text(_SPACE));
                                                    if (strstr(currStr, PGM_Text(_TTCE))>0)    { HeaterBUSTYPE = TTC_E; SIM800.println(PGM_Text(_TTCE)), K_LINE.end(),  K_LINE.begin(10400); dadada=1;}
                                               else if (strstr(currStr, PGM_Text(_WBUS))>0)     { HeaterBUSTYPE = WBUS; SIM800.println(PGM_Text(_WBUS)), K_LINE.end(),  K_LINE.begin(2400, SERIAL_8E1);dadada=1;}
                                               else if (strstr(currStr, PGM_Text(_HYDRONIC))>0) { HeaterBUSTYPE = HYDRONIC; SIM800.println(PGM_Text(_HYDRONIC)); dadada=1;}
                                               else  SIM800.println (PGM_Text(_ERROR));    
                                                                        EndSMS(); if (dadada) EEPROM.write(HeaterBUS_cell,HeaterBUSTYPE);}}      




else if (strstr(currStr, PGM_Text(_ADDRESSA))>0) {if (!webasto) { startSMS(isStringMessage); byte savadr = ConvertAddr(); 
                                                    SIM800.print(PGM_Text(_ADDRESSA));  SIM800.print (PGM_Text(_SPACE));
                                                    if (savadr>=0 && savadr<=3) {TempName(savadr); SIM800.println (F("saved OK!")); 
                                                    SMSDallasAddr();
                                                                                } 
                                                    else SIM800.println(F("is incorrect!"));
                                                    
                                                                          EndSMS(); }}      
                                                                                                                                                
                                                                                                                                                 
else if (strstr(currStr, PGM_Text(_DELTA))>0)   {if (!webasto) {delta = atoi(strstr(currStr, PGM_Text(_DELTA))+strlen(PGM_Text(_DELTA)));
               EEPROM.write(delta_cell, delta);  startSMS(isStringMessage);
               SIM800.print(PGM_Text(_DELTA)); SIM800.print (PGM_Text(_SPACE)); SIM800.print(delta); grad(); EndSMS();}}
                                                                                                                                            

               
else if (strstr(currStr, PGM_Text(_MIN))>0)   {if (!webasto) {TimeWebasto = atoi(strstr(currStr, PGM_Text(_MIN))+strlen(PGM_Text(_MIN))); // для задания время цикла работы отправить сообщение вида "25 min", где 25 время работы в мин
               if (TimeWebasto>59)  TimeWebasto = 59;
               if (TimeWebasto<=15) TimeWebasto = 15;
               EEPROM.write(TimeWebasto_cell,TimeWebasto);
               startSMS(isStringMessage); SIM800.print(F("Webasto time: ")); SIM800.print(TimeWebasto); SIM800.println(PGM_Text(_MIN)); EndSMS();}}
               
else if (strstr(currStr, PGM_Text(_STARTBYTE))>0) {int Z =atoi(strstr(currStr, PGM_Text(_STARTBYTE))+strlen(PGM_Text(_STARTBYTE))); if (Z>=0x14 && Z<=0x17) StartByte= Z+12; EEPROM.write(StartByte_cell, StartByte); 
HEATER_START[0] = StartByte;
HEATER_PRESENCE[1] = StartByte;
               startSMS(isStringMessage); SIM800.print(PGM_Text(_STARTBYTE)); SIM800.print (PGM_Text(_SPACE)); SIM800.println(StartByte, HEX);  EndSMS();}

else if (strcmp(currStr, PGM_Text(_RESETNUMBERS))==0)   {if (isStringMessage == 1) {startSMS(isStringMessage); SIM800.println(F("Phone numbers are erased")); EndSMS(); 
                                                                     
             strcpy (TelNumber[1],TelNumber[0]); strcpy (TelNumber[2], TelNumber[0]); for (int i=0; i<SizeTelNumber; i++) {EEPROM.write (i+TelNumber1_cell,  TelNumber[1][i]); EEPROM.write (i+TelNumber2_cell,  TelNumber[2][i]); }}}

else if (strcmp(currStr, PGM_Text(_WRITENUMBER2))==0 && isStringMessage == 1)   { 
                SaveNumber2 = 1; startSMS(isStringMessage); SIM800.println(F("Otpravte lyuboye SMS s nomera#2 dlya sohraneniya nomera")); EndSMS();} 
                                                                      

                                                     
else if (strcmp(currStr, PGM_Text(_BALANCE))==0)    SMSbalance();
            isStringMessage = 0;
                }
              
              
else if (isStringMessage==3){ if (strcmp(currStr, PGM_Text(_WRITENUMBER1))==0)   { strcpy (TelNumber[1], TelNumber[3]); for (int i=0; i<SizeTelNumber; i++) {EEPROM.write (i+TelNumber1_cell, TelNumber[3][i]);}
              startSMS(1); SIM800.println(F("Tel Number#1 is saving in memory"));  SIM800.print(F("Tel#1: "));  SIM800.println (TelNumber[1]); EndSMS();
              } 
                                             else if (strcmp(currStr, PGM_Text(_ZAPROS))==0)      { SMSzapros(isStringMessage);}   
                                             else if (strcmp(currStr, PGM_Text(_ZAPROSTEL))==0)   { SMSzaprosTEL();}               // Передача номеров телефонов пользователей по СМС            
               isStringMessage = 0;
              
              }           



                
else if (isStringMessage==0) 

{  
  bool cmT = 0;
  if (strstr(currStr, PGM_Text(_CMT))>0) cmT = 1;
  
     if (strcmp (TelNumber[1],TelNumber[0])!=0 && !SaveNumber2)
           {
         
                     if (cmT && strncmp (strstr(currStr, PGM_Text(_CMT))+ strlen(PGM_Text(_CMT)), TelNumber[1], SizeTelNumber)==0)  { isStringMessage = 1; KTOzapros = 1; }   
                else if (cmT && strncmp (strstr(currStr, PGM_Text(_CMT))+ strlen(PGM_Text(_CMT)), TelNumber[2], SizeTelNumber)==0)  { isStringMessage = 2; KTOzapros = 2; }   
                else if (strstr(currStr, PGM_Text(_CUSD))>0)  //если текущая строка начинается с "+CUSD",то следующая строка является запросом баланса
                  {
                  startSMS(KTOzapros);
                  SIM800.print (currStr);
                  EndSMS();
                  }
           }
                  
      else if    (cmT && !SaveNumber2) { isStringMessage = 3; for (int i =0; i<SizeTelNumber; i++) {TelNumber[3][i]=currStr[i+7];}}
      else if    (cmT &&  SaveNumber2) { for (int i =0; i<SizeTelNumber; i++) {TelNumber[3][i]=currStr[i+7]; EEPROM.write (i+TelNumber2_cell, TelNumber[3][i]);} strcpy (TelNumber[2] , TelNumber[3]); 
      startSMS(2); SIM800.println(F("Vash nomer sochranyon kak Number#2 v pamyati!")); 
      SIM800.print(F("Tel#1: ")); SIM800.println(TelNumber[1]); SIM800.print(F("Tel#2: "));  SIM800.println (TelNumber[2]); EndSMS(); SaveNumber2 = 0; } 
                  
                      if (strstr(currStr, PGM_Text(_CSQ))>0) //если текущая строка начинается с "+CSQ",то значит был запрос на уровень сигнала GSM
                       {
                      if (settingGSM) {gsmstatus++; fails = 0; delayATcommand = 400;}
                      signalLevel=atoi(strstr(currStr, PGM_Text(_CSQ))+ strlen(PGM_Text(_CSQ)));
                       if (((signalLevel>=0  && signalLevel<=2) || signalLevel ==99) && signalLevelstatus!=0) { signalLevelstatus --; }
                  else if (signalLevel==0) {signalLevelstatus =0;}
                  else if (signalLevel>=4 && signalLevel<=7 && signalLevel !=99) {signalLevelstatus ++; if  (signalLevelstatus>=3)signalLevelstatus = 3;}
                  else if (signalLevel>=8 && signalLevel !=99) {signalLevelstatus = 3;}
                  
                       if (last_signalLevelstatus==3 && signalLevelstatus==0) {               last_signalLevelstatus  = 0; }// если пропал сигнал сети 
                  else if (last_signalLevelstatus==0 && signalLevelstatus==3) {last_signalLevelstatus  = 3; prevModemReboot = currmillis; smsModemReboot_timer = 1;   }// если появился сигнал сети
                       }
                  

// ----------------------------данные для MQTT

if (strstr(currStr, PGM_Text(_CLOSED))>0 || strstr(currStr, PGM_Text(_ERROR))>0)   {
        fails = 3;
        settingGSM = 1;
      }
  
      if (strstr(currStr, PGM_Text(_OK))>0) {
  
        if (!settingGSM){ if (failresets > 4 || (failresets <= 4 && gsmstatus >= setPubPack)) {fails = 0; digitalWrite (DTR, HIGH);}}
        if (settingGSM && gsmstatus >= WaitGSM && gsmstatus < setBrokerconnect) {
             // если подключиться к MQTT серверу не удалось, оставляем настройки только для СМС
             // и запускаем таймер на час, чтобы через час ещё раз пробовать:
             if (gsmstatus == delSMS && failresets > 4) 
                  {settingGSM = 0; 
                   digitalWrite (DTR, HIGH);
                   timerMQTTreconnect = currmillis;
                   TimerMQTTreconnect = 1;
                   }  
             gsmstatus++; if (gsmstatus>16)gsmstatus=16;
             fails = 0;
             delayATcommand = 400;
                                                                                }
      }
      if (strstr(currStr, PGM_Text(_CONNECT)) >0) if (gsmstatus == setBrokerconnect) {
          gsmstatus++; if (gsmstatus>16)gsmstatus=16;
          fails = 0;
          delayATcommand = 300;
        }
        
      if (strstr(currStr, PGM_Text(_SEND_OK)) >0) {
        fails = 0;
        if (settingGSM && gsmstatus >= setAuthPack && gsmstatus <= setPubPack) {
          if (gsmstatus == setPubPack) {
            settingGSM = 0;
            if (mqtt) failresets = 0;
          } delayATcommand = 300;
          gsmstatus++; if (gsmstatus>16)gsmstatus=16;
        }
      }


//-------------------------топики MQTT, принимаемые от брокера
     
      if (strstr(currStr, PGM_Text(_CTRL_HEAT_TOP)) > 0) 
      {
        bool comanda = (bool)atoi (strstr (currStr, PGM_Text(_CTRL_HEAT_TOP)) + strlen(PGM_Text(_CTRL_HEAT_TOP)));
        if (comanda) {StartWebasto(); report = false;} 
        else {StopWebasto();}
      }
      if (strstr(currStr, PGM_Text(_CTRL_TIME_TOP)) > 0) {
        TimeWebasto = atoi (strstr (currStr,PGM_Text(_CTRL_TIME_TOP)) + strlen(PGM_Text(_CTRL_TIME_TOP)));
        EEPROM.write(TimeWebasto_cell,TimeWebasto);
        PUBsend(PGM_Text(_STAT_TIME_TOP),  Var_to_text(TimeWebasto,0), 0);
        remTime(0) ;
      }
        if (strstr(currStr, PGM_Text(_CTRL_TIMEENG_TOP)) > 0) { 
        TimeEngine = atoi (strstr (currStr,PGM_Text(_CTRL_TIMEENG_TOP)) + strlen(PGM_Text(_CTRL_TIMEENG_TOP)));
        PUBsend(PGM_Text(_STAT_ENGTIME_TOP),  PGM_TextVAR(_OK), 0);
        }
      
      if (strstr(currStr, PGM_Text(_CTRL_REFRESH_TOP)) > 0) {if (!webasto) {Queue(NEED_MQTTZAPROS); delay (100);}   MQTTsendDatastream();}
      if (strstr(currStr, PGM_Text(_CTRL_RESRES_TOP))  > 0) {ResetNumber=0; EEPROM.write (ResetNumber_cell, ResetNumber); PUBsend(PGM_Text(_STAT_RESETS_TOP),Var_to_text(ResetNumber,0),0);}
      
      if (strstr(currStr, PGM_Text(_CTRL_LOCK_TOP))    > 0 && CentrLock)  // если получили команду открыть/закрыть замки
      { 
        bool comanda = (bool)atoi(strstr (currStr, PGM_Text(_CTRL_LOCK_TOP)) + strlen(PGM_Text(_CTRL_LOCK_TOP)));
        if (comanda) {digitalWrite (StartEng, 1); StartEng_timer=1; prevStartEng = currmillis;} 
        else {digitalWrite (PUMPE, 1); Pumpe_timer=1; prevPumpe = currmillis;}
      }

      if (strstr(currStr, PGM_Text(_CTRL_DTC_CL)) > 0)    {if (!webasto) {KTOzapros = 5; Queue(NEED_DTCCLEAR);}} 
#ifdef Signalka
      if (strstr(currStr, PGM_Text(_CTRL_DVS_TOP))    > 0 && !CentrLock)  // если получили команду старт/стоп двс
      { 
        bool comanda = (bool)atoi(strstr (currStr,PGM_Text(_CTRL_DVS_TOP)) + strlen(PGM_Text(_CTRL_DVS_TOP)));
        if (comanda) {if (!engine)  {StartStop_Engine(); reportEngine = 0;}} 
        else {if (engine) {StartStop_Engine();}}
      }
#endif



// ------------------------конец данных для MQTT

              
              } 


               
        currStr[0] = 0; stringEnd = 0;
      } 
 
    else if ( currSymb[0] != '\n' && currSymb[0] != '$')  strcat (currStr,currSymb);
    if (strlen(currStr)>59) stringEnd = 1;
}
