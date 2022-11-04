/********************************************************************\
Frontend program for reading out WEEROC CITIROC1A.

We make calls to FD2XX and LALUsb routines, 
so this frontend requires such libraries to be installed.
ODB will be accessed using the new odbxx objects.

Based on the frontend for the 
CAEN DT5743 digitizer developed by T. Lindner.

L. Koerich, Nov 2022
\********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "odbxx.h"

#include "midas.h"
#include "mfe.h"
#include "unistd.h"
#include "time.h"
#include "sys/time.h"
#include "CITIROC.h"

#include "OdbDT5743.h"

// CAEN includes
#include <CAENDigitizer.h>

#define  EQ_NAME   "V1743"
#define  EQ_EVID   1
#define  EQ_TRGMSK 0x1111

#define CAEN_USE_DIGITIZERS

/* Globals */
#define N_DT5743 1

/* Hardware */
extern HNDLE hDB;
extern BOOL debug;

HNDLE hSet[N_DT5743];
DT5743_CONFIG_SETTINGS tsvc[N_DT5743];
//const char BankName[N_DT5743][5]={"D743"};
const char BankName[N_DT5743][5]={"43FS"};
const char BankNameSlow[N_DT5743][5]={"43SL"};

// VMEIO definition

/* make frontend functions callable from the C framework */

/*-- Globals -------------------------------------------------------*/

/* The frontend name (client name) as seen by other MIDAS clients   */
const char *frontend_name = "fecitiroc";
/* The frontend file name, don't change it */
const char *frontend_file_name = (char*)__FILE__;

/* frontend_loop is called periodically if this variable is TRUE    */
BOOL frontend_call_loop = FALSE;

/* a frontend status page is displayed with this frequency in ms */
INT display_period = 000;

/* maximum event size produced by this frontend */
INT max_event_size = 32 * 34000;

/* maximum event size for fragmented events (EQ_FRAGMENTED) */
INT max_event_size_frag = 5 * 1024 * 1024;

/* buffer size to hold events */
INT event_buffer_size = 2 * max_event_size + 10000;

/* VME base address */
int   dt5743_handle[N_DT5743];

int  linRun = 0;
int  done=0, stop_req=0;

// handle to CAEN digitizer;
int handle;

//time_t rawtime;
//struct tm *timeinfo;
struct timeval te;


/*-- Function declarations -----------------------------------------*/
INT frontend_init();
INT frontend_exit();
INT begin_of_run(INT run_number, char *error);
INT end_of_run(INT run_number, char *error);
INT pause_run(INT run_number, char *error);
INT resume_run(INT run_number, char *error);
INT frontend_loop();
extern void interrupt_routine(void);
INT read_trigger_event(char *pevent, INT off);
INT read_slow_event(char *pevent, INT off);
INT initialize_slow_control();
INT initialize_daq_parameters();
INT initialize_HV_parameters();

/*-- Equipment list ------------------------------------------------*/
#undef USE_INT
EQUIPMENT equipment[] = {

  { EQ_NAME,                 /* equipment name */
    {
      EQ_EVID, EQ_TRGMSK,     /* event ID, trigger mask */
      "SYSTEM",              /* event buffer */
      EQ_POLLED ,      /* equipment type */
      LAM_SOURCE(0, 0x8111),     /* event source crate 0, all stations */
      "MIDAS",                /* format */
      TRUE,                   /* enabled */
      RO_RUNNING,             /* read only when running */
      500,                    /* poll for 500ms */
      0,                      /* stop run after this event limit */
      0,                      /* number of sub events */
      0,                      /* don't log history */
      "", "", "",
    },
    read_trigger_event,       /* readout routine */
  },
  { "Citiroc1A_Slow",                 /* equipment name */
    {
      EQ_EVID, EQ_TRGMSK,     /* event ID, trigger mask */
      "SYSTEM",              /* event buffer */
      EQ_PERIODIC ,      /* equipment type */
      LAM_SOURCE(0, 0x8111),     /* event source crate 0, all stations */
      "MIDAS",                /* format */
      TRUE,                   /* enabled */
      511,             /* read only when running */
      500,                    /* poll for 500ms */
      0,                      /* stop run after this event limit */
      0,                      /* number of sub events */
      0,                      /* don't log history */
      "", "", "",
    },
    read_slow_event,       /* readout routine */
  },
  {""}
};

/********************************************************************\
            odbxx initializers

  initialize_slow_control: Add slow-control parameters to ODB.

  initialize_daq_parameters: Add daq settings to ODB.

\********************************************************************/

extern INT initialize_slow_control() {

  // Initialize all parameters
  midas::odb database_slow = {
    {"Enable temperature sensor", "00110100"}, 
    {"Temp.-sensor configuration a", "00000011"},
    {"Temp.-sensor configuration b", "00000010"},
  };

  // Add parameters to the slow-control key
  database_slow.connect(odbdir_temp);

  // Catch error
  int ret = database_slow.is_connected_odb();
  if (ret > 0) {
    return ret;
  } else {
    printf("Unable to connect with slow-control ODB. Ret: %d.\n", ret);
  }

}

extern INT initialize_daq_parameters() {

  // Initialize
  midas::odb database_daq = {
    {"DAC 00", 15},
    {"DAC 01", 0},
    {"DAC 02", 0},
    {"DAC 03", 0},
    {"DAC 04", 0},
    {"LALUsb verbosity", true},
    {"FIFO write size", 8192},
    {"FIFO read size", 32768},
    {"Read time out (1-255 ms)", 200},
    {"Write time out (1-255 ms)", 200},
  };

  // Add parameters to ODB
  database_daq.connect(odbdir_DAQ);

  // Catch error
  int ret = database_daq.is_connected_odb();
  if (ret > 0) {
    return ret;
  } else {
    printf("Unable to connect with slow-control ODB. Ret: %d.\n", ret);
  }

}

extern INT initialize_HV_parameters() {

  // Initialize
  midas::odb database_daq = {
    {"DAC 00", 0}, {"DAC 01", 0}, {"DAC 02", 0}, {"DAC 03", 0},
    {"DAC 04", 0}, {"DAC 05", 0}, {"DAC 06", 0}, {"DAC 07", 0},
    {"DAC 08", 0}, {"DAC 09", 0}, {"DAC 10", 0}, {"DAC 11", 0},
    {"DAC 12", 0}, {"DAC 13", 0}, {"DAC 14", 0}, {"DAC 15", 0},
    {"DAC 16", 0}, {"DAC 17", 0}, {"DAC 18", 0}, {"DAC 19", 0},
    {"DAC 20", 0}, {"DAC 21", 0}, {"DAC 22", 0}, {"DAC 23", 0},
    {"DAC 24", 0}, {"DAC 25", 0}, {"DAC 26", 0}, {"DAC 27", 0},
    {"DAC 28", 0}, {"DAC 29", 0}, {"DAC 30", 0}, {"DAC 31", 0},
  };

  // Add parameters to ODB
  database_daq.connect(odbdir_HV);

  // Catch error
  int ret = database_daq.is_connected_odb();
  if (ret > 0) {
    return ret;
  } else {
    printf("Unable to connect with slow-control ODB. Ret: %d.\n", ret);
  }

}

/********************************************************************\
              Callback routines for system transitions

  These routines are called whenever a system transition like start/
  stop of a run occurs. The routines are called on the following
  occations:

  frontend_init:  When the frontend program is started. This routine
                  should initialize the hardware.

  frontend_exit:  When the frontend program is shut down. Can be used
                  to releas any locked resources like memory, commu-
                  nications ports etc.

  begin_of_run:   When a new run is started. Clear scalers, open
                  rungates, etc.

  end_of_run:     Called on a request to stop a run. Can send
                  end-of-run event and close run gates.

  pause_run:      When a run is paused. Should disable trigger events.

  resume_run:     When a run is resumed. Should enable trigger events.
\********************************************************************/

/********************************************************************/

/*-- Sequencer callback info  --------------------------------------*/
void seq_callback(INT hDB, INT hseq, void *info)
{
  KEY key;

  printf("odb ... Settings %x touched\n", hseq);
  for (int b=0;b<N_DT5743;b++) {
    if (hseq == hSet[b]) {
      db_get_key(hDB, hseq, &key);
      printf("odb ... Settings %s touched\n", key.name);
    }
  }
}

INT initialize_for_run();

/*-- Frontend Init -------------------------------------------------*/
INT frontend_init()
{
  int size, status;
  char set_str[80];
  CAEN_DGTZ_BoardInfo_t       BoardInfo;
  bool CITIROC_status;
  
  // Testing odbxx 
  initialize_slow_control();
  initialize_daq_parameters();
  initialize_HV_parameters();

  // Suppress watchdog for PICe for nowma
  cm_set_watchdog_params(FALSE, 0);

  //  setbuf(stdout, NULL);
  //  setbuf(stderr, NULL);
  printf("begin of Init\n");
  /* Book Setting space */
  // DT5743_CONFIG_SETTINGS_STR(dt5743_config_settings_str); <<---- CAEN

  sprintf(set_str, "/Equipment/Citiroc1A/Settings/");
  // status = db_create_record(hDB, 0, set_str, strcomb(dt5743_config_settings_str)); <<---- CAEN
  status = db_find_key (hDB, 0, set_str, &hSet[0]);
  if (status != DB_SUCCESS) cm_msg(MINFO,"FE","Key %s not found", set_str);


  // Open communication and initialize board
  printf("Opening communication...\n");
  CITIROC_status = CITIROC_connectBoard("CT1A_31A", &CITIROC_usbID);

  printf("Closing communication...\n");
  CITIROC_status = CITIROC_disconnetBoard(&CITIROC_usbID);


  /* Enable hot-link on settings/ of the equipment */
  // size = sizeof(DT5743_CONFIG_SETTINGS);
  // if ((status = db_open_record(hDB, hSet[0], &(tsvc[0]), size, MODE_READ, seq_callback, NULL)) != DB_SUCCESS) <<---- CAEN
    // return status;

  // Open connection to digitizer
  // CAEN_DGTZ_ErrorCode ret; <<---- CAEN


  //ret = CAEN_DGTZ_OpenDigitizer(CAEN_DGTZ_USB, 0 /*link num*/, 0, 0 /*base addr*/, &handle);
  //ret = CAEN_DGTZ_OpenDigitizer(CAEN_DGTZ_USB, 0 /*link num*/, 0,  0x81110000 , &handle);
  // ret = CAEN_DGTZ_OpenDigitizer(CAEN_DGTZ_USB, 0, 0,  0 , &handle); <<---- CAEN

  // if(ret){ 
  //   cm_msg(MERROR, "frontend_init", "cannot open digitizer"); <<---- CAEN
  //   return 0;
  // }else{
  //   cm_msg(MINFO, "frontend_init", "successfully opened digitizer");		<<---- CAEN
  // }
  
  
  // ret = CAEN_DGTZ_GetInfo(handle, &BoardInfo);
  // if (ret) { 
  //   cm_msg(MERROR, "frontend_init", "error getting info"); <<---- CAEN
    
  // }
  
  cm_msg(MINFO, "frontend_init", "Connected to CAEN Digitizer Model %s", BoardInfo.ModelName);
  printf("ROC FPGA Release is %s\n", BoardInfo.ROC_FirmwareRel);
  printf("AMC FPGA Release is %s\n", BoardInfo.AMC_FirmwareRel);
  
  // If a run is going, start the digitizer running
  int state = 0; 
  size = sizeof(state); 
  db_get_value(hDB, 0, "/Runinfo/State", &state, &size, TID_INT, FALSE); 
  

  if (state == STATE_RUNNING) 
    initialize_for_run();
  
  //--------------- End of Init cm_msg debug ----------------
  
  set_equipment_status(equipment[0].name, "Initialized", "#00ff00");
  
  //exit(0);
  printf("end of Init\n");
  return SUCCESS;
}

/*-- Frontend Exit -------------------------------------------------*/
INT frontend_exit()
{
  printf("End of exit\n");
  return SUCCESS;
}

char *gBuffer = NULL;

INT initialize_for_run(){
  
  printf("Initializing digitizer for running\n");
  
  int i,j, ret = 0;
  ret |= CAEN_DGTZ_Reset(handle);
  if (ret != 0) {
    printf("Error: Unable to reset digitizer.\nPlease reset digitizer manually then restart the program\n");
    return -1;
  }
  
  int module = 0, status;
  
  // Get ODB settings
  int size = sizeof(DT5743_CONFIG_SETTINGS);
  if ((status = db_get_record (hDB, hSet[module], &tsvc[module], &size, 0)) != DB_SUCCESS)
    return status;
  
  // Set digitizer length; length must be > 64 and length%16 == 0
  if(tsvc[module].record_length % 16 == 0 && tsvc[module].record_length > 64 && 
     tsvc[module].record_length <= 1024  ){    
    ret = CAEN_DGTZ_SetRecordLength(handle, tsvc[module].record_length);
    if(ret != 0) printf("Error setting record length: %i %i\n",ret,tsvc[module].record_length);
  }else{
    cm_msg(MERROR, "initialize", "Record length of %i is invalid.  Must be %16 == 0 and > 64 and <= 1024",tsvc[module].record_length );
  }
  // Set post trigger
  ret = CAEN_DGTZ_SetSAMPostTriggerSize(handle, 0, tsvc[module].post_trigger[0]);
  ret = CAEN_DGTZ_SetSAMPostTriggerSize(handle, 1, tsvc[module].post_trigger[1]);
  ret = CAEN_DGTZ_SetSAMPostTriggerSize(handle, 2, tsvc[module].post_trigger[2]);
  ret = CAEN_DGTZ_SetSAMPostTriggerSize(handle, 3, tsvc[module].post_trigger[3]);
  if(ret != 0) printf("Error setting post trigger: %i %i %i \n",ret,tsvc[module].post_trigger[0],tsvc[module].post_trigger[1]);

  // Set Sampling frequency
  ret = CAEN_DGTZ_SetSAMSamplingFrequency(handle, tsvc[module].frequency);
  if(ret != 0) printf("Error setting frequency: %i\n",ret);
  
  // Other SAMLONG digitizatoin parameters
  ret = CAEN_DGTZ_SetSAMCorrectionLevel(handle, CAEN_DGTZ_SAM_CORRECTION_ALL);
  //ret = CAEN_DGTZ_SetSAMCorrectionLevel(handle, CAEN_DGTZ_SAM_CORRECTION_DISABLED);
  if(ret != 0) printf("Error setting CorLevel: %i\n",ret);
  ret = CAEN_DGTZ_LoadSAMCorrectionData(handle);
  if(ret != 0) printf("Error setting Correction Data: %i\n",ret);
  ret = CAEN_DGTZ_DisableSAMPulseGen(handle,0);
  if(ret != 0) printf("Error setting PulseGen: %i\n",ret);
  ret = CAEN_DGTZ_SetSAMAcquisitionMode(handle, CAEN_DGTZ_AcquisitionMode_STANDARD);
  if(ret != 0) printf("Error setting Acq Mode: %i\n",ret);
  
  // Set the DC offset
  for(int i = 0; i < 8; i++){
    ret = CAEN_DGTZ_SetChannelDCOffset(handle,i,(uint32_t)tsvc[module].dac[i]);
    if(ret != 0 && ret != -17 ) printf("Error setting DAC: %i\n",ret);
  
  }

  // Set Group enable
  ret = CAEN_DGTZ_SetGroupEnableMask(handle, tsvc[module].group_mask);
    if(ret != 0) printf("Error setting group enable: %i\n",ret);
  

  sleep(3);
  
  
  
  printf("Check values on DT5743: \n");
  uint32_t dcoffset;
  ret |= CAEN_DGTZ_GetRecordLength(handle,&dcoffset);
  printf("record length: %x\n",dcoffset);
  //  ret |= CAEN_DGTZ_GetSAMSamplingFrequency(handle,&dcoffset);
  //  printf("sampling frequency: %x\n",dcoffset);
  ret |= CAEN_DGTZ_GetChannelDCOffset(handle,0,&dcoffset);
  printf("Channel0 DC offset: %x\n",dcoffset);
  ret |= CAEN_DGTZ_GetChannelDCOffset(handle,1,&dcoffset);
  printf("Channel1 DC offset: %x\n",dcoffset);
    ret |= CAEN_DGTZ_GetChannelDCOffset(handle,2,&dcoffset);
  printf("Channel0 DC offset: %x\n",dcoffset);
  ret |= CAEN_DGTZ_GetChannelDCOffset(handle,3,&dcoffset);
  printf("Channel1 DC offset: %x\n",dcoffset);
  
  ret |= CAEN_DGTZ_SetAnalogMonOutput(handle,CAEN_DGTZ_AM_TRIGGER_MAJORITY);
  
  // enable the external trigger
  ret |= CAEN_DGTZ_SetAcquisitionMode(handle, CAEN_DGTZ_SW_CONTROLLED);
  ret |= CAEN_DGTZ_SetExtTriggerInputMode(handle, CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT);
  //ret |= CAEN_DGTZ_SetExtTriggerInputMode(handle, CAEN_DGTZ_TRGMODE_DISABLED);
  
  //Enable the trigger on channel
  //ret |= CAEN_DGTZ_SetChannelSelfTrigger(handle,CAEN_DGTZ_TRGMODE_ACQ_ONLY,1);
  
  //ret |= CAEN_DGTZ_SetChannelTriggerThreshold(handle,0,(uint32_t)tsvc[module].threshold[0]);
  //ret |= CAEN_DGTZ_SetChannelTriggerThreshold(handle,1,(uint32_t)tsvc[module].threshold[1]);
  
  //ret |= CAEN_DGTZ_SetTriggerPolarity(handle,0,CAEN_DGTZ_TriggerOnFallingEdge);
  //ret |= CAEN_DGTZ_SetTriggerPolarity(handle,1,CAEN_DGTZ_TriggerOnRisingEdge);
  
  //ret |= CAEN_DGTZ_WriteRegister(handle,0x1084,1);
  
  // Only want to allocate memory once
  static int allocatedBufferMemory = 0;
  
  if(!allocatedBufferMemory){
    
    uint32_t AllocatedSize;
    
    // Allocate buffer memory.  Seems to only need to happen once.
    int ret3 = CAEN_DGTZ_MallocReadoutBuffer(handle, &gBuffer,&AllocatedSize); /* WARNING: This malloc must be done after the digitizer programming */
    if(ret3) {
      printf("Failed to malloc readout buffer\n");
    }
    
    allocatedBufferMemory = 1;
  }
  
  // Use NIM IO for trigger in.
  // CAEN_DGTZ_WriteRegister(handle,0x811c,0x0);
  
  // Use TTL IO for trigger in.
  CAEN_DGTZ_WriteRegister(handle,0x811c,0x1);
  
  // Start the acquisition
  CAEN_DGTZ_SWStartAcquisition(handle);
  
  return ret;
}


/*-- Begin of Run --------------------------------------------------*/
INT begin_of_run(INT run_number, char *error)
{

  // Update values 
  initialize_for_run();

  //------ FINAL ACTIONS before BOR -----------
  printf("End of BOR\n");
  //sprintf(stastr,"GrpEn:0x%x", tsvc[0].group_mask); 
  set_equipment_status("feVeto", "BOR", "#00FF00");                                                                        
  return SUCCESS;
}

/*-- End of Run ----------------------------------------------------*/
INT end_of_run(INT run_number, char *error)
{

  printf("EOR\n");

	// Stop acquisition
	CAEN_DGTZ_SWStopAcquisition(handle);
  
  return SUCCESS;
}

/*-- Pause Run -----------------------------------------------------*/
INT pause_run(INT run_number, char *error)
{
  linRun = 0;
  return SUCCESS;
}

/*-- Resume Run ----------------------------------------------------*/
INT resume_run(INT run_number, char *error)
{
  linRun = 1;
  return SUCCESS;
}

/*-- Frontend Loop -------------------------------------------------*/
INT frontend_loop()
{

  /* if frontend_call_loop is true, this routine gets called when
     the frontend is idle or once between every event */
  char str[128];
  static DWORD evlimit;

  if (stop_req && done==0) {
    db_set_value(hDB,0,"/logger/channels/0/Settings/Event limit", &evlimit, sizeof(evlimit), 1, TID_DWORD); 
    if (cm_transition(TR_STOP, 0, str, sizeof(str), BM_NO_WAIT, FALSE) != CM_SUCCESS) {
      cm_msg(MERROR, "feodeap", "cannot stop run: %s", str);
    }
    linRun = 0;
    done = 1;
    cm_msg(MERROR, "feodeap","feodeap Stop requested");
  }
  return SUCCESS;
}

/*------------------------------------------------------------------*/
/********************************************************************\
  Readout routines for different events
\********************************************************************/
int Nloop, Ncount;

/*-- Trigger event routines ----------------------------------------*/
 INT poll_event(INT source, INT count, BOOL test)
/* Polling routine for events. Returns TRUE if event
   is available. If test equals TRUE, don't return. The test
   flag is used to time the polling */
{
  register int i;  // , mod=-1;
  register int lam = 0;

  for (i = 0; i < count; i++) {
    
    // Read the correct register to check number of events stored on digitizer.
    uint32_t Data;
    CAEN_DGTZ_ReadRegister(handle,0x812c,&Data);
    if(Data > 0) lam = 1;
    
    ss_sleep(1);
    if (lam) {
      Nloop = i; Ncount = count;
      if (!test){
        return lam;
      }
    }
  }
  return 0;
}

/*-- Interrupt configuration ---------------------------------------*/
 INT interrupt_configure(INT cmd, INT source, POINTER_T adr)
{
  switch (cmd) {
  case CMD_INTERRUPT_ENABLE:
    break;
  case CMD_INTERRUPT_DISABLE:
    break;
  case CMD_INTERRUPT_ATTACH:
    break;
  case CMD_INTERRUPT_DETACH:
    break;
  }
  return SUCCESS;
}

/*-- Event readout -------------------------------------------------*/
int vf48_error = 0;
#include <stdint.h>
INT read_trigger_event(char *pevent, INT off)
{
   // Get number of events in buffer
   uint32_t buffsize;
   uint32_t numEvents;    
	

   int ret2 =  CAEN_DGTZ_ReadData(handle, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, gBuffer, &buffsize);
	//int ret2 =  CAEN_DGTZ_ReadData(handle, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_2eVME, gBuffer, &buffsize);
   if(ret2){
      printf("Failed to read data,\n");
   }

   uint32_t * words = (uint32_t*)gBuffer;

   gettimeofday(&te,NULL);
   long long etime = (long long)(te.tv_sec)*1000+(int)te.tv_usec/1000;

   uint32_t etime1, etime2;
   etime1 = ((etime>>32)&0xFFFFFFFF);
   etime2 = ((etime)&0xFFFFFFFF);
   //printf("%u %u\n",etime1,etime2);


   uint32_t *pddata;
   uint32_t nEvtSze;
   uint32_t sn = SERIAL_NUMBER(pevent);

   // Create event header
   bk_init32(pevent);

   bk_create(pevent, BankName[0], TID_DWORD, (void**)&pddata);//cast to void (arturo 25/11/15)

   //Add the time to the beginning
   *pddata++ = etime1;
   *pddata++ = etime2;

   // copy data into event
   int i;
   int buffsize_32 = buffsize/4; // Calculate number of 32-bit words
   for(i = 0; i < buffsize_32; i++){
     *pddata++ = words[i];
     //printf("  data[%i] = 0x%x\n",i,words[i]);
   }

   bk_close(pevent, pddata);	

   //primitive progress bar
   //if (sn % 100 == 0) printf(".%d",bk_size(pevent));

   return bk_size(pevent);

}
 
/*-- Event readout -------------------------------------------------*/

INT read_slow_event(char *pevent, INT off)
{


   gettimeofday(&te,NULL);
   long long etime = (long long)(te.tv_sec)*1000+(int)te.tv_usec/1000;

   uint32_t etime1, etime2;
   etime1 = ((etime>>32)&0xFFFFFFFF);
   etime2 = ((etime)&0xFFFFFFFF);

   uint32_t *pddata;
   uint32_t nEvtSze;
   uint32_t sn = SERIAL_NUMBER(pevent);

   // Create event header
   bk_init32(pevent);

   bk_create(pevent, BankNameSlow[0], TID_DWORD, (void**)&pddata);//cast to void (arturo 25/11/15)

   //Add the time to the beginning
   *pddata++ = etime1;
   *pddata++ = etime2;

   // Get number of stored events
   uint32_t Data;
   CAEN_DGTZ_ReadRegister(handle,0x812c,&Data);
   *pddata++ = Data;

   bk_close(pevent, pddata);	
   
   // Send a software trigger
   if(tsvc[0].sw_trigger){
     int ret = CAEN_DGTZ_SendSWtrigger(handle);
     //printf("SW Trigger returns %i\n",ret);
   }

   return bk_size(pevent);

}
 
