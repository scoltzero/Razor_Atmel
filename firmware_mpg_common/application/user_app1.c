/**********************************************************************************************************************
File: user_app1.c                                                                

----------------------------------------------------------------------------------------------------------------------
To start a new task using this user_app1 as a template:
 1. Copy both user_app1.c and user_app1.h to the Application directory
 2. Rename the files yournewtaskname.c and yournewtaskname.h
 3. Add yournewtaskname.c and yournewtaskname.h to the Application Include and Source groups in the IAR project
 4. Use ctrl-h (make sure "Match Case" is checked) to find and replace all instances of "user_app1" with "yournewtaskname"
 5. Use ctrl-h to find and replace all instances of "UserApp1" with "YourNewTaskName"
 6. Use ctrl-h to find and replace all instances of "USER_APP1" with "YOUR_NEW_TASK_NAME"
 7. Add a call to YourNewTaskNameInitialize() in the init section of main
 8. Add a call to YourNewTaskNameRunActiveState() in the Super Loop section of main
 9. Update yournewtaskname.h per the instructions at the top of yournewtaskname.h
10. Delete this text (between the dashed lines) and update the Description below to describe your task
----------------------------------------------------------------------------------------------------------------------

Description:
This is a user_app1.c file template 

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
All Global variable names shall start with "G_UserApp1"
***********************************************************************************************************************/
/* New variables */
volatile u32 G_u32UserApp1Flags;                       /* Global state flags */


/*--------------------------------------------------------------------------------------------------------------------*/
/* Existing variables (defined in other files -- should all contain the "extern" keyword) */
extern u32 G_u32AntApiCurrentMessageTimeStamp;                    /* From ant_api.c */
extern AntApplicationMessageType G_eAntApiCurrentMessageClass;    /* From ant_api.c */
extern u8 G_au8AntApiCurrentMessageBytes[ANT_APPLICATION_MESSAGE_BYTES];  /* From ant_api.c */
extern AntExtendedDataType G_sAntApiCurrentMessageExtData;                /* From ant_api.c */

extern volatile u32 G_u32SystemFlags;                  /* From main.c */
extern volatile u32 G_u32ApplicationFlags;             /* From main.c */

extern volatile u32 G_u32SystemTime1ms;                /* From board-specific source file */
extern volatile u32 G_u32SystemTime1s;                 /* From board-specific source file */


/***********************************************************************************************************************
Global variable definitions with scope limited to this local application.
Variable names shall start with "UserApp1_" and be declared as static.
***********************************************************************************************************************/
static fnCode_type UserApp1_StateMachine;              /* The state machine function pointer */
static u32 UserApp1_u32Timeout;                        /* Timeout counter used across states */

static AntAssignChannelInfoType UserApp1_sChannelInfo;  /* ANT setup parameters */
static bool bMaster = TRUE;
/**********************************************************************************************************************
Function Definitions
**********************************************************************************************************************/

/*--------------------------------------------------------------------------------------------------------------------*/
/* Public functions                                                                                                   */
/*--------------------------------------------------------------------------------------------------------------------*/

/*
Function: Tuen all leds off
*/
static void LedOffAll(void)
{
	LedOff(RED);
	LedOff(ORANGE);
	LedOff(YELLOW);
	LedOff(GREEN);
	LedOff(CYAN);
	LedOff(BLUE);
	LedOff(PURPLE);
	LedOff(WHITE);
}

/*
Function: Tuen all leds On
*/
static void LedOnAll(void)
{
	LedOn(RED);
	LedOn(ORANGE);
	LedOn(YELLOW);
	LedOn(GREEN);
	LedOn(CYAN);
	LedOn(BLUE);
	LedOn(PURPLE);
	LedOn(WHITE);
}

/*
Function: Toggle all leds state
Requires: One bool variable, if TRUE, turn all leds off, if FALSE, turn all leds on
Require apis: static void LedOnAll(void); static void LedOffAll(void);
*/
static void LedToggleAll(bool bOn)
{
	if(bOn)
	{
		LedOffAll();
	}
	else
	{
		LedOnAll();
	}
}

/*
Function: Toggle leds state every 0.5s
          After TimeCount seconds, return TURE
Requires: TimeCount defined in user_app1.h;
Require apis: static void LedToggleAll(bool); static void LedOffAll(void)
*/
static bool bTimeCountDown(void)
{
	static u16 u16TimeCount = TimeCount * 1000;
	static u16 u16LedToggleTime = 500;
	static u8 u8LcdDisplayTimeCount = 0;
	static bool bLedOn = FALSE;
	u8 au8LcdDisplayTime[] = " . s";
	
	if( u16TimeCount-- > 0 )
	{
		if(u16LedToggleTime-- == 0)
		{
			u16LedToggleTime = 500;
			bLedOn = !bLedOn;
			
			LedToggleAll(bLedOn);
		}
		
		if(u8LcdDisplayTimeCount-- == 0)
		{
			u8LcdDisplayTimeCount = 100;
			au8LcdDisplayTime[0] = (u16TimeCount / 1000) + 48;
			au8LcdDisplayTime[2] = ((u16TimeCount / 100) % 10) + 48;
			LCDMessage(LINE2_START_ADDR + 15, au8LcdDisplayTime);
		}
		
		return FALSE;
	}
	else
	{
		u16TimeCount = TimeCount * 1000;
		u16LedToggleTime = 500;
		u8LcdDisplayTimeCount = 100;
		bLedOn = FALSE;
		LedOffAll();
		
		return TRUE;
	}
}

/*
Function: Toggle leds state every 0.5s
          After 3s , return TURE
Require apis: static void LedToggleAll(bool); static void LedOffAll(void)
*/
static bool bGameOver(void)
{
	static u16 u16TimeCount = 3000;
	static u16 u16ToggleState = 500;
	static bool bOn = FALSE;
	
	if(u16TimeCount-- > 0)
	{
		if(u16ToggleState-- == 0)
		{
			u16ToggleState = 500;
			
			LedToggleAll(bOn);
			
			bOn = !bOn;
		}
		
		return FALSE;
	}
	else
	{
		u16TimeCount = 3000;
		u16ToggleState = 500;
		
		LedOffAll();
		
		if(bMaster)
		{
			AntCloseChannelNumber(ANT_CHANNEL_USERAPP_MASTER);
		}
		else
		{
			AntCloseChannelNumber(ANT_CHANNEL_USERAPP_SLAVE);
		}
		
		return TRUE;
	}
}

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
	/* Turn all led off */
	LedOffAll();
	
	/* Goto UserApp1SM_WaitUserChooseCharactor state */
	UserApp1_StateMachine = UserApp1SM_WaitUserChooseCharactor;
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
/* Master channel assignment */
static void UserApp1SM_ChannelAssign_Master(void)
{
	/* Channel assign */
	UserApp1_sChannelInfo.AntChannel          = ANT_CHANNEL_USERAPP_MASTER;
	UserApp1_sChannelInfo.AntChannelType      = ANT_CHANNEL_TYPE_USERAPP_MASTER;
	UserApp1_sChannelInfo.AntChannelPeriodLo  = ANT_CHANNEL_PERIOD_LO_USERAPP;
	UserApp1_sChannelInfo.AntChannelPeriodHi  = ANT_CHANNEL_PERIOD_HI_USERAPP;

	UserApp1_sChannelInfo.AntDeviceIdLo       = ANT_DEVICEID_LO_USERAPP;
	UserApp1_sChannelInfo.AntDeviceIdHi       = ANT_DEVICEID_HI_USERAPP;
	UserApp1_sChannelInfo.AntDeviceType       = ANT_DEVICE_TYPE_USERAPP;
	UserApp1_sChannelInfo.AntTransmissionType = ANT_TRANSMISSION_TYPE_USERAPP;
	UserApp1_sChannelInfo.AntFrequency        = ANT_FREQUENCY_USERAPP;
	UserApp1_sChannelInfo.AntTxPower          = ANT_TX_POWER_USERAPP;

	UserApp1_sChannelInfo.AntNetwork = ANT_NETWORK_DEFAULT;

	for( u8 i = 0; i < ANT_NETWORK_NUMBER_BYTES; i++ )
	{
		UserApp1_sChannelInfo.AntNetworkKey[i] = ANT_DEFAULT_NETWORK_KEY;
	}
	
	if(AntAssignChannel(&UserApp1_sChannelInfo))
	{
		UserApp1_u32Timeout = G_u32SystemTime1ms;
		
		/* Goto UserApp1SM_WaitChannelAssign state */
		UserApp1_StateMachine = UserApp1SM_WaitChannelAssign;
	}
	else
	{
		UserApp1_StateMachine = UserApp1SM_Error;
	}
} /* end UserApp1SM_ChannelAssign_Master */

/*-------------------------------------------------------------------------------------------------------------------*/
/* Slave channel assignment */
static void UserApp1SM_ChannelAssign_Slave(void)
{
	/* Channel assign */
	UserApp1_sChannelInfo.AntChannel          = ANT_CHANNEL_USERAPP_SLAVE;
	UserApp1_sChannelInfo.AntChannelType      = ANT_CHANNEL_TYPE_USERAPP_SLAVE;
	UserApp1_sChannelInfo.AntChannelPeriodLo  = ANT_CHANNEL_PERIOD_LO_USERAPP;
	UserApp1_sChannelInfo.AntChannelPeriodHi  = ANT_CHANNEL_PERIOD_HI_USERAPP;

	UserApp1_sChannelInfo.AntDeviceIdLo       = ANT_DEVICEID_LO_USERAPP;
	UserApp1_sChannelInfo.AntDeviceIdHi       = ANT_DEVICEID_HI_USERAPP;
	UserApp1_sChannelInfo.AntDeviceType       = ANT_DEVICE_TYPE_USERAPP;
	UserApp1_sChannelInfo.AntTransmissionType = ANT_TRANSMISSION_TYPE_USERAPP;
	UserApp1_sChannelInfo.AntFrequency        = ANT_FREQUENCY_USERAPP;
	UserApp1_sChannelInfo.AntTxPower          = ANT_TX_POWER_USERAPP;

	UserApp1_sChannelInfo.AntNetwork = ANT_NETWORK_DEFAULT;

	for( u8 i = 0; i < ANT_NETWORK_NUMBER_BYTES; i++ )
	{
		UserApp1_sChannelInfo.AntNetworkKey[i] = ANT_DEFAULT_NETWORK_KEY;
	}
	
	if(AntAssignChannel(&UserApp1_sChannelInfo))
	{
		UserApp1_u32Timeout = G_u32SystemTime1ms;
		
		/* Goto UserApp1SM_WaitChannelAssign state */
		UserApp1_StateMachine = UserApp1SM_WaitChannelAssign;
	}
	else
	{
		UserApp1_StateMachine = UserApp1SM_Error;
	}
} /* end UserApp1SM_ChannelAssign_Slave */

/*-------------------------------------------------------------------------------------------------------------------*/
/* Wait for ANT channel assignment */
static void UserApp1SM_WaitChannelAssign(void)
{
	if(bMaster)
	{
		if( AntRadioStatusChannel(ANT_CHANNEL_USERAPP_MASTER) == ANT_CONFIGURED)
		{
			/* Channel assignment is successful, so open channel and
			proceed to Idle state */
			UserApp1_StateMachine = UserApp1SM_Idle;
		}
	}
	else
	{
		if( AntRadioStatusChannel(ANT_CHANNEL_USERAPP_SLAVE) == ANT_CONFIGURED)
		{
			/* Channel assignment is successful, so open channel and
			proceed to Idle state */
			UserApp1_StateMachine = UserApp1SM_Idle;
		}
	}
	
	/* Watch for time out */
	if(IsTimeUp(&UserApp1_u32Timeout, 3000))
	{
		LedOn(RED);
		
		UserApp1_StateMachine = UserApp1SM_Error;    
	}
} /* end UserApp1SM_WaitChannelAssign */

/*-------------------------------------------------------------------------------------------------------------------*/
/* Wait for User choose a charactor and initial ANT */
static void UserApp1SM_WaitUserChooseCharactor(void)
{		
						/* LCDMessages */
						/*01234567890123456789*/
	static u8 au8RollPlay[]   = " Hider or Seeker ? ";
	static u8 au8ChooseInfo[] = "    Hide  Seek     ";
	static bool bDisplay = TRUE;
	
	if(bDisplay)
	{
		bDisplay = FALSE;

		/* Clear screen and place start messages */
		LCDCommand(LCD_CLEAR_CMD);
		LCDMessage(LINE1_START_ADDR, au8RollPlay); 
		LCDMessage(LINE2_START_ADDR, au8ChooseInfo);
	}
	
	if(WasButtonPressed(BUTTON1)) //User press BUTTON1 to choose hider
	{
		ButtonAcknowledge(BUTTON1);
		bMaster = TRUE;
		bDisplay = TRUE;
		
		/* Goto UserApp1SM_ChannelAssign_Master state */
		UserApp1_StateMachine = UserApp1SM_ChannelAssign_Master;
	}
	
	if(WasButtonPressed(BUTTON2)) // User press BUTTON2 to choose seeker
	{
		ButtonAcknowledge(BUTTON2);
		bMaster = FALSE;
		bDisplay = TRUE;
		
		/* Goto UserApp1SM_ChannelAssign_Slave state */
		UserApp1_StateMachine = UserApp1SM_ChannelAssign_Slave;
	}
	
	if(bDisplay)
	{
		LCDCommand(LCD_CLEAR_CMD);
		LCDMessage(LINE1_START_ADDR, "Press BUT0 to ready ");
		LCDMessage(LINE2_START_ADDR, "Ready:     NO       ");
	}
}/* end UserApp1SM_WaitUserChooseCharactor */

/*-------------------------------------------------------------------------------------------------------------------*/
/* Wait for ??? */
static void UserApp1SM_Idle(void)
{
	/* Button0 used to open channel */
	if(WasButtonPressed(BUTTON0))
	{
		ButtonAcknowledge(BUTTON0);
		
		if(bMaster)
		{
			AntOpenChannelNumber(ANT_CHANNEL_USERAPP_MASTER);
		}
		else
		{
			AntOpenChannelNumber(ANT_CHANNEL_USERAPP_SLAVE);
		}

		UserApp1_u32Timeout = G_u32SystemTime1ms;
		
		/* Goto UserApp1SM_WaitChannelOpen state */
		UserApp1_StateMachine = UserApp1SM_WaitChannelOpen;
	}
	
	/* Hold Butotn3 for 2s to retuen restart */
	if(IsButtonHeld(BUTTON3, 2000))
	{
		if(bMaster)
		{
			AntUnassignChannelNumber(ANT_CHANNEL_USERAPP_MASTER);
		}
		else
		{
			AntUnassignChannelNumber(ANT_CHANNEL_USERAPP_SLAVE);
		}
		
		UserApp1_u32Timeout = G_u32SystemTime1ms;
		
		/* Goto UserApp1SM_WaitChannelUnassign state */
		UserApp1_StateMachine = UserApp1SM_WaitChannelUnassign;
	}
} /* end UserApp1SM_Idle() */

/*-------------------------------------------------------------------------------------------------------------------*/
/* Wait channel open */
static void UserApp1SM_WaitChannelOpen(void)
{
	if(bMaster) // Master Open
	{
		if( AntRadioStatusChannel(ANT_CHANNEL_USERAPP_MASTER) == ANT_OPEN )
		{
			LCDMessage(LINE2_START_ADDR, "Ready:     YES      ");
			
			/* Goto UserApp1SM_ChannelOpen_Master state */
			UserApp1_StateMachine = UserApp1SM_ChannelOpen_Master;
		}
	}
	else // Slave Open
	{
		if( AntRadioStatusChannel(ANT_CHANNEL_USERAPP_SLAVE) == ANT_OPEN )
		{
			LCDMessage(LINE2_START_ADDR, "Ready:     YES      ");
			
			/* Goto UserApp1SM_ChannelOpen_Slave state */
			UserApp1_StateMachine = UserApp1SM_ChannelOpen_Slave;
		}
	}

	/* Check for timeout */
	if( IsTimeUp(&UserApp1_u32Timeout, 3000) )
	{
		LCDMessage(LINE2_START_ADDR, "Ready:     NO       ");
		
		UserApp1_StateMachine = UserApp1SM_Idle;
	}
} /* end UserApp1SM_WaitChannelOpen() */

/*-------------------------------------------------------------------------------------------------------------------*/
/* Master channel open */
static void UserApp1SM_ChannelOpen_Master(void)
{
	static u8 au8Message[] = {0, 0, 0, 0, 0, 0, 0, 0};
	static u8 u8State = 0;
	static u16 u16TimeCount = SeekTime;
	static u8 u8DisplayTime = 250;
	u8 au8TimeDisplay[] = "  s";
	
	/* AntReadData() */
	if(AntReadAppMessageBuffer())
	{
		if(G_eAntApiCurrentMessageClass == ANT_TICK)
		{
			AntQueueAcknowledgedMessage(ANT_CHANNEL_USERAPP_MASTER, au8Message);
		}
		
		/* Get Seeker's message */
		if(G_eAntApiCurrentMessageClass == ANT_DATA)
		{
			/* u8State = 0 means the first connect, so start game */
			if(u8State == 0)
			{
				u8State = 1;
	
				LCDCommand(LCD_CLEAR_CMD);
				LCDMessage(LINE1_START_ADDR, "     Game Start!!    ");
				LCDMessage(LINE2_START_ADDR, "   Now! Hide!        ");
			}
			
			/* u8State = 2 means seeker start seeking,
			G_au8AntApiCurrentMessageBytes[0] is message about distance from seeker */
			if(u8State == 2)
			{
				if(G_au8AntApiCurrentMessageBytes[0] > 90)
				{
					LedOffAll();
				}
				else if(G_au8AntApiCurrentMessageBytes[0] > 86)
				{
					LedOff(RED);
					LedOff(ORANGE);
					LedOff(YELLOW);
					LedOff(GREEN);
					LedOff(CYAN);
					LedOff(BLUE);
					LedOff(PURPLE);
					LedOn(WHITE);
				}
				else if(G_au8AntApiCurrentMessageBytes[0] > 82)
				{
					LedOff(RED);
					LedOff(ORANGE);
					LedOff(YELLOW);
					LedOff(GREEN);
					LedOff(CYAN);
					LedOff(BLUE);
					LedOn(PURPLE);
					LedOn(WHITE);
				}
				else if(G_au8AntApiCurrentMessageBytes[0] > 78)
				{
					LedOff(RED);
					LedOff(ORANGE);
					LedOff(YELLOW);
					LedOff(GREEN);
					LedOff(CYAN);
					LedOn(BLUE);
					LedOn(PURPLE);
					LedOn(WHITE);
				}
				else if(G_au8AntApiCurrentMessageBytes[0] > 74)
				{
					LedOff(RED);
					LedOff(ORANGE);
					LedOff(YELLOW);
					LedOff(GREEN);
					LedOn(CYAN);
					LedOn(BLUE);
					LedOn(PURPLE);
					LedOn(WHITE);
				}
				else if(G_au8AntApiCurrentMessageBytes[0] > 70)
				{
					LedOff(RED);
					LedOff(ORANGE);
					LedOff(YELLOW);
					LedOn(GREEN);
					LedOn(CYAN);
					LedOn(BLUE);
					LedOn(PURPLE);
					LedOn(WHITE);
				}
				else if(G_au8AntApiCurrentMessageBytes[0] > 66)
				{
					LedOff(RED);
					LedOff(ORANGE);
					LedOn(YELLOW);
					LedOn(GREEN);
					LedOn(CYAN);
					LedOn(BLUE);
					LedOn(PURPLE);
					LedOn(WHITE);
				}
				else if(G_au8AntApiCurrentMessageBytes[0] > 62)
				{
					LedOff(RED);
					LedOn(ORANGE);
					LedOn(YELLOW);
					LedOn(GREEN);
					LedOn(CYAN);
					LedOn(BLUE);
					LedOn(PURPLE);
					LedOn(WHITE);
				}
				else if(G_au8AntApiCurrentMessageBytes[0] > 40) // Find!
				{
					u8State = 3;
					
					LedOffAll();
					
					LCDCommand(LCD_CLEAR_CMD);
					LCDMessage(LINE1_START_ADDR, "     Game Finish!!   ");
					LCDMessage(LINE2_START_ADDR, "     You  Lose ...   ");
				}
			}
		}
	} /* end AntReadData() */
	
	/* u8State = 1 means both seeker and master is ready (Open channel ready)
	So game start to count down. When finish, turn u8State to 2 and start seeking */
	if(u8State == 1)
	{
		if(bTimeCountDown())
		{
			LCDCommand(LCD_CLEAR_CMD);
			LCDMessage(LINE1_START_ADDR, "Seeker is coming !!");
			LCDMessage(LINE2_START_ADDR, "Be quiet !!        ");
			
			u8State = 2;
		}
	}
	
	/* u8State = 2 means seeker start seeking.
	So start time count down, if time = 0 and seeker don't find hider
	game over */
	if(u8State == 2)
	{
		if(u16TimeCount-- > 0)
		{
			if(u8DisplayTime-- == 0)
			{
				u8DisplayTime = 250;
				
				au8TimeDisplay[0] = u16TimeCount / 10000 +48;
				au8TimeDisplay[1] = (u16TimeCount / 1000) % 10 +48;
				
				LCDMessage(LINE2_START_ADDR + 15, au8TimeDisplay);
			}
		}
		else
		{
			u8State = 3;
			
			LedOffAll();
			
			
			LCDCommand(LCD_CLEAR_CMD);
			LCDMessage(LINE1_START_ADDR, "     Game Finish!!   ");
			LCDMessage(LINE2_START_ADDR, "     You  Win !!!     ");
		}
	}
	
	/* u8State = 3 means game over. So give 3s blink and then close channel */
	if(u8State == 3)
	{
		if(bGameOver())
		{
			u16TimeCount = SeekTime;
			u8DisplayTime = 250;
			u8State = 0;
			
			AntCloseChannelNumber(ANT_CHANNEL_USERAPP_MASTER);
			UserApp1_u32Timeout = G_u32SystemTime1ms;
			UserApp1_StateMachine = UserApp1SM_WaitChannelClose;
		}
	}
	
	/*---------------Button Finish Game------------------*/
	if( (u8State >= 2) || (u8State == 0) )
	{
		if(IsButtonHeld(BUTTON3, 2000))
		{
			u8State = 4;
		}
	}
	
	if(u8State == 4)
	{
		LedOffAll();
		
		u16TimeCount = SeekTime;
		u8DisplayTime = 250;
		u8State = 0;
		
		AntCloseChannelNumber(ANT_CHANNEL_USERAPP_MASTER);
		UserApp1_u32Timeout = G_u32SystemTime1ms;
		UserApp1_StateMachine = UserApp1SM_WaitChannelClose;
	}
	/*---------------------END-----------------------------*/
}/* end UserApp1SM_ChannelOpen_Master() */

/*-------------------------------------------------------------------------------------------------------------------*/
/* Slave channel open */
static void UserApp1SM_ChannelOpen_Slave(void)
{
	static u8 au8Message[]  = {0, 0, 0, 0, 0, 0, 0, 0};
	static u16 u16TimeCount = SeekTime;
	static u8 u8DisplayTime = 250;
	u8 au8TimeDisplay[] = "  s";
	static u8 u8Test = 0;
	static u8 u8State = 0;
	
	/* Always check for ANT messages */
	if(AntReadAppMessageBuffer())
	{		
		/* New data message: check what it is */
		if(G_eAntApiCurrentMessageClass == ANT_DATA)
		{
			if( (u8State == 0) || (u8State == 2) );
			{
				AntQueueAcknowledgedMessage(ANT_CHANNEL_USERAPP_SLAVE, au8Message);
			}
			
			/* u8State = 2 means seeking start 
			So check s8RSSI to confirm distance and send message to master */
			if(u8State == 2)
			{
				u8Test = abs(G_sAntApiCurrentMessageExtData.s8RSSI);
				au8Message[0] = u8Test;
				
				if(u8Test > 90)
				{
					LedOffAll();
				}
				else if(u8Test > 86)
				{
					LedOff(RED);
					LedOff(ORANGE);
					LedOff(YELLOW);
					LedOff(GREEN);
					LedOff(CYAN);
					LedOff(BLUE);
					LedOff(PURPLE);
					LedOn(WHITE);
				}
				else if(u8Test > 82)
				{
					LedOff(RED);
					LedOff(ORANGE);
					LedOff(YELLOW);
					LedOff(GREEN);
					LedOff(CYAN);
					LedOff(BLUE);
					LedOn(PURPLE);
					LedOn(WHITE);
				}
				else if(u8Test > 78)
				{
					LedOff(RED);
					LedOff(ORANGE);
					LedOff(YELLOW);
					LedOff(GREEN);
					LedOff(CYAN);
					LedOn(BLUE);
					LedOn(PURPLE);
					LedOn(WHITE);
				}
				else if(u8Test > 74)
				{
					LedOff(RED);
					LedOff(ORANGE);
					LedOff(YELLOW);
					LedOff(GREEN);
					LedOn(CYAN);
					LedOn(BLUE);
					LedOn(PURPLE);
					LedOn(WHITE);
				}
				else if(u8Test > 70)
				{
					LedOff(RED);
					LedOff(ORANGE);
					LedOff(YELLOW);
					LedOn(GREEN);
					LedOn(CYAN);
					LedOn(BLUE);
					LedOn(PURPLE);
					LedOn(WHITE);
				}
				else if(u8Test > 66)
				{
					LedOff(RED);
					LedOff(ORANGE);
					LedOn(YELLOW);
					LedOn(GREEN);
					LedOn(CYAN);
					LedOn(BLUE);
					LedOn(PURPLE);
					LedOn(WHITE);
				}
				else if(u8Test > 62)
				{
					LedOff(RED);
					LedOn(ORANGE);
					LedOn(YELLOW);
					LedOn(GREEN);
					LedOn(CYAN);
					LedOn(BLUE);
					LedOn(PURPLE);
					LedOn(WHITE);
				}
				else if(u8Test > 40) // Find!
				{
					u8State = 3;
					
					LedOffAll();
					
					LCDCommand(LCD_CLEAR_CMD);
					LCDMessage(LINE1_START_ADDR, "     Game Finish!!   ");
					LCDMessage(LINE2_START_ADDR, "     You  Win !!!     ");
				}
				/* End search */
			}
		}
		
		if(G_eAntApiCurrentMessageClass == ANT_TICK)
		{	
			if(G_au8AntApiCurrentMessageBytes[ANT_TICK_MSG_EVENT_CODE_INDEX] == EVENT_TRANSFER_TX_FAILED)
			{
			}
			
			if(G_au8AntApiCurrentMessageBytes[ANT_TICK_MSG_EVENT_CODE_INDEX] == EVENT_TRANSFER_TX_COMPLETED)
			{
				/* 
				   EVENT_TRANSFER_TX_COMPLETED,
				   So transerfer successfully
				   u8State = 0 means the first connection
				   start game 
				*/
				if(u8State == 0)
				{
					u8State = 1;
					
					LCDCommand(LCD_CLEAR_CMD);
					LCDMessage(LINE1_START_ADDR, "     Game Start!!    ");
					LCDMessage(LINE2_START_ADDR, "   Now! Wait!        ");
				}
			}
			
			if(G_au8AntApiCurrentMessageBytes[ANT_TICK_MSG_EVENT_CODE_INDEX] == EVENT_RX_FAIL_GO_TO_SEARCH)
			{
				/* Can't connect master, distance to far, so turn all led off */
				LedOffAll();
			}
		}
	} /* end AntReadAppMessageBuffer() */
	
	/* u8State = 1 means both seeker and master is ready (Open channel ready)
	So game start to count down. When finish, turn u8State to 2 and start seeking */
	if(u8State == 1)
	{
		if(bTimeCountDown())
		{
			LCDCommand(LCD_CLEAR_CMD);
			LCDMessage(LINE1_START_ADDR, "Find him !!!        ");
			LCDMessage(LINE2_START_ADDR, "Be quick !!        ");
			
			u8State = 2;
		}
	}
	
	/* u8State = 2 means seeker start seeking.
	So start time count down, if time = 0 and seeker don't find hider
	game over */
	if(u8State == 2)
	{
		if(u16TimeCount-- > 0)
		{
			if(u8DisplayTime-- == 0)
			{
				u8DisplayTime = 250;
				
				au8TimeDisplay[0] = u16TimeCount / 10000 +48;
				au8TimeDisplay[1] = (u16TimeCount / 1000) % 10 +48;
				
				LCDMessage(LINE2_START_ADDR + 15, au8TimeDisplay);
			}
		}
		else
		{
			u8State = 3;
			
			LedOffAll();
			
			
			LCDCommand(LCD_CLEAR_CMD);
			LCDMessage(LINE1_START_ADDR, "     Game Finish!!   ");
			LCDMessage(LINE2_START_ADDR, "     You  Lose ...   ");
		}
	}
	
	/* u8State = 3 means game over. So give 3s blink and then close channel */
	if(u8State == 3)
	{	
		if(bGameOver())
		{
			u16TimeCount = SeekTime;
			u8DisplayTime = 250;
			u8State = 0;
			
			AntCloseChannelNumber(ANT_CHANNEL_USERAPP_SLAVE);
			UserApp1_u32Timeout = G_u32SystemTime1ms;
			UserApp1_StateMachine = UserApp1SM_WaitChannelClose;
		}
	}
	
	/*---------------Button Finish Game------------------*/
	if( (u8State >= 2) || (u8State == 0) )
	{
		if(IsButtonHeld(BUTTON3, 2000))
		{
			u8State = 4;
		}
	}
	
	if(u8State == 4)
	{
		LedOffAll();
		
		u16TimeCount = SeekTime;
		u8DisplayTime = 250;
		u8State = 0;
		
		AntCloseChannelNumber(ANT_CHANNEL_USERAPP_MASTER);
		UserApp1_u32Timeout = G_u32SystemTime1ms;
		UserApp1_StateMachine = UserApp1SM_WaitChannelClose;
	}
	/*---------------------END-----------------------------*/
} /* end UserApp1SM_ChannelOpen_Slave() */

/*-------------------------------------------------------------------------------------------------------------------*/
/* Wait channel close */
static void UserApp1SM_WaitChannelClose(void)
{
	/* Monitor the channel status to check if channel is closed */
	if(bMaster)
	{
		if(AntRadioStatusChannel(ANT_CHANNEL_USERAPP_MASTER) == ANT_CLOSED)
		{
			AntUnassignChannelNumber(ANT_CHANNEL_USERAPP_MASTER);
			UserApp1_u32Timeout = G_u32SystemTime1ms;
			
			UserApp1_StateMachine = UserApp1SM_WaitChannelUnassign;
		}
	}
	else
	{
		if(AntRadioStatusChannel(ANT_CHANNEL_USERAPP_SLAVE) == ANT_CLOSED)
		{
			AntUnassignChannelNumber(ANT_CHANNEL_USERAPP_SLAVE);
			UserApp1_u32Timeout = G_u32SystemTime1ms;
			
			UserApp1_StateMachine = UserApp1SM_WaitChannelUnassign;
		}
	}
	
	/* Check for timeout */
	if( IsTimeUp(&UserApp1_u32Timeout, 5000) )
	{
		LedOn(RED);

		UserApp1_StateMachine = UserApp1SM_Error;
	}
} /* end UserApp1SM_WaitChannelClose() */

/*-------------------------------------------------------------------------------------------------------------------*/
/* Wait channel unassign */
static void UserApp1SM_WaitChannelUnassign(void)
{
	if(bMaster)
	{
		if(AntRadioStatusChannel(ANT_CHANNEL_USERAPP_MASTER) == ANT_UNCONFIGURED)
		{
			UserApp1_StateMachine = UserApp1SM_WaitUserChooseCharactor;
		}
	}
	else
	{
		if(AntRadioStatusChannel(ANT_CHANNEL_USERAPP_SLAVE) == ANT_UNCONFIGURED)
		{
			UserApp1_StateMachine = UserApp1SM_WaitUserChooseCharactor;
		}
	}
	
	/* Check for timeout */
	if( IsTimeUp(&UserApp1_u32Timeout, 5000) )
	{
		LedOn(RED);

		UserApp1_StateMachine = UserApp1SM_Error;
	}
} /* end UserApp1SM_WaitChannelClose() */
/*-------------------------------------------------------------------------------------------------------------------*/
/* Handle an error (for now, do nothing) */
static void UserApp1SM_Error(void)          
{
  
} /* end UserApp1SM_Error() */



/*--------------------------------------------------------------------------------------------------------------------*/
/* End of File                                                                                                        */
/*--------------------------------------------------------------------------------------------------------------------*/
