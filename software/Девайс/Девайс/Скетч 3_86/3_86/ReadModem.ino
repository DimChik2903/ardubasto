

void Queue (const byte &actioN) {needAction = actioN; 
             if (ProtocolSTATUS==STATUSBUS){NeedTimer = 1; prevNeedTimer = currmillis; if (!webasto) w_bus_init = 1;}
        else if (ProtocolSTATUS==ANALOG) {if (needAction==NEED_SMSZAPROS) SMSzapros(); if (needAction==NEED_SERVICEINFO) ServiceINFO();}
                                 } 


void readModem() //_____Цикл чтения входящих СМС-сообщений______________     
{

  static uint32_t timerMQTTreconnect = 0;
  static bool TimerMQTTreconnect = 0;

// если было неудачное подключение к MQTT, ждём час и снова пробуем
if (TimerMQTTreconnect && currmillis - timerMQTTreconnect > 3600000UL) 
    {
      TimerMQTTreconnect = 0; 
      fails = 3;
      settingGSM = 1;
    }





    if (!SIM800.available()) return;
    static bool SaveNumber2 = 0;      // флаг когда необходима запись номера#2, он true
    char currSymb = SIM800.read();

    if ('\r' == currSymb || '&' == currSymb)
       {
         if (isStringMessage!=0&&isStringMessage!=3) //если текущая строка - SMS-сообщение, отреагируем на него соответствующим образом
                {
if      (!currStr.compareTo(F("ZAPROS")))       {Queue(NEED_SMSZAPROS);}             // Передача параметров по СМС
else if (!currStr.compareTo(F("Service-info"))) {Queue(NEED_SERVICEINFO);}           // Передача сервисной информации  по СМС
else if (!currStr.compareTo(F("Erase DTC")))    {Queue(NEED_DTCCLEAR);}              // Запрос на стриание ошибок
else if (!currStr.compareTo(F("ZAPROSTEL")))    {SMSzaprosTEL();}                    // Передача номеров телефонов пользователей по СМС
else if (!currStr.compareTo(F("GSMResets-0")))  {ResetNumber=0; EEPROM.write (ResetNumber_cell, ResetNumber); Queue(NEED_SERVICEINFO);}     //сброс счетчика ресетов GSM модуля
else if (!currStr.compareTo(F("Version")))      {startSMS(isStringMessage);  SIM800.println (ver); EndSMS ();}               //запрос версии ПО


else if (!currStr.compareTo(F("Signal-level"))) {digitalWrite (DTR, LOW);  delay (150); SIM800.println(F("AT+CSQ")); digitalWrite (DTR, HIGH);}                // запрос уровня сигнала GSM
              
else if (!currStr.compareTo(F("Webasto-ON")))  { startSMS(isStringMessage); SIM800.println(F("Webasto ")); 
  
           if (!webasto)  {StartWebasto (); KTOreport = isStringMessage;}
           else SIM800.println(F("uzhe ")); SIM800.println (F("vkluchena")); EndSMS();}

                                                            
else if (!currStr.compareTo(F("Webasto-OFF")))   {startSMS(isStringMessage); SIM800.println(F("Webasto "));  
           if (webasto)StopWebasto ();  // если получили команду на выключение и вебаста в настоящий момент включена - выключаем
           else SIM800.println(F("uzhe "));  SIM800.println(F("otkluchena"));EndSMS();}          

// если получили команду на включение ДВС и он в настоящий момент выключен - включаем
else if (!currStr.compareTo(F("Engine-ON")))  {startSMS(isStringMessage); SIM800.println(F("Dvigatel "));  
           if (!engine)  { digitalWrite (StartEng, HIGH);  prevStartEng=currmillis; StartEng_timer=true; reportEngine = true; prevReportEngine = currmillis; KTOreport = isStringMessage;} 
           else SIM800.println(F("uzhe ")); SIM800.println(F("start")); EndSMS();}
                                                                                 
else if (!currStr.compareTo(F("Engine-OFF")))   {startSMS(isStringMessage); SIM800.println(F("Dvigatel "));  
           if (engine){ digitalWrite (StartEng, HIGH);  prevStartEng=currmillis; StartEng_timer=true; reportEngine = false;} // если получили команду на выключение ДВС и он в настоящий момент работает - выключаем
           else SIM800.println(F("uzhe ")); SIM800.println(F("ostanovlen")); EndSMS();}          
              
else if (!currStr.compareTo(F("Impulse")))   {if (!webasto) {ProtocolSTART = IMPULSE;  EEPROM.write(ProtocolSTART_cell,ProtocolSTART);     
                                            startSMS(isStringMessage); SIM800.println(F("zapusk GND_impulse")); EndSMS();}}

else if (!currStr.compareTo(F("Startbus")))  {if (!webasto) {ProtocolSTART = STARTBUS; EEPROM.write(ProtocolSTART_cell,ProtocolSTART);  webasto = 0;  
                                            startSMS(isStringMessage); SIM800.println(F("zapusk BUS")); EndSMS();}}

else if (!currStr.compareTo(F("Potencial"))) {if (!webasto) {ProtocolSTART = POTENCIAL; EEPROM.write(ProtocolSTART_cell,ProtocolSTART);   
                                            startSMS(isStringMessage); SIM800.println(F("zapusk +12V Potencial")); EndSMS();}}

else if (!currStr.compareTo(F("DallasAddr"))) {startSMS(isStringMessage); SMSDallasAddr(); EndSMS();}

else if (currStr.endsWith(F("Status")))   {if (!webasto) {byte st = currStr.toInt(); if (st >= STATUSBUS && st<=ANALOG )ProtocolSTATUS = st; EEPROM.write(ProtocolSTATUS_cell,ProtocolSTATUS);   
                                            startSMS(isStringMessage); SIM800.print (F("Status: "));
                                                                             if (ProtocolSTATUS == 0)SIM800.println(F("BUS")); 
                                                                        else if (ProtocolSTATUS == 1)SIM800.println(F("Analog"));
                                                                        EndSMS();}}    


else if (currStr.endsWith(F("HeaterBusType"))) {if (!webasto) {byte st = currStr.toInt(); if (st >= TTC_E && st<=HYDRONIC) HeaterBUSTYPE = st; EEPROM.write(HeaterBUS_cell,HeaterBUSTYPE);   
                                            startSMS(isStringMessage); SIM800.print (F("HeaterBUStype: "));
                                                                             if (HeaterBUSTYPE == TTC_E)     SIM800.println(F("TTC_E")), K_LINE.end(),  K_LINE.begin(10400);
                                                                        else if (HeaterBUSTYPE == WBUS)      SIM800.println(F("W-BUS")),  K_LINE.end(),  K_LINE.begin(2400, SERIAL_8E1);
                                                                        else if (HeaterBUSTYPE == HYDRONIC)  SIM800.println(F("HYDRONIC"));
                                                                             
                                                                        EndSMS();}}      




else if (currStr.endsWith(F("ADDR"))) {if (!webasto) { startSMS(isStringMessage); byte savadr = ConvertAddr(); 
                                                    if (savadr>=0 && savadr<=3) {SIM800.print(F("address ")); TempName(savadr); SIM800.println (F(" saved OK!")); 
                                                    SMSDallasAddr();
                                                                                } 
                                                    else SIM800.println(F("address is incorrect!"));
                                                    
                                                                          EndSMS(); }}      
                                                                                                                                                
                                                                                                                                              
else if (currStr.endsWith(F("Delta")))   {if (!webasto) {delta = currStr.toInt(); //
               EEPROM.write(delta_cell, delta);  startSMS(isStringMessage);
               SIM800.print(F("DeltaT: ")); SIM800.print(delta); SIM800.print(F("*C")); EndSMS();}}
                                                                                                                                            

               
else if (currStr.endsWith(F("min")))   {if (!webasto) {TimeWebasto = currStr.toInt(); // для задания время цикла работы отправить сообщение вида "25 min", где 25 время работы в мин
               if (TimeWebasto>59)  TimeWebasto = 59;
               if (TimeWebasto<=15) TimeWebasto = 15;
               EEPROM.write(TimeWebasto_cell,TimeWebasto);
               startSMS(isStringMessage); SIM800.print(F("Webasto time: ")); SIM800.print(TimeWebasto); SIM800.print(F("min")); EndSMS();}}
               
else if (currStr.endsWith(F("StartByte"))) {byte Z =currStr.toInt(); if (Z>=0x14 && Z<=0x17) StartByte= Z+12; EEPROM.write(StartByte_cell, StartByte); 
HEATER_START[0] = StartByte;
HEATER_PRESENCE[1] = StartByte;
               startSMS(isStringMessage); SIM800.print(F("StartByte: ")); SIM800.print(StartByte, HEX); SIM800.print(F("h")); EndSMS();}

else if (!currStr.compareTo(F("ResetNumbers")))   {if (isStringMessage == 1) {startSMS(isStringMessage); SIM800.println(F("Phone numbers are erased")); EndSMS(); 
                                                                     
             TelNumber[1] = TelNumber[0]; TelNumber[2] = TelNumber[0]; for (int i=0; i<SizeTelNumber; i++) {EEPROM.write (i+TelNumber1_cell,  TelNumber[1][i]); EEPROM.write (i+TelNumber2_cell,  TelNumber[2][i]); }}}

else if (!currStr.compareTo(F("WriteNumber2"))&& isStringMessage == 1)   { 
                SaveNumber2 = 1; startSMS(isStringMessage); SIM800.println(F("Otpravte lyuboye SMS s nomera#2 dlya sohraneniya nomera")); EndSMS();} 

                                                                        

                                                     
else if (!currStr.compareTo(F("Balance")))    SMSbalance();
            isStringMessage = 0;
                }
              
              
else if (isStringMessage==3){ if (!currStr.compareTo(F("WriteNumber1")))   { TelNumber[1] = TelNumber[3]; for (int i=0; i<SizeTelNumber; i++) {EEPROM.write (i+TelNumber1_cell, TelNumber[3][i]);}
              startSMS(1); SIM800.println(F("Tel Number#1 is saving in memory"));  SIM800.print("Tel#1: ");  SIM800.println (TelNumber[1]); EndSMS();
              } 
                                             else if (!currStr.compareTo(F("ZAPROS")))   { SMSzapros();}   
                                             else if (!currStr.compareTo(F("ZAPROSTEL")))   { SMSzaprosTEL();}               // Передача номеров телефонов пользователей по СМС            
               isStringMessage = 0;
              
              }           



                
else if (isStringMessage==0) {  if (TelNumber[1]!=TelNumber[0] && !SaveNumber2){
         
                     if (currStr.startsWith("+CMT: \""+TelNumber[1])) { isStringMessage = 1; KTOzapros = 1; }   
                else if (currStr.startsWith("+CMT: \""+TelNumber[2])) { isStringMessage = 2; KTOzapros = 2; }   
                else if (currStr.startsWith("+CUSD: 0,"))  //если текущая строка начинается с "+CUSD",то следующая строка является запросом баланса
                  {
                  startSMS(KTOzapros);
                  SIM800.print (currStr);
                  EndSMS();
                  }

                else if (currStr.startsWith(F("+CSQ:"))) //если текущая строка начинается с "+CSQ",то значит был запрос на уровень сигнала GSM, отправим ответ запрашивающему
                  {
                  startSMS(KTOzapros);
                  SIM800.print (currStr);
                  EndSMS();
                  }
                                                }

                else if    (currStr.startsWith(F("+CMT:")) && !SaveNumber2) { isStringMessage = 3; for (int i =0; i<SizeTelNumber; i++) {TelNumber[3][i]=currStr[i+7];}}
                else if    (currStr.startsWith(F("+CMT:")) && SaveNumber2) { for (int i =0; i<SizeTelNumber; i++) {TelNumber[3][i]=currStr[i+7]; EEPROM.write (i+TelNumber2_cell, TelNumber[3][i]);} TelNumber[2] = TelNumber[3]; 
              startSMS(2); SIM800.println(F("Vash nomer sochranyon kak Number#2 v pamyati!")); 
              SIM800.print(F("Tel#1: ")); SIM800.println(TelNumber[1]); SIM800.print(F("Tel#2: "));  SIM800.println (TelNumber[2]); EndSMS(); SaveNumber2 = 0; } 

// ----------------------------данные для MQTT

if (currStr.indexOf(F("CLOSED")) > -1 || currStr.indexOf(F("ERROR")) > -1)  {
        fails = 3;
        settingGSM = 1;
      }
      if (currStr.indexOf(F("OK")) > -1) {
        if (!settingGSM){ if (failresets > 4 || (failresets <= 4 && gsmstatus >= setPubPack)) {fails = 0; digitalWrite (DTR, HIGH);}}
        if (settingGSM && gsmstatus > WaitGSM && gsmstatus < setBrokerconnect) {
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
      if (currStr.indexOf(F("CONNECT")) > -1) if (gsmstatus == setBrokerconnect) {
          gsmstatus++; if (gsmstatus>16)gsmstatus=16;
          fails = 0;
          delayATcommand = 300;
        }
      if (currStr.indexOf(F("SMS Ready")) > -1) if (gsmstatus == WaitGSM) {
          gsmstatus++; if (gsmstatus>16)gsmstatus=16;
          fails = 0;
          delayATcommand = 300;
        }
      if (currStr.indexOf(F("SEND OK")) > -1) {
        fails = 0;
        if (settingGSM && gsmstatus >= setAuthPack && gsmstatus <= setPubPack) {
          if (gsmstatus == setPubPack) {
            settingGSM = 0;
            failresets = 0;
          } delayATcommand = 300;
          gsmstatus++; if (gsmstatus>16)gsmstatus=16;
        }
      }


//-------------------------топики MQTT, принимаемые от брокера
     
      if (currStr.indexOf(F("h/ctrl")) > -1) 
      {
        bool comanda = currStr.substring(currStr.indexOf(F("h/ctrl")) + 6).toInt();
        if (comanda) {StartWebasto(); report = false;} 
        else {StopWebasto();}
      }
      if (currStr.indexOf(F("h/time/ctrl")) > -1) {
        String TimeW = currStr.substring(currStr.indexOf(F("h/time/ctrl")) + 11);
        TimeWebasto = TimeW.toInt();
        EEPROM.write(TimeWebasto_cell,TimeWebasto);
        PUBsend("h/time/stat",  TimeW, 0);
        remTime(0) ;
      }
      if (currStr.indexOf(F("h/refr0")) > -1) {if (!webasto) {Queue(NEED_MQTTZAPROS); delay (100);}   MQTTsendDatastream();}
      if (currStr.indexOf(F("h/resresets")) > -1) {ResetNumber=0; EEPROM.write (ResetNumber_cell, ResetNumber); PUBsend("h/resets",String(ResetNumber),0);}
      

// ------------------------конец данных для MQTT

              
              } 


               
        currStr = "";
      } 
 
    else if ('\n' != currSymb && '$' != currSymb)  currStr += String(currSymb);
    if (currStr.length()>60) currStr += "\r\n";
}
