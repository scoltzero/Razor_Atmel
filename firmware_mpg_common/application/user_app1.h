/**********************************************************************************************************************
File: user_app1.h                                                                

----------------------------------------------------------------------------------------------------------------------
To start a new task using this user_app1 as a template:
1. Follow the instructions at the top of user_app1.c
2. Use ctrl-h to find and replace all instances of "user_app1" with "yournewtaskname"
3. Use ctrl-h to find and replace all instances of "UserApp1" with "YourNewTaskName"
4. Use ctrl-h to find and replace all instances of "USER_APP1" with "YOUR_NEW_TASK_NAME"
5. Add #include yournewtaskname.h" to configuration.h
6. Add/update any special configurations required in configuration.h (e.g. peripheral assignment and setup values)
7. Delete this text (between the dashed lines)
----------------------------------------------------------------------------------------------------------------------

Description:
Header file for user_app1.c

**********************************************************************************************************************/

#ifndef __USER_APP1_H
#define __USER_APP1_H

/**********************************************************************************************************************
Type Definitions
**********************************************************************************************************************/


/**********************************************************************************************************************
Constants / Definitions
**********************************************************************************************************************/
/* Required constants for ANT channel configuration */
/* Required constants for ANT channel configuration */
#define ANT_CHANNEL_TYPE_USERAPP_MASTER        CHANNEL_TYPE_MASTER    /* ANT MASTER */
#define ANT_CHANNEL_TYPE_USERAPP_SLAVE         CHANNEL_TYPE_SLAVE     /* ANT SLAVE */
#define ANT_CHANNEL_USERAPP_MASTER              ANT_CHANNEL_0         // Channel 0 - 7
#define ANT_CHANNEL_USERAPP_SLAVE               ANT_CHANNEL_0         // Channel 0 - 7

#define ANT_DEVICEID_DEC_USERAPP        (u8)135
#define ANT_DEVICEID_LO_USERAPP         (u8)0x87              // Low byte of two-byte Device #
#define ANT_DEVICEID_HI_USERAPP         (u8)0x00              // High byte of two-byte Device #
#define ANT_DEVICE_TYPE_USERAPP         (u8)1                 // 1 - 255
#define ANT_TRANSMISSION_TYPE_USERAPP   (u8)1                 // 1-127 (MSB is pairing bit)

#define ANT_CHANNEL_PERIOD_DEC_USERAPP  (u16)8192
#define ANT_CHANNEL_PERIOD_HEX_USERAPP  (u16)0x2000
#define ANT_CHANNEL_PERIOD_LO_USERAPP   (u8)0x00              // Low byte of two-byte channel period 0x0001 - 0x7fff
#define ANT_CHANNEL_PERIOD_HI_USERAPP   (u8)0x20              // High byte of two-byte channel period 

#define ANT_FREQUENCY_USERAPP           (u8)50                // 2400MHz + this number 0 - 99
#define ANT_TX_POWER_USERAPP            RADIO_TX_POWER_4DBM   // RADIO_TX_POWER_0DBM, RADIO_TX_POWER_MINUS5DBM, RADIO_TX_POWER_MINUS10DBM, RADIO_TX_POWER_MINUS20DBM

#define TimeCount                       (u16)10                // 1000 means 1 second
#define SeekTime                        (u16)60000
/**********************************************************************************************************************
Function Declarations
**********************************************************************************************************************/

/*--------------------------------------------------------------------------------------------------------------------*/
/* Public functions                                                                                                   */
/*--------------------------------------------------------------------------------------------------------------------*/
static void LedOnAll(void);
static void LedOffAll(void);
static void LedToggleAll(bool);
static bool bTimeCountDown(void);
static bool bGameOver(void);

/*--------------------------------------------------------------------------------------------------------------------*/
/* Protected functions                                                                                                */
/*--------------------------------------------------------------------------------------------------------------------*/
void UserApp1Initialize(void);
void UserApp1RunActiveState(void);


/*--------------------------------------------------------------------------------------------------------------------*/
/* Private functions                                                                                                  */
/*--------------------------------------------------------------------------------------------------------------------*/

/***********************************************************************************************************************
State Machine Declarations
***********************************************************************************************************************/
static void UserApp1SM_WaitUserChooseCharactor(void);

static void UserApp1SM_ChannelAssign_Master(void);
static void UserApp1SM_ChannelAssign_Slave(void);
static void UserApp1SM_WaitChannelAssign(void);

static void UserApp1SM_Idle(void);

static void UserApp1SM_ChannelOpen_Master(void);
static void UserApp1SM_ChannelOpen_Slave(void);
static void UserApp1SM_WaitChannelOpen(void);

static void UserApp1SM_WaitChannelClose(void);
static void UserApp1SM_WaitChannelUnassign(void);

static void UserApp1SM_Error(void);

#endif /* __USER_APP1_H */


/*--------------------------------------------------------------------------------------------------------------------*/
/* End of File                                                                                                        */
/*--------------------------------------------------------------------------------------------------------------------*/

