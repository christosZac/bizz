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
 * \addtogroup hih6130
 * @{
 *
 * \file
 *         HIH6130 temperature/humidity sensor dirver
 * \author
 *         Nenad Ilic <nenad@relayr.io>
 */
/*---------------------------------------------------------------------------*/
#include <stdio.h>
#include "contiki.h"
#include "dev/i2c.h"
#include "hih6130.h"
#include "lib/sensors.h"

/*---------------------------------------------------------------------------*/
#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(M, ...) printf("DEBUG %s:%d: " M, __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define PRINTF(...)
#endif
/*---------------------------------------------------------------------------*/
#define HIH6130_ADDR                   (0x27)
/* Humidity + temperature data is 4 bytes long */
#define HIH6130_FULL_DATA_LENGTH       (4)
/* Bit mask for the status bits in the first byte transferred */
#define HIH6130_STATUS_MASK            (0xc0)
/* Humidity is stored in the first 2 bytes of data */
#define HIH6130_HUMIDITY_DATA_LENGTH   (2)
/* Bit mask for the humidity data */
#define HIH6130_HUMIDITY_MASK          (0x3fff)
/* Temperature data is left adjusted within the word */
#define HIH6130_TEMPERATURE_SHIFT      (2)

#define HIH6130_STATUS_OK              (0x00)
#define HIH6130_STATUS_STALE_DATA      (0x40)
#define HIH6130_STATUS_COMMAND_MODE    (0x80)
#define HIH6130_STATUS_DIAGNOSTIC      (0xc0)
/*---------------------------------------------------------------------------*/
static uint8_t enabled;
static uint8_t hih6130_buffer[HIH6130_FULL_DATA_LENGTH];
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
static int
hih6130_sensor_read()
{
  i2c_master_enable();
  if(i2c_single_send(HIH6130_ADDR, 0) == I2C_MASTER_ERR_NONE) {
    if(i2c_burst_receive(HIH6130_ADDR,
                         hih6130_buffer,
                         HIH6130_FULL_DATA_LENGTH) == I2C_MASTER_ERR_NONE) {
      /* data is in big-endian format, with status bits in the first byte. */
      switch(hih6130_buffer[0] & HIH6130_STATUS_MASK) {
      case HIH6130_STATUS_OK:
        PRINTF("HIH6130: Status OK\n");
        return HIH6130_SUCCESS;
      case HIH6130_STATUS_STALE_DATA:
        PRINTF("HIH6130: stale data\n");
        return HIH6130_SUCCESS;
      default:
        PRINTF("HIH6130: Status unknown\n");
        return HIH6130_ERROR;
      }
    }
    PRINTF("HIH6130: I2C receive error\n");
  }
  PRINTF("HIH6130: I2C send error\n");
  return HIH6130_ERROR;
}
/*---------------------------------------------------------------------------*/
static int
hih6130_measurement_request(void)
{

  i2c_master_enable();
  if(i2c_single_send(HIH6130_ADDR, 0) == I2C_MASTER_ERR_NONE) {
    return HIH6130_SUCCESS;
  }
  return HIH6130_ERROR;
}
/*---------------------------------------------------------------------------*/
static int
value(int type)
{
  uint32_t val = HIH6130_ERROR;

  if(!enabled) {
    PRINTF("HIH6130: sensor not started\n");
  }

  if(type == HIH6130_VAL_TEMP) {
    val = ((hih6130_buffer[2] << 8) | hih6130_buffer[3]) >> HIH6130_TEMPERATURE_SHIFT;
    val = (val * 10) - 40000;
  } else if(type == HIH6130_VAL_HUMIDITY) {
    val = ((hih6130_buffer[0] << 8) | hih6130_buffer[1]) & HIH6130_HUMIDITY_MASK;
    val = (val * 6);
  } else {
    PRINTF("HIH6130: invalid value requested\n");
  }

  return val;
}
/*---------------------------------------------------------------------------*/
static int
configure(int type, int value)
{
  switch(type) {
  case HIH6130_ACTIVE:
    if(value) {
      i2c_init(I2C_SDA_PORT, I2C_SDA_PIN, I2C_SCL_PORT, I2C_SCL_PIN,
               I2C_SCL_NORMAL_BUS_SPEED);
      enabled = value;
      return HIH6130_SUCCESS;
    }

  case HIH6130_MEASUREMENT_REQUEST:
    if(hih6130_measurement_request() != HIH6130_SUCCESS) {
      PRINTF("HIH6130: failed to send measurement request to the sensor\n");
      return HIH6130_ERROR;
    }
    return HIH6130_SUCCESS;

  case HIH6130_SENSOR_READ:
    return hih6130_sensor_read();

  default:
    return HIH6130_ERROR;
  }

  return HIH6130_ERROR;
}
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(hih6130, "HIH6130 sensor", value, configure, status);
/*---------------------------------------------------------------------------*/
/** @} */
