void StartWebasto()
{
 if (ProtocolSTART==IMPULSE)
  {
     digitalWrite (OutWebasto_GndImp, LOW); 
     GND_impulse_timer = 1;
     prevGND_impulse = currmillis;
  }

 else 
  {
     if (ProtocolSTART==STARTBUS) {StartMessageRepeat = 0;  }  else StartMessageRepeat = 4;
   // if (ProtocolSTATUS==STATUSBUS) {Queue(NEED_MQTTZAPROS);}  
     webasto = 1; digitalWrite (OutWebasto_12V, HIGH);
     prevWorkCycleHeater=currmillis;
     WorkCycleHeater_timer = true;
     if (!CentrLock) {digitalWrite (PUMPE, 1);} // включаем помпу
  }
report = true; prevReport = currmillis;
w_bus_init = 1;

}

void StopWebasto()
{
 if (ProtocolSTART==IMPULSE)
   { 
      digitalWrite (OutWebasto_GndImp, LOW); 
      GND_impulse_timer = 1;
      prevGND_impulse = currmillis;
   }
 else 
   {
      StopMessageRepeat = 0;
      webasto = 0; digitalWrite (OutWebasto_12V, LOW);
      WorkCycleHeater_timer = false;
      if (!CentrLock) {Pumpe_timer=1; prevPumpe = currmillis;} // включаем таймер, по окончании которого помпа выключится
   }
   report = false;
   
   if (ProtocolSTATUS==STATUSBUS) startWebasto_OK = 0;
}


void StartStop_Engine()
{
  digitalWrite (StartEng, HIGH); prevStartEng=currmillis; StartEng_timer=true; prevWorkCycleEngine = currmillis;
}





