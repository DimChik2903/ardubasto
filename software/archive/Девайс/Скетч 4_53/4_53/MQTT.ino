void cipsend_begin () 
{
  digitalWrite (DTR, LOW); delay (150);
  SIM800.println (F("AT+CIPSEND"));
  delay (250);
}

char *Bool_to_text(const bool &var)
{
   static char boolya[5]={0};
   if (var) boolya[0]='1'; else boolya[0]='0';  
   boolya[1]='&';
   boolya[2]='$';
   boolya[3]='\0';
   
   return boolya;
}

char *Var_to_text(const int &var, const bool &enDstr) 
{
      sprintf (buffVAR, "%d",   var);
  if (enDstr) strcat (buffVAR, "&$");
   return buffVAR;
}

char *Float_to_text(const float &var) 
{
  dtostrf(var, 6, 2, buffVAR);
  return buffVAR;
}

char *PGM_Text (const byte &numba) 
{
  strcpy_P(buffeR,(char*)pgm_read_word(&(PGMtable[numba])));
  return buffeR;
}

char *PGM_TextVAR (const byte &numba) 
{
  strcpy_P(buffVAR,(char*)pgm_read_word(&(PGMtable[numba])));
  return buffVAR;
}

const char *Waitfail(const bool &wAit){if (wAit)return PGM_TextVAR(_WAIT); else return PGM_TextVAR(_FAIL); }


void WaitData (const bool &waiT)
{
  bool VARprint = 0; 
  bool wORf = waiT;
  
  
if (waiT && !NeedTimer) {wORf = 0; if (TimerVklData) wORf = 1;}
if (!noData) VARprint = 1;
  
  PUBsend (PGM_Text(_tHEAT_TOP),      VARprint ? Var_to_text(HeaterC,0)     : Waitfail(wORf), 1);
 // PUBsend (PGM_Text(_STAT_NAPR_TOP),  VARprint ? Float_to_text(Vpit)        : Waitfail(wORf), 1);
       if (ProtocolSTATUS==STATUSBUS) {PUBsend (PGM_Text(_STAT_NAPR_TOP),  VARprint ? Float_to_text(Vpit)        : Waitfail(wORf), 1);}
  else if (ProtocolSTATUS==ANALOG){PUBsend (PGM_Text(_STAT_NAPR_TOP), Float_to_text(Vpit),1);}
 
  
  PUBsend (PGM_Text(_STAT_POWER_TOP), VARprint ? Var_to_text(PowerHeater,0) : Waitfail(wORf), 1);
  PUBsend (PGM_Text(_STAT_DTS_TOP),   VARprint ? Var_to_text(DTC[0],0)      : Waitfail(wORf), 1); //(waiT ? WAIT : FAIL),  1);   
  PUBsend (PGM_Text(_STAT_WPUMP_TOP),  VARprint ? Bool_to_text(waterpump) : Bool_to_text(0),1);
  PUBsend (PGM_Text(_STAT_PLUG_TOP),   VARprint ? Bool_to_text(plug     ) : Bool_to_text(0),1);
  PUBsend (PGM_Text(_STAT_FAN_TOP),    VARprint ? Bool_to_text(airfan   ) : Bool_to_text(0),1);
  PUBsend (PGM_Text(_STAT_FPUMP_TOP),  VARprint ? Bool_to_text(fuelpump ) : Bool_to_text(0),1);
}




void remTime   ( bool s) {
  int rem = TimeWebasto - ((currmillis - prevWorkCycleHeater) / 60000ul);
  
  PUBsend (PGM_Text(_STAT_REMTIME_TOP),    webasto ? Var_to_text(rem,0) : PGM_TextVAR(_VYKL), s);
}
void engTime   () {
  int rem = 0 ; 
  if (EngWorkTimer) {rem = TimeEngine - ((currmillis - prevWorkCycleEngine) / 60000UL);}
  
  PUBsend (PGM_Text(_STAT_ENGTIME_TOP),    engine ? Var_to_text(rem,0) : PGM_TextVAR(_VYKL), 1);
}




void AUTHsend () 
{
 cipsend_begin ();
 uint16_t timeBroker       = TIMEBROKER;   
  
  SIM800.write (0x10);
  SIM800.write (strlen(PROTOCOLIUS) + strlen(MQTTNAME) + strlen(MQTTUSER) +  strlen(MQTTPASSWORD) + 12);
  SIM800.write ((byte)0);
  SIM800.write (strlen(PROTOCOLIUS));
  SIM800.write (PROTOCOLIUS);
  SIM800.write (0x03);
  SIM800.write (0xC2);
  
  timeBroker *=60;
  byte * timeBr = (byte*)&timeBroker;
  SIM800.write(*(timeBr+=1)); // время сеанса связи с брокером, сек (ст. байт)
  SIM800.write(*(--timeBr));  // время сеанса связи с брокером, сек (мл. байт)
  SIM800.write ((byte)0);
  SIM800.write (strlen(MQTTNAME));
  SIM800.write (MQTTNAME);
  SIM800.write ((byte)0);
  SIM800.write (strlen(MQTTUSER));
  SIM800.write (MQTTUSER);
  SIM800.write ((byte)0);
  SIM800.write (strlen(MQTTPASSWORD));
  SIM800.write (MQTTPASSWORD);
  SIM800.write (0x1A);
}

void PUBsend  (const char* topPub, const char *Command, const bool &multidata) {

  if (!multidata) {cipsend_begin ();}
  SIM800.write (0x30);
  SIM800.write (strlen (topPub) + strlen(Command) + 2);
  SIM800.write ((byte)0x00);
  SIM800.write (strlen (topPub));
  SIM800.write (topPub);
  SIM800.write (Command);
  if (!multidata) {SIM800.write(0x1A);  digitalWrite (DTR, HIGH);}
}




void SUBsend (const char *topSub) {
  cipsend_begin ();
  SIM800.write (0x82);
  SIM800.write (strlen(topSub) + 5);
  SIM800.write ((byte)0);
  SIM800.write (0x01);
  SIM800.write ((byte)0);
  SIM800.write (strlen(topSub));
  SIM800.write (topSub);
  SIM800.write ((byte)0x00);
  SIM800.write (0x1A);
}

void  MQTTsendDatastream() 
{
  cipsend_begin ();
  PUBsend (PGM_Text(_STAT_TIME_TOP),  Var_to_text(TimeWebasto,0),     1);
  remTime(true);
  PUBsend (PGM_Text(_STAT_HEAT_TOP),  Bool_to_text(webasto),    1);
  PUBsend (PGM_Text(_STAT_FLAME_TOP), Bool_to_text(startWebasto_OK), 1);
  WaitData (1);
  PUBsend (PGM_Text(_tDVS_TOP),     Var_to_text(DS18B20[EngineC].Temper, 0),   1);
  PUBsend (PGM_Text(_tUL_TOP),      Var_to_text(DS18B20[UlicaC].Temper, 0),   1);
  PUBsend (PGM_Text(_tSAL_TOP),     Var_to_text(DS18B20[SalonC].Temper, 0),   1);
  PUBsend (PGM_Text(_tVYH_TOP),     Var_to_text(DS18B20[VyhlopC].Temper,0),   1);
  PUBsend (PGM_Text(_STAT_RESETS_TOP), Var_to_text(ResetNumber, 0),   1);
  PUBsend (PGM_Text(_STAT_LEVEL_TOP),  Var_to_text(signalLevel, 0),   1);
      #ifdef  Signalka
   PUBsend (PGM_Text(_STAT_ENGINE_TOP),Bool_to_text(engine),   1);
   PUBsend (PGM_Text(_STAT_IGN_TOP),   Bool_to_text(ignition), 1);
   PUBsend (PGM_Text(_STAT_OHR_TOP),   Bool_to_text(ohrana),   1);
   PUBsend (PGM_Text(_STAT_TREV_TOP),  Bool_to_text(trevoga),  1);
   engTime();
      #endif
  SIM800.write (0x1A);
  prev_refreshMQTT = currmillis;
  digitalWrite (DTR, HIGH);
 }
