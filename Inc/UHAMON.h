/*
 * UHAMON.h
 *
 *  Created on: 1. 1. 2024
 *      Author: martin
 *      Brief: The universal sw module in UHA ecosystem for remote monitoring (and manipulation) of FW global variables. (Module is inspered and partially compatible with CanMasterv2 by Zdenek Beran)
 *             The variables are as type-less chunks of memory at specified address.
 *              -For read access are always returned 4 bytes. The monitoring PC application should know the datatype of requested variable..
 *              -For write access there are specific commands for writing 1, 2 or 4 bytes. The addres for writing is set PRIOR to this command, by a specific command UHAMON_CMD_WR_RQST
 *             In general the module's API works with 8-byte input/output data fields, which can be mapped to messages of various interfaces (UART, CAN, ..)
 *
 */

#ifndef INC_UHAMON_H_
#define INC_UHAMON_H_


#include "main.h"

#define UHAMON_VERSION    101


/***** D e f i n e *****/
#define UHAMON_BUFFER_IDX_MAX 10

// UHAMON commands

#define UHAMON_CMD_VAR_WR_RQST 0x31  // variable write request
#define UHAMON_CMD_VAR_WR_UI8  0x32  // variable write 8bits
#define UHAMON_CMD_VAR_WR_UI16 0x33  // variable write 16bits
#define UHAMON_CMD_VAR_WR_UI32 0x34  // variable write 32bits
#define UHAMON_CMD_VAR_READ    0x38  // variable read
#define UHAMON_CMD_PAR_READ    0x39  // parameter read
//#define UHAMON_CMD_BLOCK_READ  0x3A  // ram read data


// CAN parameters
#define UHAMON_PAR_VERSION    0x00

/***** T y p e s *****/
typedef struct
{
  uint16_t Reserved;
  uint16_t Index;
  uint32_t Address;
} sUhamonBufferItem;


// Public API

void UHAMON_Init();

 // UHAMON receive function - has to be called in case of new frame at given interface is received
// p_data is data pointer to 8 bytes
uint8_t UHAMON_ProcessInput(uint8_t *p_data);

// pointer to UHAMON frame send function
// f_send is function for send UHAMON frame
// p_data is data pointer to 8 bytes
void UHAMON_SetSendFunction(void (*f_send)(uint8_t *p_data));



// Optional (for memory data transfer)
//uint8_t UHAMON_BlockDataSend_10ms(uint8_t cntFrames);   // CAN data send function  - has to be called periodically with period 10ms (uses function f_send)
                                // sends specified number of frames and return the number of the next frames waiting to be sent








#endif /* INC_UHAMON_H_ */
