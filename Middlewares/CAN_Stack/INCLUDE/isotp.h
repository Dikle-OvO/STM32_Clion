#ifndef __ISOTP_H__
#define __ISOTP_H__

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "isotp_defines.h"
#ifdef __cplusplus
#include <stdint.h>

extern "C" {
#endif

typedef enum {
    ISOTP_FRAME_RESUTL_IDLE,
    ISOTP_SEND_RESULT_WAIT,
    ISOTP_SEND_RESULT_SUCCESS,
    ISOTP_SEND_RESULT_FAIL,
} IsoTpSendResultTypes;

typedef enum {
    ISOTP_RET_OK         = 0,
    ISOTP_RET_ERROR      = -1,
    ISOTP_RET_INPROGRESS = -2,
    ISOTP_RET_OVERFLOW   = -3,
    ISOTP_RET_WRONG_SN   = -4,
    ISOTP_RET_NO_DATA    = -5,
    ISOTP_RET_TIMEOUT    = -6,
    ISOTP_RET_LENGTH     = -7,
}ISOTP_RET;

#define ISOTP_PROTOCOL_RESULT_OK            0
#define ISOTP_PROTOCOL_RESULT_TIMEOUT_A    -1
#define ISOTP_PROTOCOL_RESULT_TIMEOUT_BS   -2
#define ISOTP_PROTOCOL_RESULT_TIMEOUT_CR   -3
#define ISOTP_PROTOCOL_RESULT_WRONG_SN     -4
#define ISOTP_PROTOCOL_RESULT_INVALID_FS   -5
#define ISOTP_PROTOCOL_RESULT_UNEXP_PDU    -6
#define ISOTP_PROTOCOL_RESULT_WFT_OVRN     -7
#define ISOTP_PROTOCOL_RESULT_BUFFER_OVFLW -8
#define ISOTP_PROTOCOL_RESULT_ERROR        -9


typedef ISOTP_RET (*sendCanFunPtr)(const uint32_t arbitration_id, const uint8_t *data, const uint8_t size);
typedef uint32_t (*getMsFunPtr)(void);
typedef void (*printFuncPtr)(const char *format, ...);
typedef void (*lockPtr)(void);
typedef void (*rxCallback)(uint32_t canid, const uint8_t *data, const uint16_t size, void* user_data);
typedef void (*rxErrorCallback)(uint32_t canid, int status, void* user_data);
typedef void (*txResultCallback)(uint32_t canid, int status, void* user_data);

typedef struct {
    uint16_t As_ms;
    uint16_t Bs_ms;
    uint16_t Ar_ms;
    uint16_t Cr_ms;
    uint8_t BlockSize;           /*流控帧BS,Max number of messages the receiver can receive at one time*/
    uint8_t STmin;               /*流控帧ST*,minimum time gap allowed between the transmission of consecutive frame*/
    uint8_t max_WFT_Number;      /*Maximum number of times to wait for a flow control frame to be sent*/
    uint8_t frame_padding_char;  /*message padding characters*/
    uint8_t canfd;               /* 0:can; 1:canfd */
} isotp_configPara;


/**
 * @brief Struct containing the data for linking an application to a CAN instance.
 * The data stored in this struct is used internally and may be used by software programs
 * using this library.
 */
typedef struct IsoTpLink {
    /* sender paramters */
    uint32_t send_id; /* used to reply consecutive frame */
    /* message buffer */
    uint8_t *send_buffer;
    uint16_t send_buf_size;
    uint16_t send_size;
    uint16_t send_offset;
    /* multi-frame flags */
    uint8_t send_sn;
    uint16_t send_bs_remain; /* Remaining block size */
    uint8_t send_st_min;     /* Separation Time between consecutive frames, unit millis */
    uint8_t send_wtf_count;  /* Maximum number of FC.Wait frame transmissions  */
    uint32_t send_timer_st;  /* Last time send consecutive frame */

    uint32_t send_timer;
    uint8_t send_status;
    int send_protocol_result;
    

    /* receiver paramters */
    uint32_t receive_id;
    /* message buffer */
    uint8_t *receive_buffer;
    uint16_t receive_buf_size;
    uint16_t receive_size;
    uint16_t receive_offset;
    /* multi-frame control */
    uint8_t receive_dlc;
    uint8_t receive_sn;
    uint8_t receive_bs_count;  /* Maximum number of FC.Wait frame transmissions  */
    uint32_t receive_timer;
    uint8_t receive_status;
    int recv_protocol_result;
    
    const isotp_configPara *config;
    sendCanFunPtr uds_send_can; /* send can message. should return ISOTP_RET_OK when success.*/
    getMsFunPtr uds_get_ms;     /* get millisecond */
    printFuncPtr print_func;

    rxCallback rx_callback;
    txResultCallback tx_result_callback;
    rxErrorCallback rx_error_callback;
    void* user_data; // it will be passed to the above functions above (rx_callback，tx_result_callback，rx_error_callback)
    
    uint8_t frame_type ;
    IsoTpSendResultTypes frame_status;
    IsoTpCanMessage frameCache;
    uint8_t frameCacheLen ;
    bool disable_flow_control;
} IsoTpLink;

typedef struct {
    uint32_t sendid; /*transfer address*/
    uint32_t recvid; /*transfer address*/
    uint16_t recv_buff_size;
    rxCallback rx_callback;
    txResultCallback tx_result_callback;
    rxErrorCallback rx_error_callback;
    void* user_data; // it will be passed to the above functions above (rx_callback，tx_result_callback，rx_error_callback)
    sendCanFunPtr uds_send_can; /* send can message. should return ISOTP_RET_OK when success.*/
    getMsFunPtr uds_get_ms;     /* get millisecond */
    printFuncPtr print_func;
    bool disable_flow_control;   
    const isotp_configPara *config;
} Isotp_initPara;

/**
 * @brief Initialises the ISO-TP library.
 *
 * @param link The @code IsoTpLink @endcode instance used for transceiving data.
 * @param initPara The parameter used to initialize the library.
 */
void isotp_init_link(IsoTpLink *link, Isotp_initPara *initPara);

/**
 * @brief Polling function; call this function periodically to handle timeouts, send consecutive frames, etc.
 *
 * @param link The @code IsoTpLink @endcode instance used.
 */
void isotp_poll(IsoTpLink *link);

/**
 * @brief Sends ISO-TP frames via CAN, using the ID set in the initialising function.
 *
 * Single-frame messages will be sent immediately when calling this function.
 * Multi-frame messages will be sent consecutively when calling isotp_poll.
 *
 * @param link The @code IsoTpLink @endcode instance used for transceiving data.
 * @param payload The payload to be sent. (Up to 4095 bytes).
 * @param size The size of the payload to be sent.
 *
 * @return Possible return values:
 *  - @code ISOTP_RET_OVERFLOW @endcode
 *  - @code ISOTP_RET_INPROGRESS @endcode
 *  - @code ISOTP_RET_OK @endcode
 *  - The return value of the user shim function uds_send_can().
 */
int isotp_send(IsoTpLink *link, const uint8_t *payload, uint16_t size);


/**
 * @brief Handles incoming CAN messages.
 * Determines whether an incoming message is a valid ISO-TP frame or not and handles it accordingly.
 *
 * @param link The @code IsoTpLink @endcode instance used for transceiving data.
 * @param data The data received via CAN.
 * @param len The length of the data received.
 */
void isotp_on_can_message(IsoTpLink *link, uint32_t canid, uint8_t *data, uint8_t len);

void isotp_send_confirm(IsoTpLink *link, IsoTpSendResultTypes result);
#ifdef __cplusplus
}
#endif

#endif // __ISOTP_H__
