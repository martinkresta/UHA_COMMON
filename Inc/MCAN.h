/*
 * MCAN.h
 *
 *  Created on: Aug 14, 2021
 *      Author: Martin
 */


#ifndef INC_MCAN_H_
#define INC_MCAN_H_


#include "main.h"
#include "circbuf.h"


/// boolean for false condition
#ifndef FALSE
#define FALSE (0)
#endif
/// boolean for true condition
#ifndef TRUE
#define TRUE  (1)
#endif

#define RX_NMT_BUFF_SIZE 200
#define RX_BUFF_SIZE 80
#define TX_BUFF_SIZE 100      // needs 6,4kB of free Heap = 0x1900
#define RX_BUFF_WARN RX_BUFF_SIZE/2
#define TX_BUFF_WARN TX_BUFF_SIZE/2
#define MAX_NUM_OF_NODES	8
#define CO_DEFAULT_HB_TO	10000  // 10sec

/* ---- TX supervision (see VarDefs/can_tx_stall_2026-11-12.md) ----
 * A node sends its heartbeat every CO_DEFAULT_HB_TO/2 = 5 s, so a healthy node has something to send at least
 * every 5 s and completes it within milliseconds. If frames are pending (TX ring or HW mailboxes) and no frame has
 * completed for MCAN_TX_STALL_MS, the CAN peripheral is recovered (abort mailboxes, enter + leave init mode).
 * Independent of that, no completed frame at all for MCAN_TX_DEAD_MS is a stall too (covers a software-side stall,
 * e.g. a corrupted TX ring pointer, where nothing looks pending). */
#ifndef MCAN_TX_STALL_MS
#define MCAN_TX_STALL_MS          3000u
#endif
#ifndef MCAN_TX_DEAD_MS
#define MCAN_TX_DEAD_MS           12000u
#endif
/* Consecutive recoveries without a successful transmission in between before NVIC_SystemReset(). 0 = never reset.
 * 5 -> reset after about 18 s (pending case) or 27 s (dead case) without any transmitted frame.
 * Note: a node alone on the bus (no ACK) resets itself too, and a reset toggles the relays. */
#ifndef MCAN_TX_STALL_RESET_AFTER
#define MCAN_TX_STALL_RESET_AFTER 5u
#endif
/* Error counter monitor: immediate log on a new EWG/EPV/BOFF (at most one per HOLDOFF), and a summary at the end
 * of each WINDOW if TEC or REC peaked at >= LEVEL or a flag was seen. */
#ifndef MCAN_ERRMON_WINDOW_MS
#define MCAN_ERRMON_WINDOW_MS     60000u
#endif
#ifndef MCAN_ERRMON_HOLDOFF_MS
#define MCAN_ERRMON_HOLDOFF_MS    10000u
#endif
#ifndef MCAN_ERRMON_LEVEL
#define MCAN_ERRMON_LEVEL         16u
#endif

/* Error report payload byte 0 (MCAN_ReportError() -> 0x620 eleError, bytes 2..7 of the frame) */
#define MCAN_ERR_TX_STALL     0x01  // [1, event 1=recovery 2=reset, TEC, REC, ESR&0x7F (LEC<<4|BOFF<<2|EPVF<<1|EWGF), recoveries sat.]
#define MCAN_ERR_ERRCNT       0x02  // [2, flags (bit0 EWG, bit1 EPV, bit2 BOF, bit7 1=immediate 0=summary), TEC peak, REC peak, error types (bit0 stuff, 1 form, 2 ACK, 3 bit rec., 4 bit dom., 5 CRC), TEC now]
#define MCAN_ERR_TX_STALL_SW  0x03  // [3, HAL state at stall, ring level sat., flags (bit0..2 TME0..2, bit3 TxBuf NULL, bit4 ring not empty, bit5 HAL state repaired), mailbox-busy count sat., reason 1=pending 2=dead]
#define MCAN_ERR_PTR_REPAIR   0x04  // [4, mask (bit0 TxBuf, bit1 Rx0Buf, bit2 Rx1Buf, bit3 CanHw, bit7 shadow also bad), corrupted value uint32 LE]
/* 1 = MCAN_Init() sets ABOM (automatic bus-off recovery) regardless of the CubeMX setting. */
#ifndef MCAN_FORCE_ABOM
#define MCAN_FORCE_ABOM           1
#endif


#define ID_RANGE	0x0F    // maximal nodeID



#define NMT 0x000
#define HB	0x700


#define NMT_CMD_START           0x01
#define NMT_CMD_STOP            0x02
#define NMT_CMD_PREOP           0x80
#define NMT_CMD_RESET           0x81
#define NMT_CMD_RESET_COM       0x82

#define NODEID_IOBOARD_D			1
#define NODEID_IOBOARD_U			2
#define NODEID_TECHM		 			3
#define NODEID_ELECON					4
#define NODEID_RECU 					5
#define NODEID_EVSE						6
#define NODEID_ELECON_D       7    // elecon dilna



typedef struct
{
	CAN_RxHeaderTypeDef header;
	uint8_t data[8];
}s_CanRxMsg;

typedef struct
{
	CAN_TxHeaderTypeDef header;
	uint8_t data[8];
}s_CanTxMsg;

typedef enum
{
    eNS_NMT_BOOTUP                 = 0x00,
    eNS_NMT_STOPPED                = 0x04,
    eNS_NMT_RUN				             = 0x05,
    eNS_NMT_PREOPERATIONAL         = 0x7F,
		eNS_NMT_UNKNOWN								 = 0xFF
}e_NodeStatus;


typedef enum
{
	eET_NONE = 0,
	eET_MALLOC_FAIL,
	eET_RX0_BUFF_WARN,
	eET_RX1_BUFF_WARN,
	eET_TX_BUFF_WARN,
	eET_RX0_BUFF_FULL,
	eET_RX1_BUFF_FULL,
	eET_TX_BUFF_FULL
}e_ErrType;


typedef struct
{
	uint8_t											nodeId;
	uint16_t               	 		timeout_cnt;
	uint16_t               	 		timeout;
	e_NodeStatus           	 		canStatus;
}s_CanNode;


typedef struct
{
	s_CanNode Node;  						// this node
	CAN_HandleTypeDef	*CanHw;		// underlying CAN driver HAL handle
	CB_handle	 Rx0Buf;
	CB_handle	 Rx1Buf;
	CB_handle	 TxBuf;						// TX buffer
	s_CanTxMsg HbMsg;						// prepared template for heartbeat Ms
	s_CanNode	 *SlaveNodes;		  // Array for holding slave nodes
}CAN_handle;


typedef enum
{
	eMTE_RECOVERY = 1,   // TX stall detected, CAN peripheral recovered
	eMTE_RESET    = 2    // TX stall persists, NVIC_SystemReset() follows right after the callback
}e_McanTxEvent;

// cheap CAN diagnostics, read with MCAN_GetDiag() (SysInfo, UHAMON, debugger)
typedef struct
{
	uint32_t txOkCnt;          // frames completed with TXOK
	uint32_t txErrCnt;         // mailbox completions with TERR (transmission error)
	uint32_t txArbLostCnt;     // mailbox completions with ALST (arbitration lost; NART=0 retries anyway)
	uint32_t txAbortCnt;       // mailbox completions without TXOK/TERR/ALST (aborted by recovery)
	uint32_t txHwBusyCnt;      // MCAN_Transmit() calls with a frame waiting but no free mailbox
	uint32_t txRingDropCnt;    // frames lost because the TX ring was full (oldest overwritten)
	uint32_t lastProgressTick; // HAL_GetTick() of last TXOK, or of last time nothing was pending
	uint32_t lastTxOkTick;     // HAL_GetTick() of last TXOK only
	uint32_t lastRecoveryTick; // HAL_GetTick() of last recovery
	uint32_t errLogCnt;        // MCAN_ERR_ERRCNT reports sent
	uint32_t ptrRepairCnt;     // corrupted mMcan pointers restored from the shadow copy
	uint32_t halStateRepairCnt;// hcan->State found outside READY/LISTENING at a recovery
	uint16_t halErrSeen;       // all HAL_CAN_ERROR_* bits (EWG..CRC) seen since start
	uint32_t stallRecoveryCnt; // total recoveries since start
	uint32_t esrAtStall;       // CAN ESR / TSR captured at the last recovery
	uint32_t tsrAtStall;
	uint32_t busOffCnt;        // BOFF rising edges seen by the 10 ms poll (short bus-offs with ABOM can be missed)
	uint32_t errPassiveCnt;    // EPVF rising edges seen by the 10 ms poll
	uint16_t stallFailCnt;     // recoveries since the last successful transmission
	uint16_t txRingLevel;      // current TX ring fill
	uint16_t txRingMax;        // TX ring high-water mark
	uint8_t  tecMax;           // highest transmit error counter seen
	uint8_t  recMax;           // highest receive error counter seen
}s_McanDiag;


void MCAN_Init(CAN_HandleTypeDef	* can, uint8_t id);

void MCAN_Update_10ms(void);

void MCAN_Transmit(void);

void MCAN_Start(void);

e_NodeStatus MCAN_GetNodeStatus(uint8_t nodeId);

uint8_t MCAN_GetRxMessage(s_CanRxMsg* msg);

uint8_t MCAN_SendFrame(uint16_t cobid, uint8_t* data, uint8_t dlc);

s_CanNode* MCAN_GetNodesPt(void);

const s_McanDiag* MCAN_GetDiag(void);   // fills txRingLevel on each call

uint32_t MCAN_GetEsr(void);             // live CAN ESR (TEC/REC/LEC/BOFF/EPVF/EWGF)

uint32_t MCAN_GetTsr(void);             // live CAN TSR (mailbox state)

uint32_t MCAN_GetHalErrorCode(void);    // hcan->ErrorCode (HAL_CAN_ERROR_* bits, sticky)

// Called from MCAN_Update_10ms() (main context) with a 6 byte payload (MCAN_ERR_*). beforeReset = 1: NVIC_SystemReset()
// follows right after. Weak empty default in MCAN.c; LOG.c provides the real one (0x620 eleError, RTC backup id).
void MCAN_ReportError(const uint8_t payload[6], uint8_t beforeReset);

#ifdef MCAN_UNIT_TEST
CAN_handle* MCAN_TestGetHandle(void);
#endif


// Required interface ! - This fcns have to be defined in application
//void MCAN_NotifySlaveStateReceived(uint8_t nodeid, e_NodeStatus status);

#endif /* INC_MCAN_H_ */
