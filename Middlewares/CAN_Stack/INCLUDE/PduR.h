#ifndef __PDUR_H__
#define __PDUR_H__

#include <assert.h>
#include "Platform_Types.h"
#include "Pdur_Cfg.h"

#ifndef PDUR_ENABLE_DYNAMIC_MEMORY
#include "flex_ringbuffer.h"
#endif

/** 获取数组元素的个数 */
#ifndef GET_ARRAY_NUM
#define GET_ARRAY_NUM(array) (sizeof(array) / sizeof((array)[0]))
#endif

typedef enum {
    PDUR_TX_RET_OK = 0,
    PDUR_TX_RET_NO_INPROGRESS = -1,
    PDUR_TX_RET_ERROR = -2,
    PDUR_TX_RET_TIMEOUT = -3,
    PDUR_TX_RET_UNINIT = -4,
    PDUR_TX_RET_PARA_ERROR = -5,
    PDUR_TX_RET_BUSOFF = -6,
} Pdur_TX_RET;

typedef struct
{
    uint8_t sw_major_version;
    uint8_t sw_minor_version;
    uint8_t sw_patch_version;
} PduR_VersionInfoType;

typedef struct
{
    uint32_t id;
    uint32_t mask;
} PdrRCanFilter;

#pragma pack(1)

typedef struct
{
    uint32_t can_id;
    uint8_t  flags;
    uint8_t  length;
    uint8_t  data[64];
} PduRCanFrame;

typedef struct
{
    uint8_t      module_id;
    uint8_t      can_port;
    uint8_t      mb_idx;
    uint32_t     timeout;
    PduRCanFrame frame;
} PudR_Node_t;
#pragma pack()

typedef struct
{
    uint8_t        module_id;            /* The id of the upper level module */
    uint8_t        can_port;             /* can_port for rx(filtering with the driver) & tx (sending on this port)*/
    uint8_t        mb_idx;               /* only used for tx, bound to module_id */
    uint8_t        filter_count;         /* number of canFilters */
    PdrRCanFilter *can_filters;          /* canid rx filters for modules, if rx (driverid & mask)  is equal to (id & mask), the message is passed to the upper layer */
    void (*tx_confirm)(uint32_t can_id); /* callback function for tx confirm */
    /* rx callback function,  when a message is received and passesboth the CAN port check and the filter check,  is passed to the upper layers.  */
    void (*rx_indication)(const uint32_t identifier, const uint8_t flag, const uint8_t *data, const uint8_t len);
    void (*tx_error)(uint32_t status); /* callback function for tx error handling*/
} PduR_ModuleConfig_t;

typedef struct
{
    bool tx_enable; /* internal flag for enabling tx */
    bool rx_enable; /* internal flag for enabling rx */
} PduR_ModuleStatus_t;

typedef struct
{
    const PduR_ModuleConfig_t *config; /* module config , it is recommended to use const static declarations*/
    PduR_ModuleStatus_t        status; /*module status，ram data, will init in pudr_init()*/
} PduR_ModuleType_t;

/* callback function Configuration parameters */
/* Note: Except for the print function, all other functions must have a valid call. It is mandatory to implement them !!! */
typedef struct
{
    int (*send_message)(const uint8_t can_port, const uint8_t mb_idx, const uint32_t can_id, const uint8_t can_flag, const uint8_t *data, const uint8_t size);
    void (*send_abort)(uint8_t can_port, uint8_t mb_idx); /* abort sending with specified mailbox*/
    void (*event_notify)(void); /* event notify function, the `PduR_EventProcess()` must be executed when the event is triggered.*/
    bool (*timer_change_period)(int period);
    bool (*timer_start)(void); /* start the timer, If the timer times out, the `PduR_TxTimeoutCallback` function must be called. */
    bool (*timer_stop)(void);
    void (*irq_disable)(void); /* if use dynamic memory, irq_disable and irq_enable should be implemented, or it will cause memory leak */
    void (*irq_enable)(void);  /* if use ringbuffer, irq_disable and irq_enable could be freertos api */
} Pdur_BspCallback_t;

typedef struct
{
    PduR_ModuleType_t       *modules;
    uint8_t                  moudles_num;
    const Pdur_BspCallback_t bsp_callbacks;
} PduR_ConfigGroup_t;

typedef struct
{
    const PduR_ConfigGroup_t *config_group;
    // internal data
    void       *tx_list;
    void       *rx_list;
    uint8_t     send_status;
    PudR_Node_t send_info;
} PduR_LinkType;


#ifdef PDUR_ENABLE_DYNAMIC_MEMORY

/**
 * @Description: init the pdur library with dynamic memory allocation
 * @
 * @param   *link :
 * @param   *ConfigPtr :
 * @return  [void]
 */
void PduR_Init(PduR_LinkType *link, const PduR_ConfigGroup_t *ConfigPtr);
#else

/**
 * @Description: init the pdur library with static memory allocation
 * @
 * @param   *link :
 * @param   *ConfigPtr :
 * @param   *txRingBuff :
 * @param   *txBuff :
 * @param   txBufSize :
 * @param   *rxRingBuff :
 * @param   *rxBuff :
 * @param   rxBufSize :
 * @return  [void]
 */
void PduR_Init(PduR_LinkType *link, const PduR_ConfigGroup_t *ConfigPtr, flex_ring_buffer_t *txRingBuff, uint8_t *txBuff, int txBufSize, flex_ring_buffer_t *rxRingBuff, uint8_t *rxBuff, int rxBufSize);
#endif

/**
 * @Description: Upper layers send data via the pdur library
 * @
 * @param   *link : pdur handle link
 * @param   module_id : this id is configured in the pdur_config.c, pdur will send message with this module's can_port and mb_idx
 * @param   can_id : cand id of the message
 * @param   flag : can flag of the message, such as EFF, RTR, ERR, etc.  pdur only transparent transmission is performed
 * @param   *payload : The payload to be sent
 * @param   size : The size of the payload to be sent
 * @param   timeout : If the message is not sent successfully within the timeout period, it is discarded. If timeout set 0, waits until sent success.
 * @return  [void]
 */
Pdur_TX_RET PduR_Send(PduR_LinkType *link, uint8_t module_id, uint32_t can_id, uint8_t flag, uint8_t *payload, int32_t size, int timeout);

/**
 * @fn      PduR_EventProcess()
 * @brief
 * @retval  None
 */
/**
 * @Description: event handler. Receive and send buffer data , passthrough rxconfirm and txerror to upper layers.
 * @             PduR_EventProcess should be called when event_notify() is triggered
 * @
 * @param   *link :
 * @return  [void]
 */
void PduR_EventProcess(PduR_LinkType *link);

/**
 * @Description: flush the rx buffer, discard all the messages in the buffer.
 * @
 * @param   *link :
 * @return  [void]
 */
void PduR_RxBufferFlush(PduR_LinkType *link);

/**
 * @Description: flush the tx buffer, discard all the messages in the buffer.
 * @
 * @param   *link :
 * @return  [void]
 */
void PduR_TxBufferFlush(PduR_LinkType *link);

/**
 * @Description: control the rx enable or disable of the specified module. usually used for 0x28 service.
 * @
 * @param   *link :
 * @param   module_id :
 * @param   enable :
 * @return  [void]
 */
void PduR_RxControl(PduR_LinkType *link, uint8_t module_id, bool enable);

/**
 * @Description: control the tx enable or disable of the specified module. usually used for 0x28 service.
 * @
 * @param   *link :
 * @param   module_id :
 * @param   enable :
 * @return  [void]
 */
void PduR_TxControl(PduR_LinkType *link, uint8_t module_id, bool enable);

/**
 * @Description: busoff callback function, called when busoff is detected. dpur will stop message sending and wait for bus recovery.
 * @
 * @param   *link :
 * @param   can_port : not used in this function, reserved for future muti-port support
 * @return  [void]
 */
void PduR_BusOff(PduR_LinkType *link, const uint8_t can_port);

/**
 * @Description: busoff recovery callback function, called when busoff recovery is detected. dpur will resume message sending.
 * @
 * @param   *link :
 * @param   can_port :  not used in this function, reserved for future muti-port support
 * @return  [void]
 */
void PduR_BusOffRecovery(PduR_LinkType *link, const uint8_t can_port);

/**
 * @Description:
 * @
 * @param   *versioninfo :
 * @return  [void]
 */
void PduR_GetVersion(PduR_VersionInfoType *versioninfo);

/**
 * @Description: driver layer callback function, when a message is received, driver layer should call this function.
 * @
 * @param   *link :
 * @param   can_port : the message's can_port, only matched the module's can_port configurition will be passed to upper layers
 * @param   can_id :
 * @param   flag : can flag of the message, such as EFF, RTR, ERR, etc.  pdur only transparent transmission is performed
 * @param   *payload :
 * @param   size :
 * @return  [void]
 */
void PduR_RxCallback(PduR_LinkType *link, const uint8_t can_port, uint32_t can_id, uint8_t flag, uint8_t *payload, int32_t size);

/**
 * @Description: driver layer callback function, when a message is sent successfully, driver layer should call this function.
 * @
 * @param   *link :
 * @param   can_port :
 * @param   mb_idx :
 * @param   can_id :
 * @return  [void]
 */
void PduR_TxConfirmCallback(PduR_LinkType *link, const uint8_t can_port, const uint8_t mb_idx, uint32_t can_id);  // canif

/**
 * @Description: pdur tx timeout callback function, called by Pdur_bsp_callbacks's timer
 * @
 * @
 * @param   *link :
 * @return  [void]
 */
void PduR_TxTimeoutCallback(PduR_LinkType *link);


#endif
