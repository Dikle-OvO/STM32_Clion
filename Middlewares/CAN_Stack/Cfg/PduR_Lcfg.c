//#include "BSP.h"

//#include "can_task.h"
#include "PduR_Lcfg.h"
#include "Com_Lcfg.h"
#include "Nm_Lcfg.h"
#include "util_macro.h"
#include "util_platform.h"
#include <string.h>

#include "main.h"

#ifdef PDUR_ENABLE_DYNAMIC_MEMORY
#else
static uint8_t            rxPdurBuffer[2048], txPdurBuffer[2048];
static flex_ring_buffer_t rxPdurRB, txPdurRB;
#endif

PduR_LinkType gPduRLink;
//TimerHandle_t pdurTimer1;

static void EventNotifyPduR(void)
{
//    can_task_notify(CAN_NOTIFY_PDUR);
	PduR_EventProcess(&gPduRLink);
}

static bool PdurTimerChangePeriod(int period)
{
//    return pdPASS == xTimerChangePeriod(pdurTimer1, pdMS_TO_TICKS(period), 0);
}

static bool PdurTimerStart(void)
{
//    BaseType_t ret = xTimerStart(pdurTimer1, 0);
//    if (pdPASS != ret) {
//        LOG_ERR("PduRTimer1 start failed:%d", ret);
//    }

//    return pdPASS == ret;
}

static bool PdurTimerStop(void)
{
//    return pdPASS == xTimerStop(pdurTimer1, 0);
}

//void PdurTimer1Cb(TimerHandle_t xTimer)
//{
//    PduR_TxTimeoutCallback(&gPduRLink);
//}

static void irq_disable()
{
//    local_irq_disable();
}
static void irq_enable()
{
//    local_irq_enable();
}

static int CanSendMsg(uint8_t port, uint8_t mb_idx, const uint32_t canid, const uint8_t can_flag, const uint8_t *data, const uint8_t size)
{
    int ret = 0;

#ifdef CONFIG_ENABLE_CANFD
    struct canfd_frame tx_frame = {
        .can_id = canid,
        .flags = can_flag,
        .len = size,
        .data = { 0 },
    };
#else
//    struct can_frame tx_frame = {
//        .can_id = canid,
//        .len = size,
//        .data = { 0 },
//    };
#endif

    // if ((data == NULL) || (size == 0) || (size > sizeof(tx_frame.Data))) {
    //    return -1;
    // }

    // memcpy(tx_frame.Data, (uint8_t *)data, 8);// origin:size
    HAL_StatusTypeDef status = CAN_SendData(&hcan, canid, 0, data, 8);  // CAN If send Msg

//		util_printf("PduR has been trig\r\n");
//    ret = ecual_can_transmit(&can1, mb_idx, &tx_frame);
		PduR_TxConfirmCallback(&gPduRLink, port, mb_idx, canid); //confirm
    return ret;
}

static void CanAbortSendMsg(uint8_t port, uint8_t mb_idx)
{
//    ecual_can_tx_abort(&can1, mb_idx);
}

const static PdrRCanFilter filterNM[] = {
    { 0x500, 0x700 },
};

void RecvNMCallback(const uint32_t canid, const uint8_t isCANFD, const uint8_t *data, uint8_t len)
{
    PduInfoType pdu;
    pdu.MetaDataPtr = NULL;
    pdu.SduDataPtr  = (uint8_t *)data;
    pdu.SduLength   = len;
    CanNm_RxIndication(0, &pdu);
}

const static PduR_ModuleConfig_t nm_cfg = {
    .module_id = PDUR_MID_NM,
    .can_filters = (PdrRCanFilter *)filterNM,
    .filter_count = U_COUNTOF(filterNM),
    .can_port = 0,
    .mb_idx = CAN_MAILBOX_NM,
    .rx_indication = RecvNMCallback,
    .tx_confirm = NULL,
    .tx_error = NULL,
};

const static PdrRCanFilter filterCom[] = {
    { 0x200, 0x000 },
};

void RecvComCallback(const uint32_t canid, const uint8_t isCANFD, const uint8_t *data, const uint8_t len)
{
    ComPduMetaDataType metaData;
    PduInfoType        pduInfo;

    metaData.channel = 0;
    metaData.canId = canid;
    pduInfo.MetaDataPtr = (uint8_t *)&metaData;
    pduInfo.SduDataPtr = (uint8_t *)data;
    pduInfo.SduLength = len;
//		printf("PduR Re Trig");
    Com_RxIndication(0, &pduInfo);
}

void SendComConfirmCallback(uint32_t canid) {
    if (canid == 0x100) { //此处的CAN ID就是发送报文的ID
        Com_TxConfirmation(MSG_TX_IDX_BLE_Status_0x100, E_OK);
    }
}

const static PduR_ModuleConfig_t com_cfg = {
    .module_id = PDUR_MID_COM,
    .can_filters = (PdrRCanFilter *)filterCom,
    .filter_count = U_COUNTOF(filterCom),
    .can_port = 2, //origin:COM_CHANNEL1，这个接口对应后续上传的时候RXIndiction指定的CAN口，对应的才会到达
    .mb_idx = CAN_MAILBOX_COM,
    .rx_indication = RecvComCallback,
    .tx_confirm = SendComConfirmCallback,
    .tx_error = NULL,
};

//const static PdrRCanFilter filterUDS[] = {
//    { 0x7CE, 0x7FF },
//    { 0x6CE, 0x7FF },
//};

//void UdsRxCallback(const uint32_t canid, const uint8_t isCANFD, const uint8_t *data, const uint8_t len)
//{
//    Nm_ModeType nm_mode = NM_MODE_BUS_SLEEP;
//    Nm_GetState(0, NULL, &nm_mode);
//    if (NM_MODE_NETWORK == nm_mode && (len >= 8)) {
//        UDSServerReceiveCAN(canid, (uint8_t *)data, len);
//    }
//}

//void UdsTxConfirmCallback(uint32_t canid)
//{
//    UDSSendConfirm(ISOTP_SEND_RESULT_SUCCESS);
//}

//void UdsTxErrorCallback(uint32_t status)
//{
//    UDSSendConfirm(ISOTP_SEND_RESULT_FAIL);
//}

//const static PduR_ModuleConfig_t uds_cfg = {
//    .module_id = PDUR_MID_UDS,
//    .can_filters = (PdrRCanFilter *)filterUDS,
//    .filter_count = sizeof(filterUDS) / sizeof(filterUDS[0]),
//    .can_port = 0,
//    .mb_idx = CAN_MAILBOX_UDS,
//    .rx_indication = UdsRxCallback,
//    .tx_confirm = UdsTxConfirmCallback,
//    .tx_error = UdsTxErrorCallback,
//};

//static PdrRCanFilter filterGateway_pub[] = {
//    { 0x000, 0x000 },
//};

//static void GatewayRxCallback_pub(const uint32_t canid, const uint8_t flags, const uint8_t *data, const uint8_t len)
//{
//    if (canid >= 0x200 && canid <= 0x300) {
//        // PduR_Send(&gPriPduRLink,PDUR_MID_GATEWAY_PRV, canid, flags,(uint8_t
//        // *)data,len,100);
//    }
//}

//const static PduR_ModuleConfig_t gateway_cfg = {
//    .module_id = PDUR_MID_GATEWAY_PUB,
//    .can_filters = (PdrRCanFilter *)filterGateway_pub,
//    .filter_count = ARRAY_SIZE(filterGateway_pub),
//    .can_port = COM_CHANNEL_DKM,
//    .mb_idx = CAN_MAILBOX_COM,
//    .rx_indication = GatewayRxCallback_pub,
//    .tx_confirm = NULL,
//    .tx_error = NULL,
//};

static PduR_ModuleType_t pub_modules[] = {
    {
     .config = &nm_cfg,
     .status = { true, true },
     },
    {
     .config = &com_cfg,
     .status = { true, true },
     },
//    {
//     .config = &uds_cfg,
//     .status = { true, true },
//     },
//    {
//     .config = &gateway_cfg,
//     .status = { true, true },
//     },
};

const static PduR_ConfigGroup_t configGroup = {
    .modules = pub_modules,
    .moudles_num = U_COUNTOF(pub_modules),
    .bsp_callbacks = {
                      .send_message = CanSendMsg,
                      .send_abort = CanAbortSendMsg,
                      .event_notify = EventNotifyPduR,
                      .timer_change_period = PdurTimerChangePeriod,
                      .timer_start = PdurTimerStart,
                      .timer_stop = PdurTimerStop,
                      .irq_disable = irq_disable,
                      .irq_enable = irq_enable,
                      },
};

void PduR_Config_Init(void)
{
    PduR_VersionInfoType version;
    PduR_GetVersion(&version);
    util_printf("PduR Version: %d.%d.%d\r\n", version.sw_major_version, version.sw_minor_version, version.sw_patch_version);

//    pdurTimer1 = xTimerCreate("PduRTimer1", pdMS_TO_TICKS(100), pdFALSE, NULL, PdurTimer1Cb);
//    if (NULL == pdurTimer1) {
//        LOG_ERR("PduRTimer1 create failed");
//    }

#ifdef PDUR_ENABLE_DYNAMIC_MEMORY
    PduR_Init(&gPduRLink, &configGroup);
#else
    PduR_Init(&gPduRLink, &configGroup, &txPdurRB, txPdurBuffer, sizeof(txPdurBuffer), &rxPdurRB, rxPdurBuffer, sizeof(rxPdurBuffer));
#endif
}
