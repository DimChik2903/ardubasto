void Heater_BUS ()
{
  static byte requiredmessage =  1;   // что отправляем в данный момент поддержание старта, запрос параметров или запрос ошибок
  static byte header = 0;             // состояние заголовка 
  static byte message_size = 0;       // размер тела принимаемого сообщения, кол-во байт
  static byte j = 2;                  // инкремент
  static byte n = 2;
  const byte bufsize = 140;           // размер буфера принятого сообщения
  static byte buf [bufsize] = {0};    // буфер принятого сообщения
  static byte checksum = 0;           // контрольная сумма входящего сообщения
  static uint32_t prevRESETheader=0;  // таймер сброса заголовка если в момент приёма сообщения данные оборвались
  static bool RESETheader_timer = 0;  // таймер сброса заголовка если в момент приёма сообщения данные оборвались

#ifdef WBUS_heaters
if      (HeaterBUSTYPE == WBUS){
if (webasto) 
{
    if (StartMessageRepeat<4 && (currmillis-Prev_PeriodW_BusStartStop>800) && w_bus_init >= 9){
  sendMessage (HEATER_START, sizeof(HEATER_START)); 
  StartMessageRepeat++; 
  Prev_PeriodW_BusStartStop = currmillis;
  
    }
  if (StartMessageRepeat>=4)
  { 
      if (currmillis-Prev_PeriodW_BusMessage>1000)  
       {
         if (requiredmessage==1) sendMessage (HEATER_PRESENCE, sizeof(HEATER_PRESENCE));  
    else if (requiredmessage==2) sendMessage(HEATER_STATUS_VEVO, sizeof(HEATER_STATUS_VEVO));
    else if (requiredmessage==3) sendMessage(HEATER_STATUS_VEVO03, sizeof(HEATER_STATUS_VEVO03)); 
    else if (requiredmessage==4) sendMessage(HEATER_STATUS_VEVO07, sizeof(HEATER_STATUS_VEVO07)); 
    else if (requiredmessage>=5 && requiredmessage<=7) 
               {
        DTC_REQUEST_wbus15[2] = requiredmessage-4;
        sendMessage (DTC_REQUEST_wbus15, sizeof(DTC_REQUEST_wbus15)); 
               }
    
requiredmessage++; if (requiredmessage > 7) requiredmessage = 1;
StopMessageRepeat = 0;
    
    Prev_PeriodW_BusMessage = currmillis;
       }
  }
}

else if (StopMessageRepeat<4 && (currmillis-Prev_PeriodW_BusStartStop>800))
{
  sendMessage (HEATER_STOP, sizeof(HEATER_STOP));
  StopMessageRepeat++; 
  StartMessageRepeat = 0;
  Prev_PeriodW_BusStartStop = currmillis;
}


     if (w_bus_init==1) {K_LINE.end(); pinMode (TX, OUTPUT); digitalWrite(TX, 0); timerInit = millis(); w_bus_init++;}
else if (w_bus_init==2 && millis() - timerInit>24) {timerInit = currmillis; digitalWrite(TX, 1); w_bus_init++; }
else if (w_bus_init==3 && millis() - timerInit>24) {K_LINE.begin (2400,SERIAL_8E1 ); w_bus_init=9; 
                                                    if (needAction>0) sendMessage (HEATER_BEGIN, sizeof(HEATER_BEGIN));}


if (K_LINE.available()){
    

 // первый старт байт
 if (header == 0){buf[0]=K_LINE.read();  
         if (buf[0]==0x4F || buf[0]==0x44){header = 1; RESETheader_timer=1; prevRESETheader = currmillis; }
         else {header = 0; RESETheader_timer=0;}
         }                  

 // длина сообщения
 else if (header == 1 ){buf[1]=K_LINE.read(); message_size = buf[1]; if (message_size > bufsize) message_size = bufsize;  header = 4;j=2;n=2;checksum = 0;} 

 // пишем тело сообщения 
 else if (header == 4 && j< message_size+n) {
 buf[j] = K_LINE.read();
 
 if (j<message_size+n-1) checksum^= buf[j]; // подсчёт КС
 
 if (j==message_size+n-1) header = 5; 
  j++;} 

 } // end of K_LINE.available()

 // сообщение приняли, действуем
 if (header == 5) {  
   
for(byte i = 0; i<n; i++) checksum^=buf[i]; // прибавляем к контрольной сумме старт байты

 // если контрольная сумма верна: 
if ( checksum == buf[message_size+n-1]) {

prevInitreset = currmillis;  // нулим таймер сброса инита
noData = 0;
   if (buf[0]==0x44 && buf[2]==0xC4 && buf[3]==0x00) {w_bus_init = 1; delay (500); K_LINE.flush();}  // если получили от котла сбой инита, делаем переинит 
   if (buf[2]==0xD1 && buf[3]==0x0A)  {w_bus_init = 10; if (WBUS_VER != buf[4]) WBUS_VER = buf[4], EEPROM.write(WBUS_VER_cell, WBUS_VER);}  //если получили ответ на старт коммуникации 
   if ((buf[2]^0x80)==HEATER_START[0] && buf[3]==HEATER_START[1])  {StartMessageRepeat =4;} // ответ на команду запуск котла 
   if (buf[2]==0x90)  {StopMessageRepeat=4;} // если получили ответ на команду стоп котла    
   if (buf[2]==0xD6 && buf[3]==0x03)  {DTCareCleared();}  // если получили ответ на команду стирания ошибок                 
   

 
  
     if (buf[2]==0xD0 && buf[3]==0x05)         // если получили сообщение с текущими данными 50 05
     { 
                  
            HeaterC = corTemp (buf[4]);                  // проверяем t котла (W-BUS 1.5)
            Vpit    =  (float)buf[5]/14.64; 
            if (needAction>0 && needAction<NEED_DTCCLEAR) {w_bus_init = 12; sendMessage (HEATER_STATUS_VEVO03, sizeof(HEATER_STATUS_VEVO03));}
     }     
          
     else   if (buf[2]==0xD0 && buf[3]==0x03)         // если получили сообщение с текущими данными 50 03
     { 
        waterpump       = (buf[4] & 0x08)>>3;     // флаг помпы
        airfan          = (buf[4] & 0x10)>>4;     // флаг нагнетателя воздуха
        plug            = (buf[4] & 0x20)>>5;     // флаг штифта накала
        fuelpump        = (buf[4] & 0x40)>>6;     // флаг топл насоса
        startWebasto_OK = (buf[4] & 0x80)>>7; last_Flame = currmillis;  //флаг пламени
        
         if (needAction>0 && needAction<NEED_DTCCLEAR) {w_bus_init = 13; countDTC = 1; DTC_REQUEST_wbus15[2] = countDTC;  sendMessage (DTC_REQUEST_wbus15, sizeof(DTC_REQUEST_wbus15));}
     } 
     
    
     else   if (buf[2]==0xD0 && buf[3]==0x07)         // если получили сообщение с текущими данными 50 07
     {
             if (buf[4]!= 0x14 && buf[4]!= 0x15 && buf[4]!= 0x34 && buf[4]!= 0x35 && buf[4]!= 0x15 && buf[4]!= 0x36 && buf[4]!= 0x37)  PowerHeater = 0; 
        else if (buf[4]== 0x14) PowerHeater = 50;
        else if (buf[4]== 0x15) PowerHeater = 100;
         
     } 
  
      else  if (buf[2]==0xD0 && buf[3]==0x01)         // если получили сообщение с DTC
        {
            DTC[0] = 0;                                                              // обнулим количество ошибок
           for (byte h = 1; h< 5; h++) DTC[(countDTC-1)*4+h] = buf[h+3];             // запомним все параметры (байты) одной ошибки
            
      for (byte h = 0; h< 3; h++) if (DTC[h*4+1] > 0 && DTC[h*4+1]!= 0xFF) DTC[0]++; // посчитаем количество ошибок
        
        if (needAction>0 && needAction<NEED_DTCCLEAR && w_bus_init<16){ w_bus_init = countDTC+13;
        DTC_REQUEST_wbus15[2] = countDTC+1; sendMessage (DTC_REQUEST_wbus15, sizeof(DTC_REQUEST_wbus15));} 
       }
  
}   // конец контрольная сумма верна:  


// если контрольная сумма не совпала: 

//else K_LINE.println("CRC fail!!!" );
  
message_size = 0; header=0; RESETheader_timer=0; j=2; checksum = 0;
}




}// end WBUS 
#endif

#ifdef TTE_C_heaters
if (HeaterBUSTYPE == TTC_E)
{

if (currmillis-Prev_PeriodW_BusMessage>1000 && w_bus_init >= 10) 
{
    Prev_PeriodW_BusMessage = currmillis;

 if (webasto)
 {   
    if (StartMessageRepeat<4 && ProtocolSTART==STARTBUS)
    {
      sendMessage (START_TTC, sizeof(START_TTC)); 
      StartMessageRepeat++; 
    }
    
    else if (StartMessageRepeat>=4)
    { 
         if (requiredmessage==1 && ProtocolSTART==STARTBUS) sendMessage (START_PRESENCE, sizeof(START_PRESENCE)); 
        if (ProtocolSTATUS==STATUSBUS){ 
         
             if (requiredmessage==2) sendMessage (REQUEST_2A0101, sizeof(REQUEST_2A0101));
             if (requiredmessage==3) sendMessage (REQUEST_2A0102, sizeof(REQUEST_2A0102));  
             if (requiredmessage==4) sendMessage (REQUEST_DTC,    sizeof(REQUEST_DTC)), countDTC = 0;
                                      }
            requiredmessage++; if (requiredmessage > 4) requiredmessage = 1;
            StopMessageRepeat = 0;
    }

 }
else if (StopMessageRepeat<4 && ProtocolSTART==STARTBUS)
  {
   sendMessage (STOP_TTC, sizeof(STOP_TTC));
   StopMessageRepeat++; 
   StartMessageRepeat = 0;
  }

}
 
     if (w_bus_init==1) {K_LINE.end(); pinMode (TX, OUTPUT); digitalWrite(TX, 0); timerInit = millis(); w_bus_init++;}
else if (w_bus_init==2 && millis() - timerInit>299)  {timerInit = millis(); digitalWrite(TX, 1); w_bus_init++; }
else if (w_bus_init==3 && millis() - timerInit>49)   {timerInit = millis(); digitalWrite(TX, 0); w_bus_init++; }
else if (w_bus_init==4 && millis() - timerInit>24)   {timerInit = millis(); digitalWrite(TX, 1); w_bus_init++; }
else if (w_bus_init==5 && millis() - timerInit>3024) {K_LINE.begin (10400); w_bus_init=9;  sendMessage (START_SESSION, sizeof(START_SESSION));}

if (K_LINE.available()){
    

 // первый старт байт
 if (header == 0){buf[0]=K_LINE.read();  
         if (!bitRead (buf[0],6) && bitRead (buf[0],7)){header = 1; RESETheader_timer=1; prevRESETheader = currmillis; }
         
         }                  

 // второй старт байт
 else if (header == 1){buf[1]=K_LINE.read(); if (buf[1]==0xF1){ header = 2;} else {header = 0; RESETheader_timer=0;}} 

 // третий старт байт
 else if (header == 2){ 
  buf[2]=K_LINE.read(); 
  if (buf[2]==0x51){ message_size = buf[0]; 
  if (buf[0] !=0x80) {header = 4;  message_size&=~0x80; j=3; n=3;}
  else {header = 3; j=4;n=4;}
  if (message_size > bufsize) message_size = bufsize;  checksum = 0;} else {header = 0; RESETheader_timer=0; }
  
                          }  
// если размер сообщения указан в дополнительном байте (нулевой байт 0x80) читаем этот дополнительный байт:
else if (header == 3){
  buf[3]=K_LINE.read(); 
  message_size = buf[3]; 
  if (message_size > bufsize) message_size = bufsize;  
  checksum = 0; header = 4;  
                         }

  // пишем тело сообщения 
 else if (header == 4 && j< message_size+n+1) {
 buf[j] = K_LINE.read(); 
 if (j<message_size+n) checksum+= buf[j]; // подсчёт КС
 
 if (j==message_size+n) header = 5; 
  j++;} 

 } // end of K_LINE.available()

 // сообщение приняли, действуем
 if (header == 5) {  

for(byte i = 0; i<n; i++) checksum+=buf[i]; // прибавляем к контрольной сумме старт байты

//for (byte i=0; i<message_size+n+1; i++) {Serial.print (buf[i], HEX); Serial.print(" ");}

 // если контрольная сумма верна: 
if (buf[message_size+n] == checksum) {

prevInitreset = currmillis;  // нулим таймер сброса инита
  noData = 0;
   
  if (buf[n]== 0xC1) {if (needAction != NEED_DTCCLEAR) w_bus_init = 10; else w_bus_init = 15; waiting_NextMes(); }//Serial.println ("StartSession OK!!!");
  if (buf[n]== 0x73 && buf[n+1]==0x22 && buf[n+2]== 0xFF) StartMessageRepeat =4;  // если получили ответ на команду старт
  if (buf[n]== 0x73 && buf[n+1]==0x22 && buf[n+2]== 0x00) StopMessageRepeat  =4;  // если получили ответ на команду стоп

  
  if (buf[n]== 0x6A && buf[n+1]== 0x01) 
     {
      
     HeaterC = corTemp (buf[n+2]); Vpit = (float)buf[n+4]/14.64; 
     if  (needAction>0 && needAction<NEED_DTCCLEAR) {w_bus_init = 12; waiting_NextMes();} 
     }
  if (buf[n]== 0x6A && buf[n+1]== 0x02) {
               // nagnetatel = buf[n+2]*100/255 ;
                  plug = buf[n+3];      
                  if (buf[n+5] != 0)fuelpump = 100.0/(float)buf[n+5]; else fuelpump = 0;
                  airfan = (bool)buf[n+6];     
                  plug       = (buf[n+6] & 0x02)>>1;         
                  waterpump  = (buf[n+6] & 0x04)>>2;    
                  fuelpump   = (buf[n+6] & 0x08)>>3;        
                  blowerfan  = (buf[n+6] & 0x10)>>4;        
              //  StartCommand = bitRead (buf[n+7], 2);     
                  startWebasto_OK = (buf[n+7] & 0x20)>>5; last_Flame = currmillis;                      
                                       
                                        
         if (needAction>0 && needAction<NEED_DTCCLEAR) {w_bus_init = 13; waiting_NextMes(); }                               
                                        }
  if (buf[n]== 0xE1 && buf[n+1]== 0xFF && buf[n+2]== 0xFF) // если получено сообщение с количеством ошибок 
  {
    DTC[0] = buf[n+3]; countDTC = 0;
    if (needAction>0 && needAction<NEED_DTCCLEAR) w_bus_init = 14;
  }     

  if (buf[n]== 0xE1 && buf[n+1]!= 0xFF) // если получено сообщение с кодом неисправности
  {
    DTC[countDTC*4+1] = buf[n+1];
    DTC[countDTC*4+2] = buf[n+2];
    DTC[countDTC*4+3] = buf[n+4];
    DTC[countDTC*4+4] = buf[n+5];
    countDTC++;
  }

  if (buf[n]== 0x54) DTCareCleared();
  
  }   

// если контрольная сумма не совпала: 
//else Serial.println("CRC fail!!!" );
message_size = 0; header=0; RESETheader_timer=0; j=3; checksum = 0;
}

} // end TTC_E
#endif


// таймер сброса заголовка если данные оборвались по середине сообщения
if (RESETheader_timer && currmillis - prevRESETheader > 500) {RESETheader_timer = 0; header = 0; if (HeaterBUSTYPE == TTC_E)j=3; else if (HeaterBUSTYPE == WBUS)j=2; }   

if (currmillis - last_Flame>30000 && ProtocolSTATUS==STATUSBUS) {startWebasto_OK=0; last_Flame = currmillis;}  // делаем статус "нет пламени" через 30 сек, если не получаем сообщения от котла

if (Initreset && currmillis - prevInitreset>10000) 
            {Initreset = 0; w_bus_init = 0; noData = 1 ; waterpump = 0; airfan =0; plug = 0; fuelpump = 0;}  // сброс инита, если прошло более 18 сек после отправки последнего сообщения


if (w_bus_init==10 && HeaterBUSTYPE == WBUS)
{
    if (needAction>0 && needAction < NEED_DTCCLEAR) 
   {
          sendMessage (HEATER_STATUS_VEVO, sizeof(HEATER_STATUS_VEVO));
     w_bus_init=11;
   }

    if (needAction==NEED_DTCCLEAR) { sendMessage (HEATER_DTC_ERASE, sizeof(HEATER_DTC_ERASE)); w_bus_init = 20;  }
}

if ((NeedTimer && currmillis - prevNeedTimer>11000) || w_bus_init == 14)  {NeedTimer = 0; 
            
             if (needAction == NEED_SMSZAPROS)   SMSzapros(KTOzapros);
        else if (needAction == NEED_SERVICEINFO) ServiceINFO(KTOzapros);
        else if (needAction == NEED_MQTTZAPROS)  
        {
          cipsend_begin (); 
          WaitData(0);  
          SIM800.write (0x1A); digitalWrite (DTR, HIGH);
        }
        else if (needAction == NEED_DTCCLEAR) 
               { 
                  if (KTOzapros==5) {PUBsend(PGM_Text(_STAT_DTS_TOP),  Waitfail (0) ,  0);}
                else {startSMS(KTOzapros); SIM800.println (F("DTC not cleared. Heater no answer!")); EndSMS();}
               }
        if (HeaterBUSTYPE == WBUS)w_bus_init = 9; 
        if (HeaterBUSTYPE == TTC_E)w_bus_init = 10; 
           needAction = 0; 
        } 
        
 

}



void sendMessage(const byte *command, const size_t size)
{
 Initreset = 1;  prevInitreset = currmillis;  // включение таймера сброса инита
  const byte siZe = size+3;
  byte Mes[siZe];
  byte Checksum = 0;
  for(byte i=0; i<siZe; i++) {
    if (i==0) Mes[i] = 0xF4;
    if (i==1) Mes[i]=size+1; 
    if (i==2) {for (byte t=0; t<size; t++ ) {Mes[i]=command[t]; Checksum^=Mes[i] ;K_LINE.write (Mes[i]); K_LINE.read(); i++;}}
    if (i!=siZe-1) Checksum^=Mes[i];
    else Mes[i] = Checksum;    
    K_LINE.write (Mes[i]); K_LINE.read();
      }
  
  
  }


int8_t corTemp (const byte &byte_T)
{
  int8_t temperature=-120;
if (byte_T >= 242) temperature = -40;
else if (byte_T >= 229 && byte_T <= 241) temperature = map (byte_T, 241,229,-35,-12); 
else if (byte_T == 228) temperature = -11;
else if (byte_T == 227) temperature = -10;
else if (byte_T >= 217 && byte_T <= 226) temperature = map (byte_T, 226,217,-8,1); 
else if (byte_T >= 211 && byte_T <= 216) temperature = map (byte_T, 216,211,1,5); 
else if (byte_T >= 168 && byte_T <= 210) temperature = map (byte_T, 210,168,6,30); 
else if (byte_T >= 124 && byte_T <= 167) temperature = map (byte_T, 167,124,30,50); 
else if (byte_T >= 83  && byte_T <= 123) temperature = map (byte_T, 123,83,50,70); 
else if (byte_T >= 67  && byte_T <= 82)  temperature = map (byte_T, 82,67,71,80); 
else if (byte_T >= 53  && byte_T <= 66)  temperature = map (byte_T, 66,53,81,90); 
else if (byte_T >= 42  && byte_T <= 52)  temperature = map (byte_T, 52,42,91,100); 
else if (byte_T >= 34  && byte_T <= 41)  temperature = map (byte_T, 41,34,101,110); 
else if (byte_T >= 27  && byte_T <= 33)  temperature = map (byte_T, 33,27,111,120); 
else if (byte_T >= 20  && byte_T <= 26)  temperature = map (byte_T, 26,20,122,133); 
else if (byte_T <= 19 ) temperature = 135;
return temperature;
    }


   