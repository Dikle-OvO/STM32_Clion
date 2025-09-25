#include "PduR.h"
#include <stdlib.h>
#include <string.h>
#include "util_platform.h"

#ifdef PDUR_ENABLE_DYNAMIC_MEMORY
#include "util_list.h"
#endif

#define PRINT_LOG(format, ...)              \
    do {                                    \
        util_printf(format, ##__VA_ARGS__); \
    } while (0)


#define UDS_SW_MAJOR_VERSION 2
#define UDS_SW_MINOR_VERSION 0
#define UDS_SW_PATCH_VERSION 0

typedef enum {
    PDUR_SEND_STATUS_IDLE,
    PDUR_SEND_STATUS_SUCCESS,
    PDUR_SEND_STATUS_INPROGRESS,
    PDUR_SEND_STATUS_TIMEOUT,
    PDUR_SEND_STATUS_ERROR,
    PDUR_SEND_STATUS_BUSOFF,
} PdurSendStatusTypes; // 发送状态类型

typedef enum {
    PDUR_RECEIVE_STATUS_IDLE,
    PDUR_RECEIVE_STATUS_INPROGRESS,
    PDUR_RECEIVE_STATUS_FULL,
} PdurReceiveStatusTypes; // 接收状态类型

static void irq_disable(PduR_LinkType *link)
{
    if (link->config_group->bsp_callbacks.irq_disable) {
        link->config_group->bsp_callbacks.irq_disable();
    }
}

static void irq_enable(PduR_LinkType *link)
{
    if (link->config_group->bsp_callbacks.irq_enable) {
        link->config_group->bsp_callbacks.irq_enable();
    }
}

#ifdef PDUR_ENABLE_DYNAMIC_MEMORY
typedef struct {
    struct list_head node;
    uint8_t         *data;  // malloc
    uint16_t         len;
} MessageNode_t;

static void *malloc_message_node(uint16_t size)
{
    MessageNode_t *new_message_node = malloc(sizeof(MessageNode_t));
    if (new_message_node != NULL) {
        new_message_node->data = malloc(size);
        if (new_message_node->data != NULL) {
            return new_message_node;
        } else {
            free(new_message_node);
        }
    }
    return NULL;
}

static void free_message_node(MessageNode_t *node)
{
    free(node->data);
    free(node);
    return;
}

// 初始化链表
static void list_init(void **list_head)
{
    *list_head = malloc(sizeof(struct list_head));
    assert(list_head);
    INIT_LIST_HEAD((struct list_head *)*list_head);
}
#else
static void list_init(void **list_head, flex_ring_buffer_t *ringBuffHandle, uint8_t *buff, int bufSize)
{
    assert(ringBuffHandle);
    assert(buff);
    *list_head = ringBuffHandle;
    flex_ring_buffer_init(ringBuffHandle, buff, bufSize);
    return;
}
#endif

static bool list_is_empty(void *list)
{
#ifdef PDUR_ENABLE_DYNAMIC_MEMORY
    return (bool)list_empty((struct list_head *)list);
#else
    return flex_ring_buffer_num_items((flex_ring_buffer_t *)list) == 0;
#endif
}

// 入队
static bool list_enqueue(PduR_LinkType *link, void *list, uint8_t *data, int dataLen)
{
    bool ret = false;
#ifdef PDUR_ENABLE_DYNAMIC_MEMORY
    MessageNode_t *new_message_node = malloc_message_node(dataLen);
    if (new_message_node != NULL) {
        new_message_node->len = dataLen;
        memcpy(new_message_node->data, data, dataLen);
        irq_disable(link);
        list_add_tail(&new_message_node->node, (struct list_head *)list);
        irq_enable(link);
        ret = true;
    }
//				static int i=0;
//				util_printf("enqueue:%d\r\n",++i);
#else
    irq_disable(link);
    ret = flex_ring_buffer_queue((flex_ring_buffer_t *)list, data, dataLen);
    irq_enable(link);
#endif
    return ret;
}

// 出队
static uint8_t list_dequeue(PduR_LinkType *link, void *list, uint8_t *data, int dataLen)
{
#ifdef PDUR_ENABLE_DYNAMIC_MEMORY
    uint8_t           ret_len = 0;
    struct list_head *head = (struct list_head *)list;
    if (!list_is_empty(head)) {
        MessageNode_t *message_node = list_entry(head->next, MessageNode_t, node);
        assert(message_node->len <= dataLen);
        memcpy(data, message_node->data, message_node->len);
        ret_len = message_node->len;
        irq_disable(link);
        list_del(&message_node->node);
        irq_enable(link);
        free_message_node(message_node);
			
//				static int i=0;
//				util_printf("dequeue:%d\r\n",++i);
    }
    return ret_len;
#else
    return flex_ring_buffer_dequeue((flex_ring_buffer_t *)list, data, dataLen);
#endif
		

}

// 查找模块配置
static void lookUpModule(PduR_LinkType *link, uint8_t module_id, PduR_ModuleType_t **module)
{
    for (int i = 0; i < link->config_group->moudles_num; i++) {
        PduR_ModuleType_t *temp_module = &link->config_group->modules[i];
        if (temp_module->config->module_id == module_id) {
            *module = temp_module;
            return;
        }
    }
    *module = NULL;
}

static void send_message(PduR_LinkType *link, PduR_ModuleType_t *module)
{
    int tempTimeout = 0;
    if ((tempTimeout = link->send_info.timeout - util_get_ticks()) > 0) {
        link->send_info.can_port = module->config->can_port;
        link->send_info.mb_idx = module->config->mb_idx;
        link->send_status = PDUR_SEND_STATUS_INPROGRESS;
        link->config_group->bsp_callbacks.timer_change_period(tempTimeout);
        link->config_group->bsp_callbacks.timer_start();
        link->config_group->bsp_callbacks.send_message(
            link->send_info.can_port,
            link->send_info.mb_idx,
            link->send_info.frame.can_id,
            link->send_info.frame.flags,
            link->send_info.frame.data,
            link->send_info.frame.length);
    } else {
        if (module->config->tx_error) {
            module->config->tx_error(0);
        }
        link->config_group->bsp_callbacks.event_notify();
    }
}

static void send_progress_idle(PduR_LinkType *link)
{
    PduR_ModuleType_t *module = NULL;
    if (list_dequeue(link, link->tx_list, (uint8_t *)&link->send_info, sizeof(link->send_info)) > 0) {
        lookUpModule(link, link->send_info.module_id, &module);
        if (module != NULL) {
            send_message(link, module);
        } else {
            PRINT_LOG("PDUR: the module %d's config can not find\n",
                      link->send_info.module_id);
        }
    }
}

static void send_progress_success(PduR_LinkType *link)
{
    PduR_ModuleType_t *module = NULL;
    link->config_group->bsp_callbacks.timer_stop();
    lookUpModule(link, link->send_info.module_id, &module);
    if ((module != NULL) && (module->config->tx_confirm != NULL)) {
        module->config->tx_confirm(link->send_info.frame.can_id);
    }
    link->send_status = PDUR_SEND_STATUS_IDLE;
    if (!list_is_empty(link->tx_list)) {
        link->config_group->bsp_callbacks.event_notify();
    }
}

static void send_progress_timeout(PduR_LinkType *link)
{
    PduR_ModuleType_t *module = NULL;
    PRINT_LOG("%s:%x\n", "PDUR: tx timeout", link->send_info.frame.can_id);
    link->config_group->bsp_callbacks.send_abort(link->send_info.can_port,
                                                 link->send_info.mb_idx);
    lookUpModule(link, link->send_info.module_id, &module);
    if ((module != NULL) && (module->config->tx_error != NULL)) {
        module->config->tx_error(0);
    }
    link->send_status = PDUR_SEND_STATUS_IDLE;
    if (!list_is_empty(link->tx_list)) {
        link->config_group->bsp_callbacks.event_notify();
    }
}

static void send_progress_error(PduR_LinkType *link)
{
    PduR_ModuleType_t *module = NULL;
    PRINT_LOG("%s\n", "PDUR: busoff");
    link->config_group->bsp_callbacks.timer_stop();
    link->config_group->bsp_callbacks.send_abort(link->send_info.can_port,
                                                 link->send_info.mb_idx);
    lookUpModule(link, link->send_info.module_id, &module);
    if ((module != NULL) && (module->config->tx_error != NULL)) {
        module->config->tx_error(0);
    }
    link->send_status = PDUR_SEND_STATUS_BUSOFF;
}

static void send_progress(PduR_LinkType *link)
{
    switch (link->send_status) {
        case PDUR_SEND_STATUS_IDLE:
            send_progress_idle(link);
            break;
        case PDUR_SEND_STATUS_SUCCESS:
            send_progress_success(link);
            break;
        case PDUR_SEND_STATUS_INPROGRESS:
            break;
        case PDUR_SEND_STATUS_TIMEOUT:
            send_progress_timeout(link);
            break;
        case PDUR_SEND_STATUS_ERROR:
            send_progress_error(link);
            break;
        case PDUR_SEND_STATUS_BUSOFF:
            break;
        default:
            PRINT_LOG("%s\n", "PDUR:status error");
            link->send_status = PDUR_SEND_STATUS_IDLE;
            break;
    }
}

static bool router_condition_check(PduR_ModuleType_t *module,
                                   PudR_Node_t       *msg_node)
{
    if (module->status.rx_enable &&
        module->config->can_port == msg_node->can_port) {
        for (int j = 0; j < module->config->filter_count; j++) {
            PdrRCanFilter *filter = &(module->config->can_filters[j]);
            if ((msg_node->frame.can_id & filter->mask) ==
                (filter->id & filter->mask)) {
                return true;
            }
        }
    }
    return false;
}

static void receive_progress(PduR_LinkType *link)
{
    PudR_Node_t msg_node;
    if (list_dequeue(link, link->rx_list, (uint8_t *)&msg_node, sizeof(msg_node)) > 0) {
        for (int i = 0; i < link->config_group->moudles_num; i++) {
            PduR_ModuleType_t *module = &(link->config_group->modules[i]);
            if (router_condition_check(module, &msg_node)) {
                PduRCanFrame *frame = &msg_node.frame;
                if (module->config->rx_indication != NULL) {
                    module->config->rx_indication(frame->can_id, frame->flags, frame->data, frame->length);
                }
            }
        }
    }
    if (!list_is_empty(link->rx_list)) {
        link->config_group->bsp_callbacks.event_notify();
    }
}

#ifdef PDUR_ENABLE_DYNAMIC_MEMORY
void PduR_Init(PduR_LinkType *link, const PduR_ConfigGroup_t *ConfigPtr)
#else
void PduR_Init(PduR_LinkType *link, const PduR_ConfigGroup_t *ConfigPtr, flex_ring_buffer_t *txRingBuff, uint8_t *txBuff, int txBufSize, flex_ring_buffer_t *rxRingBuff, uint8_t *rxBuff, int rxBufSize)
#endif
{
    assert(link);
    assert(ConfigPtr);
    for (int i = 0; i < ConfigPtr->moudles_num; i++) {
        ConfigPtr->modules[i].status.rx_enable = true;
        ConfigPtr->modules[i].status.tx_enable = true;
    }
#ifdef PDUR_ENABLE_DYNAMIC_MEMORY
    list_init(&link->tx_list);
    list_init(&link->rx_list);
#else
    list_init(&link->tx_list, txRingBuff, txBuff, txBufSize);
    list_init(&link->rx_list, rxRingBuff, rxBuff, rxBufSize);
#endif
    link->config_group = ConfigPtr;
    link->send_status = PDUR_SEND_STATUS_IDLE;
}

// 发送PDU
Pdur_TX_RET PduR_Send(PduR_LinkType *link, uint8_t module_id, uint32_t can_id, uint8_t flag, uint8_t *payload, int32_t size, int timeout)
{
    assert(link);
    assert(payload);
    assert(size <= 64);
    if (link->send_status == PDUR_SEND_STATUS_BUSOFF) {
        return PDUR_TX_RET_BUSOFF;
    }
    PduR_ModuleType_t *module = NULL;
    lookUpModule(link, module_id, &module);
    if (module == NULL) {
        return PDUR_TX_RET_PARA_ERROR;
    } else if (module->status.tx_enable == false) {
        return PDUR_TX_RET_OK;
    }

    Pdur_TX_RET ret = PDUR_TX_RET_OK;
    PudR_Node_t msg_node;
    msg_node.module_id = module_id;
    msg_node.timeout = util_get_ticks() + (timeout == 0 ? 1000 : timeout);
    msg_node.frame.can_id = can_id;
    msg_node.frame.flags = flag;
    msg_node.frame.length = size;
    memcpy(&msg_node.frame.data, payload, size);

    if (list_enqueue(
            link,
            link->tx_list,
            (uint8_t *)&msg_node,
            offsetof(PudR_Node_t, frame) + offsetof(PduRCanFrame, data) + size)) {
        ret = PDUR_TX_RET_OK;
//							static int i=0;
//							util_printf("enqueue:%d\r\n",++i);
    } else {
        PRINT_LOG("PDUR:buff full,id = 0x%03X\n", can_id);
        ret = PDUR_TX_RET_ERROR;
    }
    link->config_group->bsp_callbacks.event_notify();
    return ret;
}

void PduR_RxCallback(PduR_LinkType *link, const uint8_t can_port, uint32_t can_id, uint8_t flag, uint8_t *payload, int32_t size)
{
    assert(link);
    assert(payload);
    assert(size <= 64);
    PudR_Node_t msg_node;
    msg_node.can_port = can_port;
    msg_node.frame.can_id = can_id;
    msg_node.frame.flags = flag;
    msg_node.frame.length = size;
    memcpy(&msg_node.frame.data, payload, size);

    list_enqueue(
        link,
        link->rx_list,
        (uint8_t *)&msg_node,
        offsetof(PudR_Node_t, frame) + offsetof(PduRCanFrame, data) + size);
    link->config_group->bsp_callbacks.event_notify();
}

void PduR_TxConfirmCallback(PduR_LinkType *link, const uint8_t can_port, const uint8_t mb_idx, uint32_t can_id)
{
    if (link->send_status == PDUR_SEND_STATUS_INPROGRESS) {
        if (can_id == link->send_info.frame.can_id) {
            link->send_status = PDUR_SEND_STATUS_SUCCESS;
        }
        link->config_group->bsp_callbacks.event_notify();
    }
}

void PduR_TxTimeoutCallback(PduR_LinkType *link)
{
    if (link->send_status == PDUR_SEND_STATUS_INPROGRESS) {
        link->send_status = PDUR_SEND_STATUS_TIMEOUT;
        link->config_group->bsp_callbacks.event_notify();
    }
}

void PduR_EventProcess(PduR_LinkType *link)
{
    send_progress(link);
    receive_progress(link);
}

void PduR_BusOff(PduR_LinkType *link, const uint8_t can_port)
{
    link->send_status = PDUR_SEND_STATUS_ERROR;
    link->config_group->bsp_callbacks.event_notify();
}

void PduR_BusOffRecovery(PduR_LinkType *link, const uint8_t can_port)
{
    link->send_status = PDUR_SEND_STATUS_IDLE;
    link->config_group->bsp_callbacks.event_notify();
}

void PduR_RxBufferFlush(PduR_LinkType *link)
{
    PudR_Node_t message_node;
    while ((list_dequeue(link, link->rx_list, (uint8_t *)&message_node, sizeof(message_node)) > 0));
}

void PduR_TxBufferFlush(PduR_LinkType *link)
{
    PudR_Node_t message_node;
    link->send_status = PDUR_SEND_STATUS_IDLE;
    link->config_group->bsp_callbacks.timer_stop();
    link->config_group->bsp_callbacks.send_abort(link->send_info.can_port,
                                                 link->send_info.mb_idx);
    memset(&link->send_info, 0, sizeof(link->send_info));
    while ((list_dequeue(link, link->tx_list, (uint8_t *)&message_node, sizeof(message_node)) > 0));
}

void PduR_RxControl(PduR_LinkType *link, uint8_t module_id, bool enable)
{
    PduR_ModuleType_t *module = NULL;
    lookUpModule(link, link->send_info.module_id, &module);
    if (module == NULL) {
        module->status.rx_enable = enable;
    }
    return;
}

void PduR_TxControl(PduR_LinkType *link, uint8_t module_id, bool enable)
{
    PduR_ModuleType_t *module = NULL;
    lookUpModule(link, link->send_info.module_id, &module);
    if (module == NULL) {
        module->status.tx_enable = enable;
    }
    return;
}

void PduR_GetVersion(PduR_VersionInfoType *versioninfo)
{
    if (versioninfo) {
        (versioninfo)->sw_major_version = UDS_SW_MAJOR_VERSION;
        (versioninfo)->sw_minor_version = UDS_SW_MINOR_VERSION;
        (versioninfo)->sw_patch_version = UDS_SW_PATCH_VERSION;
    }
}
