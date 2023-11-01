void StartWebasto()
{
 
     if (ProtocolSTART==STARTBUS) {StartMessageRepeat = 0;  }  else StartMessageRepeat = 4;
     webasto = 1; 
     if (ProtocolSTATUS==STATUSBUS) {TimerVklData = 1; timerVklData = currmillis;}  
     prevWorkCycleHeater=currmillis;
     WorkCycleHeater_timer = true;
     if (!CentrLock) {digitalWrite (PUMPE, 1);} // включаем помпу
  
report = true; prevReport = currmillis;
w_bus_init = 1;

}

void StopWebasto()
{
 
      StopMessageRepeat = 0;
      webasto = 0; 
      WorkCycleHeater_timer = false;
      if (!CentrLock) {Pumpe_timer=1; prevPumpe = currmillis;} // включаем таймер, по окончании которого помпа выключится
   
   report = false;
   
   if (ProtocolSTATUS==STATUSBUS) startWebasto_OK = 0;
}


void StartStop_Engine()
{
  digitalWrite (StartEng, HIGH); prevStartEng=currmillis; StartEng_timer=true; prevWorkCycleEngine = currmillis;
}





