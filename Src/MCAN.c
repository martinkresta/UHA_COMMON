/*
 * MCAN.c
 *
 *  Created on: Aug 14, 2021
 *      Author: Martin
 */

#include "UI.h"
#include "MCAN.h"
#include "circbuf.h"
#include <string.h>


uint16_t Filter_Calc_Mask_Code (uint16_t from,uint16_t to);
void Set_Filter_Id_List (CAN_HandleTypeDef *hcan, uint8_t fifo, uint8_t bank, uint16_t id_1, uint8_t rtr_1, uint16_t id_2, uint8_t rtr_2, uint16_t id_3, uint8_t rtr_3, uint16_t id_4, uint8_t rtr_4);
void Set_Filter_Id_Mask (CAN_HandleTypeDef *hcan, uint8_t fifo,  uint8_t bank, uint16_t id_1_from, uint16_t id_1_to, uint16_t id_2_from, uint16_t id_2_to);

static uint8_t Send(s_CanTxMsg msg);
static void PollTxCompletion(uint32_t now);
static void SuperviseTx(uint32_t now);
static void RecoverTx(uint32_t now);

static CAN_handle mMcan;

static s_McanDiag mDiag;
static uint8_t mLastBoff;
static uint8_t mLastEpv;

static s_CanNode mNodes[MAX_NUM_OF_NODES];



// initialization of CAN node, and buffers
void MCAN_Init(CAN_HandleTypeDef	* can, uint8_t id)
{
	uint8_t bank_offset = 0;
	uint8_t i;

	mMcan.Rx0Buf = CB_Create(sizeof(s_CanRxMsg),RX_NMT_BUFF_SIZE);
	mMcan.Rx1Buf = CB_Create(sizeof(s_CanRxMsg),RX_BUFF_SIZE);
	mMcan.TxBuf = CB_Create(sizeof(s_CanTxMsg),TX_BUFF_SIZE);
	if (mMcan.Rx0Buf == NULL || mMcan.Rx1Buf == NULL ||mMcan.TxBuf == NULL)
	{
		// TBD
	}

	mMcan.CanHw = can;
	// for devices with two cans check which HW is used and adjust filter banks offset
	#ifdef CAN2
	if(mMcan.CanHw->Instance == CAN2)
	{
		bank_offset = 15;
	}
	#endif


	mMcan.Node.nodeId = id;
	mMcan.Node.timeout = CO_DEFAULT_HB_TO/2;
	mMcan.Node.timeout_cnt = 0;

	mMcan.HbMsg.header.DLC = 1;
	mMcan.HbMsg.header.StdId = HB + mMcan.Node.nodeId;
	mMcan.HbMsg.header.ExtId = 0x00;
	mMcan.HbMsg.header.RTR = CAN_RTR_DATA;
	mMcan.HbMsg.header.IDE = CAN_ID_STD;

	// init network
	for (i = 0; i < MAX_NUM_OF_NODES; i++)
	{
		mNodes[i].timeout = CO_DEFAULT_HB_TO;
		mNodes[i].canStatus = eNS_NMT_UNKNOWN;
		mNodes[i].nodeId = i;
	}
	mNodes[mMcan.Node.nodeId].canStatus = mMcan.Node.canStatus;



	// set the filter of FIFO0 for hearbeats (0x700 - 0x70F)
	Set_Filter_Id_Mask(mMcan.CanHw, CAN_FILTER_FIFO0, 1 + bank_offset , HB, HB+ID_RANGE, 0,0); // HeartBeats from all devices => FMI = 0/4

	// set the filter of FIFO1 for all other messages (0x000 - 0x6FF)
	Set_Filter_Id_Mask(mMcan.CanHw, CAN_FILTER_FIFO1, 2 + bank_offset , 0x000, 0x6FF, 0,0);


	// EWG/EPV/BOF only make HAL record them in hcan->ErrorCode (sticky, readable in debugger). No node enables the
	// CAN SCE interrupt in the NVIC, so this adds no interrupt load; HAL evaluates ERRI inside the RX interrupts.
	HAL_CAN_ActivateNotification(mMcan.CanHw, CAN_IT_RX_FIFO0_MSG_PENDING | CAN_IT_RX_FIFO1_MSG_PENDING | CAN_IT_LAST_ERROR_CODE| CAN_IT_ERROR |
	                             CAN_IT_ERROR_WARNING | CAN_IT_ERROR_PASSIVE | CAN_IT_BUSOFF);

	mMcan.CanHw->Instance->MCR &= ~0x00010000;	// enable CAN in debug mode (clear DBF bit)

#if MCAN_FORCE_ABOM
	// Automatic bus-off recovery. ELECON, ELECON_D and ELECON_TEST had AutoBusOff = DISABLE in CubeMX and there was
	// no software recovery, so a bus-off was permanent until reset. MCAN_Init() runs after HAL_CAN_Init() and
	// before HAL_CAN_Start(), i.e. still in initialization mode, where MCR may be changed.
	SET_BIT(mMcan.CanHw->Instance->MCR, CAN_MCR_ABOM);
	mMcan.CanHw->Init.AutoBusOff = ENABLE;
#endif

	memset(&mDiag, 0, sizeof(mDiag));
	mLastBoff = 0;
	mLastEpv = 0;
}



// Start CANOPEN protocols
void MCAN_Start(void)
{
	HAL_CAN_Start(mMcan.CanHw);  // start can
	mDiag.lastProgressTick = HAL_GetTick();
	mMcan.Node.canStatus = eNS_NMT_RUN;
	mNodes[mMcan.Node.nodeId].canStatus = mMcan.Node.canStatus;
}

s_CanNode* MCAN_GetNodesPt(void)
{
	return mNodes;
}

// Periodic update function for synchronous CAN handling
void MCAN_Update_10ms(void)
{

	int i;
	s_CanRxMsg msg;
	s_CanTxMsg TxMsg;
	s_CanNode* tmp;
	uint32_t	TxMailbox;


	// Check messages from FIFO 0 (NMT, HB)

	while(0 == CB_Get(mMcan.Rx0Buf,(uint8_t*)&msg))  // process all messages in buffer
	{
		if (msg.header.StdId >= HB && msg.header.StdId <= HB + ID_RANGE) // received HB
		{
			uint8_t nodeId = msg.header.StdId - HB;
			if (nodeId < MAX_NUM_OF_NODES)  // valid heartbeat received
			{
				mNodes[nodeId].canStatus = (e_NodeStatus)msg.data[0];
				mNodes[nodeId].timeout_cnt = 0;
			}
		}
		if(msg.header.StdId == NMT)   // NMT command received
		{
		  if(msg.data[1] == mMcan.Node.nodeId)
		  {
		    switch (msg.data[0])  // NMT commants
		    {
		      case NMT_CMD_START:
		        break;
		      case NMT_CMD_STOP:
            break;
		      case NMT_CMD_PREOP:
            break;
		      case NMT_CMD_RESET:
		        NVIC_SystemReset();
            break;
		      case NMT_CMD_RESET_COM:
		        NVIC_SystemReset();
            break;
		    }
		  }
		}


	}

	// Check timeout of all nodes
	for(i = 0; i < MAX_NUM_OF_NODES; i++)
	{
		mNodes[i].timeout_cnt += 10;  // 10ms period of execution of this fcn
		if (mNodes[i].timeout_cnt > mNodes[i].timeout &&  // HB period elapsed
				mNodes[i].canStatus != eNS_NMT_UNKNOWN)
		{
			mNodes[i].timeout_cnt = 0;
			mNodes[i].canStatus = eNS_NMT_UNKNOWN;
		}
	}

	// Send heartbeat if it's time
	mMcan.Node.timeout_cnt += 10;
	if (mMcan.Node.timeout_cnt >= mMcan.Node.timeout)
	{
		mMcan.Node.timeout_cnt = 0;
		mMcan.HbMsg.data[0] = mMcan.Node.canStatus;
		Send(mMcan.HbMsg);
	}

	SuperviseTx(HAL_GetTick());
}


// function to be called periodically at a rate of transmitting can messages. (for example every 5 ms)
// One CAN message is is sent if TX buffer is not empty
void MCAN_Transmit(void)
{
	s_CanTxMsg TxMsg;
	uint32_t	TxMailbox;

	// Must run before HAL_CAN_AddTxMessage(): hardware clears RQCPx/TXOKx when the mailbox is reused.
	PollTxCompletion(HAL_GetTick());

	if  (0 == CB_Probe(mMcan.TxBuf,(uint8_t*)&TxMsg))  // fetch the message from the buffer
	{
		if (HAL_OK == HAL_CAN_AddTxMessage(mMcan.CanHw, &TxMsg.header, TxMsg.data, &TxMailbox))  // transmit the message
		{
			CB_Remove(mMcan.TxBuf);  // remove the message from the buffer only if transmission was succesfull
		}
		else
		{
			mDiag.txHwBusyCnt++;     // all 3 mailboxes pending
		}
	}
}

const s_McanDiag* MCAN_GetDiag(void)
{
	mDiag.txRingLevel = CB_NumOfItems(mMcan.TxBuf);
	return &mDiag;
}

uint32_t MCAN_GetEsr(void)
{
	return READ_REG(mMcan.CanHw->Instance->ESR);
}

uint32_t MCAN_GetTsr(void)
{
	return READ_REG(mMcan.CanHw->Instance->TSR);
}

uint32_t MCAN_GetHalErrorCode(void)
{
	return mMcan.CanHw->ErrorCode;
}

// weak default; LOG.c overrides it
__attribute__((weak)) void MCAN_TxStallCallback(e_McanTxEvent event, const s_McanDiag* diag)
{
	(void)event;
	(void)diag;
}

// Gets a message from receive buffer to be processed by application
// returns 1 if message is available, otherwise return 0
uint8_t MCAN_GetRxMessage( s_CanRxMsg* msg)
{
	if (0 == CB_Get(mMcan.Rx1Buf,(uint8_t*)msg))
	{
		return 1;
	}

	return 0;
}



// returns CAN satatus of connected node
e_NodeStatus MCAN_GetNodeStatus(uint8_t nodeId)
{
	if (nodeId < MAX_NUM_OF_NODES)
	{
		if (nodeId == mMcan.Node.nodeId)  // this node
		{
			return mMcan.Node.canStatus;
		}
		else   // remote node
		{
			return mNodes[nodeId].canStatus;
		}
	}
	return eNS_NMT_UNKNOWN;
}


// Sends general CAN data message
uint8_t MCAN_SendFrame(uint16_t cobid, uint8_t* data, uint8_t dlc)
{
	s_CanTxMsg msg;
	msg.header.DLC = dlc;
	msg.header.StdId = cobid;
	msg.header.ExtId = 0x00;
	msg.header.RTR = CAN_RTR_DATA;
	msg.header.IDE = CAN_ID_STD;

	memcpy(&msg.data[0], data, dlc);
	return Send(msg);
}


/*************** Private Fcns **************/

// CAN transmit
// returns 0 = queued, 1 = queued but the oldest frame in the full TX ring was overwritten
static uint8_t Send(s_CanTxMsg msg)
{
	uint8_t dropped = 0;
	uint16_t level;
	uint32_t primask = __get_PRIMASK();

	__disable_irq();   // CB_Put() itself is not protected; keep put + bookkeeping atomic against CB_Probe/Remove
	if (CB_NumOfItems(mMcan.TxBuf) >= TX_BUFF_SIZE)
	{
		dropped = 1;
		mDiag.txRingDropCnt++;
	}
	CB_Put(mMcan.TxBuf,(uint8_t*) &msg);  // instert to Tx buffer (overwrites the oldest when full)
	level = CB_NumOfItems(mMcan.TxBuf);
	if (level > mDiag.txRingMax)
	{
		mDiag.txRingMax = level;
	}
	__set_PRIMASK(primask);

	return dropped;
}

#define MCAN_TME_ALL (CAN_TSR_TME0 | CAN_TSR_TME1 | CAN_TSR_TME2)

// Counts finished mailboxes (RQCPx) and clears their status bits. Only called from MCAN_Transmit().
static void PollTxCompletion(uint32_t now)
{
	static const uint32_t rqcp[3] = {CAN_TSR_RQCP0, CAN_TSR_RQCP1, CAN_TSR_RQCP2};
	static const uint32_t txok[3] = {CAN_TSR_TXOK0, CAN_TSR_TXOK1, CAN_TSR_TXOK2};
	static const uint32_t terr[3] = {CAN_TSR_TERR0, CAN_TSR_TERR1, CAN_TSR_TERR2};
	static const uint32_t alst[3] = {CAN_TSR_ALST0, CAN_TSR_ALST1, CAN_TSR_ALST2};
	uint32_t tsr = READ_REG(mMcan.CanHw->Instance->TSR);
	uint32_t clr = 0;
	uint8_t i;

	for (i = 0; i < 3; i++)
	{
		if (tsr & rqcp[i])
		{
			clr |= rqcp[i];
			if (tsr & txok[i])
			{
				mDiag.txOkCnt++;
				mDiag.lastProgressTick = now;
				mDiag.stallFailCnt = 0;
			}
			else if (tsr & terr[i])
			{
				mDiag.txErrCnt++;
			}
			else if (tsr & alst[i])
			{
				mDiag.txArbLostCnt++;
			}
			else
			{
				mDiag.txAbortCnt++;
			}
		}
	}
	if (clr)
	{
		WRITE_REG(mMcan.CanHw->Instance->TSR, clr);  // rc_w1: clears RQCPx together with TXOKx/ALSTx/TERRx
	}
}

// 10 ms: error counter statistics + stall detection
static void SuperviseTx(uint32_t now)
{
	uint32_t esr = READ_REG(mMcan.CanHw->Instance->ESR);
	uint32_t tsr = READ_REG(mMcan.CanHw->Instance->TSR);
	uint8_t tec = (uint8_t)((esr & CAN_ESR_TEC) >> CAN_ESR_TEC_Pos);
	uint8_t rec = (uint8_t)((esr & CAN_ESR_REC) >> CAN_ESR_REC_Pos);
	uint8_t boff = (esr & CAN_ESR_BOFF) ? 1 : 0;
	uint8_t epv = (esr & CAN_ESR_EPVF) ? 1 : 0;

	if (tec > mDiag.tecMax) mDiag.tecMax = tec;
	if (rec > mDiag.recMax) mDiag.recMax = rec;
	if (boff && !mLastBoff) mDiag.busOffCnt++;
	if (epv && !mLastEpv) mDiag.errPassiveCnt++;
	mLastBoff = boff;
	mLastEpv = epv;

	if (((tsr & MCAN_TME_ALL) == MCAN_TME_ALL) && CB_IsEmpty(mMcan.TxBuf))
	{
		mDiag.lastProgressTick = now;   // nothing to send is not a stall
		return;
	}
	if ((uint32_t)(now - mDiag.lastProgressTick) >= MCAN_TX_STALL_MS)
	{
		RecoverTx(now);
	}
}

// Abort the pending mailboxes, then enter and leave initialization mode. This is the bus-off recovery the reference
// manual prescribes for ABOM = 0, and it also clears any other stuck transmit state. Filters, IER and BTR are kept;
// HAL state stays READY (HAL_CAN_Stop/Start are not used because HAL_CAN_Start() sets HAL_CAN_STATE_ERROR when it
// cannot leave init mode within 10 ms, e.g. on a busy or shorted bus, and AddTxMessage would then fail for good).
static void RecoverTx(uint32_t now)
{
	CAN_TypeDef* can = mMcan.CanHw->Instance;
	uint32_t loops = 0;

	mDiag.esrAtStall = READ_REG(can->ESR);
	mDiag.tsrAtStall = READ_REG(can->TSR);
	mDiag.stallRecoveryCnt++;
	mDiag.stallFailCnt++;
	mDiag.lastProgressTick = now;   // next attempt one full MCAN_TX_STALL_MS later

	WRITE_REG(can->TSR, CAN_TSR_ABRQ0 | CAN_TSR_ABRQ1 | CAN_TSR_ABRQ2);

	SET_BIT(can->MCR, CAN_MCR_INRQ);
	while (((READ_REG(can->MSR) & CAN_MSR_INAK) == 0U) && (loops < 100000U))   // bounded, well below 1 ms at 80 MHz
	{
		loops++;
	}
#if MCAN_FORCE_ABOM
	SET_BIT(can->MCR, CAN_MCR_ABOM);
#endif
	CLEAR_BIT(can->MCR, CAN_MCR_INRQ);   // hardware leaves init mode after 11 recessive bits; no need to wait

#if (MCAN_TX_STALL_RESET_AFTER > 0)
	if (mDiag.stallFailCnt > MCAN_TX_STALL_RESET_AFTER)
	{
		MCAN_TxStallCallback(eMTE_RESET, &mDiag);
		NVIC_SystemReset();
		return;
	}
#endif
	MCAN_TxStallCallback(eMTE_RECOVERY, &mDiag);
}


// interrupt handlers - callbacks
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
 {
	s_CanRxMsg tmp;
	HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &tmp.header, tmp.data);
	if (mMcan.CanHw == hcan)
	{
		CB_Put(mMcan.Rx0Buf,(uint8_t*) &tmp);
		//UI_LED_B_SetMode(eUI_BLINK_ONCE);  // blue blink
	}

}


void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
	s_CanRxMsg tmp;
	HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO1, &tmp.header, tmp.data);
	if (mMcan.CanHw == hcan)
	{
		CB_Put(mMcan.Rx1Buf,(uint8_t*) &tmp);
		//UI_LED_B_SetMode(eUI_BLINK_ONCE);  // blue blink
	}

}


/*********************************************************************/
void Set_Filter_Id_List (CAN_HandleTypeDef *hcan, uint8_t fifo, uint8_t bank, uint16_t id_1, uint8_t rtr_1, uint16_t id_2, uint8_t rtr_2, uint16_t id_3, uint8_t rtr_3, uint16_t id_4, uint8_t rtr_4)
{
	CAN_FilterTypeDef			 filterInitStructure;

	filterInitStructure.SlaveStartFilterBank  = 14;


	filterInitStructure.FilterBank									= bank;
	filterInitStructure.FilterMode									=	CAN_FILTERMODE_IDLIST;
	filterInitStructure.FilterScale									=	CAN_FILTERSCALE_16BIT;
	filterInitStructure.FilterFIFOAssignment				=	fifo;


	filterInitStructure.FilterIdLow 						= id_1 << 5;
	if(rtr_1)
	{
		filterInitStructure.FilterIdLow 					+= 16;
	}

	filterInitStructure.FilterMaskIdLow 				= id_2 << 5;
	if(rtr_2)
	{
		filterInitStructure.FilterMaskIdLow 			+= 16;
	}

	filterInitStructure.FilterIdHigh 						= id_3 << 5;
	if(rtr_3)
	{
		filterInitStructure.FilterIdHigh 					+= 16;
	}

	filterInitStructure.FilterMaskIdHigh 				= id_4 << 5;
	if(rtr_4)
	{
		filterInitStructure.FilterMaskIdHigh 			+= 16;
	}
	filterInitStructure.FilterActivation				=	ENABLE;


	HAL_CAN_ConfigFilter(hcan,&filterInitStructure);
}

/************************************************************************
  Function	  : CAN_FILTER_Filter_Mask
  Input		 		: -
  Output	  	: -
  Description : -
************************************************************************/
void Set_Filter_Id_Mask (CAN_HandleTypeDef *hcan, uint8_t fifo,  uint8_t bank, uint16_t id_1_from, uint16_t id_1_to, uint16_t id_2_from, uint16_t id_2_to)
{
	CAN_FilterTypeDef			 filterInitStructure;

	filterInitStructure.SlaveStartFilterBank  = 14;


	filterInitStructure.FilterBank									= bank;


	filterInitStructure.FilterMode									=	CAN_FILTERMODE_IDMASK;
	filterInitStructure.FilterScale									=	CAN_FILTERSCALE_16BIT;
	filterInitStructure.FilterFIFOAssignment				=	fifo;

	filterInitStructure.FilterIdHigh								=	id_2_from << 5;
	filterInitStructure.FilterIdLow 								=	id_1_from << 5;
	filterInitStructure.FilterMaskIdHigh						= Filter_Calc_Mask_Code(id_2_from,id_2_to);
	filterInitStructure.FilterMaskIdLow 						= Filter_Calc_Mask_Code(id_1_from,id_1_to);

	filterInitStructure.FilterActivation						=	ENABLE;

	HAL_CAN_ConfigFilter(hcan,&filterInitStructure);
}

/************************************************************************
  Function	  : CAN_FILTER_Calc_Mask_Code
  Input		 		: -
  Output	  	: -
  Description : -
************************************************************************/
uint16_t Filter_Calc_Mask_Code (uint16_t from,uint16_t to)
{
	uint16_t	help			 	= 0x00;
	uint8_t		shift_cnt		= 0x00;
	uint8_t			first			= FALSE;

	for (shift_cnt = 0; shift_cnt < 16; shift_cnt++)
	{
		if(((from & 0x8000) == (to & 0x8000)) && (!first))
		{
			from = from << 1;
			to = to << 1;
		}
		else
		{
			help = help << 1;
			first = TRUE;
			help |= 0x01;

			from = from << 1;
			to = to << 1;
		}
	}
	help = (~help) << 5;
	return help;
}

