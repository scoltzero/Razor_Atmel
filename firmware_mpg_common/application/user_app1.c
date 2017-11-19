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

static u8 UserApp1_au8MasterName[9]   = "0\0\0\0\0\0\0\0";

static AntAssignChannelInfoType sMasterChannel;
static AntAssignChannelInfoType sSlaveChannel;
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
  u8 au8WelcomeMessage[] = "Assign your role";
  u8 au8Instructions[] = "B0 Hider  B1 Seeker";


  /* Clear screen and place start messages */
#ifdef EIE1
  LCDCommand(LCD_CLEAR_CMD);
  LCDMessage(LINE1_START_ADDR, au8WelcomeMessage); 
  LCDMessage(LINE2_START_ADDR, au8Instructions); 

  
  
  LedOff(ORANGE);
  LedOff(RED);
  LedOff(PURPLE);
  LedOff(CYAN);
  LedOff(RED);
  LedOff(BLUE);
  LedOff(GREEN);
  LedOff(WHITE);
  /* Start with LED0 in RED state = channel is not configured */

#endif /* EIE1 */
  

  
 /* Configure ANT for this application */
  sMasterChannel.AntChannel          = ANT_CHANNEL_0;
  sMasterChannel.AntChannelType      = CHANNEL_TYPE_MASTER;
  sMasterChannel.AntChannelPeriodLo  = ANT_CHANNEL_PERIOD_LO_USERAPP;
  sMasterChannel.AntChannelPeriodHi  = ANT_CHANNEL_PERIOD_HI_USERAPP;
  
  sMasterChannel.AntDeviceIdLo       = ANT_DEVICEID_LO_USERAPP;
  sMasterChannel.AntDeviceIdHi       = ANT_DEVICEID_HI_USERAPP;
  sMasterChannel.AntDeviceType       = ANT_DEVICE_TYPE_USERAPP;
  sMasterChannel.AntTransmissionType = ANT_TRANSMISSION_TYPE_USERAPP;
  sMasterChannel.AntFrequency        = ANT_FREQUENCY_USERAPP;
  sMasterChannel.AntTxPower          = ANT_TX_POWER_USERAPP;

  sMasterChannel.AntNetwork          = ANT_NETWORK_DEFAULT;
  
  sSlaveChannel.AntChannel           = ANT_CHANNEL_1;
  sSlaveChannel.AntChannelType       = CHANNEL_TYPE_SLAVE;
  sSlaveChannel.AntChannelPeriodHi   = ANT_CHANNEL_PERIOD_HI_DEFAULT;
  sSlaveChannel.AntChannelPeriodLo   = ANT_CHANNEL_PERIOD_LO_DEFAULT;
  
  sSlaveChannel.AntDeviceIdLo       = 0x3C;
  sSlaveChannel.AntDeviceIdHi       = 0x14;
  sSlaveChannel.AntDeviceType       = ANT_DEVICE_TYPE_USERAPP;
  sSlaveChannel.AntTransmissionType = ANT_TRANSMISSION_TYPE_USERAPP;
  sSlaveChannel.AntFrequency        = ANT_FREQUENCY_USERAPP;
  sSlaveChannel.AntTxPower          = ANT_TX_POWER_USERAPP;
  
  sSlaveChannel.AntNetwork          = ANT_NETWORK_DEFAULT;
  
  for(u8 i = 0; i < ANT_NETWORK_NUMBER_BYTES; i++)
  {
    sMasterChannel.AntNetworkKey[i] = ANT_DEFAULT_NETWORK_KEY;
    sSlaveChannel.AntNetworkKey[i] = ANT_DEFAULT_NETWORK_KEY;
  }
    
  /* If good initialization, set state to Idle */
  if( AntAssignChannel(&sMasterChannel) )
  {
    /* Channel assignment is queued so start timer */

    UserApp1_u32Timeout = G_u32SystemTime1ms;
    LedOn(RED);
    
    
    UserApp1_StateMachine = UserApp1SM_WaitChannelAssign;
  }
  else
  {
    /* The task isn't properly initialized, so shut it down and don't run */

    LedBlink(RED, LED_4HZ);

    
    UserApp1_StateMachine = UserApp1SM_Error;
  }

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
/* Wait for the ANT channel assignment to finish */
static void UserApp1SM_WaitChannelAssign(void)
{
  /* Check if the channel assignment is complete */
  if(AntRadioStatusChannel(ANT_CHANNEL_0) == ANT_CONFIGURED)
  {
    AntAssignChannel(&sSlaveChannel);
    UserApp1_StateMachine = UserApp1SM_AntConfigureSlave;
  }
  
  /* Monitor for timeout */
  if( IsTimeUp(&UserApp1_u32Timeout, 5000) )
  {
    DebugPrintf("\n\r***Channel assignment timeout***\n\n\r");
    UserApp1_StateMachine = UserApp1SM_Error;
  }
      
} /* end UserApp1SM_WaitChannelAssign() */

static void UserApp1SM_AntConfigureSlave(void)
{
  if(AntRadioStatusChannel(ANT_CHANNEL_1) == ANT_CONFIGURED)
  {
    AntQueueBroadcastMessage(ANT_CHANNEL_0, UserApp1_au8MasterName);
    AntQueueBroadcastMessage(ANT_CHANNEL_1, UserApp1_au8MasterName);
       
    UserApp1_StateMachine = UserApp1SM_Assignrole;
  }
}



static void UserApp1SM_Assignrole(void)
{

  if(WasButtonPressed(BUTTON0))
  {
    ButtonAcknowledge(BUTTON0);

    LCDCommand(LCD_CLEAR_CMD);
    LCDMessage(LINE1_START_ADDR, "B0 Start"); 
    
    UserApp1_StateMachine=UserApp1SM_Hider;
  }
  
  if(WasButtonPressed(BUTTON1))
  {
    ButtonAcknowledge(BUTTON1);

    LCDCommand(LCD_CLEAR_CMD);
    LCDMessage(LINE1_START_ADDR, "B0 Start");
    
    UserApp1_StateMachine=UserApp1SM_Seeker;
  }
  
}




static void UserApp1SM_Hider(void)
{
  static u16 u16Counter=10000;
  static bool bCountdown=FALSE;
//  static bool bSend = FALSE;

  
  /* Look for BUTTON 0 to open channel */
  if(WasButtonPressed(BUTTON0))
  {
    /* Got the button, so complete one-time actions before next state */
    ButtonAcknowledge(BUTTON0);
    bCountdown=TRUE;
//    bSend=TRUE;
    LCDCommand(LCD_CLEAR_CMD);
  }
  
//  if(bSend)
//  {
//   if( AntReadAppMessageBuffer() )
//    {
//      /* New data message: check what it is */
//      if(G_eAntApiCurrentMessageClass == ANT_TICK)
//      {
//        UserApp1_au8MasterName[6] = ANT_DEVICEID_HI_USERAPP;
//        UserApp1_au8MasterName[7] = ANT_DEVICEID_LO_USERAPP;
//        AntQueueBroadcastMessage(ANT_CHANNEL_0,UserApp1_au8MasterName);
//      }
//      else if(G_eAntApiCurrentMessageClass == ANT_DATA)
//      {
//        if(G_au8AntApiCurrentMessageBytes[5] == 0x01)
//        {
//          bCountdown=TRUE;
//        }
//      }
//    }  
//  }
  
  if(bCountdown)
  {
    u16Counter--;
    if(u16Counter==9000)
    {
      LCDMessage(LINE1_START_ADDR, "Hider            "); 
      LCDMessage(LINE2_START_ADDR, "9");
    }
    
    if(u16Counter==8000)
    {
      LCDMessage(LINE1_START_ADDR, "Hider            "); 
      LCDMessage(LINE2_START_ADDR, "8");
    }
    
    if(u16Counter==7000)
    {
      LCDMessage(LINE1_START_ADDR, "Hider            "); 
      LCDMessage(LINE2_START_ADDR, "7");
    }
    
    if(u16Counter==6000)
    {
      LCDMessage(LINE1_START_ADDR, "Hider            "); 
      LCDMessage(LINE2_START_ADDR, "6");
    }
    
    if(u16Counter==5000)
    {
      LCDMessage(LINE1_START_ADDR, "Hider            "); 
      LCDMessage(LINE2_START_ADDR, "5");
    }
    
    if(u16Counter==4000)
    {
      LCDMessage(LINE1_START_ADDR, "Hider            "); 
      LCDMessage(LINE2_START_ADDR, "4");
    }
    
    if(u16Counter==3000)
    {
      LCDMessage(LINE1_START_ADDR, "Hider            "); 
      LCDMessage(LINE2_START_ADDR, "3");
    }

    if(u16Counter==2000)
    {
      LCDMessage(LINE1_START_ADDR, "Hider            "); 
      LCDMessage(LINE2_START_ADDR, "2");
    }
    
    if(u16Counter==1000)
    {
      LCDMessage(LINE1_START_ADDR, "Hider            "); 
      LCDMessage(LINE2_START_ADDR, "1");
    }
    
    if(u16Counter==0)
    {     
      LCDMessage(LINE1_START_ADDR, "Come Catch Me  "); 
      LCDMessage(LINE2_START_ADDR, "HAHAHAHAHA!");
      AntOpenChannelNumber(ANT_CHANNEL_0);
      AntOpenChannelNumber(ANT_CHANNEL_1);
      bCountdown=FALSE;

      UserApp1_StateMachine = UserApp1SM_ChannelOpen2;
    }

  }
}
  /*-------------------------------------------------------------------------------------------------------------------*/
/* Wait for a message to be queued */
static void UserApp1SM_Seeker(void)
{
  static u16 u16Counter=10000;
  static bool bCountdown=FALSE;
//  static bool bReceive = FALSE;

  
  /* Look for BUTTON 0 to open channel */
  if(WasButtonPressed(BUTTON0))
  {
    /* Got the button, so complete one-time actions before next state */
    ButtonAcknowledge(BUTTON0);
    bCountdown=TRUE;
    LCDCommand(LCD_CLEAR_CMD);
  }
  
  if(bCountdown)
  {
    u16Counter--;
    if(u16Counter==9000)
    {
      LCDMessage(LINE1_START_ADDR, "Seeker            "); 
      LCDMessage(LINE2_START_ADDR, "9");
    }
    
    if(u16Counter==8000)
    {
       
      LCDMessage(LINE2_START_ADDR, "8");
    }
    
    if(u16Counter==7000)
    {
      
      LCDMessage(LINE2_START_ADDR, "7");
    }
    
    if(u16Counter==6000)
    {
      
      LCDMessage(LINE2_START_ADDR, "6");
    }
    
    if(u16Counter==5000)
    {
      
      LCDMessage(LINE2_START_ADDR, "5");
    }
    
    if(u16Counter==4000)
    {
      
      LCDMessage(LINE2_START_ADDR, "4");
    }
    
    if(u16Counter==3000)
    {
      
      LCDMessage(LINE2_START_ADDR, "3");
    }

    if(u16Counter==2000)
    {
      
      LCDMessage(LINE2_START_ADDR, "2");
    }
    
    if(u16Counter==1000)
    {
      
      LCDMessage(LINE2_START_ADDR, "1");
    }
    
    if(u16Counter==0)
    {     
      LCDMessage(LINE1_START_ADDR, "Ready or not  "); 
      LCDMessage(LINE2_START_ADDR, "Here I come!");
      
      AntOpenChannelNumber(ANT_CHANNEL_1);
      AntOpenChannelNumber(ANT_CHANNEL_0);
      bCountdown=FALSE;

      UserApp1_StateMachine = UserApp1SM_ChannelOpen;
    }

  }
}
  
  
    
/*-------------------------------------------------------------------------------------------------------------------*/
/* Wait for channel to open */
static void UserApp1SM_WaitChannelOpen(void)
{
  /* Monitor the channel status to check if channel is opened */
  if(AntRadioStatusChannel(ANT_CHANNEL_USERAPP) == ANT_OPEN)
  {
    UserApp1_StateMachine = UserApp1SM_ChannelOpen;
  }
  
  /* Check for timeout */
  if( IsTimeUp(&UserApp1_u32Timeout, TIMEOUT_VALUE) )
  {
    AntCloseChannelNumber(ANT_CHANNEL_USERAPP);

    UserApp1_StateMachine = UserApp1SM_Assignrole;
  }
    
} /* end UserApp1SM_WaitChannelOpen() */


/*-------------------------------------------------------------------------------------------------------------------*/
/* Channel is open, so monitor data */
static void UserApp1SM_ChannelOpen(void)
{
  static s8 s8RssiChannel0=-99;
  static u8 au8Temp[]={'-',1,1,'d','B','m','\0'};
  static u8 u8Temp;
  static bool bFound=FALSE;
  static u8 u8CountS = 0;

//  /* Check for BUTTON0 to close channel */
//  if(WasButtonPressed(BUTTON0))
//  {
//    /* Got the button, so complete one-time actions before next state */
//    ButtonAcknowledge(BUTTON0);
//    
//    /* Queue close channel and change LED to blinking green to indicate channel is closing */
//    AntCloseChannelNumber(ANT_CHANNEL_USERAPP);
//
//
//    UserApp1_StateMachine = UserApp1SM_WaitChannelOpen;
//  }


  
    /* Always check for ANT messages */
  if(bFound==FALSE)
  {
    if( AntReadAppMessageBuffer() )
    {
      /* New data message: check what it is */
      if(G_eAntApiCurrentMessageClass == ANT_DATA)
      {
        if(G_sAntApiCurrentMessageExtData.u8Channel == 0x01)
        {
          s8RssiChannel0 = G_sAntApiCurrentMessageExtData.s8RSSI;
          if(s8RssiChannel0 >= -55)
          {
            UserApp1_au8MasterName[0]=0x01;
            u8CountS++;
          }
//          else
//          {
//            UserApp1_au8MasterName[0]=0x00;
//            AntQueueBroadcastMessage(ANT_CHANNEL_1,UserApp1_au8MasterName );
//          }
        }
      }
       
      AntQueueBroadcastMessage(ANT_CHANNEL_1,UserApp1_au8MasterName );

      u8Temp = abs(s8RssiChannel0);
      au8Temp[1] = u8Temp/10 + 48;
      au8Temp[2] = u8Temp%10 + 48;
      LCDMessage(LINE1_END_ADDR-6, au8Temp);
    

      if(s8RssiChannel0>-120&&s8RssiChannel0<-110)
      {
        LedOn(ORANGE);
        LedOff(RED);
        LedOff(PURPLE);
        LedOff(CYAN);
        LedOff(RED);
        LedOff(BLUE);
        LedOff(GREEN);
        LedOff(WHITE);
      }
      if(s8RssiChannel0>-110&&s8RssiChannel0<-100)
      {
        LedOn(ORANGE);
        LedOn(RED);
        LedOff(PURPLE);
        LedOff(CYAN);
        LedOff(RED);
        LedOff(BLUE);
        LedOff(GREEN);
        LedOff(WHITE);
      }
      if(s8RssiChannel0>-100&&s8RssiChannel0<-90)
      {
        LedOn(ORANGE);
        LedOn(RED);
        LedOn(PURPLE);
        LedOff(CYAN);
        LedOff(RED);
        LedOff(BLUE);
        LedOff(GREEN);
        LedOff(WHITE);
      }
      if(s8RssiChannel0>-90&&s8RssiChannel0<-80)
      {
        LedOn(ORANGE);
        LedOn(RED);
        LedOn(PURPLE);
        LedOn(CYAN);
        LedOff(RED);
        LedOff(BLUE);
        LedOff(GREEN);
        LedOff(WHITE);
      }
      if(s8RssiChannel0>-80&&s8RssiChannel0<-70)
      {
        LedOn(ORANGE);
        LedOn(RED);
        LedOn(PURPLE);
        LedOn(CYAN);
        LedOn(RED);
        LedOff(BLUE);
        LedOff(GREEN);
        LedOff(WHITE);
      }
      if(s8RssiChannel0>-70&&s8RssiChannel0<-65)
      {
        LedOn(ORANGE);
        LedOn(RED);
        LedOn(PURPLE);
        LedOn(CYAN);
        LedOn(RED);
        LedOn(BLUE);
        LedOff(GREEN);
        LedOff(WHITE);
      }
      if(s8RssiChannel0>-65&&s8RssiChannel0<-60)
      {
        LedOn(ORANGE);
        LedOn(RED);
        LedOn(PURPLE);
        LedOn(CYAN);
        LedOn(RED);
        LedOn(BLUE);
        LedOn(GREEN);
        LedOff(WHITE);
      }
      if(s8RssiChannel0>-60&&s8RssiChannel0<-55)
      {
        LedOn(ORANGE);
        LedOn(RED);
        LedOn(PURPLE);
        LedOn(CYAN);
        LedOn(RED);
        LedOn(BLUE);
        LedOn(GREEN);
        LedOn(WHITE);
        
      }
      if(s8RssiChannel0>=-55&&(u8CountS==16))
      {
        LedOff(ORANGE);
        LedOff(RED);
        LedOff(PURPLE);
        LedOff(CYAN);
        LedOff(RED);
        LedOff(BLUE);
        LedOff(GREEN);
        LedOff(WHITE);
        LCDMessage(LINE1_START_ADDR, "I Found You!");
        LCDMessage(LINE2_START_ADDR, "10 seconds switch");
        u8CountS=0;
        UserApp1_StateMachine = UserApp1SM_SeekerFound;
      }
    }

    
    
  }
  
  
            
} /* end UserApp1SM_ChannelOpen() */

static void UserApp1SM_ChannelOpen2(void)
{
  static s8 s8RssiChannel01=-90;
  static u8 au8Temp[]={'-',1,1,'d','B','m','\0'};
  static u8 u8Temp;
  static bool bFound1=FALSE;
  static bool bClose = FALSE;
//
//  /* Check for BUTTON0 to close channel */
//  if(WasButtonPressed(BUTTON0))
//  {
//    /* Got the button, so complete one-time actions before next state */
//    ButtonAcknowledge(BUTTON0);
//    
//    /* Queue close channel and change LED to blinking green to indicate channel is closing */
//    AntCloseChannelNumber(ANT_CHANNEL_USERAPP);
//
//
//    UserApp1_StateMachine = UserApp1SM_WaitChannelOpen;
//  }


  
    /* Always check for ANT messages */
  if(bFound1==FALSE)
  {
    if( AntReadAppMessageBuffer() )
    {
      /* New data message: check what it is */
      if(G_eAntApiCurrentMessageClass == ANT_DATA)
      {
        if(G_sAntApiCurrentMessageExtData.u8Channel == 0)
        {
          s8RssiChannel01 = G_sAntApiCurrentMessageExtData.s8RSSI;
//          if(G_au8AntApiCurrentMessageBytes[0] == 0x01)
//          {
//            bClose = TRUE;
//            
//          }
          
          //s8RssiChannel01=G_au8AntApiCurrentMessageBytes[7];
          //s8RssiChannel01 = G_sAntApiCurrentMessageExtData.s8RSSI;
        }
      }


      u8Temp = abs(s8RssiChannel0 b1);
      au8Temp[1] = u8Temp/10 + 48;
      au8Temp[2] = u8Temp%10 + 48;
      LCDMessage(LINE1_END_ADDR-6, au8Temp);
    
//
//      if(s8RssiChannel01>-120&&s8RssiChannel01<-110)
//      {
//        LedOn(ORANGE);
//        LedOff(RED);
//        LedOff(PURPLE);
//        LedOff(CYAN);
//        LedOff(RED);
//        LedOff(BLUE);
//        LedOff(GREEN);
//        LedOff(WHITE);
//      }
//      if(s8RssiChannel01>-110&&s8RssiChannel01<-100)
//      {
//        LedOn(ORANGE);
//        LedOn(RED);
//        LedOff(PURPLE);
//        LedOff(CYAN);
//        LedOff(RED);
//        LedOff(BLUE);
//        LedOff(GREEN);
//        LedOff(WHITE);
//      }
//      if(s8RssiChannel01>-100&&s8RssiChannel01<-90)
//      {
//        LedOn(ORANGE);
//        LedOn(RED);
//        LedOn(PURPLE);
//        LedOff(CYAN);
//        LedOff(RED);
//        LedOff(BLUE);
//        LedOff(GREEN);
//        LedOff(WHITE);
//      }
//      if(s8RssiChannel01>-90&&s8RssiChannel01<-80)
//      {
//        LedOn(ORANGE);
//        LedOn(RED);
//        LedOn(PURPLE);
//        LedOn(CYAN);
//        LedOff(RED);
//        LedOff(BLUE);
//        LedOff(GREEN);
//        LedOff(WHITE);
//      }
//      if(s8RssiChannel01>-80&&s8RssiChannel01<-70)
//      {
//        LedOn(ORANGE);
//        LedOn(RED);
//        LedOn(PURPLE);
//        LedOn(CYAN);
//        LedOn(RED);
//        LedOff(BLUE);
//        LedOff(GREEN);
//        LedOff(WHITE);
//      }
//      if(s8RssiChannel01>-70&&s8RssiChannel01<-60)
//      {
//        LedOn(ORANGE);
//        LedOn(RED);
//        LedOn(PURPLE);
//        LedOn(CYAN);
//        LedOn(RED);
//        LedOn(BLUE);
//        LedOff(GREEN);
//        LedOff(WHITE);
//      }
//      if(s8RssiChannel01>-60&&s8RssiChannel01<-50)
//      {
//        LedOn(ORANGE);
//        LedOn(RED);
//        LedOn(PURPLE);
//        LedOn(CYAN);
//        LedOn(RED);
//        LedOn(BLUE);
//        LedOn(GREEN);
//        LedOff(WHITE);
//      }
//      if(s8RssiChannel01>-50&&s8RssiChannel01<-45)
//      {
//        LedOn(ORANGE);
//        LedOn(RED);
//        LedOn(PURPLE);
//        LedOn(CYAN);
//        LedOn(RED);
//        LedOn(BLUE);
//        LedOn(GREEN);
//        LedOn(WHITE);
//        
//      }
      if(s8RssiChannel01>=-55)
      {
        LedOff(ORANGE);
        LedOff(RED);
        LedOff(PURPLE);
        LedOff(CYAN);
        LedOff(RED);
        LedOff(BLUE);
        LedOff(GREEN);
        LedOff(WHITE);
        LCDMessage(LINE1_START_ADDR, "You Found Me!");
        LCDMessage(LINE2_START_ADDR, "10 seconds switch");
       
        UserApp1_StateMachine = UserApp1SM_SeekerFound;
      }
    }

    
  }
  
  
       
}
/*-------------------------------------------------------------------------------------------------------------------*/
/* Wait for channel to close */
static void UserApp1SM_WaitChannelClose(void)
{
  /* Monitor the channel status to check if channel is closed */
  if(AntRadioStatusChannel(ANT_CHANNEL_USERAPP) == ANT_CLOSED)
  {
#ifdef MPG1
    LedOff(GREEN);
    LedOn(YELLOW);
#endif /* MPG1 */

#ifdef MPG2
    LedOn(GREEN0);
    LedOn(RED0);
#endif /* MPG2 */
    UserApp1_StateMachine = UserApp1SM_Assignrole;
  }
  
  /* Check for timeout */
  if( IsTimeUp(&UserApp1_u32Timeout, TIMEOUT_VALUE) )
  {
#ifdef MPG1
    LedOff(GREEN);
    LedOff(YELLOW);
    LedBlink(RED, LED_4HZ);
#endif /* MPG1 */

#ifdef MPG2
    LedBlink(RED0, LED_4HZ);
    LedOff(GREEN0);
#endif /* MPG2 */
    
    UserApp1_StateMachine = UserApp1SM_Error;
  }
    
} /* end UserApp1SM_WaitChannelClose() */


/*-------------------------------------------------------------------------------------------------------------------*/
/* Handle an error */
static void UserApp1SM_Error(void)          
{

} /* end UserApp1SM_Error() */



static void UserApp1SM_SeekerFound(void)
{    
  static u16 u16Counter2=10000;
  u8 au8WelcomeMessage[] = "Assign your role";
  u8 au8Instructions[] = "B0 Hider  B1 Seeker";
  static bool bLEDFlag=FALSE;
  
  if(!bLEDFlag)
  {
    LedBlink(ORANGE, LED_2HZ);
    LedBlink(RED, LED_2HZ);
    LedBlink(PURPLE, LED_2HZ);
    LedBlink(CYAN, LED_2HZ);
    LedBlink(RED, LED_2HZ);
    LedBlink(BLUE, LED_2HZ);
    LedBlink(GREEN, LED_2HZ);
    LedBlink(WHITE, LED_2HZ);
    bLEDFlag=TRUE;
  }
//     AntCloseChannelNumber(ANT_CHANNEL_0);
//     AntCloseChannelNumber(ANT_CHANNEL_1);
    
  u16Counter2--;

  if(u16Counter2==0)
  {
    LedOff(ORANGE);
    LedOff(RED);
    LedOff(PURPLE);
    LedOff(CYAN);
    LedOff(RED);
    LedOff(BLUE);
    LedOff(GREEN);
    LedOff(WHITE);    
    LCDCommand(LCD_CLEAR_CMD);
    LCDMessage(LINE1_START_ADDR, au8WelcomeMessage); 
    LCDMessage(LINE2_START_ADDR, au8Instructions); 
     
    AntCloseChannelNumber(ANT_CHANNEL_0);
    AntCloseChannelNumber(ANT_CHANNEL_1);
    
    UserApp1_StateMachine = UserApp1SM_Assignrole;
    u16Counter2=10000;
    bLEDFlag=FALSE;
  }
  

}

/*--------------------------------------------------------------------------------------------------------------------*/
/* End of File                                                                                                        */
/*--------------------------------------------------------------------------------------------------------------------*/
