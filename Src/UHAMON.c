/*
 * UHAMON.c
 *
 *  Created on: 1. 1. 2024
 *      Author: martin
 *      Brief: The universal sw module in UHA ecosystem for remote monitoring (and manipulation) of FW global variables.
 *             The variables are accessed by address and size (datatype)  parameters.
 *             The variables can be accessed via UART or CAN interface, thus some UART and CAN API is expected.
 *
 */

#include "UHAMON.h"



/***** V a r i a b l e s *****/
// General
sUhamonBufferItem mUhamonBuffer[UHAMON_BUFFER_IDX_MAX];
uint32_t mUhamonEnable = 0;
uint16_t mUhamonKey = 0;


/*****  P r o t o t y p e s *****/
void (*Uhamon_Send)(uint8_t *p_data) = NULL;
void UHAMON_Parameter_Read(uint16_t index, uint32_t value);


// Private functions

//-------------------------------------------------------------------------------------------------
static void Variable_WriteRqst(uint16_t index, uint32_t address)  // variable write request
{
  int i = 0;

  while ((i < UHAMON_BUFFER_IDX_MAX) && (mUhamonBuffer[i].Reserved > 0)) i++;

  if (i < UHAMON_BUFFER_IDX_MAX)
  {
    mUhamonBuffer[i].Reserved = 1;
    mUhamonBuffer[i].Index = index;
    mUhamonBuffer[i].Address = address;
  }
}

//-------------------------------------------------------------------------------------------------
static void Variable_Write_1byte(uint16_t index, uint8_t val)      // variable write 8bits
{
  int i = 0;
  uint8_t *pointer;

  while (i < UHAMON_BUFFER_IDX_MAX)
  {
    if (mUhamonBuffer[i].Index == index)
    {
      pointer = (uint8_t *)mUhamonBuffer[i].Address;
      *pointer = val;
      mUhamonBuffer[i].Reserved = 0;
      return;
    }
    i++;
  }
}

//-------------------------------------------------------------------------------------------------
static void Variable_Write_2bytes(uint16_t index, uint16_t val)    // variable write 16bits
{
  int i = 0;
  uint16_t *pointer;

  while (i < UHAMON_BUFFER_IDX_MAX)
  {
    if (mUhamonBuffer[i].Index == index)
    {
      pointer = (uint16_t *)mUhamonBuffer[i].Address;
      *pointer = val;
      mUhamonBuffer[i].Reserved = 0;
      return;
    }
    i++;
  }
}

//-------------------------------------------------------------------------------------------------
static void Variable_Write_4bytes(uint16_t index, uint32_t val)    // variable write 32bits
{
  int i = 0;
  uint32_t *pointer;

  while (i < UHAMON_BUFFER_IDX_MAX)
  {
    if (mUhamonBuffer[i].Index == index)
    {
      pointer = (uint32_t *)mUhamonBuffer[i].Address;
      *pointer = val;
      mUhamonBuffer[i].Reserved = 0;
      return;
    }
    i++;
  }
}

//-------------------------------------------------------------------------------------------------
static void Variable_Read(uint16_t index, uint32_t address)     // variable read process
{
  uint8_t data[8];

  data[0] = 0;
  data[1] = UHAMON_CMD_VAR_READ;
  *(uint16_t *)(data + 2) = index;
  *(uint32_t *)(data + 4) = *((uint32_t *)address);

  if (Uhamon_Send != NULL) (*Uhamon_Send)(data);
}

//-------------------------------------------------------------------------------------------------
static void Parameter_Read(uint16_t ParamId)      // parameter read process
{
  uint8_t data[8];
  uint32_t value = 0;

  switch (ParamId)
  {
    case UHAMON_PAR_VERSION:
      value = (uint32_t) UHAMON_VERSION;
      break;
    default:
      return;
  }

  data[0] = 0;
  data[1] = UHAMON_CMD_PAR_READ;
  *(uint16_t *)(data + 2) = ParamId;
  *(uint32_t *)(data + 4) = value;

  if (Uhamon_Send != NULL) (*Uhamon_Send)(data);
}

// Public API


void UHAMON_Init()
{
  Uhamon_Send = NULL;
  mUhamonEnable = 1;
}


// UHAMON receive function - has to be called in case of new frame at given interface is received
// p_data is data pointer to 8 bytes
uint8_t UHAMON_ProcessInput(uint8_t *p_data)
{
  switch (*((uint8_t *)(p_data + 1)))   // command is coded in second byte.  first byte can be used by interface layer (CANopen address, etc..)
  {
    case UHAMON_CMD_VAR_WR_RQST:
      if (mUhamonEnable) Variable_WriteRqst(*((uint16_t *)(p_data + 2)), *((uint32_t *)(p_data + 4)));
      break;
    case UHAMON_CMD_VAR_WR_UI8:
      if (mUhamonEnable) Variable_Write_1byte(*((uint16_t *)(p_data + 2)), *((uint8_t *)(p_data + 4)));
      break;
    case UHAMON_CMD_VAR_WR_UI16:
      if (mUhamonEnable) Variable_Write_2bytes(*((uint16_t *)(p_data + 2)), *((uint16_t *)(p_data + 4)));
      break;
    case UHAMON_CMD_VAR_WR_UI32:
      if (mUhamonEnable) Variable_Write_4bytes(*((uint16_t *)(p_data + 2)), *((uint32_t *)(p_data + 4)));
      break;
    case UHAMON_CMD_VAR_READ:
      if (mUhamonEnable) Variable_Read(*((uint16_t *)(p_data + 2)), *((uint32_t *)(p_data + 4)));
      break;
    case UHAMON_CMD_PAR_READ:
      if (mUhamonEnable) Parameter_Read(*((uint16_t *)(p_data + 2)));
      break;
    default:
      return 1;
      break;
  }
  return 0;
}

// pointer to UHAMON frame send function
// f_send is function for send UHAMON frame
// p_data is data pointer to 8 bytes
void UHAMON_SetSendFunction(void (*f_send)(uint8_t *p_data))
{
  Uhamon_Send = f_send;
}

