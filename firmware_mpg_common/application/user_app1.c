/**********************************************************************************************************************
File: user_app.c                                                                

----------------------------------------------------------------------------------------------------------------------
To start a new task using this user_app as a template:
 1. Copy both user_app.c and user_app.h to the Application directory
 2. Rename the files yournewtaskname.c and yournewtaskname.h
 3. Add yournewtaskname.c and yournewtaskname.h to the Application Include and Source groups in the IAR project
 4. Use ctrl-h (make sure "Match Case" is checked) to find and replace all instances of "user_app" with "yournewtaskname"
 5. Use ctrl-h to find and replace all instances of "UserApp1" with "YourNewTaskName"
 6. Use ctrl-h to find and replace all instances of "USER_APP" with "YOUR_NEW_TASK_NAME"
 7. Add a call to YourNewTaskNameInitialize() in the init section of main
 8. Add a call to YourNewTaskNameRunActiveState() in the Super Loop section of main
 9. Update yournewtaskname.h per the instructions at the top of yournewtaskname.h
10. Delete this text (between the dashed lines) and update the Description below to describe your task
----------------------------------------------------------------------------------------------------------------------

Description:
This is a user_app.c file template 

------------------------------------------------------------------------------------------------------------------------
API:

Public functions:


Protected System functions:
void UserApp1Initialize(void)
Runs required initialzation for the task.  Should only be called once in main init section.

void UserApp1RunActiveState(void)
Runs current task state.  Should only be called once in main loop.


**********************************************************************************************************************/

#include "configuration.h"

/***********************************************************************************************************************
Global variable definitions with scope across entire project.
All Global variable names shall start with "G_"
***********************************************************************************************************************/
/* New variables */
volatile u32 G_u32UserApp1Flags;                       /* Global state flags */


/*--------------------------------------------------------------------------------------------------------------------*/
/* Existing variables (defined in other files -- should all contain the "extern" keyword) */
extern u32 G_u32AntApiCurrentDataTimeStamp;                       /* From ant_api.c */
extern AntApplicationMessageType G_eAntApiCurrentMessageClass;    /* From ant_api.c */
extern u8 G_au8AntApiCurrentMessageBytes[ANT_APPLICATION_MESSAGE_BYTES];  /* From ant_api.c */
extern AntExtendedDataType G_sAntApiCurrentMessageExtData;        /* From ant_api.c */

extern volatile u32 G_u32SystemFlags;                  /* From main.c */
extern volatile u32 G_u32ApplicationFlags;             /* From main.c */

extern volatile u32 G_u32SystemTime1ms;                /* From board-specific source file */
extern volatile u32 G_u32SystemTime1s;                 /* From board-specific source file */



/***********************************************************************************************************************
Global variable definitions with scope limited to this local application.
Variable names shall start with "UserApp1_" and be declared as static.
***********************************************************************************************************************/
static u32 UserApp1_u32DataMsgCount = 0;             /* Counts the number of ANT_DATA packets received */
static u32 UserApp1_u32TickMsgCount = 0;             /* Counts the number of ANT_TICK packets received */

static fnCode_type UserApp1_StateMachine;            /* The state machine function pointer */
static u32 UserApp1_u32Timeout;                      /* Timeout counter used across states */
static AntAssignChannelInfoType sAntSetupData_seek;
static AntAssignChannelInfoType sAntSetupData_hide;
static AntChannelNumberType ANT_CHANNEL_USERAPP;
static u32 u32WaitTime;

static u8 u8rssi=99;
static u8 u8Lastrssi=99;
static RssiType RssiLevel=rssi0;
static RssiType LastRssiLevel=rssi0;

static u8 au8TestMessage_hide[] = {0x13, 0x19, 0, 0, 0xA5, 0, 0, 0};
/**********************************************************************************************************************
Function Definitions
**********************************************************************************************************************/

/*--------------------------------------------------------------------------------------------------------------------*/
/* Public functions                                                                                                   */
/*--------------------------------------------------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------------------------------------------------*/
/* Protected functions                                                                                                */
/*--------------------------------------------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------------------------------------------
Function: UserApp1Initialize

Description:
Initializes the State Machine and its variables.

Requires:
  -

Promises:
  - 
*/
void UserApp1Initialize(void)
{
  LCDCommand(LCD_CLEAR_CMD);
  LCDMessage(LINE1_START_ADDR, "  Choice the role!  "); 
  LCDMessage(LINE2_START_ADDR, "B1 SEEKER B2 HIDER");
  
 /* Configure ANT for this application */
  /*Seek Channel1*/
  sAntSetupData_hide.AntChannel          = ANT_CHANNEL_USERAPP_HIDE;
  sAntSetupData_hide.AntChannelType      = CHANNEL_TYPE_MASTER;
  sAntSetupData_hide.AntChannelPeriodLo  = ANT_CHANNEL_PERIOD_LO_USERAPP;
  sAntSetupData_hide.AntChannelPeriodHi  = ANT_CHANNEL_PERIOD_HI_USERAPP;
  
  sAntSetupData_hide.AntDeviceIdLo       = ANT_DEVICEID_LO_USERAPP;
  sAntSetupData_hide.AntDeviceIdHi       = ANT_DEVICEID_HI_USERAPP;
  sAntSetupData_hide.AntDeviceType       = ANT_DEVICE_TYPE_USERAPP;
  sAntSetupData_hide.AntTransmissionType = ANT_TRANSMISSION_TYPE_USERAPP;
  sAntSetupData_hide.AntFrequency        = ANT_FREQUENCY_USERAPP;
  sAntSetupData_hide.AntTxPower          = ANT_TX_POWER_USERAPP;
  
  sAntSetupData_hide.AntNetwork = ANT_NETWORK_DEFAULT;
  /* Hide Channel0 */
  sAntSetupData_seek.AntChannel          = ANT_CHANNEL_USERAPP_SEEK;
  sAntSetupData_seek.AntChannelType      = CHANNEL_TYPE_SLAVE;
  sAntSetupData_seek.AntChannelPeriodLo  = 0;
  sAntSetupData_seek.AntChannelPeriodHi  = 0;
  
  sAntSetupData_seek.AntDeviceIdLo       = ANT_DEVICEID_LO_USERAPP;
  sAntSetupData_seek.AntDeviceIdHi       = ANT_DEVICEID_HI_USERAPP;
  sAntSetupData_seek.AntDeviceType       = ANT_DEVICE_TYPE_USERAPP;
  sAntSetupData_seek.AntTransmissionType = ANT_TRANSMISSION_TYPE_USERAPP;
  sAntSetupData_seek.AntFrequency        = ANT_FREQUENCY_USERAPP;
  sAntSetupData_seek.AntTxPower          = ANT_TX_POWER_USERAPP; 

  sAntSetupData_seek.AntNetwork = ANT_NETWORK_DEFAULT;
  
  for(u8 i = 0; i < ANT_NETWORK_NUMBER_BYTES; i++)
  {
    sAntSetupData_hide.AntNetworkKey[i] = ANT_DEFAULT_NETWORK_KEY;
    sAntSetupData_seek.AntNetworkKey[i] = ANT_DEFAULT_NETWORK_KEY;
  }
  
  UserApp1_StateMachine = UserApp1SM_WaitChannel0Assign;
 
} /* end UserApp1Initialize() */


/*----------------------------------------------------------------------------------------------------------------------
Function UserApp1RunActiveState()

Description:
Selects and runs one iteration of the current state in the state machine.
All state machines have a TOTAL of 1ms to execute, so on average n state machines
may take 1ms / n to execute.

Requires:
  - State machine function pointer points at current state

Promises:
  - Calls the function to pointed by the state machine function pointer
*/
void UserApp1RunActiveState(void)
{
  UserApp1_StateMachine();

} /* end UserApp1RunActiveState */


/*--------------------------------------------------------------------------------------------------------------------*/
/* Private functions                                                                                                  */
/*--------------------------------------------------------------------------------------------------------------------*/


/**********************************************************************************************************************
State Machine Function Definitions
**********************************************************************************************************************/

/*-------------------------------------------------------------------------------------------------------------------*/
/* Wait for the ANT channel0 assignment to finish */
static void UserApp1SM_WaitChannel0Assign(void)
{ 
   /* If good initialization, set state to Idle */
  if( AntAssignChannel(&sAntSetupData_seek) )
  {
    LedOn(RED);
    /* Channel assignment is queued so start timer */
    UserApp1_u32Timeout = G_u32SystemTime1ms;
    UserApp1_StateMachine = UserApp1SM_CheckChannel0Assign;
  }
  else
  {
    /* The task isn't properly initialized, so shut it down and don't run */
    LedBlink(RED, LED_4HZ);
    
    UserApp1_StateMachine = UserApp1SM_Error;
  }
}/* end UserApp1SM_WaitChannel0Assign() */
  
/*-------------------------------------------------------------------------------------------------------------------*/
/* Check if the channel0 assignment is complete */
static void UserApp1SM_CheckChannel0Assign(void)
{
  if(AntRadioStatusChannel(ANT_CHANNEL_USERAPP_SEEK) == ANT_CONFIGURED)
  {
#ifdef EIE1
    LedOff(RED);
    LedOn(YELLOW);
#endif /* EIE1 */

    UserApp1_StateMachine = UserApp1SM_WaitChannel1Assign;
  }
  
  /* Monitor for timeout */
  if( IsTimeUp(&UserApp1_u32Timeout, 5000) )
  {
    DebugPrintf("\n\r***Channel assignment timeout***\n\n\r");
    UserApp1_StateMachine = UserApp1SM_Error;
  }
}/* end UserApp1SM_CheckChannel0Assign() */
      
/*-------------------------------------------------------------------------------------------------------------------*/
/* Wait for the ANT channel1 assignment to finish */
static void UserApp1SM_WaitChannel1Assign(void)
{ 
   /* If good initialization, set state to Idle */
  if( AntAssignChannel(&sAntSetupData_hide) )
  {
    LedOn(RED);
    /* Channel assignment is queued so start timer */
    UserApp1_u32Timeout = G_u32SystemTime1ms;
    UserApp1_StateMachine = UserApp1SM_CheckChannel1Assign;
  }
  else
  {
    /* The task isn't properly initialized, so shut it down and don't run */
    LedBlink(RED, LED_4HZ);
    
    UserApp1_StateMachine = UserApp1SM_Error;
  }
}/* end UserApp1SM_WaitChannel1Assign() */
  
/*-------------------------------------------------------------------------------------------------------------------*/
/* Check if the channel1 assignment is complete */
static void UserApp1SM_CheckChannel1Assign(void)
{
  if(AntRadioStatusChannel(ANT_CHANNEL_USERAPP_HIDE) == ANT_CONFIGURED)
  {
#ifdef EIE1
    LedOff(RED);
    LedOn(YELLOW);
#endif /* EIE1 */

    UserApp1_StateMachine = UserApp1SM_Idle;
  }
  
  /* Monitor for timeout */
  if( IsTimeUp(&UserApp1_u32Timeout, 5000) )
  {
    DebugPrintf("\n\r***Channel assignment timeout***\n\n\r");
    UserApp1_StateMachine = UserApp1SM_Error;
  }
}/* end UserApp1SM_CheckChannel1Assign() */


  /*-------------------------------------------------------------------------------------------------------------------*/
/* Wait for a message to be queued */
static void UserApp1SM_Idle(void)
{

  if(WasButtonPressed(BUTTON1))
  {
    ButtonAcknowledge(BUTTON1);
    ANT_CHANNEL_USERAPP = ANT_CHANNEL_USERAPP_SEEK;
    LCDCommand(LCD_CLEAR_CMD);
    LCDMessage(LINE1_START_ADDR, "PRESS B0 START"); 
    UserApp1_StateMachine = UserApp1SM_Readytostart; 
  }
  
   if(WasButtonPressed(BUTTON2))
  {
    ButtonAcknowledge(BUTTON2);
    ANT_CHANNEL_USERAPP = ANT_CHANNEL_USERAPP_HIDE;
    LCDCommand(LCD_CLEAR_CMD);
    LCDMessage(LINE1_START_ADDR, "PRESS B0 START");
    UserApp1_StateMachine = UserApp1SM_Readytostart;
  } 
} /* end UserApp1SM_Idle() */

  /*-------------------------------------------------------------------------------------------------------------------*/
/* Ready*/    
static void UserApp1SM_Readytostart(void)
{
  LedOff(WHITE);
  LedOff(PURPLE);
  LedOff(ORANGE);
  
  if(WasButtonPressed(BUTTON0))
  {
    ButtonAcknowledge(BUTTON0);
    
    LCDCommand(LCD_CLEAR_CMD);
    if( ANT_CHANNEL_USERAPP == ANT_CHANNEL_USERAPP_SEEK)
    {
      
      LCDMessage(LINE1_START_ADDR, "SEEKER");
      LedBlink(CYAN,LED_1HZ);   
    }
  
    if( ANT_CHANNEL_USERAPP == ANT_CHANNEL_USERAPP_HIDE)
    {
      
      LCDMessage(LINE1_START_ADDR, "HIDER");
    }

    u32WaitTime = G_u32SystemTime1ms;
    UserApp1_StateMachine = UserApp1SM_wait3s; 
  }
}

/*-------------------------------------------------------------------------------------------------------------------*/
/* Channel is open, so monitor data */
static void UserApp1SM_wait3s(void)
{
  if( IsTimeUp(&u32WaitTime,3000) )
  {
    if( ANT_CHANNEL_USERAPP == ANT_CHANNEL_USERAPP_SEEK)
    {
      LCDCommand(LCD_CLEAR_CMD);
      LCDMessage(LINE1_START_ADDR, "Ready or not?");
      LCDMessage(LINE2_START_ADDR, "Here I come!");
      LedOff(CYAN);
    }
    
    AntOpenChannelNumber(ANT_CHANNEL_USERAPP_SEEK); 
    AntOpenChannelNumber(ANT_CHANNEL_USERAPP_HIDE);
    
    UserApp1_StateMachine = UserApp1SM_seek;
    UserApp1_u32Timeout = G_u32SystemTime1ms;
    UserApp1_StateMachine = UserApp1SM_WaitChannelOpen;
  }
}
/*-------------------------------------------------------------------------------------------------------------------*/
/* Wait for channel to open */
static void UserApp1SM_WaitChannelOpen(void)
{ 
  if((AntRadioStatusChannel(ANT_CHANNEL_USERAPP_SEEK) == ANT_OPEN)&&
     (AntRadioStatusChannel(ANT_CHANNEL_USERAPP_HIDE) == ANT_OPEN))
  {
    LedOff(YELLOW);
    LedOn(GREEN);
    UserApp1_StateMachine = UserApp1SM_Gamestart;
  }
  /* Check for timeout */
  if( IsTimeUp(&UserApp1_u32Timeout, TIMEOUT_VALUE) )
  {
    AntCloseChannelNumber(ANT_CHANNEL_USERAPP);

    LedOff(GREEN);
    LedOn(YELLOW);
 
    UserApp1_StateMachine = UserApp1SM_Idle;
  }  
} /* end UserApp1SM_WaitChannelOpen() */

/*-------------------------------------------------------------------------------------------------------------------*/
/* Wait for game to start */
static void UserApp1SM_Gamestart(void)
{
  if( ANT_CHANNEL_USERAPP == ANT_CHANNEL_USERAPP_SEEK)
  {
    UserApp1_StateMachine = UserApp1SM_seek;   
  }
  
  if( ANT_CHANNEL_USERAPP == ANT_CHANNEL_USERAPP_HIDE)
  {
    UserApp1_StateMachine = UserApp1SM_hide;
  }
}


/*-------------------------------------------------------------------------------------------------------------------*/
/* Channel is open, so monitor data */
static void UserApp1SM_seek(void)
{
  static u8 u8LastState = 0xff;
  static u8 au8TickMessage[] = "EVENT x\n\r";  /* "x" at index [6] will be replaced by the current code */
  static u8 au8DataContent_seek[] = "xxxxxxxxxxxxxxxx";
  static u8 au8LastAntData[ANT_APPLICATION_MESSAGE_BYTES] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  static u8 au8TestMessage_seek[] = {0, 0, 0, 0, 0xA5, 0, 0, 0};
  static u8 au8LCDRssi[]="-99dB";
  bool bGotNewData;

  static bool bBUZZER1=TRUE;
  static bool bFound=FALSE;


    
  u8Lastrssi=0-G_sAntApiCurrentMessageExtData.s8RSSI;
 /*Check for BUTTON0 to close channel */
  if(WasButtonPressed(BUTTON0))
  {
    /* Got the button, so complete one-time actions before next state */
    ButtonAcknowledge(BUTTON0);
    
    /* Queue close channel and change LED to blinking green to indicate channel is closing */
    AntCloseChannelNumber(ANT_CHANNEL_USERAPP_SEEK);
    AntCloseChannelNumber(ANT_CHANNEL_USERAPP_HIDE);
    u8LastState = 0xff;

#ifdef MPG1
    LedOff(YELLOW);
    LedOff(BLUE);
    LedBlink(GREEN, LED_2HZ);
#endif /* MPG1 */    
 
    /* Set timer and advance states */
    UserApp1_u32Timeout = G_u32SystemTime1ms;
    UserApp1_StateMachine = UserApp1SM_WaitChannelClose;
  } /* end if(WasButtonPressed(BUTTON0)) */
  

  /* Always check for ANT messages */
  if( AntReadAppMessageBuffer() )
  {
      if(G_sAntApiCurrentMessageExtData.u8Channel==ANT_CHANNEL_0)
  {
     /* New data message: check what it is */
    if(G_eAntApiCurrentMessageClass == ANT_DATA)
    {
      UserApp1_u32DataMsgCount++;
      
      /* We are synced with a device, so blue is solid */
      LedOff(GREEN);
      LedOn(BLUE);

      /* Check if the new data is the same as the old data and update as we go */
      bGotNewData = FALSE;
      for(u8 i = 0; i < ANT_APPLICATION_MESSAGE_BYTES; i++)
      {
        if(G_au8AntApiCurrentMessageBytes[i] != au8LastAntData[i])
        {
          bGotNewData = TRUE;
          au8LastAntData[i] = G_au8AntApiCurrentMessageBytes[i];
          au8DataContent_seek[2 * i]     = HexToASCIICharUpper(G_au8AntApiCurrentMessageBytes[i] / 16);
          au8DataContent_seek[2 * i + 1] = HexToASCIICharUpper(G_au8AntApiCurrentMessageBytes[i] % 16); 
        }
        
        if((au8DataContent_seek[0]=='1')&&
          (au8DataContent_seek[1]=='3')&&
          (au8DataContent_seek[2]=='1')&&
          (au8DataContent_seek[3]=='9'))
          {
            bFound=TRUE;
          }
      }
   
      if(bGotNewData)
      {
        /* We got new data: show on LCD */

      /* Update our local message counter and send the message back */

        /* Check for a special packet and respond */
#ifdef MPG1
        if(G_au8AntApiCurrentMessageBytes[0] == 0xA5)
        {
          LedOff(LCD_RED);
          LedOff(LCD_GREEN);
          LedOff(LCD_BLUE);
          
          if(G_au8AntApiCurrentMessageBytes[1] == 1)
          {
            LedOn(LCD_RED);
          }
          
          if(G_au8AntApiCurrentMessageBytes[2] == 1)
          {
            LedOn(LCD_GREEN);
          }

          if(G_au8AntApiCurrentMessageBytes[3] == 1)
          {
            LedOn(LCD_BLUE);
          }
        }
#endif /* MPG1 */    

      } /* end if(bGotNewData) */
    } /* end if(G_eAntApiCurrentMessageClass == ANT_DATA) */
    
    else if(G_eAntApiCurrentMessageClass == ANT_TICK)
    {
      UserApp1_u32TickMsgCount++;

      /* Look at the TICK contents to check the event code and respond only if it's different */
      if(u8LastState != G_au8AntApiCurrentMessageBytes[ANT_TICK_MSG_EVENT_CODE_INDEX])
      {
        /* The state changed so update u8LastState and queue a debug message */
        u8LastState = G_au8AntApiCurrentMessageBytes[ANT_TICK_MSG_EVENT_CODE_INDEX];
        au8TickMessage[6] = HexToASCIICharUpper(u8LastState);
        DebugPrintf(au8TickMessage);

        /* Parse u8LastState to update LED status */
        switch (u8LastState)
        {
#ifdef MPG1
          /* If we are paired but missing messages, blue blinks */
          case EVENT_RX_FAIL:
          {
            LedOff(GREEN);
            LedBlink(BLUE, LED_2HZ);
            break;
          }

          /* If we drop to search, LED is green */
          case EVENT_RX_FAIL_GO_TO_SEARCH:
          {
            LedOff(BLUE);
            LedOn(GREEN);
            break;
          }
#endif /* MPG 1 */

          /* If the search times out, the channel should automatically close */
          case EVENT_RX_SEARCH_TIMEOUT:
          {
            DebugPrintf("Search timeout event\r\n");
            break;
          }

          case EVENT_CHANNEL_CLOSED:
          {
            DebugPrintf("Channel closed event\r\n");
            break;
          }

            default:
          {
            DebugPrintf("Unexpected Event\r\n");
            break;
          }
        } /* end switch (G_au8AntApiCurrentMessageBytes) */
      } /* end if (u8LastState != G_au8AntApiCurrentMessageBytes[ANT_TICK_MSG_EVENT_CODE_INDEX]) */
    } /* end else if(G_eAntApiCurrentMessageClass == ANT_TICK) */
  }
  } /* end AntReadAppMessageBuffer() */
  
  /* A slave channel can close on its own, so explicitly check channel status */
  if(AntRadioStatusChannel(ANT_CHANNEL_USERAPP) != ANT_OPEN)
  {
#ifdef MPG1
    LedBlink(GREEN, LED_2HZ);
    LedOff(BLUE);
#endif /* MPG1 */

    u8LastState = 0xff;
    
    UserApp1_u32Timeout = G_u32SystemTime1ms;
    UserApp1_StateMachine = UserApp1SM_WaitChannelClose;
  } /* if(AntRadioStatusChannel(ANT_CHANNEL_USERAPP) != ANT_OPEN) */
  


if(G_sAntApiCurrentMessageExtData.u8Channel==ANT_CHANNEL_0)
{
  if(u8Lastrssi!=(0-G_sAntApiCurrentMessageExtData.s8RSSI))
  {
    u8Lastrssi=0-G_sAntApiCurrentMessageExtData.s8RSSI;
    u8rssi=0-G_sAntApiCurrentMessageExtData.s8RSSI;
    au8LCDRssi[1]=(u8rssi/10)+'0';
    au8LCDRssi[2]=(u8rssi%10)+'0';
    LCDClearChars(0x4D,5);
    LCDMessage(0x4D, au8LCDRssi);
    LastRssiLevel=RssiLevel;
    
    if((u8rssi<80)&&(u8rssi>30))
    {
      RssiLevel=rssi1;
      if(u8rssi<60)
      {
        RssiLevel=rssi2;
        if(u8rssi<45)
        {
          RssiLevel=rssi3;
        }
      }
    }
    else
    {
      RssiLevel=rssi0;
    }
     
  }/*End Read Rssi*/
} 
  if(RssiLevel!=LastRssiLevel)
  {
    switch(RssiLevel)
    {
    case rssi0:
      {
      LedOn(ORANGE);
      LedOff(PURPLE);
      LedOff(WHITE);     
      PWMAudioSetFrequency(BUZZER1,300);
      break;
      }
    
    case rssi1:
      {
      LedOn(ORANGE);
      LedOn(PURPLE);
      LedOff(WHITE);
      PWMAudioSetFrequency(BUZZER1,600);
      break;
      } 
    
    case rssi2:
      {
      LedOn(ORANGE);
      LedOn(PURPLE);
      LedOn(WHITE);
      PWMAudioSetFrequency(BUZZER1,1000);
      break;
      }
    case rssi3:
      {
       if(bFound)
        {       
          LedBlink(ORANGE,LED_1HZ);
          LedBlink(PURPLE,LED_1HZ);
          LedBlink(WHITE,LED_1HZ);  
          
          LCDCommand(LCD_CLEAR_CMD);
          LCDMessage(LINE1_START_ADDR, "  FOUND YOU!  ");
          
          au8TestMessage_seek[0]=0x10;
          AntQueueBroadcastMessage(ANT_CHANNEL_USERAPP_SEEK, au8TestMessage_seek);
          au8TestMessage_seek[0]=0x00;
          
          u8LastState = 0xff;
          
          bFound=FALSE;
          
          au8DataContent_seek[0]='0';
          au8DataContent_seek[1]='0';
          au8DataContent_seek[2]='0';
          au8DataContent_seek[3]='0';
          
          u32WaitTime = G_u32SystemTime1ms;
          UserApp1_StateMachine = UserApp1SM_Foundhider;         
        }
       else
       {
         LCDCommand(LCD_CLEAR_CMD);
         LCDMessage(LINE1_START_ADDR, "  not this  ");
       }

      break;
      } 
    }
  }/*End */
 
 
if(WasButtonPressed(BUTTON3))
  {
   ButtonAcknowledge(BUTTON3);
   bBUZZER1=!bBUZZER1;  
  }
  
if(bBUZZER1)
  {
   PWMAudioOn(BUZZER1);
   }
else
  {
   PWMAudioOff(BUZZER1);
  }

  

  
     
} /* end UserApp1SM_ChannelOpen() */
/*-------------------------------------------------------------------------------------------------------------------*/
/* seek found*/
static void UserApp1SM_Foundhider(void)
{
  PWMAudioOff(BUZZER1);
  u8rssi=99;
  u8Lastrssi=99;
  RssiLevel=rssi0;
  LastRssiLevel=rssi0;

  
  if(IsTimeUp(&u32WaitTime, 3000))
  { 
    LCDCommand(LCD_CLEAR_CMD);
    LCDMessage(LINE1_START_ADDR, "PRESS B0 TO START"); 
    
    AntCloseChannelNumber(ANT_CHANNEL_USERAPP_SEEK);
    AntCloseChannelNumber(ANT_CHANNEL_USERAPP_HIDE);
  switch(ANT_CHANNEL_USERAPP)
  {
  case ANT_CHANNEL_USERAPP_SEEK:
    {
    ANT_CHANNEL_USERAPP = ANT_CHANNEL_USERAPP_HIDE;

    break;
    }
    
  case ANT_CHANNEL_USERAPP_HIDE:
    {
    ANT_CHANNEL_USERAPP = ANT_CHANNEL_USERAPP_SEEK;
    au8TestMessage_hide[0] = '0';

    break;
    }
    
   default:break; 
  }

#ifdef MPG1
    LedOff(YELLOW);
    LedOff(BLUE);
    LedBlink(GREEN, LED_2HZ);
#endif /* MPG1 */    
 
    /* Set timer and advance states */
    UserApp1_u32Timeout = G_u32SystemTime1ms;
    UserApp1_StateMachine = UserApp1SM_WaitChannelClose;   
  }
  
}


/*-------------------------------------------------------------------------------------------------------------------*/
/* Channel is open, so monitor data */
static void UserApp1SM_hide(void)
{

  static u8 au8DataContent_hide[] = "xxxxxxxxxxxxxxxx";
  static u8 au8LastAntData[ANT_APPLICATION_MESSAGE_BYTES] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  static u8 au8TestMessage[] = {0x13, 0x19, 0x00, 0x00, 0xA5, 0, 0, 0};


  /* Check for BUTTON0 to close channel */
  if(WasButtonPressed(BUTTON0))
  {
    /* Got the button, so complete one-time actions before next state */
    ButtonAcknowledge(BUTTON0);
    
    /* Queue close channel and change LED to blinking green to indicate channel is closing */
    AntCloseChannelNumber(ANT_CHANNEL_USERAPP_SEEK);
    AntCloseChannelNumber(ANT_CHANNEL_USERAPP_HIDE);

#ifdef MPG1
    LedOff(YELLOW);
    LedOff(BLUE);
    LedBlink(GREEN, LED_2HZ);
#endif /* MPG1 */    
 
    /* Set timer and advance states */
    UserApp1_u32Timeout = G_u32SystemTime1ms;
    UserApp1_StateMachine = UserApp1SM_WaitChannelClose;
  } /* end if(WasButtonPressed(BUTTON0)) */
  
 
  /* Always check for ANT messages */
  if( AntReadAppMessageBuffer() )
  { if(G_sAntApiCurrentMessageExtData.u8Channel==ANT_CHANNEL_1)
{
     /* New data message: check what it is */
    if(G_eAntApiCurrentMessageClass == ANT_DATA)
    { 
       
      /* We are synced with a device, so blue is solid */
      LedOff(GREEN);
      LedOn(BLUE);
      
      /* Check if the new data is the same as the old data and update as we go */

      for(u8 i = 0; i < ANT_APPLICATION_MESSAGE_BYTES; i++)
      {

        if(G_au8AntApiCurrentMessageBytes[i] != au8LastAntData[i])
        {
          au8LastAntData[i] = G_au8AntApiCurrentMessageBytes[i];

          au8DataContent_hide[2 * i]     = HexToASCIICharUpper(G_au8AntApiCurrentMessageBytes[i] / 16);
          au8DataContent_hide[2 * i + 1] = HexToASCIICharUpper(G_au8AntApiCurrentMessageBytes[i] % 16); 
        }
      } 
           
    }/* end if(G_eAntApiCurrentMessageClass == ANT_DATA) */
    
    else if(G_eAntApiCurrentMessageClass == ANT_TICK)
    {
       au8TestMessage[7]++;
        if(au8TestMessage[7] == 0)
         {
            au8TestMessage[6]++;
            if(au8TestMessage[6] == 0)
          {
            au8TestMessage[5]++;
          }
         }
        LedOff(BLUE);
        AntQueueBroadcastMessage(ANT_CHANNEL_USERAPP_HIDE, au8TestMessage_hide);

    } /* end else if(G_eAntApiCurrentMessageClass == ANT_TICK) */
  }
  } /* end AntReadAppMessageBuffer() */
  
  if(au8DataContent_hide[0]=='1')
  {  
    LCDCommand(LCD_CLEAR_CMD);
    LCDMessage(LINE1_START_ADDR, " YOU FOUND ME!"); 
    
    au8DataContent_hide[0] = '0';
    u32WaitTime = G_u32SystemTime1ms;
    UserApp1_StateMachine = UserApp1SM_Foundhider;
  }


} /* end UserApp1SM_ChannelOpen() */


/*-------------------------------------------------------------------------------------------------------------------*/
/* Wait for channel to close */
static void UserApp1SM_WaitChannelClose(void)
{
  /* Monitor the channel status to check if channel is closed */
  if((AntRadioStatusChannel(ANT_CHANNEL_USERAPP_SEEK) == ANT_CLOSED)&&
     (AntRadioStatusChannel(ANT_CHANNEL_USERAPP_HIDE) == ANT_CLOSED))
  {
#ifdef MPG1
    LedOff(GREEN);
    LedOn(YELLOW);
#endif /* MPG1 */
    G_sAntApiCurrentMessageExtData.s8RSSI=-99;
    UserApp1_StateMachine = UserApp1SM_Readytostart;
  }
  
  /* Check for timeout */
  if( IsTimeUp(&UserApp1_u32Timeout, TIMEOUT_VALUE) )
  {
#ifdef MPG1
    LedOff(GREEN);
    LedOff(YELLOW);
    LedBlink(RED, LED_4HZ);
#endif /* MPG1 */

    UserApp1_StateMachine = UserApp1SM_Error;
  }
    
} /* end UserApp1SM_WaitChannelClose() */


/*-------------------------------------------------------------------------------------------------------------------*/
/* Handle an error */
static void UserApp1SM_Error(void)          
{

} /* end UserApp1SM_Error() */



/*--------------------------------------------------------------------------------------------------------------------*/
/* End of File                                                                                                        */
/*--------------------------------------------------------------------------------------------------------------------*/

