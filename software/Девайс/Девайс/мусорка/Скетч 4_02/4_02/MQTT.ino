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

//SIM800.write (0x04);    // время сеанса связи с брокером, сек (ст. байт)
//SIM800.write (0xB0);    // время сеанса связи с брокером, сек (мл. байт)

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

  byte len = strlen (topPub) + strlen (MQTTUSER);
  
  if (!multidata) {cipsend_begin ();}
  SIM800.write (0x30);
  SIM800.write (len + strlen (Command) + 3 );
  SIM800.write ((byte)0x00);
  SIM800.write (len+1);
  SIM800.write (MQTTUSER);
  SIM800.print (F("/"));
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
  PUBsend (STAT_TIME_TOP,  Var_to_text(TimeWebasto,0),     1);
  remTime(true);
  PUBsend (STAT_HEAT_TOP,  Var_to_text(webasto,ENDSTR),    1);
  PUBsend (STAT_FLAME_TOP, Var_to_text(startWebasto_OK,0), 1);
  WaitData (1);
  PUBsend (tDVS_TOP,     Var_to_text(Temper(EngineC),0),   1);
  PUBsend (tUL_TOP,      Var_to_text(Temper(UlicaC), 0),   1);
  PUBsend (tSAL_TOP,     Var_to_text(Temper(SalonC), 0),   1);
  PUBsend (tVYH_TOP,     Var_to_text(Temper(VyhlopC),0),   1);
  PUBsend (STAT_RESETS_TOP, Var_to_text(ResetNumber, 0),   1);
  PUBsend (STAT_LEVEL_TOP,  Var_to_text(signalLevel, 0),   1);
      #ifdef  Signalka
   PUBsend (STAT_ENGINE_TOP,Var_to_text(engine, ENDSTR),   1);
   PUBsend (STAT_IGN_TOP,   Var_to_text(ignition,0),       1);
   PUBsend (STAT_OHR_TOP,   Var_to_text(ohrana, ENDSTR),   1);
   PUBsend (STAT_TREV_TOP,  Var_to_text(trevoga, 0),       1);
   engTime();
      #endif
  SIM800.write (0x1A);
  prev_refreshMQTT = currmillis;
  digitalWrite (DTR, HIGH);
 }



char *Var_to_text(const int &var, const bool &enDstr) 
{
      sprintf (buffeR, "%d",   var);
  if (enDstr) strcat (buffeR, "&$");
   return buffeR;
}

char *Float_to_text(const float &var) 
{
  
  float cel, drob;
  drob = modff(var, &cel);
  int Cel = (int)var;
  int Drob = abs(drob*100);
  if (drob<0)  sprintf (buffeR, "-%u.%u", Cel, Drob); 
  else         sprintf (buffeR, "%u.%u",  Cel, Drob); 
  return buffeR;
}

char *PGM_Text (const byte &numba) 
{
  strcpy_P(buffeR,(char*)pgm_read_word(&(PGMtable[numba])));
  return buffeR;
}



void WaitData (const bool &waiT)
{
  bool StrinG = 0; 
  if (waiT && !NeedTimer) StrinG = 1;
  if (!waiT && !noData)   StrinG = 1;
  PUBsend (tHEAT_TOP,      StrinG ? Var_to_text(HeaterC,0)     : Waitfail(waiT), 1);
  PUBsend (STAT_NAPR_TOP,  StrinG ? Float_to_text(Vpit)        : Waitfail(waiT), 1);
  PUBsend (STAT_POWER_TOP, StrinG ? Var_to_text(PowerHeater,0) : Waitfail(waiT), 1);
  PUBsend (STAT_DTS_TOP,   StrinG ? Var_to_text(DTC[0],0)      : Waitfail(waiT), 1); //(waiT ? WAIT : FAIL),  1);   
}

const char *Waitfail(const bool &wAit){if (wAit)return WAIT; else return FAIL; }

void remTime   ( bool s) {
  int rem = TimeWebasto - ((currmillis - prevWorkCycleHeater) / 60000ul);
  PUBsend (STAT_REMTIME_TOP,    webasto ? Var_to_text(rem,0) : VYKL, s);
}
void engTime   () {
  
  int rem = TimeEngine - ((currmillis - prevWorkCycleEngine) / 60000ul);
  PUBsend (STAT_ENGTIME_TOP,    engine ? Var_to_text(rem,0) : VYKL, 1);
}

void cipsend_begin () 
{
  digitalWrite (DTR, LOW); delay (150);
  SIM800.println (F("AT+CIPSEND"));
  delay (250);
}

