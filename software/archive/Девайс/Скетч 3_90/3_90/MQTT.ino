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

void PUBsend  (char topPub[30], const String Command, const bool &multidata) {

  if (!multidata) {cipsend_begin ();}
  SIM800.write (0x30);
  SIM800.write (strlen (topPub) + Command.length() + 2);
  SIM800.write ((byte)0x00);
  SIM800.write (strlen (topPub));
  SIM800.write (topPub);
  for (byte i = 0; i < Command.length(); i++) SIM800.write (Command[i]);
  if (!multidata) {SIM800.write(0x1A);  digitalWrite (DTR, HIGH);}
}

void SUBsend (char topSub[20]) {
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
  PUBsend (R_TIME_TOP,  String(TimeWebasto), 1);
  remTime(true);
  PUBsend (R_HEAT_TOP,       String(webasto) + ENDSTR,  1);
  PUBsend (R_FLAME_TOP,      String(startWebasto_OK), 1);
      
  PUBsend (R_POWER_TOP,     NeedTimer ? WAIT : String(PowerHeater),1);
  PUBsend (tHEAT_TOP, NeedTimer ? WAIT : String(HeaterC),1);
 
  PUBsend (R_NAPR_TOP,      NeedTimer ? WAIT : String(Vpit), 1);

  PUBsend (tDVS_TOP,   String(Temper(EngineC)), 1);
  PUBsend (tUL_TOP,    String(Temper(UlicaC)), 1);
  PUBsend (tSAL_TOP,   String(Temper(SalonC)), 1);
  PUBsend (tVYH_TOP,   String(Temper(VyhlopC)), 1);
  PUBsend (R_RESETS_TOP,     String(ResetNumber), 1);
  PUBsend (R_DTS_TOP,       String(DTC[0]), 1);
  PUBsend (R_LEVEL_TOP,   String(signalLevel), 1);
  SIM800.write (0x1A);
  prev_refreshMQTT = currmillis;
  digitalWrite (DTR, HIGH);
 }

void remTime   ( bool s) {
  int rem = TimeWebasto - ((currmillis - prevWorkCycleHeater) / 60000ul);
  PUBsend (R_REMTIME_TOP,    webasto ? String(rem) : "выкл", s);
}

void cipsend_begin () 
{
  digitalWrite (DTR, LOW); delay (150);
  SIM800.println (F("AT+CIPSEND"));
  delay (250);
}

