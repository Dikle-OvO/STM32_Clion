#ifndef __ISOTP_TYPES__
#define __ISOTP_TYPES__

/**************************************************************
 * compiler specific defines
 *************************************************************/
#ifdef __GNUC__
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define ISOTP_BYTE_ORDER_LITTLE_ENDIAN
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#else
#error "unsupported byte ordering"
#endif
#endif

/**************************************************************
 * OS specific defines
 *************************************************************/
#ifdef _WIN32
#define snprintf _snprintf
#endif

#ifdef _WIN32
#define ISOTP_BYTE_ORDER_LITTLE_ENDIAN
#define __builtin_bswap8  _byteswap_uint8
#define __builtin_bswap16 _byteswap_uint16
#define __builtin_bswap32 _byteswap_uint32
#define __builtin_bswap64 _byteswap_uint64
#endif

/**************************************************************
 * internal used defines
 *************************************************************/


/*the max data size of canfd*/
#define BUFF_SIZE 64


/*  invalid bs */
#define ISOTP_INVALID_BS       0xFFFF

/* can fram defination */
#if defined(ISOTP_BYTE_ORDER_LITTLE_ENDIAN)
typedef struct {
    uint8_t reserve_1:4;
    uint8_t type:4;
    uint8_t reserve_2[BUFF_SIZE-1];
} IsoTpPciType;

typedef struct {
    uint8_t SF_DL:4;
    uint8_t type:4;
    uint8_t data[BUFF_SIZE-1];
} IsoTpSingleFrame;

typedef struct {
    uint8_t FF_DL_high:4;
    uint8_t type:4;
    uint8_t FF_DL_low;
    uint8_t data[BUFF_SIZE-2];
} IsoTpFirstFrame;

typedef struct {
    uint8_t reserve_1:4;
    uint8_t type:4;
    uint8_t SF_DL;
    uint8_t data[62];
} IsoTp_canfdSingleFrame;

typedef struct {
    uint8_t reserve_1:4;
    uint8_t type:4;
    uint8_t reserve_2;
    uint8_t FF_DL_Arr[4];
    uint8_t data[58];
} IsoTp_canfdFirstFrame;

typedef struct {
    uint8_t SN:4;
    uint8_t type:4;
    uint8_t data[BUFF_SIZE-1];
} IsoTpConsecutiveFrame;

typedef struct {
    uint8_t FS:4;
    uint8_t type:4;
    uint8_t BS;
    uint8_t STmin;
    uint8_t reserve[BUFF_SIZE-3];
} IsoTpFlowControl;

#else

typedef struct {
    uint8_t type:4;
    uint8_t reserve_1:4;
    uint8_t reserve_2[BUFF_SIZE-1];
} IsoTpPciType;

/*
* single frame
* +-------------------------+-----+
* | byte #0                 | ... |
* +-------------------------+-----+
* | nibble #0   | nibble #1 | ... |
* +-------------+-----------+ ... +
* | PCIType = 0 | SF_DL     | ... |
* +-------------+-----------+-----+
*/
typedef struct {
    uint8_t type:4;
    uint8_t SF_DL:4;
    uint8_t data[BUFF_SIZE-1];
} IsoTpSingleFrame;

typedef struct {
    uint8_t type:4;
    uint8_t reserve_1:4;
    uint8_t SF_DL;
    uint8_t data[62];
} IsoTp_canfdSingleFrame;
/*
* first frame
* +-------------------------+-----------------------+-----+
* | byte #0                 | byte #1               | ... |
* +-------------------------+-----------+-----------+-----+
* | nibble #0   | nibble #1 | nibble #2 | nibble #3 | ... |
* +-------------+-----------+-----------+-----------+-----+
* | PCIType = 1 | FF_DL                             | ... |
* +-------------+-----------+-----------------------+-----+
*/
typedef struct {
    uint8_t type:4;
    uint8_t FF_DL_high:4;
    uint8_t FF_DL_low;
    uint8_t data[BUFF_SIZE-2];
} IsoTpFirstFrame;

typedef struct {
    uint8_t type:4;
    uint8_t reserve_1:4;
    uint8_t reserve_2;
    uint8_t FF_DL_Arr[4];
    uint8_t data[58];
} IsoTp_canfdFirstFrame;
/*
* consecutive frame
* +-------------------------+-----+
* | byte #0                 | ... |
* +-------------------------+-----+
* | nibble #0   | nibble #1 | ... |
* +-------------+-----------+ ... +
* | PCIType = 0 | SN        | ... |
* +-------------+-----------+-----+
*/
typedef struct {
    uint8_t type:4;
    uint8_t SN:4;
    uint8_t data[BUFF_SIZE-1];
} IsoTpConsecutiveFrame;

/*
* flow control frame
* +-------------------------+-----------------------+-----------------------+-----+
* | byte #0                 | byte #1               | byte #2               | ... |
* +-------------------------+-----------+-----------+-----------+-----------+-----+
* | nibble #0   | nibble #1 | nibble #2 | nibble #3 | nibble #4 | nibble #5 | ... |
* +-------------+-----------+-----------+-----------+-----------+-----------+-----+
* | PCIType = 1 | FS        | BS                    | STmin                 | ... |
* +-------------+-----------+-----------------------+-----------------------+-----+
*/
typedef struct {
    uint8_t type:4;
    uint8_t FS:4;
    uint8_t BS;
    uint8_t STmin;
    uint8_t reserve[BUFF_SIZE-3];
} IsoTpFlowControl;

#endif

typedef struct {
    uint8_t ptr[BUFF_SIZE];
} IsoTpDataArray;

typedef struct {
    union {
        IsoTpPciType            common;
        IsoTpSingleFrame        single_frame;
        IsoTp_canfdSingleFrame  single_frame_canfd;
        IsoTp_canfdFirstFrame   first_frame_canfd;
        IsoTpFirstFrame         first_frame;
        IsoTpConsecutiveFrame   consecutive_frame;
        IsoTpFlowControl        flow_control;
        IsoTpDataArray          data_array;
    } as;
} IsoTpCanMessage;

/**************************************************************
 * protocol specific defines
 *************************************************************/

/* Private: Protocol Control Information (PCI) types, for identifying each frame of an ISO-TP message.
 */
typedef enum {
    ISOTP_PCI_TYPE_SINGLE             = 0x0,
    ISOTP_PCI_TYPE_FIRST_FRAME        = 0x1,
    TSOTP_PCI_TYPE_CONSECUTIVE_FRAME  = 0x2,
    ISOTP_PCI_TYPE_FLOW_CONTROL_FRAME = 0x3
} IsoTpProtocolControlInformation;

/* Private: Protocol Control Information (PCI) flow control identifiers.
 */
typedef enum {
    PCI_FLOW_STATUS_CONTINUE = 0x0,
    PCI_FLOW_STATUS_WAIT     = 0x1,
    PCI_FLOW_STATUS_OVERFLOW = 0x2
} IsoTpFlowStatus;

/* ISOTP sender status */
typedef enum {
    ISOTP_SEND_STATUS_IDLE,
    ISOTP_SEND_STATUS_WAIT_SEND,
    ISOTP_SEND_STATUS_INPROGRESS_SF,
    ISOTP_SEND_STATUS_INPROGRESS_FF,
    ISOTP_SEND_STATUS_INPROGRESS_CF,
    ISOTP_SEND_STATUS_ERROR,
} IsoTpSendStatusTypes;

/* ISOTP receiver status */
typedef enum {
    ISOTP_RECEIVE_STATUS_IDLE,
    ISOTP_RECEIVE_STATUS_WAIT_SEND_FC_FLOW,
    ISOTP_RECEIVE_STATUS_WAIT_SEND_FC_CONTINUE,
    ISOTP_RECEIVE_STATUS_INPROGRESS,
    ISOTP_RECEIVE_STATUS_FULL,
} IsoTpReceiveStatusTypes;

#endif

