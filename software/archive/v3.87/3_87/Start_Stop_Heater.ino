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
     StartMessageRepeat = 0;
     webasto = 1; digitalWrite (OutWebasto_12V, HIGH);
     prevWorkCycleHeater=currmillis;
     WorkCycleHeater_timer = true;
     if (!CentrLock) {digitalWrite (PUMPE, 1);} // включаем помпу
  }
report = true; prevReport = currmillis;
w_bus_init = NEED;

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

