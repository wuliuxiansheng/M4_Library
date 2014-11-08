//-----------------------------------------------------------------------------
// MAEVARM M4 STM32F373 mZigbee Peripheral 
// version: 1.0
// date: March 26, 2013
// author: Neel Shah (neels@seas.upenn.edu)
// Based on Anaren Zigbee Module ZM Library
// SPI is actually all done through I2C communications in this 
//-----------------------------------------------------------------------------


#include "mGeneral.h"
#include "mBus.h"
#include "mZigbee.h"
#include "stdio.h"

 
__IO uint8_t zGPIOstate=0x0F;
uint8_t moduleResult = MODULE_SUCCESS;
uint8_t sequenceNumber = 0; //SEND_DATA_REQUEST incremented
uint8_t zmBuf[0xFF];
static uint8_t transactionSequenceNumber = 0; //AF_DATA_REQUEST incremented
static uint8_t acknowledgmentMode = AF_MAC_ACK; //MAC or APS, final or intermediate too
uint32_t timeFromChipSelectToSrdyLow = 0;
uint32_t timeWaitingForSrsp = 0;
uint8_t zAddr = 0x5E;

const struct moduleConfiguration DEFAULT_MODULE_CONFIGURATION_COORDINATOR = {
    COORDINATOR,
    DEFAULT_CHANNEL_MASK,
    ANY_PAN,
    DEFAULT_POLL_RATE_MS,
    DEFAULT_STARTUP_OPTIONS,
    SECURITY_MODE_OFF,
    NULL
};
const struct moduleConfiguration DEFAULT_MODULE_CONFIGURATION_ROUTER = {
    ROUTER,
    DEFAULT_CHANNEL_MASK,
    ANY_PAN,
    DEFAULT_POLL_RATE_MS,
    DEFAULT_STARTUP_OPTIONS,
    SECURITY_MODE_OFF,
    NULL
};
const struct moduleConfiguration DEFAULT_MODULE_CONFIGURATION_END_DEVICE = {
    END_DEVICE,
    DEFAULT_CHANNEL_MASK,
    ANY_PAN,
    DEFAULT_POLL_RATE_MS,
    DEFAULT_STARTUP_OPTIONS,
    SECURITY_MODE_OFF,
    NULL
};

#define CHIP_SELECT_TO_SRDY_LOW_TIMEOUT     2500    //us
#define WAIT_FOR_SRSP_TIMEOUT               2000000 //>100ms
#define WFDS_POLL_INTERVAL_MS               100
#define MINIMUM_BUILD_ID                    0x10
#define MODULE_TYPE_MASK                    0x0F    
#define MAXIMUM_MODULE_TYPE                 0x05
#define MAX_OLD_BUILD_ID                    0x1F
#define IS_VALID_MODULE_TYPE(x)             ((x>=0x20) && (x<=0x24))
#define BUILD_ID_FW_2_4_0                   0x7E

#define INVALID_DEVICETYPE 33

#define METHOD_ZDO_STARTUP_FROM_APP         0x31
#define METHOD_ZDO_IEEE_ADDR_REQ            0x32
#define METHOD_ZDO_IEEE_ADDR_RSP            0x33
#define METHOD_ZDO_NWK_ADDR_REQ             0x34
#define METHOD_ZDO_NWK_ADDR_RSP             0x35
#define METHOD_MODULE_RESET                 0x0100
#define METHOD_SYS_VERSION                  0x0200
#define METHOD_SYS_RANDOM                   0x0300
#define METHOD_SET_PAN_ID                   0x0400
#define METHOD_SET_CHANNEL                  0x0500
#define METHOD_SET_CHANNEL_MASK             0x0600
#define METHOD_GET_DEVICE_INFO              0x0700
#define METHOD_SYS_NV_READ                  0x0800
#define METHOD_SYS_NV_WRITE                 0x0900
#define METHOD_SYS_GPIO                     0x0A00
#define METHOD_SET_STARTUP_OPTIONS          0x0C00
#define METHOD_SET_ZIGBEE_DEVICE_TYPE       0x0D00
#define METHOD_SET_CALLBACKS                0x0E00
#define METHOD_WAIT_FOR_MESSAGE             0x0F00
#define METHOD_GET_CONFIGURATION_PARAMETER  0x1000
//#define METHOD_SYS_SET_TX_POWER             0x1100
#define METHOD_SET_SECURITY_MODE            0x1200
#define METHOD_SET_SECURITY_KEY             0x1300
#define METHOD_SET_POLL_RATE                0x1400
#define METHOD_AF_SEND_DATA                 0x2300
// #define METHOD_AF_DATA_STORE                0x2400
// #define METHOD_AF_DATA_REQUEST_EXT          0x2600
// #define METHOD_AF_DATA_RETRIEVE             0x2700
// #define METHOD_AF_RETRIEVE_EXTENDED_MESSAGE 0x2800
#define METHOD_AF_SET_ACK_MODE              0x2900
#define METHOD_WAIT_FOR_DEVICE_STATE        0x6000
#define METHOD_START_MODULE                 0x6100
#define METHOD_SET_MODULE_RF_POWER          0x6200    

static uint8_t zRfPower(uint8_t productId)
{
    if ((productId == BUILD_ID_FW_2_4_0) || (!(IS_VALID_MODULE_TYPE(productId))))
    {
        return MODULE_SUCCESS;
    }
                                        //0xX0, 0xX1, 0xX2, 0xX3, 0xX4
    // US PA_TABLE Settings             {0xF5, 0xF5, 0xE5, 0xE5, 0xC5};    
    const uint8_t rfPowerLevelTable[] = {3, 3, 10, 20, 18};    
    uint8_t rfPowerLevel = rfPowerLevelTable[(productId & MODULE_TYPE_MASK)];
    uint8_t actualRfPowerLevel = 0;
    #define SYS_SET_TX_POWER_PAYLOAD_LEN 1
    zmBuf[0] = SYS_SET_TX_POWER_PAYLOAD_LEN;
    zmBuf[1] = MSB(SYS_SET_TX_POWER);
    zmBuf[2] = LSB(SYS_SET_TX_POWER);
    
    zmBuf[3] = rfPowerLevel;
    RETURN_RESULT_IF_FAIL(zSendMessage(), METHOD_SYS_SET_TX_POWER); 
    actualRfPowerLevel = zmBuf[SYS_SET_TX_POWER_RESULT_FIELD];
    return MODULE_SUCCESS;
}
uint8_t zInit(uint8_t slaveAddr, const struct moduleConfiguration* mc)
{
    EXTI_InitTypeDef   EXTI_InitStructure;
    GPIO_InitTypeDef   GPIO_InitStructure;
    NVIC_InitTypeDef   NVIC_InitStructure;
    
    zAddr = slaveAddr;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

    //SRDY as in input on PA13
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP ;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    mBusWrite(zAddr,zGPIOen,0x0B); //SS0 1 3 as GPIO
    mWait(2000);
    mBusWrite(zAddr,zGPIOmd,0x55); //SS0 1 3 as PP
    mWait(2000);
    mBusWrite(zAddr,zGPIOwr,0x0F); //SS0 1 3 all set
    mWait(2000);
    mBusWrite(zAddr,zSpiConfig,zSpiInit); //SPI config    
    zGPIOstate = 0x0F;

    zRadioOFF();
    zSSset();

    zPinkON;

    RETURN_RESULT_IF_FAIL(zReset(), METHOD_START_MODULE);
    RETURN_RESULT_IF_FAIL(zStartupOptions(mc->startupOptions), METHOD_START_MODULE);   
    RETURN_RESULT_IF_FAIL(zReset(), METHOD_START_MODULE);
    uint8_t productId = zmBuf[SYS_RESET_IND_PRODUCTID_FIELD]; 
    RETURN_RESULT_IF_EXPRESSION_TRUE((productId < MINIMUM_BUILD_ID), METHOD_START_MODULE,
                                     ZM_INVALID_MODULE_CONFIGURATION);       
    zRfPower(productId);
    if (mc->deviceType == END_DEVICE)
        RETURN_RESULT_IF_FAIL(zPollRate(mc->endDevicePollRate), METHOD_START_MODULE);
    RETURN_RESULT_IF_FAIL(zDeviceType(mc->deviceType), METHOD_START_MODULE);     // Set Zigbee Device Type
    RETURN_RESULT_IF_FAIL(zChannelMask(mc->channelMask), METHOD_START_MODULE);
    RETURN_RESULT_IF_FAIL(zPanID(mc->panId), METHOD_START_MODULE);
    RETURN_RESULT_IF_FAIL(zCallbacks(CALLBACKS_ENABLED), METHOD_START_MODULE);
    if (mc->securityMode != SECURITY_MODE_OFF)        // Note: If a coordinator has ZCD_NV_SECURITY_MODE = 00, router must have ZCD_NV_SECURITY_MODE = 01 or else they won't communicate
    {
        RETURN_RESULT_IF_FAIL(zSecurityMode(mc->securityMode), METHOD_START_MODULE);
        RETURN_RESULT_IF_FAIL(zSecurityKey(mc->securityKey), METHOD_START_MODULE);
    }
    zLed(LED_SET_DIRECTION,ALL_LEDS);
    zLed(zON,ALL_LEDS);
    RETURN_RESULT_IF_FAIL(zRegApp(), METHOD_START_MODULE);    // Configure the Module for our application 
    RETURN_RESULT_IF_FAIL(zStartApp(), METHOD_START_MODULE);        // Start your engines
    #define START_TIMEOUT 15000
    uint8_t temp = zWaitDevState(mc->deviceType, START_TIMEOUT);
    zLed(zOFF,ALL_LEDS);
    if(temp!=MODULE_SUCCESS)
        return temp;
    if(mc->deviceType == COORDINATOR)
        zLed(zON, LED0);
    else if(mc->deviceType == ROUTER)
        zLed(zON, LED1);
    else
        zLed(zOFF, LED0 | LED1);
    zPinkOFF;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource13);
    
    EXTI_InitStructure.EXTI_Line = EXTI_Line13;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);
    NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x00;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure); 

    zIntDisable;

    return temp;
}
uint8_t zReset(void)
{
    zRadioOFF();
    mWaitms(1);
    zRadioON(); 
    #define MODULE_RESET_INITIAL_DELAY_MS   200
    mWaitms(MODULE_RESET_INITIAL_DELAY_MS);                        //Necessary to allow proper module startup
    unsigned int elapsedTime = 0;       //now, poll for SRDY going low...
    do
    {
        mWaitms(10);
        elapsedTime += 10;
    }
    while ((elapsedTime < 2000) && (!(MODULE_HAS_MESSAGE_WAITING())));

    RETURN_RESULT_IF_EXPRESSION_TRUE(SRDY_IS_HIGH(), METHOD_MODULE_RESET, TIMEOUT);
    return (zGetMessage());
}
uint8_t zRegApp(void)
{
    #define AF_REGISTER_PAYLOAD_LEN 9
    zmBuf[0] = AF_REGISTER_PAYLOAD_LEN;
    zmBuf[1] = MSB(AF_REGISTER);
    zmBuf[2] = LSB(AF_REGISTER);      
    
    zmBuf[3] = DEFAULT_ENDPOINT;
    zmBuf[4] = LSB(DEFAULT_PROFILE_ID);
    zmBuf[5] = MSB(DEFAULT_PROFILE_ID);
    zmBuf[6] = LSB(DEVICE_ID);
    zmBuf[7] = MSB(DEVICE_ID);
    zmBuf[8] = DEVICE_VERSION;
    zmBuf[9] = LATENCY_NORMAL;
    zmBuf[10] = 0; //number of binding input clusters
    zmBuf[11] = 0; //number of binding output clusters
    RETURN_RESULT(zSendMessage(), METHOD_AF_REGISTER_GENERIC_APPLICATION);
}
uint8_t zStartApp(void)
{
    #define ZDO_STARTUP_FROM_APP_PAYLOAD_LEN 1    
    #define NO_START_DELAY 0
    zmBuf[0] = ZDO_STARTUP_FROM_APP_PAYLOAD_LEN;
    zmBuf[1] = MSB(ZDO_STARTUP_FROM_APP);
    zmBuf[2] = LSB(ZDO_STARTUP_FROM_APP);      
    zmBuf[3] = NO_START_DELAY;
    RETURN_RESULT(zSendMessage(), ZDO_STARTUP_FROM_APP);
}
uint8_t zMacAddr(uint16_t shortAddress, uint8_t requestType, uint8_t startIndex)
{
    RETURN_INVALID_PARAMETER_IF_TRUE(((requestType != SINGLE_DEVICE_RESPONSE) && (requestType != INCLUDE_ASSOCIATED_DEVICES)), METHOD_ZDO_IEEE_ADDR_REQ);  
    #define ZDO_IEEE_ADDR_REQ_PAYLOAD_LEN 4
    zmBuf[0] = ZDO_IEEE_ADDR_REQ_PAYLOAD_LEN;
    zmBuf[1] = MSB(ZDO_IEEE_ADDR_REQ);             
    zmBuf[2] = LSB(ZDO_IEEE_ADDR_REQ);      
    zmBuf[3] = LSB(shortAddress);
    zmBuf[4] = MSB(shortAddress);
    zmBuf[5] = requestType;
    zmBuf[6] = startIndex;
    
    RETURN_RESULT_IF_FAIL(zSendMessage(), METHOD_ZDO_IEEE_ADDR_REQ);     
    #define ZDO_IEEE_ADDR_RSP_TIMEOUT 10
    RETURN_RESULT_IF_FAIL(zWaitForMsg(ZDO_IEEE_ADDR_RSP, ZDO_IEEE_ADDR_RSP_TIMEOUT), METHOD_ZDO_IEEE_ADDR_RSP);
    RETURN_RESULT(zmBuf[ZDO_IEEE_ADDR_RSP_STATUS_FIELD], METHOD_ZDO_IEEE_ADDR_RSP);
}
uint8_t zShortAddr(uint8_t* ieeeAddress, uint8_t requestType, uint8_t startIndex)
{
    RETURN_INVALID_PARAMETER_IF_TRUE(((requestType != SINGLE_DEVICE_RESPONSE) && (requestType != INCLUDE_ASSOCIATED_DEVICES)), METHOD_ZDO_NWK_ADDR_REQ);
    
    #define ZDO_NWK_ADDR_REQ_PAYLOAD_LEN 10
    zmBuf[0] = ZDO_NWK_ADDR_REQ_PAYLOAD_LEN;
    zmBuf[1] = MSB(ZDO_NWK_ADDR_REQ);
    zmBuf[2] = LSB(ZDO_NWK_ADDR_REQ);      
    memcpy(zmBuf+3, ieeeAddress, 8);
    zmBuf[11] = requestType;
    zmBuf[12] = startIndex;
    
    RETURN_RESULT_IF_FAIL(zSendMessage(), METHOD_ZDO_NWK_ADDR_REQ);     
    #define ZDO_NWK_ADDR_RSP_TIMEOUT 10
    RETURN_RESULT_IF_FAIL(zWaitForMsg(ZDO_NWK_ADDR_RSP, ZDO_NWK_ADDR_RSP_TIMEOUT), METHOD_ZDO_NWK_ADDR_RSP);
    RETURN_RESULT(zmBuf[ZDO_NWK_ADDR_RSP_STATUS_FIELD], METHOD_ZDO_NWK_ADDR_RSP);
}
uint8_t zTx(uint16_t destinationShortAddress, uint8_t* data, uint8_t dataLength)
{
    RETURN_INVALID_LENGTH_IF_TRUE( ((dataLength > MAXIMUM_PAYLOAD_LENGTH) || (dataLength == 0)), METHOD_AF_SEND_DATA); 
    #define AF_DATA_REQUEST_PAYLOAD_LEN 10
    zmBuf[0] = AF_DATA_REQUEST_PAYLOAD_LEN + dataLength;
    zmBuf[1] = MSB(AF_DATA_REQUEST);
    zmBuf[2] = LSB(AF_DATA_REQUEST);      
    zmBuf[3] = LSB(destinationShortAddress); 
    zmBuf[4] = MSB(destinationShortAddress);
    zmBuf[5] = DEFAULT_ENDPOINT;
    zmBuf[6] = DEFAULT_ENDPOINT;
    zmBuf[7] = 0x42; //LSB(clusterId); 
    zmBuf[8] = 0x42; //MSB(clusterId); 
    zmBuf[9] = transactionSequenceNumber++;
    zmBuf[10] = acknowledgmentMode;
    zmBuf[11] = DEFAULT_RADIUS;
    zmBuf[12] = dataLength; 
    memcpy(zmBuf+AF_DATA_REQUEST_PAYLOAD_LEN+3, data, dataLength);
    RETURN_RESULT_IF_FAIL(zSendMessage(), METHOD_AF_SEND_DATA); 
    #define AF_DATA_REQUEST_SRSP_STATUS_FIELD   SRSP_PAYLOAD_START
    RETURN_RESULT_IF_FAIL(zmBuf[AF_DATA_REQUEST_SRSP_STATUS_FIELD], METHOD_AF_SEND_DATA); 
    #define AF_DATA_CONFIRM_TIMEOUT 2
    RETURN_RESULT_IF_FAIL(zWaitForMsg(AF_DATA_CONFIRM, AF_DATA_CONFIRM_TIMEOUT), METHOD_AF_SEND_DATA);
    RETURN_RESULT(zmBuf[AF_DATA_CONFIRM_STATUS_FIELD], METHOD_AF_SEND_DATA);  
}
uint8_t zRx(uint8_t* data, volatile uint8_t* dataLength)
{
    zGetMessage();
    if (zmBuf[SRSP_LENGTH_FIELD] > 13)
    {
        zHexPrintf(zmBuf, (zmBuf[SRSP_LENGTH_FIELD] + SRSP_HEADER_SIZE));
        *dataLength = zmBuf[19];
        memcpy(data, zmBuf+20, zmBuf[19]);
        //zHexPrintf(zmBuf, (zmBuf[SRSP_LENGTH_FIELD] + SRSP_HEADER_SIZE));
        zmBuf[SRSP_LENGTH_FIELD] = 0;
        return 1;
    }
    else
    {
        *dataLength = 0;
        return 0;
    }
}
uint8_t zAckMode(uint8_t ackMode)
{
    RETURN_INVALID_PARAMETER_IF_TRUE(((ackMode != AF_MAC_ACK) && (ackMode != AF_APS_ACK)), METHOD_AF_SET_ACK_MODE);
    acknowledgmentMode = ackMode;
    return MODULE_SUCCESS;
}
uint8_t zbWriteConfiguration(uint8_t zcd, uint8_t zcdLength, uint8_t* data)
{
    #define ZB_WRITE_CONFIGURATION_LEN  2  //excluding payload length
    zmBuf[0] = ZB_WRITE_CONFIGURATION_LEN + zcdLength;
    zmBuf[1] = MSB(ZB_WRITE_CONFIGURATION);
    zmBuf[2] = LSB(ZB_WRITE_CONFIGURATION);
    
    zmBuf[3] = zcd;
    zmBuf[4] = zcdLength;
    memcpy(zmBuf+5, data, zcdLength);
    return (zSendMessage()); 
}
uint8_t zDeviceType(uint8_t deviceType)
{ 
    RETURN_INVALID_PARAMETER_IF_TRUE( (deviceType > END_DEVICE), METHOD_SET_ZIGBEE_DEVICE_TYPE);      
    uint8_t data[1];
    data[0] = deviceType;
    RETURN_RESULT(zbWriteConfiguration(ZCD_NV_LOGICAL_TYPE, ZCD_NV_LOGICAL_TYPE_LEN, data), METHOD_SET_ZIGBEE_DEVICE_TYPE);
}
uint8_t zPanID(uint16_t panId)
{
    uint8_t data[2];
    data[0] = LSB(panId);
    data[1] = MSB(panId);
    RETURN_RESULT((zbWriteConfiguration(ZCD_NV_PANID, ZCD_NV_PANID_LEN, data)), METHOD_SET_PAN_ID);
}
uint8_t zChannelMask(uint32_t channelMask)
{
    RETURN_INVALID_PARAMETER_IF_TRUE( ((channelMask < MIN_CHANNEL_MASK) || (channelMask > MAX_CHANNEL_MASK)), METHOD_SET_CHANNEL_MASK);
    uint8_t data[4];
    data[0] = LSB(channelMask);
    data[1] = (channelMask & 0xFF00) >> 8;
    data[2] = (channelMask & 0xFF0000) >> 16;
    data[3] = channelMask >> 24;    
    RETURN_RESULT((zbWriteConfiguration(ZCD_NV_CHANLIST, ZCD_NV_CHANLIST_LEN, data)), METHOD_SET_CHANNEL_MASK);
}
uint8_t zChannel(uint8_t channel)
{
    RETURN_INVALID_PARAMETER_IF_TRUE( ((!IS_VALID_CHANNEL(channel)) && (channel != 0x00)), METHOD_SET_CHANNEL);
    uint32_t channelMask = 1;
    return (zChannelMask((channel == 0) ? ANY_CHANNEL_MASK : (channelMask << channel)));
}
uint8_t zCallbacks(uint8_t cb)
{ 
    RETURN_INVALID_PARAMETER_IF_TRUE( ((cb != CALLBACKS_ENABLED) && (cb != CALLBACKS_DISABLED)), METHOD_SET_CALLBACKS);      
    uint8_t data[1];
    data[0] = cb;
    RETURN_RESULT(zbWriteConfiguration(ZCD_NV_ZDO_DIRECT_CB, ZCD_NV_ZDO_DIRECT_CB_LEN, data), METHOD_SET_CALLBACKS);
}
uint8_t zStartupOptions(uint8_t option)
{ 
    //STARTOPT_CLEAR_CONFIG restores all settings to factory defaults. Must restart.
    //STARTOPT_CLEAR_STATE only clears network settings (PAN ID, channel, etc.)
    RETURN_INVALID_PARAMETER_IF_TRUE( (option > (STARTOPT_CLEAR_CONFIG + STARTOPT_CLEAR_STATE)), METHOD_SET_STARTUP_OPTIONS);       
    uint8_t data[1];
    data[0] = option;
    RETURN_RESULT(zbWriteConfiguration(ZCD_NV_STARTUP_OPTION, ZCD_NV_STARTUP_OPTION_LEN, data), METHOD_SET_STARTUP_OPTIONS);     
}
uint8_t zSecurityMode(uint8_t securityMode)
{
    //securityMode must be SECURITY_MODE_OFF, or SECURITY_MODE_PRECONFIGURED_KEYS, or SECURITY_MODE_COORD_DIST_KEYS. 
    RETURN_INVALID_PARAMETER_IF_TRUE( (securityMode > SECURITY_MODE_COORD_DIST_KEYS), METHOD_SET_SECURITY_MODE);     
    uint8_t data[1];
    data[0] = (securityMode > 0);               // Configure security on/off:
    RETURN_RESULT_IF_FAIL(zbWriteConfiguration(ZCD_NV_SECURITY_MODE, ZCD_NV_SECURITY_MODE_LEN, data), METHOD_SET_SECURITY_MODE);       
    if (securityMode != SECURITY_MODE_OFF)      // If turning security off, don't need to set pre-configured keys on/off
    {
        data[0] = (securityMode == SECURITY_MODE_PRECONFIGURED_KEYS);        //Now, configure pre-configured keys on/off:
        RETURN_RESULT(zbWriteConfiguration(ZCD_NV_PRECFGKEYS_ENABLE, ZCD_NV_PRECFGKEYS_ENABLE_LEN, data), METHOD_SET_SECURITY_MODE);         
    } else {
        return MODULE_SUCCESS;        
    }
}
uint8_t zSecurityKey(uint8_t* key)
{
    //16 byte security key 
    RETURN_INVALID_PARAMETER_IF_TRUE( (key == NULL), METHOD_SET_SECURITY_KEY);
    RETURN_RESULT(zbWriteConfiguration(ZCD_NV_PRECFGKEY, ZCD_NV_PRECFGKEY_LEN, key), METHOD_SET_SECURITY_KEY);
}
uint8_t zPollRate(uint16_t pollRate)
{ 
    //in milliseconds (ms)
    RETURN_INVALID_PARAMETER_IF_TRUE((pollRate > 65000), METHOD_SET_POLL_RATE);      
    uint8_t data[2];
    data[0] = LSB(pollRate);
    data[1] = MSB(pollRate);
    RETURN_RESULT((zbWriteConfiguration(ZCD_NV_POLL_RATE, ZCD_NV_POLL_RATE_LEN, data)), METHOD_SET_POLL_RATE);
}
uint8_t zReadConfigParam(uint8_t configId)
{
    #define LENGTH_OF_LARGEST_CONFIG_PARAMETER 17  //ZCD_NV_USERDESC is largest
    #define ZB_READ_CONFIGURATION_PAYLOAD_LEN 1
    zmBuf[0] = ZB_READ_CONFIGURATION_PAYLOAD_LEN;
    zmBuf[1] = MSB(ZB_READ_CONFIGURATION);
    zmBuf[2] = LSB(ZB_READ_CONFIGURATION);  
    zmBuf[3] = configId;
    RETURN_RESULT_IF_FAIL(zSendMessage(), METHOD_GET_CONFIGURATION_PARAMETER);    
    #define ZB_READ_CONFIGURATION_STATUS_FIELD            SRSP_PAYLOAD_START
    RETURN_RESULT(zmBuf[ZB_READ_CONFIGURATION_STATUS_FIELD], METHOD_GET_CONFIGURATION_PARAMETER);   
}
uint8_t zReadDeviceInfo(uint8_t dip)
{
    RETURN_INVALID_PARAMETER_IF_TRUE( (dip > MAX_DEVICE_INFORMATION_PROPERTY), METHOD_GET_DEVICE_INFO);
    #define ZB_GET_DEVICE_INFO_PAYLOAD_LEN 1
    zmBuf[0] = ZB_GET_DEVICE_INFO_PAYLOAD_LEN;
    zmBuf[1] = MSB(ZB_GET_DEVICE_INFO);
    zmBuf[2] = LSB(ZB_GET_DEVICE_INFO);
    zmBuf[3] = dip;
    RETURN_RESULT(zSendMessage(), METHOD_GET_DEVICE_INFO); 
}
uint8_t zRandom(void)
{
    #define SYS_RANDOM_PAYLOAD_LEN 0
    zmBuf[0] = SYS_RANDOM_PAYLOAD_LEN;
    zmBuf[1] = MSB(SYS_RANDOM);
    zmBuf[2] = LSB(SYS_RANDOM);  
    RETURN_RESULT(zSendMessage(), METHOD_SYS_RANDOM);    
}
uint8_t zNVSize(uint8_t nvItem)
{
    if ((nvItem < 5) || (nvItem==8))
    {
        return 2;
    } else {
        return 16;
    }
}
uint8_t zNVRead(uint8_t nvItem)
{
    RETURN_INVALID_PARAMETER_IF_TRUE( ((nvItem < MIN_NV_ITEM) || (nvItem > MAX_NV_ITEM) || (nvItem == NV_ITEM_RESERVED)), METHOD_SYS_NV_READ);
    
    #define SYS_NV_READ_PAYLOAD_LEN 3  
    zmBuf[0] = SYS_NV_READ_PAYLOAD_LEN;
    zmBuf[1] = MSB(SYS_NV_READ);
    zmBuf[2] = LSB(SYS_NV_READ);  
    
    zmBuf[3] = nvItem;         //item number, 1-6
    zmBuf[4] = 0x0F;           //MSB of item number, but only items 1-6 are supported
    zmBuf[5] = 0;              //offset from beginning of the NV item, not used
    RETURN_RESULT_IF_FAIL(zSendMessage(), METHOD_SYS_NV_READ); 
    RETURN_RESULT(zmBuf[SYS_NV_READ_STATUS_FIELD], METHOD_SYS_NV_READ);
}
uint8_t zNVWrite(uint8_t nvItem, uint8_t* data)
{
    RETURN_INVALID_PARAMETER_IF_TRUE( ((nvItem < MIN_NV_ITEM) || (nvItem > MAX_NV_ITEM) || (nvItem == NV_ITEM_RESERVED)), METHOD_SYS_NV_WRITE);
    uint8_t nvItemSize = zNVSize(nvItem);  
    #define SYS_NV_WRITE_PAYLOAD_LEN 4  //excludes length of NV item    
    zmBuf[0] = SYS_NV_WRITE_PAYLOAD_LEN + nvItemSize;
    zmBuf[1] = MSB(SYS_NV_WRITE);
    zmBuf[2] = LSB(SYS_NV_WRITE);  
    
    zmBuf[3] = nvItem;         //item number, 1-6
    zmBuf[4] = 0x0F;           //MSB of item number, but only items 1-6 are supported
    zmBuf[5] = 0;              //offset from beginning of the NV item, not used
    zmBuf[6] = nvItemSize;     //length
    
    memcpy(zmBuf+7, data, nvItemSize);
    RETURN_RESULT(zSendMessage(), METHOD_SYS_NV_WRITE);       
}
uint8_t zLed(uint8_t operation, uint8_t value)
{
    RETURN_INVALID_PARAMETER_IF_TRUE( ((operation > GPIO_OPERATION_MAX) || 
                                       ((value > 0x0F) && (operation != GPIO_SET_INPUT_MODE))), METHOD_SYS_GPIO);
    #define SYS_GPIO_PAYLOAD_LEN 2
    zmBuf[0] = SYS_GPIO_PAYLOAD_LEN;
    zmBuf[1] = MSB(SYS_GPIO);
    zmBuf[2] = LSB(SYS_GPIO);
    
    zmBuf[3] = operation;
    zmBuf[4] = value;
    RETURN_RESULT(zSendMessage(), METHOD_SYS_GPIO);        
}
uint8_t zWaitForMsg(uint16_t messageType, uint8_t timeoutSecs)
{
    RETURN_INVALID_PARAMETER_IF_TRUE(((messageType==0)||(timeoutSecs==0)),METHOD_WAIT_FOR_MESSAGE);     
    #define WFM_POLL_INTERVAL_MS   10
    uint16_t intervals = timeoutSecs * 1000 / WFM_POLL_INTERVAL_MS; //how many times to check   
    while (intervals--)
    {
        if(zNewMsg())                           // If there's a message waiting for us
        {
            zGetMessage();
            if (zmBuf[SRSP_LENGTH_FIELD] > 0)
            {
                if (CONVERT_TO_INT(zmBuf[2], zmBuf[1]) == messageType)
                    return MODULE_SUCCESS;
            }
        }
        mWaitms(WFM_POLL_INTERVAL_MS);
    }
    RETURN_RESULT(TIMEOUT, METHOD_WAIT_FOR_MESSAGE);    
}
uint8_t zNewMsg(void)
{
  return (SRDY_IS_LOW());
}
uint8_t zSREQ(void)
{
  uint32_t timeLeft1 = CHIP_SELECT_TO_SRDY_LOW_TIMEOUT;
  uint32_t timeLeft2 = WAIT_FOR_SRSP_TIMEOUT;
  zSSclear();
  mBlueON;  
  while (SRDY_IS_HIGH() && (timeLeft1 != 0))  //wait until SRDY goes low
    timeLeft1--;
  if (timeLeft1 == 0)                         //SRDY did not go low in time, so return an error
    return ZM_PHY_CHIP_SELECT_TIMEOUT;
  timeFromChipSelectToSrdyLow = (CHIP_SELECT_TO_SRDY_LOW_TIMEOUT - timeLeft1);
  zWrite(zmBuf, (*zmBuf + 3));              // *bytes (first byte) is length after the first 3 bytes, all frames have at least the first 3 bytes
  *zmBuf = 0; *(zmBuf+1) = 0; *(zmBuf+2) = 0; //poll message is 0,0,0
  while (SRDY_IS_LOW() && (timeLeft2 != 0))    //wait for data
    timeLeft2--;
  if (timeLeft2 == 0)
    return ZM_PHY_SRSP_TIMEOUT;
  timeWaitingForSrsp = (WAIT_FOR_SRSP_TIMEOUT - timeLeft2);
  zWrite(zmBuf, 1);
  if (*zmBuf > 0)                             // *bytes (first byte) contains number of bytes to receive
    zWrite(zmBuf+1, *zmBuf+2);                //write-to-read: read data into buffer                                  
  else
    zWrite(zmBuf+1,2);
  zSSset();
  return MODULE_SUCCESS;
}
uint8_t zGetMessage(void)
{
  *zmBuf = 0; *(zmBuf+1) = 0; *(zmBuf+2) = 0;  //poll message is 0,0,0 
  return(zSREQ());
}
uint8_t zSendMessage(void)
{
    uint8_t expectedSrspCmdMsb = zmBuf[1] + SRSP_OFFSET;
    uint8_t expectedSrspCmdLsb = zmBuf[2];
    uint8_t result = zSREQ();
    if (result != MODULE_SUCCESS) 
        return result;
    if((zmBuf[SRSP_CMD_MSB_FIELD] == expectedSrspCmdMsb) && (zmBuf[SRSP_CMD_LSB_FIELD] == expectedSrspCmdLsb))    //verify the correct SRSP was received
        return MODULE_SUCCESS;
    else         
        return ZM_PHY_INCORRECT_SRSP;   //Wrong SRSP received
}
void zWrite(unsigned char *bytes, unsigned char numBytes)
{
    // printf("S: ");
    // for(uint8_t i=0;i<numBytes;i++)
    // {
    //     printf("%02X",bytes[i]);
    // }
    // printf("\r\n");
    while(!mBusWriteBurst(zAddr,zSpi,numBytes,bytes));
    mWait(2000);
    while(!mBusReadBurstNoAdd(zAddr,numBytes,bytes));
    // printf("R: ");
    // for(uint8_t i=0;i<numBytes;i++)
    // {
    //     printf("%02X",bytes[i]);
    // }
    // printf("\r\n");
}
void zRadioON(void)
{
    zGPIOstate |= resetPin;
    mBusWrite(zAddr,zGPIOwr,zGPIOstate);
}
void zRadioOFF(void)
{
    zGPIOstate &= (~resetPin);
    mBusWrite(zAddr,zGPIOwr,zGPIOstate);
}
void zSSclear(void)
{
    zGPIOstate &= (~zSs);
    mBusWrite(zAddr,zGPIOwr,zGPIOstate);
}
void zSSset(void)
{
    zGPIOstate |= (zSs);
    mBusWrite(zAddr,zGPIOwr,zGPIOstate);
}
void zPinkon(void)
{
    zGPIOstate&=(~pinkPin);
    mBusWrite(zAddr,zGPIOwr,zGPIOstate);
}
void zPinkoff(void)
{
    zGPIOstate|=pinkPin;
    mBusWrite(zAddr,zGPIOwr,zGPIOstate);
}
void zPinktoggle(void)
{
    zGPIOstate^=pinkPin;
    mBusWrite(zAddr,zGPIOwr,zGPIOstate);
}
void zPink(uint8_t val)  
{
    switch(val)
    {
        case OFF: zPinkOFF;break;
        case ON: zPinkON;break;
        case TOGGLE: zPinkTOGGLE;break;
        default: break;
    }
}
uint8_t zVersion(void)
{
    #define SYS_VERSION_PAYLOAD_LEN 0
    zmBuf[0] = SYS_VERSION_PAYLOAD_LEN;
    zmBuf[1] = MSB(SYS_VERSION);
    zmBuf[2] = LSB(SYS_VERSION);   
    RETURN_RESULT(zSendMessage(), METHOD_SYS_VERSION);     
}
uint8_t zWaitDevState(uint8_t deviceType, uint16_t timeoutMs)
{
    unsigned char expectedState;
    switch (deviceType)
    {
        case ROUTER: 
            expectedState = DEV_ROUTER; break;
        case END_DEVICE: 
            expectedState = DEV_END_DEVICE; break;
        case COORDINATOR: 
            expectedState = DEV_ZB_COORD; break;
        default: 
            expectedState = INVALID_DEVICETYPE;
    }
    RETURN_INVALID_PARAMETER_IF_TRUE( ((!(IS_VALID_DEVICE_STATE(expectedState))) || (timeoutMs < WFDS_POLL_INTERVAL_MS)), METHOD_WAIT_FOR_DEVICE_STATE);
    uint16_t intervals = timeoutMs / WFDS_POLL_INTERVAL_MS;                   // how many times to check
    uint8_t state = 0xFF;
    while(intervals--)
    {
        if(zNewMsg())                                          // If there's a message waiting for us
        {
            zGetMessage();
            if(CONVERT_TO_INT(zmBuf[2],zmBuf[1]) == ZDO_STATE_CHANGE_IND)       // if it's a state change message
            {
                state = zmBuf[SRSP_PAYLOAD_START];
                if (state == expectedState)                                         // if it's the state we're expecting
                    return MODULE_SUCCESS;                                                //Then we're done!
            } 
        }
        mWaitms(WFDS_POLL_INTERVAL_MS);
    }
    RETURN_RESULT(TIMEOUT, METHOD_WAIT_FOR_DEVICE_STATE);
}
char* zResetReason(uint8_t reason)
{
    switch (reason)
    {
        case 0:     return "Power-up";
        case 1:     return "External";
        case 2:     return "Watch-dog";
        default:    return "Unknown";
    }
}
void zResetPrintf(void)
{
    if (IS_SYS_RESET_IND())
    {
        uint8_t* v = zmBuf + SYS_RESET_IND_START_FIELD;
        printf("%s (%u), TransportRev=%u, ProductId=0x%02X, FW Rev=%u.%u.%u\r\n",
               zResetReason(v[0]), v[0], v[1], v[2], v[3], v[4], v[5]);
    } else {
        printf("Error - not a SYS_RESET_IND. Expected type 0x%04X; Received type 0x%04X; Contents:\r\n", SYS_RESET_IND, MODULE_COMMAND());
        printf("zb: ");
        zHexPrintf(zmBuf, zmBuf[0]+3);
        printf("\r\n"); 
    }
}
void zVersionPrintf(void)
{
    if (IS_SYS_VERSION_SRSP())
    {
        uint8_t* v = zmBuf + SYS_VERSION_RESULT_START_FIELD;
        printf("Version: TransportRev=%u, ProductId=0x%02X, FW Rev=%u.%u.%u\r\n", 
               v[0], v[1], v[2], v[3], v[4]);    
    } else {
        printf("Error - not a SYS_VERSION SRSP. Expected type 0x%04X; Received type 0x%04X; Contents:\r\n", (SYS_VERSION + SRSP), MODULE_COMMAND());
        printf("zb: ");
        zHexPrintf(zmBuf, zmBuf[0]+3);
        printf("\r\n");
    }
}
uint8_t zNwkConfigPrintf(void)
{
    uint8_t result = MODULE_SUCCESS;
    printf("Module Configuration Parameters\r\n");
    
    result = zReadConfigParam(ZCD_NV_PANID);
    if (result != MODULE_SUCCESS) return result;
    printf("    ZCD_NV_PANID                %04X\r\n", 
           (CONVERT_TO_INT(zmBuf[ZB_READ_CONFIGURATION_START_OF_VALUE_FIELD], 
                           zmBuf[ZB_READ_CONFIGURATION_START_OF_VALUE_FIELD+1])));
    
    result = zReadConfigParam(ZCD_NV_CHANLIST);
    if (result != MODULE_SUCCESS) return result;
    printf("    ZCD_NV_CHANLIST             %02X %02X %02X %02X\r\n", 
           zmBuf[ZB_READ_CONFIGURATION_START_OF_VALUE_FIELD], 
           zmBuf[ZB_READ_CONFIGURATION_START_OF_VALUE_FIELD + 1], 
           zmBuf[ZB_READ_CONFIGURATION_START_OF_VALUE_FIELD + 2], 
           zmBuf[ZB_READ_CONFIGURATION_START_OF_VALUE_FIELD + 3]);
    
    result = zReadConfigParam(ZCD_NV_SECURITY_MODE);
    if (result != MODULE_SUCCESS) return result;
    printf("    ZCD_NV_SECURITY_MODE        %02X\r\n", zmBuf[ZB_READ_CONFIGURATION_START_OF_VALUE_FIELD]);
    
    result = zReadConfigParam(ZCD_NV_PRECFGKEYS_ENABLE);
    if (result != MODULE_SUCCESS) return result;
    printf("    ZCD_NV_PRECFGKEYS_ENABLE    %02X\r\n", zmBuf[ZB_READ_CONFIGURATION_START_OF_VALUE_FIELD]);    
    
    result = zReadConfigParam(ZCD_NV_PRECFGKEY);
    if (result != MODULE_SUCCESS) return result;
    printf("    ZCD_NV_PRECFGKEY            ");    
    zHexPrintf(zmBuf+ZB_READ_CONFIGURATION_START_OF_VALUE_FIELD, ZCD_NV_PRECFGKEY_LEN);
    
    return MODULE_SUCCESS;
}
uint8_t zDevInfoPrintf(void)
{
    int i;
    printf("Device Information Properties (MSB first)\r\n");
    uint8_t result = MODULE_SUCCESS;
    result = zReadDeviceInfo(DIP_STATE);
    if (result != MODULE_SUCCESS) return result;
    // switch (zmBuf[SRSP_DIP_VALUE_FIELD])
    // {
    //     case DEV_HOLD:              memcpy("DEV_HOLD";break;
    //     case DEV_INIT:              char temp[8]="DEV_INIT";break;
    //     case DEV_NWK_DISC:          char temp[12]="DEV_NWK_DISC";break;   
    //     case DEV_NWK_JOINING:       char temp[15]="DEV_NWK_JOINING";break;
    //     case DEV_NWK_REJOIN:        char temp[14]="DEV_NWK_REJOIN";break;
    //     case DEV_END_DEVICE_UNAUTH: char temp[21]="DEV_END_DEVICE_UNAUTH";break;    
    //     case DEV_END_DEVICE:        char temp[14]="DEV_END_DEVICE";break;
    //     case DEV_ROUTER:            char temp[10]="DEV_ROUTER";break;
    //     case DEV_COORD_STARTING:    char temp[18]="DEV_COORD_STARTING";break;
    //     case DEV_ZB_COORD:          char temp[12]="DEV_ZB_COORD";break;
    //     case DEV_NWK_ORPHAN:        char temp[14]="DEV_NWK_ORPHAN";break;       
    //     default:                    char temp[7]="Unknown";break;
    // }
    // printf("    Device State:               %s (%u)\r\n", temp, (zmBuf[SRSP_DIP_VALUE_FIELD])); 
    
    result = zReadDeviceInfo(DIP_MAC_ADDRESS);
    printf("    MAC Address:                ");
    if (result != MODULE_SUCCESS) return result;
    for (i = SRSP_DIP_VALUE_FIELD+7; i>=SRSP_DIP_VALUE_FIELD; i--)
        printf("%02X ", zmBuf[i]);
    printf("\r\n");
    
    result = zReadDeviceInfo(DIP_SHORT_ADDRESS);
    if (result != MODULE_SUCCESS) return result;
    printf("    Short Address:              %04X\r\n", CONVERT_TO_INT(zmBuf[SRSP_DIP_VALUE_FIELD] , zmBuf[SRSP_DIP_VALUE_FIELD+1]));
    
    result = zReadDeviceInfo(DIP_PARENT_SHORT_ADDRESS);
    if (result != MODULE_SUCCESS) return result;
    printf("    Parent Short Address:       %04X\r\n", CONVERT_TO_INT(zmBuf[SRSP_DIP_VALUE_FIELD] , zmBuf[SRSP_DIP_VALUE_FIELD+1]));
    
    result = zReadDeviceInfo(DIP_PARENT_MAC_ADDRESS);
    if (result != MODULE_SUCCESS) return result;
    printf("    Parent MAC Address:         ");
    for (i = SRSP_DIP_VALUE_FIELD+7; i>=SRSP_DIP_VALUE_FIELD; i--)
        printf("%02X ", zmBuf[i]);
    printf("\r\n");
    
    result = zReadDeviceInfo(DIP_CHANNEL);
    if (result != MODULE_SUCCESS) return result;
    printf("    Device Channel:             %u\r\n", zmBuf[SRSP_DIP_VALUE_FIELD]);
    
    result = zReadDeviceInfo(DIP_PANID);
    if (result != MODULE_SUCCESS) return result;
    printf("    PAN ID:                     %04X\r\n", CONVERT_TO_INT(zmBuf[SRSP_DIP_VALUE_FIELD], zmBuf[SRSP_DIP_VALUE_FIELD+1]));
    
    result = zReadDeviceInfo(DIP_EXTENDED_PANID);
    if (result != MODULE_SUCCESS) return result;
    printf("    Extended PAN ID:            ");
    for (i = SRSP_DIP_VALUE_FIELD+7; i>=SRSP_DIP_VALUE_FIELD; i--)
        printf("%02X ", zmBuf[i]);
    printf("\r\n");
    
    return MODULE_SUCCESS;
}
void zHexPrintf(uint8_t* toPrint, uint16_t numBytes)
{
    int i = 0;
    for (i=0; i<numBytes; i++)
        printf("%02X ", toPrint[i]);
    printf("\r\n");
}





// void displayMessages()
// {
//   zGetMessage();
//   if (zmBuf[SRSP_LENGTH_FIELD] > 0)
//   {
//     switch ( (CONVERT_TO_INT(zmBuf[SRSP_CMD_LSB_FIELD], zmBuf[SRSP_CMD_MSB_FIELD])) )
//     {
//     case AF_DATA_CONFIRM:
//       {
//         printf("AF_DATA_CONFIRM\r\n");
//         break;
//       }
//     case AF_INCOMING_MSG:
//       {
//         printf("AF_INCOMING_MSG\r\n");
//         printAfIncomingMsgHeader(zmBuf);
//         printf("\r\n");  
//         printf("Payload: ");
//         zHexPrintf(zmBuf+SRSP_HEADER_SIZE+17, zmBuf[SRSP_HEADER_SIZE+16]);
//         break;
//       }
//     case AF_INCOMING_MSG_EXT:
//       {
//         printf("AF_INCOMING_MSG_EXT\r\n");
//         uint16_t len = AF_INCOMING_MESSAGE_EXT_LENGTH();
//         printf("Extended Message Received, L%u ", len);
//         break;
//       }
//     case ZDO_IEEE_ADDR_RSP:
//       {
//         printf("ZDO_IEEE_ADDR_RSP\r\n");
//         displayZdoAddressResponse(zmBuf + SRSP_PAYLOAD_START);
//         break;
//       }
//     case ZDO_NWK_ADDR_RSP:
//       {
//         printf("ZDO_NWK_ADDR_RSP\r\n");
//         displayZdoAddressResponse(zmBuf + SRSP_PAYLOAD_START);
//         break;
//       }
//     case ZDO_END_DEVICE_ANNCE_IND:
//       {
//         printf("ZDO_END_DEVICE_ANNCE_IND received!\r\n");
//         displayZdoEndDeviceAnnounce(zmBuf);
//         break;
//       }
//     case ZB_FIND_DEVICE_CONFIRM:
//       {
//         printf("ZB_FIND_DEVICE_CONFIRM\r\n");
//         break;
//       }
//     default:
//       {
//         printf("Message received, type 0x%04X\r\n", (CONVERT_TO_INT(zmBuf[SRSP_CMD_LSB_FIELD], zmBuf[SRSP_CMD_MSB_FIELD])));
//         zHexPrintf(zmBuf, (zmBuf[SRSP_LENGTH_FIELD] + SRSP_HEADER_SIZE));
//       }
//     }
//     zmBuf[SRSP_LENGTH_FIELD] = 0;
//   }
// }




