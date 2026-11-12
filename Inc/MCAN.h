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
 * completed for MCAN_TX_STALL_MS, the CAN peripheral is recovered (abort mailboxes, enter + leave init mode). */
#ifndef MCAN_TX_STALL_MS
#define MCAN_TX_STALL_MS          3000u
#endif
/* Consecutive recoveries without a successful transmission in between before NVIC_SystemReset(). 0 = never reset.
 * Default off: a node alone on the bus (no ACK) would otherwise reset itself, and a reset toggles the relays. */
#ifndef MCAN_TX_STALL_RESET_AFTER
#define MCAN_TX_STALL_RESET_AFTER 0u
#endif
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

// Called from MCAN_Update_10ms() (main context) on a TX stall. Weak empty default in MCAN.c;
// LOG.c provides the real one (0x620 error event, RTC backup id before a reset).
void MCAN_TxStallCallback(e_McanTxEvent event, const s_McanDiag* diag);


// Required interface ! - This fcns have to be defined in application
//void MCAN_NotifySlaveStateReceived(uint8_t nodeid, e_NodeStatus status);

#endif /* INC_MCAN_H_ */
