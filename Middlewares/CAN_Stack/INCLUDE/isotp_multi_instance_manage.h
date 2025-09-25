#ifndef ISOTP_INSTANCE_MANAGE_H
#define ISOTP_INSTANCE_MANAGE_H

#include "isotp.h"

typedef enum {
    TP_STATE_RUNNING,  // 接收数据并发送
    TP_STATE_SUSPEND,  // 接收数据，但不发送
    TP_STATE_STOPPED,  // 停止接收数据并不发送
} tp_state;

void isotp_manage_init(ISOTP_RET (*send_message)(const uint32_t can_id, const uint8_t *data, const uint8_t size));

/**
 * @Description:
 * @
 * @param   sendid :
 * @param   recvid :
 * @param   enable_flow_control : 是否使能流控帧功能, instance manage 内部有一个默认的 instance，在未调用add_instance时使用isotp_manage_send，会发送一个不需要流控的tp帧
 * @param   rx_callback :
 * @param   rx_error_callback :
 * @param   tx_result_callback :
 * @param   user_data : 作为用户参数在以上三个回调函数中传递
 * @return  [void]
 */
ISOTP_RET isotp_manage_add_instance(uint32_t         sendid,
                                    uint32_t         recvid,
                                    uint16_t         max_recv_size,
                                    rxCallback       rx_callback,
                                    txResultCallback tx_result_callback,
                                    rxErrorCallback  rx_error_callback,
                                    bool             disable_flow_control,
                                    void            *user_data);

int isotp_manage_send(uint32_t sendid, const uint8_t *data, const uint16_t size);

/**
 * @Description:
 * @
 * @param   sendid : instace id
 * @param   state : 目标状态
 * @param   timeout : 目标状态持续的时间，单位ms，0表示一直维持当前状态
 * @param   state_after_timeout :  超时后进入的状态
 * @return  [void]
 */
int isotp_manage_control(uint32_t sendid, tp_state state, uint32_t timeout, tp_state state_after_timeout);

int isotp_manage_get_state(uint32_t sendid, tp_state *state);

void isotp_manage_poll();


// callback function for pdur or driver
void isotp_manage_rx_callback(uint32_t canId, uint8_t *payload, const uint8_t payload_size);

void isotp_manage_tx_result_callback(uint32_t canId, IsoTpSendResultTypes status);

#endif  // ISOTP_INSTANCE_MANAGE_H