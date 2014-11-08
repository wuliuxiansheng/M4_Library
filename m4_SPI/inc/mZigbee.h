//-----------------------------------------------------------------------------
// MAEVARM M4 STM32F373 mZigbee Peripheral 
// version: 1.0
// date: April 5, 2013
// author: Neel Shah (neels@seas.upenn.edu)
// Based on Anaren Zigbee Module ZM Library
// SPI is actually all done through I2C communications in this 
//-----------------------------------------------------------------------------


#ifndef m_zigbee__
#define m_zigbee__
//#include "mGeneral.h"

#define zIntDisable  NVIC_DisableIRQ(EXTI15_10_IRQn)
#define zIntEnable   NVIC_EnableIRQ(EXTI15_10_IRQn)


#define zPinkON     zPinkon();
#define zPinkOFF    zPinkoff();
#define zPinkTOGGLE zPinktoggle();

#define DEFAULT_ENDPOINT        0x42 
#define DEFAULT_PROFILE_ID      0x4242
#define DEVICE_ID               0x4567
#define DEVICE_VERSION          0x89
//Values for latencyRequested field of struct applicationConfiguration. Not used in Simple API.
#define LATENCY_NORMAL          0
#define LATENCY_FAST_BEACONS    1
#define LATENCY_SLOW_BEACONS    2

#define SRDY_IS_HIGH() (GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_13))
#define SRDY_IS_LOW() (!GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_13))
//i2c to spi converter defines 

#define zSpi       	0x04 //slave select 2 active yellow
#define zSpiConfig 	0xF0 
#define zSpiInit   	0x00 //Clock polarity 0 phase 0 msb first 1843khz
#define zGPIOwr    	0xF4
#define zGPIOrd    	0xF5
#define zGPIOen    	0xF6
#define zGPIOmd   	0xF7 
#define pinkPin   	0x01
#define resetPin  	0x02
#define zYellow 	  0x04
#define zSs     	  0x08

extern uint8_t moduleResult;

struct moduleConfiguration
{
    uint8_t deviceType;
    uint32_t channelMask;
    uint16_t panId;
    uint16_t endDevicePollRate;
    uint8_t startupOptions;
    uint8_t securityMode;
    uint8_t* securityKey;
};

extern const struct moduleConfiguration DEFAULT_MODULE_CONFIGURATION_COORDINATOR;
extern const struct moduleConfiguration DEFAULT_MODULE_CONFIGURATION_ROUTER;
extern const struct moduleConfiguration DEFAULT_MODULE_CONFIGURATION_END_DEVICE;

#define AF_MAC_ACK                      0x00    //Require Acknowledgement from next device on route
#define AF_APS_ACK                      0x10    //Require Acknowledgement from final destination (if using AFZDO)

#define AF_INCOMING_MESSAGE_CLUSTER_LSB_FIELD           (SRSP_PAYLOAD_START+2)
#define AF_INCOMING_MESSAGE_CLUSTER_MSB_FIELD           (SRSP_PAYLOAD_START+3)
#define AF_INCOMING_MESSAGE_CLUSTER()                   (CONVERT_TO_INT(zmBuf[AF_INCOMING_MESSAGE_CLUSTER_LSB_FIELD], zmBuf[AF_INCOMING_MESSAGE_CLUSTER_MSB_FIELD]))
#define AF_INCOMING_MESSAGE_SHORT_ADDRESS_LSB_FIELD     (SRSP_PAYLOAD_START+4)
#define AF_INCOMING_MESSAGE_SHORT_ADDRESS_MSB_FIELD     (SRSP_PAYLOAD_START+5)
#define AF_INCOMING_MESSAGE_SHORT_ADDRESS()             (CONVERT_TO_INT(zmBuf[AF_INCOMING_MESSAGE_SHORT_ADDRESS_LSB_FIELD], zmBuf[AF_INCOMING_MESSAGE_SHORT_ADDRESS_MSB_FIELD]))
#define AF_INCOMING_MESSAGE_LQI_FIELD                   (SRSP_PAYLOAD_START+9)
#define AF_INCOMING_MESSAGE_PAYLOAD_LEN_FIELD           (SRSP_PAYLOAD_START+16)
#define AF_INCOMING_MESSAGE_PAYLOAD_START_FIELD         (SRSP_PAYLOAD_START+17)
#define IS_AF_INCOMING_MESSAGE()                        (CONVERT_TO_INT(zmBuf[SRSP_CMD_LSB_FIELD], zmBuf[SRSP_CMD_MSB_FIELD]) == AF_INCOMING_MSG)
#define AF_INCOMING_MESSAGE_PAYLOAD_LEN                 (zmBuf[AF_INCOMING_MESSAGE_PAYLOAD_LEN_FIELD])

#define AF_INCOMING_MESSAGE_EXT_CLUSTER_LSB_FIELD       (SRSP_PAYLOAD_START+2)
#define AF_INCOMING_MESSAGE_EXT_CLUSTER_MSB_FIELD       (SRSP_PAYLOAD_START+3)
#define AF_INCOMING_MESSAGE_EXT_CLUSTER()               (CONVERT_TO_INT(zmBuf[AF_INCOMING_MESSAGE_EXT_CLUSTER_LSB_FIELD], zmBuf[AF_INCOMING_MESSAGE_EXT_CLUSTER_MSB_FIELD]))
#define AF_INCOMING_MESSAGE_EXT_ADDRESSING_MODE_FIELD   (SRSP_PAYLOAD_START+4)
#define AF_INCOMING_MESSAGE_EXT_SHORT_ADDRESS_LSB_FIELD (SRSP_PAYLOAD_START+5)  //IF USING SHORT ADDRESSING
#define AF_INCOMING_MESSAGE_EXT_SHORT_ADDRESS_MSB_FIELD (SRSP_PAYLOAD_START+6)
#define AF_INCOMING_MESSAGE_EXT_SHORT_ADDRESS()         (CONVERT_TO_INT(zmBuf[AF_INCOMING_MESSAGE_EXT_SHORT_ADDRESS_LSB_FIELD], zmBuf[AF_INCOMING_MESSAGE_EXT_SHORT_ADDRESS_MSB_FIELD]))
#define AF_INCOMING_MESSAGE_EXT_ADDRESS_START_FIELD     (SRSP_PAYLOAD_START+5)  //IF USING LONG ADDRESSING
#define AF_INCOMING_MESSAGE_EXT_LQI_FIELD               (SRSP_PAYLOAD_START+18)
#define AF_INCOMING_MESSAGE_EXT_TIMESTAMP_START_FIELD   (SRSP_PAYLOAD_START+20) 
#define AF_INCOMING_MESSAGE_EXT_PAYLOAD_LEN_LSB_FIELD   (SRSP_PAYLOAD_START+25)
#define AF_INCOMING_MESSAGE_EXT_PAYLOAD_LEN_MSB_FIELD   (SRSP_PAYLOAD_START+26)
#define AF_INCOMING_MESSAGE_EXT_PAYLOAD_START_FIELD     (SRSP_PAYLOAD_START+27)
#define AF_INCOMING_MESSAGE_EXT_LENGTH()                (CONVERT_TO_INT(zmBuf[AF_INCOMING_MESSAGE_EXT_PAYLOAD_LEN_LSB_FIELD], zmBuf[AF_INCOMING_MESSAGE_EXT_PAYLOAD_LEN_MSB_FIELD]))

#define IS_AF_INCOMING_MESSAGE_EXT()                    (CONVERT_TO_INT(zmBuf[SRSP_CMD_LSB_FIELD], zmBuf[SRSP_CMD_MSB_FIELD]) == AF_INCOMING_MSG_EXT)

#define AF_DATA_RETRIEVE_SRSP_STATUS_FIELD              (SRSP_PAYLOAD_START)
#define AF_DATA_RETRIEVE_SRSP_LENGTH_FIELD              (SRSP_PAYLOAD_START+1)
#define AF_DATA_RETRIEVE_SRSP_PAYLOAD_START_FIELD       (SRSP_PAYLOAD_START+2)

#define AF_DATA_CONFIRM_STATUS_FIELD                    (SRSP_PAYLOAD_START)
#define AF_DATA_CONFIRM_ENDPOINT_FIELD                  (SRSP_PAYLOAD_START+1)
#define AF_DATA_CONFIRM_TRANS_ID_FIELD                  (SRSP_PAYLOAD_START+2)

#define AF_DATA_CONFIRM_STATUS                          (zmBuf[AF_DATA_CONFIRM_STATUS_FIELD])
#define AF_DATA_CONFIRM_TRANS_ID                        (zmBuf[AF_DATA_CONFIRM_TRANS_ID_FIELD])


#define DESTINATION_ADDRESS_MODE_SHORT 2
#define DESTINATION_ADDRESS_MODE_LONG 3
#define INTRA_PAN 0  //same PAN
#define AF_DATA_REQUEST_MAX_PAYLOAD_LENGTH      MAXIMUM_PAYLOAD_LENGTH
#define AF_DATA_REQUEST_EXT_MAX_PAYLOAD_LENGTH 230
#define MAXIMUM_AF_DATA_RETRIEVE_PAYLOAD_LENGTH 248
#define AF_DATA_REQUEST_EXT_MAX_TOTAL_PAYLOAD_LENGTH 600

//Module Commands
// SYS Interface
#define ZB_WRITE_CONFIGURATION          0x2605
#define ZB_READ_CONFIGURATION           0x2604
#define SYS_GPIO                        0x210E
#define SYS_VERSION                     0x2102
#define SYS_RANDOM                      0x210C
#define SYS_NV_READ                     0x2108
#define SYS_NV_WRITE                    0x2109
#define SYS_RESET_IND                   0x4180
#define SYS_STACK_TUNE                  0x210F
#define SYS_SET_TX_POWER                0x2114
// Simple API commands
#define ZB_APP_REGISTER_REQUEST         0x260A
#define ZB_APP_START_REQUEST            0x2600
#define ZB_SEND_DATA_REQUEST            0x2603
#define ZB_SEND_DATA_CONFIRM            0x4683
#define ZB_GET_DEVICE_INFO              0x2606
#define ZB_FIND_DEVICE_REQUEST          0x2607
#define ZB_FIND_DEVICE_CONFIRM          0x4685
#define ZB_PERMIT_JOINING_REQUEST       0x2608
#define ZB_START_CONFIRM                0x4680 //will receive this asynchronously
#define ZB_RECEIVE_DATA_INDICATION      0x4687 //will receive this asynchronously
// AF commands:
#define AF_REGISTER                     0x2400
#define AF_DATA_REQUEST                 0x2401
#define AF_DATA_REQUEST_EXT             0x2402
#define AF_DATA_STORE                   0x2411
#define AF_DATA_RETRIEVE                0x2412
#define AF_DATA_CONFIRM                 0x4480
#define AF_INCOMING_MSG                 0x4481 //will receive this asynchronously
#define AF_INCOMING_MSG_EXT             0x4482
// ZDO commands:
#define ZDO_STARTUP_FROM_APP            0x2540
#define ZDO_IEEE_ADDR_REQ               0x2501
#define ZDO_IEEE_ADDR_RSP               0x4581
#define ZDO_NWK_ADDR_REQ                0x2500
#define ZDO_NWK_ADDR_RSP                0x4580
#define ZDO_USER_DESC_REQ               0x2508
#define ZDO_USER_DESC_RSP               0x4588
#define ZDO_USER_DESC_SET               0x250B
#define ZDO_USER_DESC_CONF              0x4589 //will receive this asynchronously
#define ZDO_NODE_DESC_REQ               0x2502
#define ZDO_NODE_DESC_RSP               0x4582
#define ZDO_STATE_CHANGE_IND            0x45C0 //will receive this asynchronously
#define ZDO_END_DEVICE_ANNCE_IND        0x45C1 //will receive this asynchronously
// UTIL commands:
#define UTIL_ADDRMGR_NWK_ADDR_LOOKUP    0x2741
// Other commands:
#define ERROR_SRSP                      0x6000 // returned if SREQ is unknown
// Received in SRSP message
#define SRSP_STATUS_SUCCESS             0x00
// expected Response message types
#define NO_RESPONSE_EXPECTED            0x00

#define DEFAULT_CHANNEL_MASK		(CHANNEL_MASK_11 | CHANNEL_MASK_14 | CHANNEL_MASK_17 | CHANNEL_MASK_20 | CHANNEL_MASK_23)
#define DEFAULT_POLL_RATE_MS		2000
#define DEFAULT_STARTUP_OPTIONS		(STARTOPT_CLEAR_CONFIG + STARTOPT_CLEAR_STATE)
#define GENERIC_APPLICATION_CONFIGURATION   0
/** The module operation completed successfully */
#define MODULE_SUCCESS                  (0x00)
/** A parameter was incorrect */
#define INVALID_PARAMETER	            (0x31)
/** The module driver timed out and could not process the request */
#define TIMEOUT                         (0x32)
/** The length of a parameter was incorrect */
#define INVALID_LENGTH                  (0x33)
/** The Zigbee Cluster parameter was incorrect */
#define INVALID_CLUSTER                 (0x34)
//** The Module did not respond to chip select being asserted.
#define ZM_PHY_CHIP_SELECT_TIMEOUT      (0x35)
//** The Module did not respond with a Synchronous Response in time
#define ZM_PHY_SRSP_TIMEOUT             (0x36)
//** The Synchronous Response returned did not match the Synchronous Request
#define ZM_PHY_INCORRECT_SRSP           (0x37)
//** The module is not configured correctly. See startModule() in module_utilities.c */
#define ZM_INVALID_MODULE_CONFIGURATION (0x38)
//** An error occured that doesn't fit into one of the other categories
#define ZM_PHY_OTHER_ERROR              (0x3F)

#define SYS_RESET_IND_START_FIELD       (SRSP_PAYLOAD_START)
#define SYS_RESET_IND_PRODUCTID_FIELD   (SYS_RESET_IND_START_FIELD + 2)
#define SYS_VERSION_RESULT_START_FIELD (SRSP_PAYLOAD_START)
#define SYS_RANDOM_RESULT_LSB_FIELD (SRSP_PAYLOAD_START)
#define SYS_RANDOM_RESULT_MSB_FIELD (SRSP_PAYLOAD_START+1)
#define SYS_RANDOM_RESULT()           (CONVERT_TO_INT(zmBuf[SYS_RANDOM_RESULT_LSB_FIELD], zmBuf[SYS_RANDOM_RESULT_MSB_FIELD]))
//channel
#define CHANNEL_MIN                             11
#define CHANNEL_MAX                             26
#define CHANNEL_MASK_11                     0x800
#define CHANNEL_MASK_12                    0x1000
#define CHANNEL_MASK_13                    0x2000
#define CHANNEL_MASK_14                    0x4000
#define CHANNEL_MASK_15                    0x8000
#define CHANNEL_MASK_16                   0x10000
#define CHANNEL_MASK_17                   0x20000
#define CHANNEL_MASK_18                   0x40000
#define CHANNEL_MASK_19                   0x80000
#define CHANNEL_MASK_20                  0x100000
#define CHANNEL_MASK_21                  0x200000
#define CHANNEL_MASK_22                  0x400000
#define CHANNEL_MASK_23                  0x800000
#define CHANNEL_MASK_24                 0x1000000
#define CHANNEL_MASK_25                 0x2000000
#define CHANNEL_MASK_26                 0x4000000
#define ANY_CHANNEL_MASK                0x7FFF800  //Channel 11-26 bitmask
#define MIN_CHANNEL_MASK                CHANNEL_MASK_11
#define MAX_CHANNEL_MASK                ANY_CHANNEL_MASK
#define IS_VALID_CHANNEL(channel)       ((channel>=CHANNEL_MIN) && (channel<=CHANNEL_MAX))
//PAN 
#define ANY_PAN                         0xFFFF
#define MAX_PANID                       0xFFF7
#define IS_VALID_PANID(id)              (id<=MAX_PANID)
//device information paramter DIP
#define DIP_STATE                       0x00
#define DIP_MAC_ADDRESS                 0x01
#define DIP_SHORT_ADDRESS               0x02
#define DIP_PARENT_SHORT_ADDRESS        0x03
#define DIP_PARENT_MAC_ADDRESS          0x04
#define DIP_CHANNEL                     0x05
#define DIP_PANID                       0x06
#define DIP_EXTENDED_PANID              0x07
#define MAX_DEVICE_INFORMATION_PROPERTY 0x07
//Field Lengths
#define DIP_MAC_ADDRESS_LENGTH          8
#define DIP_SHORT_ADDRESS_LENGTH        2
#define DIP_PARENT_SHORT_ADDRESS_LENGTH 2
#define DIP_PARENT_MAC_ADDRESS_LENGTH   8
#define DIP_CHANNEL_LENGTH              1
#define DIP_PANID_LENGTH                2
#define DIP_EXTENDED_PANID_LENGTH       8
//Values for DIP_STATE:
#define DEV_HOLD                        0
#define DEV_INIT                        1
#define DEV_NWK_DISC                    2
#define DEV_NWK_JOINING                 3
#define DEV_NWK_REJOIN                  4
#define DEV_END_DEVICE_UNAUTH           5
#define DEV_END_DEVICE                  6
#define DEV_ROUTER                      7
#define DEV_COORD_STARTING              8
#define DEV_ZB_COORD                    9
#define DEV_NWK_ORPHAN                  10
#define MAX_DEVICE_STATE                10
#define SRSP_DIP_VALUE_FIELD (SRSP_HEADER_SIZE+1) //index in zmBuf[] of the start of the Device Information Property field. LSB is first.
#define IS_VALID_DEVICE_STATE(state)    (state <= MAX_DEVICE_STATE)
#define SRSP                            0x4000
#define MODULE_COMMAND()                (CONVERT_TO_INT(zmBuf[SRSP_CMD_LSB_FIELD], zmBuf[SRSP_CMD_MSB_FIELD]))
#define IS_SYS_RESET_IND()              (MODULE_COMMAND() == SYS_RESET_IND)
#define IS_SYS_VERSION_SRSP()           (MODULE_COMMAND() == (SYS_VERSION + SRSP))

#define SINGLE_DEVICE_RESPONSE                          0
#define INCLUDE_ASSOCIATED_DEVICES                      1
//for ZDO_END_DEVICE_ANNCE_IND
#define FROM_ADDRESS_LSB                                (SRSP_PAYLOAD_START)
#define FROM_ADDRESS_MSB                                (SRSP_PAYLOAD_START+1)
#define SRC_ADDRESS_LSB                                 (SRSP_PAYLOAD_START+2)
#define SRC_ADDRESS_MSB                                 (SRSP_PAYLOAD_START+3)
#define IS_ZDO_END_DEVICE_ANNCE_IND()                   (CONVERT_TO_INT(zmBuf[SRSP_CMD_LSB_FIELD], zmBuf[SRSP_CMD_MSB_FIELD]) == ZDO_END_DEVICE_ANNCE_IND)
#define GET_ZDO_END_DEVICE_ANNCE_IND_FROM_ADDRESS()     (CONVERT_TO_INT(zmBuf[FROM_ADDRESS_LSB], zmBuf[FROM_ADDRESS_MSB]))
#define GET_ZDO_END_DEVICE_ANNCE_IND_SRC_ADDRESS()      (CONVERT_TO_INT(zmBuf[SRC_ADDRESS_LSB], zmBuf[SRC_ADDRESS_MSB]))
#define ZDO_END_DEVICE_ANNCE_IND_MAC_START_FIELD        (SRSP_PAYLOAD_START+4)
#define ZDO_END_DEVICE_ANNCE_IND_CAPABILITIES_FIELD                  (SRSP_PAYLOAD_START + 9)
#define ZDO_END_DEVICE_ANNCE_IND_CAPABILITIES_FLAG_DEVICETYPE_ROUTER     0x02
#define ZDO_END_DEVICE_ANNCE_IND_CAPABILITIES_FLAG_MAINS_POWERED         0x04
#define ZDO_END_DEVICE_ANNCE_IND_CAPABILITIES_FLAG_RX_ON_WHEN_IDLE       0x08
#define ZDO_END_DEVICE_ANNCE_IND_CAPABILITIES_FLAG_SECURITY_CAPABILITY   0x40

#define ZDO_USER_DESC_RSP_STATUS_FIELD                  (SRSP_PAYLOAD_START + 2)
#define ZDO_NODE_DESC_RSP_STATUS_FIELD                  (SRSP_PAYLOAD_START + 2)

#define ZDO_IEEE_ADDR_RSP_STATUS_FIELD                  (SRSP_PAYLOAD_START)
#define ZDO_NWK_ADDR_RSP_STATUS_FIELD                   (SRSP_PAYLOAD_START)

#define ZB_READ_CONFIGURATION_START_OF_VALUE_FIELD    SRSP_PAYLOAD_START + 3

#define ZCD_NV_USERDESC                 0x81
#define ZCD_NV_USERDESC_LEN             17
#define ZCD_NV_CHANLIST                 0x84
#define ZCD_NV_CHANLIST_LEN             4
#define ZCD_NV_PANID                    0x83
#define ZCD_NV_PANID_LEN                2
#define ZCD_NV_STARTUP_OPTION           0x03
#define ZCD_NV_STARTUP_OPTION_LEN       1
#define ZCD_NV_LOGICAL_TYPE             0x87
#define ZCD_NV_LOGICAL_TYPE_LEN         1
#define ZCD_NV_ZDO_DIRECT_CB            0x8F
#define ZCD_NV_ZDO_DIRECT_CB_LEN        1
#define ZCD_NV_POLL_RATE                0x24
#define ZCD_NV_POLL_RATE_LEN            2
#define ZCD_NV_QUEUED_POLL_RATE         0x25
#define ZCD_NV_QUEUED_POLL_RATE_LEN     2
#define ZCD_NV_RESPONSE_POLL_RATE       0x26
#define ZCD_NV_RESPONSE_POLL_RATE_LEN   2
//For Security:
#define ZCD_NV_PRECFGKEY                0x62
#define ZCD_NV_PRECFGKEY_LEN            16
#define ZCD_NV_PRECFGKEYS_ENABLE        0x63
#define ZCD_NV_PRECFGKEYS_ENABLE_LEN    1
#define ZCD_NV_SECURITY_MODE            0x64
#define ZCD_NV_SECURITY_MODE_LEN        1
// Security Modes:
#define SECURITY_MODE_OFF                0
#define SECURITY_MODE_PRECONFIGURED_KEYS 1
#define SECURITY_MODE_COORD_DIST_KEYS    2
//  Non-volatile memory item storage
#define SYS_NV_READ_STATUS_FIELD        (SRSP_PAYLOAD_START)
#define SYS_NV_READ_RESULT_START_FIELD  (SRSP_PAYLOAD_START + 2)
#define SYS_NV_WRITE_STATUS_FIELD        (SRSP_PAYLOAD_START)
#define MIN_NV_ITEM                     1
#define MAX_NV_ITEM                     9
#define MAX_NV_ITEM_READ                MAX_NV_ITEM
#define MAX_NV_ITEM_USER                6
#define NV_ITEM_RESERVED                7 // Do not use

#define SYS_GPIO_READ_RESULT_FIELD   (SRSP_PAYLOAD_START)
#define LED_SET_DIRECTION     0x00
#define GPIO_SET_INPUT_MODE   0x01
#define zOFF                  0x02
#define zON                   0x03
#define zTOGGLE               0x04
#define GPIO_READ             0x05
#define GPIO_OPERATION_MAX    0x05
//GPIO pin definitions
#define ALL_LEDS              0x0F  //GPIO 0-3
#define LED0                  0x04 //co ord
#define LED1                  0x08 //router
#define LED2                  0x02 //send
#define LED3                  0x01 //error
//options for GPIO_SET_INPUT_MODE
#define GPIO_INPUT_MODE_ALL_PULL_DOWNS  0xF0
#define GPIO_INPUT_MODE_ALL_PULL_UPS    0x00
#define GPIO_INPUT_MODE_ALL_TRI_STATE   0x0F
#define GPIO_DIRECTION_ALL_INPUTS       0x00
//RF Transmit Power values, from Table 1 in cc2530 datasheet, p 21
#define RF_OUTPUT_POWER_PLUS_4_5_DBM    0xF5  //+4.5dBm
#define RF_OUTPUT_POWER_PLUS_2_5_DBM    0xE5  //+2.5dBm
#define RF_OUTPUT_POWER_PLUS_1_0_DBM    0xD5  //+1.0dBm
#define RF_OUTPUT_POWER_MINUS_0_5_DBM   0xC5  //-0.5dBm
#define RF_OUTPUT_POWER_MINUS_1_5_DBM   0xB5  //-1.5dBm
#define RF_OUTPUT_POWER_MINUS_3_0_DBM   0xA5  //-3.0dBm
#define RF_OUTPUT_POWER_MINUS_10_0_DBM  0x65  //-10.0dBm
#define RF_OUTPUT_POWER_MINUS_12_0_DBM  0x55  //-10.0dBm
#define RF_OUTPUT_POWER_MINUS_14_0_DBM  0x45  //-10.0dBm
#define RF_OUTPUT_POWER_MINUS_16_0_DBM  0x35  //-10.0dBm
#define RF_OUTPUT_POWER_MINUS_18_0_DBM  0x25  //-10.0dBm
#define RF_OUTPUT_POWER_MINUS_20_0_DBM  0x15  //-20.0dBm
#define RF_OUTPUT_POWER_MINUS_22_0_DBM  0x05  //-10.0dBm

#define COORDINATOR                     0x00
#define ROUTER                          0x01
#define END_DEVICE                      0x02
#define IS_VALID_ZIGBEE_DEVICE_TYPE(type)   ((type == COORDINATOR) || (type == ROUTER) || (type == END_DEVICE))
//  Set Startup Options
#define STARTOPT_CLEAR_CONFIG           0x01
#define STARTOPT_CLEAR_STATE            0x02
#define STARTOPT_AUTO                   0x04
//  Set Callbacks
#define CALLBACKS_DISABLED              0
#define CALLBACKS_ENABLED               1
//  Set Tx Power
#define SYS_SET_TX_POWER_RESULT_FIELD        (SRSP_PAYLOAD_START)

#define DEFAULT_RADIUS                  0x0F    //Maximum number of hops to get to destination
#define MAXIMUM_PAYLOAD_LENGTH          81      //Updated in 2.4.0: 99B w/o security, 81B w/ NWK security, 66B w/ APS security
#define ALL_DEVICES                     0xFFFF
#define ALL_ROUTERS_AND_COORDINATORS    0xFFFC


#define HANDLE_ERROR(errorCode, methodId) //be silent

#define RETURN_RESULT(operation, methodId) moduleResult = operation; return moduleResult;
#define RETURN_RESULT_IF_EXPRESSION_TRUE(expression, methodId, errorCode) if(expression){return errorCode;}
#define RETURN_INVALID_PARAMETER_IF_TRUE(_expression, _methodId)   RETURN_RESULT_IF_EXPRESSION_TRUE((_expression), (_methodId), INVALID_PARAMETER)
#define RETURN_INVALID_LENGTH_IF_TRUE(_expression, _methodId)   RETURN_RESULT_IF_EXPRESSION_TRUE((_expression), (_methodId), INVALID_LENGTH)
#define RETURN_INVALID_CLUSTER_IF_TRUE(_expression, _methodId)   RETURN_RESULT_IF_EXPRESSION_TRUE((_expression), (_methodId), INVALID_CLUSTER)
#define RETURN_RESULT_IF_FAIL(operation, methodId) moduleResult = operation;if(moduleResult!=MODULE_SUCCESS){return moduleResult;}

#define CONVERT_TO_INT(lsb,msb) ((lsb) + 0x0100*(msb))
#define LSB(num) ((num) & 0xFF)
#define MSB(num) ((num) >> 8)
//#define HINIBBLE(b) ((b)&0xF0) >> 4
//#define LONIBBLE(b) ((b)&0x0F)
//#define BYTES_TO_LONG(byteArray) (( ((unsigned long)byteArray[0] << 24) + ((unsigned long)byteArray[1] << 16) + ((unsigned long)byteArray[2] << 8) + ((unsigned long)byteArray[3] ) ) );
//#define NO_TIMEOUT                      0xFF
//#define SRSP_BUFFER_SIZE        20
#define SRSP_HEADER_SIZE        3
#define SRSP_OFFSET             0x40
#define SRSP_PAYLOAD_START      3
#define SRSP_LENGTH_FIELD       0  
#define SRSP_CMD_LSB_FIELD      2
#define SRSP_CMD_MSB_FIELD      1

#define MODULE_HAS_MESSAGE_WAITING()  (SRDY_IS_LOW())

uint8_t zInit(uint8_t slaveAddr, const struct moduleConfiguration* mc);
uint8_t zReset(void);
uint8_t zRegApp(void);
uint8_t zStartApp(void);
uint8_t zMacAddr(uint16_t shortAddress, uint8_t requestType, uint8_t startIndex);
uint8_t zShortAddr(uint8_t* ieeeAddress, uint8_t requestType, uint8_t startIndex);
uint8_t zTx(uint16_t destinationShortAddress, uint8_t* data, uint8_t dataLength);
uint8_t zRx(uint8_t* data, volatile uint8_t* dataLength);
uint8_t zAckMode(uint8_t ackMode);
uint8_t zbWriteConfiguration(uint8_t zcd, uint8_t zcdLength, uint8_t* data);
uint8_t zDeviceType(uint8_t deviceType);
uint8_t zPanID(uint16_t panId);
uint8_t zChannelMask(uint32_t channelMask);
uint8_t zChannel(uint8_t channel);
uint8_t zCallbacks(uint8_t cb);
uint8_t zStartupOptions(uint8_t option);
uint8_t zSecurityMode(uint8_t securityMode);
uint8_t zSecurityKey(uint8_t* key);
uint8_t zPollRate(uint16_t pollRate);
uint8_t zReadConfigParam(uint8_t configId);
uint8_t zReadDeviceInfo(uint8_t dip);
uint8_t zRandom(void);
uint8_t zNVSize(uint8_t nvItem);
uint8_t zNVRead(uint8_t nvItem);
uint8_t zNVWrite(uint8_t nvItem, uint8_t* data);
uint8_t zLed(uint8_t operation, uint8_t value);
uint8_t zWaitForMsg(uint16_t messageType, uint8_t timeoutSecs);
uint8_t zNewMsg(void);
uint8_t zSREQ(void);
uint8_t zGetMessage(void);
uint8_t zSendMessage(void);
void zWrite(unsigned char *bytes, unsigned char numBytes);
void zRadioON(void);
void zRadioOFF(void);
void zSSclear(void);
void zSSset(void);
void zPinkon(void);
void zPinkoff(void);
void zPinktoggle(void);
void zPink(uint8_t val);
uint8_t zVersion(void);
uint8_t zWaitDevState(uint8_t deviceType, uint16_t timeoutMs);
char* zResetReason(uint8_t reason);
void zResetPrintf(void);
void zVersionPrintf(void);
uint8_t zNwkConfigPrintf(void);
uint8_t zDevInfoPrintf(void);
void zHexPrintf(uint8_t* toPrint, uint16_t numBytes);

#endif
