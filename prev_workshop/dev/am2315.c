/*
 * Copyright (c) 2016, relayr http://relayr.io/
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*---------------------------------------------------------------------------*/
/**
 * \addtogroup am2315
 * @{
 *
 * \file
 *         AM2315 I2C temperature/humidity sensor dirver
 * \author
 *         Zachiotis Christos <zachiotis.sc@gmail.com>
 */
/*---------------------------------------------------------------------------*/
#include <stdio.h>
#include "contiki.h"
#include "dev/i2c.h"
#include "am2315.h"
#include "lib/sensors.h"

/*---------------------------------------------------------------------------*/
#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(M, ...) printf("DEBUG %s:%d: " M, __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define PRINTF(...)
#endif
/*---------------------------------------------------------------------------*/
static uint8_t enabled;
/*---------------------------------------------------------------------------*/
static int
status(int type)
{
  switch(type) {
  case SENSORS_ACTIVE:
  case SENSORS_READY:
    return enabled;
  }
  return 0;
}

/*---------------------------------------------------------------------------*/
static void
am2315_wake_up(void)
{
  /* Make random writes so sensor wakes up! */
  i2c_single_send(AM2315_ADDR, 0);
  i2c_single_send(AM2315_ADDR, 0);
  i2c_single_send(AM2315_ADDR, 0);
}
/*---------------------------------------------------------------------------*/
static int
am2315_sensor_read(uint8_t reg, uint8_t code, uint8_t *buf, uint8_t num)
{
  /* Buffer for formatting the request fields.                        *
   * That would be: FunctionCode, StartRegister, RegistersNumber.     */
  uint8_t req_buf[AM2315_REQUEST_LENGTH]={AM2315_READ_CODE, reg, 2};

  i2c_master_enable();
  /* Make some dump readings to wake up sensor. */
  am2315_wake_up();
  
  /* Send the request and receive the responce from AM2315. */
  if(i2c_burst_send(AM2315_ADDR, req_buf, AM2315_REQUEST_LENGTH) == I2C_MASTER_ERR_NONE) {
    if(i2c_burst_receive(AM2315_ADDR,
                         buf,
                         num) == I2C_MASTER_ERR_NONE) {
      return AM2315_SUCCESS;
    }
    PRINTF("AM2315: I2C receive error\n");
    return AM2315_ERROR;
  }
  PRINTF("AM2315: I2C send error\n");
  return AM2315_ERROR;
}
/*---------------------------------------------------------------------------*/
static int
am2315_read_user_reg()
{
  uint8_t buf[5];
  /* Read a user register and receive all response fields (Code, Length, 
   * Reg_high, Reg_low, CRC).                                                         */
  if(am2315_sensor_read(AM2315_USER_REG, AM2315_READ_CODE, buf, 5) != AM2315_ERROR) {
    return AM2315_SUCCESS;
  }
  PRINTF("AM2315: Could not read sensor\n");
  return AM2315_ERROR;
}
/*---------------------------------------------------------------------------*/
static int
am2315_read_temp(int16_t *val)
{
  uint8_t buf[6];
  /* Read the temp register and receive all response fields (Code, Length,   *
   * Reg_high, Reg_low, CRC_high, CRC_Low).                                  */
  if(am2315_sensor_read(AM2315_TEMP_ADDR, AM2315_READ_CODE, buf, 6) != AM2315_ERROR) {
    /* The actual reading is in bytes 2 and 3. Convert to int16_t */
    *val = ((buf[2]) << 8) + buf[3];
    /* Mask for detecting negative temperature. */
    if(buf[2] & 0x80){
      *val = -1 * (*val);
    }
    return AM2315_SUCCESS;
  }
  PRINTF("AM2315: Could not read sensor\n");
  return AM2315_ERROR;
}
/*---------------------------------------------------------------------------*/
static int
am2315_read_hum(int16_t *val)
{
  uint8_t buf[6];
  /* Read the temp register and receive all response fields (Code, Length,   *
   * Reg_high, Reg_low, CRC_high, CRC_Low).                                  */
  if(am2315_sensor_read(AM2315_HUM_ADDR, AM2315_READ_CODE, buf, 6) != AM2315_ERROR) {
    /* The actual reading is in bytes 2 and 3. Convert to int16_t*/
    *val = (buf[2] << 8) + buf[3];
    return AM2315_SUCCESS;
  }
  PRINTF("AM2315: Could not read sensor\n");
  return AM2315_ERROR;
}
/*---------------------------------------------------------------------------*/
static int
value(int type)
{
  /* A signed int is needed for negative temperatures. */
  int16_t val = AM2315_ERROR;

  if(!enabled) {
    PRINTF("AM2315: Sensor not started\n");
  }

  /* Depending on the request type, the proper function is called. */
  if(type == AM2315_VAL_TEMP) {
      /* Reads and converts the AM2315 temperature. */
      am2315_read_temp(&val);
  } else if(type == AM2315_VAL_HUM) {
      /* Reads and converts the AM2315 humidity. */
      am2315_read_hum(&val);
  } else {
    PRINTF("AM2315: Invalid value requested\n");
  }

  return val;
}
/*---------------------------------------------------------------------------*/
static int
configure(int type, int value)
{
  switch(type) {
  case AM2315_ACTIVE:
    if(value) {
      //Initialize I2C channel
      i2c_init(I2C_SDA_PORT, I2C_SDA_PIN, I2C_SCL_PORT, I2C_SCL_PIN,
               I2C_SCL_NORMAL_BUS_SPEED);
      //Read a random user register to establish communication and connectivity
      if(am2315_read_user_reg() != AM2315_ERROR) {
        enabled = value;
        PRINTF("AM2315: Sensor enabled.\n");
        return AM2315_SUCCESS;  
      }     
    }

  default:
    return AM2315_ERROR;
  }

  return AM2315_ERROR;
}
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(am2315, "AM2315 sensor", value, configure, status);
/*---------------------------------------------------------------------------*/
/** @} */
