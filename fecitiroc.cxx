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


#include "OdbDT5743.h"

// CAEN includes
#include <CAENDigitizer.h>

#include "CITIROC.h"

#define  EQ_NAME   "V1743"
#define  EQ_EVID   1
#define  EQ_TRGMSK 0x1111

#define CAEN_USE_DIGITIZERS

/* Globals */
#define N_DT5743 1
bool CITIROC_status;
int CITIROC_usbID;
const char* CITIROC_serialNumber = "CT1A_31A";

/* Hardware */
extern HNDLE hDB;
extern BOOL debug;

HNDLE hSet[N_DT5743];
DT5743_CONFIG_SETTINGS tsvc[N_DT5743];
//const char BankName[N_DT5743][5]={"D743"};
const char BankName[N_DT5743][5]={"43FS"};
const char BankNameSlow[N_DT5743][5]={"43SL"};

// extern int CITIROC_usbID;

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

  midas::odb database_asic = {
    {"chn", std::array<int,32>{15, 14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
    {"calibDacQ", std::array<int,32>{15, 14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
    {"enDiscri", std::array<int,1>{1}},
    // {"ppDiscri", {"1"}},
    // {"latchDiscri", {"1"}},
    // {"enDiscriT", {"1"}},
    // {"ppDiscriT", {"1"}},
    // {"enCalibDacQ", {"1"}},
    // {"ppCalibDacQ", {"1"}},
    // {"enCalibDacT", {"1"}},
    // {"ppCalibDacT", {"1"}},
    // {"mask", {"1"}},
    // {"ppThHg", {"1"}},
    // {"enThHg", {"1"}},
    // {"ppThLg", {"1"}},
    // {"enThLg", {"1"}},
    // {"biasSca", {"1"}},
    // {"ppPdetHg", {"1"}},
    // {"enPdetHg", {"1"}},
    // {"ppPdetLg", {"1"}},
    // {"enPdetLg", {"1"}},
    // {"scaOrPdHg", {"1"}},
    // {"scaOrPdLg", {"1"}},
    // {"bypassPd", {"1"}},
    // {"selTrigExtPd", {"1"}},
    // {"ppFshBuffer", {"1"}},
    // {"enFsh", {"1"}},
    // {"ppFsh", {"1"}},
    // {"ppSshLg", {"1"}},
    // {"enSshLg", {"1"}},
    // {"shapingTimeLg", std::array<int, 3>{}},
    // {"ppSshHg", {"1"}},
    // {"enSshHg", {"1"}},
    // {"shapingTimeHg", std::array<int, 3>{}},
    // {"paLgBias", {"1"}},
    // {"ppPaHg", {"1"}},
    // {"enPaHg", {"1"}},
    // {"ppPaLg", {"1"}},
    // {"enPaLg", {"1"}},
    // {"fshOnLg", {"1"}},
    // {"enInputDac", {"1"}},
    // {"dacRef", {"1"}},
    // {"inputDac", std::array<int, 8>{}},
    // {"cmdInputDac", std::array<int, 8>{}},
    // {"paHgGain (15)", {"1"}},
    // {"paLgGain (15)", {"1"}},
    // {"CtestHg (15)", {"1"}},
    // {"CtestLg (15)", {"1"}},
    // {"enPa (15)", {"1"}},
    // {"ppTemp", {"1"}},
    // {"enTemp", {"1"}},
    // {"ppBg", {"1"}},
    // {"enBg", {"1"}},
    // {"enThresholdDac1", {"1"}},
    // {"ppThresholdDac1", {"1"}},
    // {"enThresholdDac2", {"1"}},
    // {"ppThresholdDac2", {"1"}},

    // {"threshold1", std::array<int, 10>{}},
    // {"threshold2", std::array<int, 10>{}},
    
    // {"enHgOtaQ", {"1"}},
    // {"ppHgOtaQ", {"1"}},
    // {"enLgOtaQ", {"1"}},
    // {"ppLgOtaQ", {"1"}},
    // {"enProbeOtaQ", {"1"}},
    // {"ppProbeOtaQ", {"1"}},
    // {"testBitOtaQ", {"1"}},
    // {"enValEvtReceiver", {"1"}},
    // {"ppValEvtReceiver", {"1"}},
    // {"enRazChnReceiver", {"1"}},
    // {"ppRazChnReceiver", {"1"}},
    // {"enDigitalMuxOutput", {"1"}},
    // {"enOr32", {"1"}},
    // {"enNor32Oc", {"1"}},
    // {"triggerPolarity", {"1"}},
    // {"enNor32TOc", {"1"}},
    // {"enTriggersOutput", {"1"}},
  };

  midas::odb database_asic_addresses = {
    {"chn", "0"},
    {"calibDacQ", {"128"}},
    {"enDiscri", {"256"}},
    {"ppDiscri", {"257"}},
    {"latchDiscri", {"258"}},
    {"enDiscriT", {"259"}},
    {"ppDiscriT", {"260"}},
    {"enCalibDacQ", {"261"}},
    {"ppCalibDacQ", {"262"}},
    {"enCalibDacT", {"263"}},
    {"ppCalibDacT", {"264"}},
    {"mask", {"265"}},
    {"ppThHg", {"297"}},
    {"enThHg", {"298"}},
    {"ppThLg", {"299"}},
    {"enThLg", {"300"}},
    {"biasSca", {"301"}},
    {"ppPdetHg", {"302"}},
    {"enPdetHg", {"303"}},
    {"ppPdetLg", {"304"}},
    {"enPdetLg", {"305"}},
    {"scaOrPdHg", {"306"}},
    {"scaOrPdLg", {"307"}},
    {"bypassPd", {"308"}},
    {"selTrigExtPd", {"309"}},
    {"ppFshBuffer", {"310"}},
    {"enFsh", {"311"}},
    {"ppFsh", {"312"}},
    {"ppSshLg", {"313"}},
    {"enSshLg", {"314"}},
    {"shapingTimeLg", {"315"}},
    {"ppSshHg", {"318"}},
    {"enSshHg", {"319"}},
    {"shapingTimeHg", {"320"}},
    {"paLgBias", {"323"}},
    {"ppPaHg", {"324"}},
    {"enPaHg", {"325"}},
    {"ppPaLg", {"326"}},
    {"enPaLg", {"327"}},
    {"fshOnLg", {"328"}},
    {"enInputDac", {"329"}},
    {"dacRef", {"330"}},
    {"inputDac", {"331"}},
    {"cmdInputDac", {"339"}},
    {"paHgGain", {"619"}},
    {"paLgGain", {"625"}},
    {"CtestHg", {"631"}},
    {"CtestLg", {"632"}},
    {"enPa", {"633"}},
    {"ppTemp", {"1099"}},
    {"enTemp", {"1100"}},
    {"ppBg", {"1101"}},
    {"enBg", {"1102"}},
    {"enThresholdDac1", {"1103"}},
    {"ppThresholdDac1", {"1104"}},
    {"enThresholdDac2", {"1105"}},
    {"ppThresholdDac2", {"1106"}},
    {"threshold1", {"1107"}},
    {"threshold2", {"1117"}},
    {"enHgOtaQ", {"1127"}},
    {"ppHgOtaQ", {"1128"}},
    {"enLgOtaQ", {"1129"}},
    {"ppLgOtaQ", {"1130"}},
    {"enProbeOtaQ", {"1131"}},
    {"ppProbeOtaQ", {"1132"}},
    {"testBitOtaQ", {"1133"}},
    {"enValEvtReceiver", {"1134"}},
    {"ppValEvtReceiver", {"1135"}},
    {"enRazChnReceiver", {"1136"}},
    {"ppRazChnReceiver", {"1137"}},
    {"enDigitalMuxOutput", {"1138"}},
    {"enOr32", {"1139"}},
    {"enNor32Oc", {"1140"}},
    {"triggerPolarity", {"1141"}},
    {"enNor32TOc", {"1142"}},
    {"enTriggersOutput", {"1143"}},

  };

  // Add parameters to the slow-control key
  database_slow.connect(odbdir_temp);
  database_asic.connect_and_fix_structure(odbdir_asic_values);
  database_asic_addresses.connect_and_fix_structure(odbdir_asic_addresses);

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
  
  // Testing odbxx 
  initialize_slow_control();
  initialize_daq_parameters();
  initialize_HV_parameters();
  bool CITIROC_status = CITIROC_sendASIC();

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
  // CITIROC_status = CITIROC_connect(CITIROC_serialNumber, &CITIROC_usbID);

  // if(!CITIROC_status){ 
  //   cm_msg(MERROR, "frontend_init", "Cannot open CITIROC board");
  //   return 0;
  // }else{
  //   cm_msg(MINFO, "frontend_init", "Successfully opened CITIROC board");
  // }

  printf("Closing communication...\n");
  // CITIROC_status = CITIROC_disconnet(&CITIROC_usbID);
  
  cm_msg(MINFO, "frontend_init", "Connected to CITIROC board of serial no. %s", CITIROC_serialNumber);
  
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
  CITIROC_status |= CITIROC_reset(CITIROC_usbID);
  if (CITIROC_status != 0) {
    printf("Error: Unable to reset digitizer.\n");
    CITIROC_raiseException();
    return -1;
  }
  
  int module = 0, status;
  
  CITIROC_status = CITIROC_initialize(CITIROC_usbID);
  sleep(3);
  CITIROC_status = CITIROC_testParameters(CITIROC_usbID);
  CITIROC_status = CITIROC_enableDAQ(CITIROC_usbID);
  
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
	
  byte* fifo20, fifo21, fifo23, fifo24;
  int wordCount = 0;
  CITIROC_status = CITIROC_readFIFO(CITIROC_usbID, fifo20, fifo21, fifo23, fifo24, wordCount);


  //  int ret2 =  CAEN_DGTZ_ReadData(handle, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, gBuffer, &buffsize);
	//int ret2 =  CAEN_DGTZ_ReadData(handle, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_2eVME, gBuffer, &buffsize);
   if(CITIROC_status){
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
 
