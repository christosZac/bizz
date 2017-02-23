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
 * \addtogroup fc2231
 * @{
 *
 * \file
 *         FC2231 Load cell sensor dirver
 * \author
 *         Zachiotis Christos <zachiotis.sc@gmail.com>
 */
/*---------------------------------------------------------------------------*/
#include <stdio.h>
#include "contiki.h"
#include "dev/adc-zoul.h"
#include "dev/gpio.h"
#include "fc2231.h"

/* Defining control signals and assigned GPIO ports for     *
 * multiplexing functions.                                  */
#define SELECT_PIN_0 6
#define SELECT_PIN_1 7
#define ENABLE_PIN   6


#define LOAD_SELECT_0_BASE GPIO_PORT_TO_BASE(GPIO_B_NUM)
#define LOAD_SELECT_0_MASK GPIO_PIN_MASK(SELECT_PIN_0)
#define LOAD_SELECT_1_BASE GPIO_PORT_TO_BASE(GPIO_B_NUM)
#define LOAD_SELECT_1_MASK GPIO_PIN_MASK(SELECT_PIN_1)
#define LOAD_ENABLE_BASE GPIO_PORT_TO_BASE(GPIO_C_NUM)
#define LOAD_ENABLE_MASK GPIO_PIN_MASK(ENABLE_PIN)

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
/* Function for converting milliVolts to gramms.     *
 * Designed and calibrated for the FC2231 (50lbs).   *
 * The input is mV*10 (as taken from adc), output is *
 * in grams.                                         */

uint16_t
convert2kg(uint16_t mV)
{
  uint32_t grams;

  grams = (mV - FC2231_CONSTANT) * 1000 / FC2231_SLOPE;  

  return (uint16_t) grams;
}
/*---------------------------------------------------------------------------*/
/* The setup is using 4 multiplexed load cells, which are combined *
 * to get the total weight. The mux is controlled by 2 select bits. */

static int
fc2315_read(uint16_t *val)
{
      uint16_t load = 0;
      uint16_t aux_load = 0;
      uint16_t debug_load = 0;

     /* Select first cell 00. */
      GPIO_WRITE_PIN(LOAD_SELECT_0_BASE,LOAD_SELECT_0_MASK, 0x00);
      GPIO_WRITE_PIN(LOAD_SELECT_1_BASE,LOAD_SELECT_1_MASK, 0x00);
      /* Read the analog input. */
      aux_load = adc_zoul.value(ZOUL_SENSORS_ADC1);
      debug_load += aux_load;
      PRINTF("\nFC2231: Load cell 0: %u,\n", aux_load); 
      /* Convert and add to total. */
      load += convert2kg(aux_load);

      /* Select first cell 01. */
      GPIO_WRITE_PIN(LOAD_SELECT_0_BASE,LOAD_SELECT_0_MASK, 0xFF);
      GPIO_WRITE_PIN(LOAD_SELECT_1_BASE,LOAD_SELECT_1_MASK, 0x00);
      /* Read the analog input. */
      aux_load = adc_zoul.value(ZOUL_SENSORS_ADC1);
      PRINTF("FC2231: Load cell 1: %u,\n", aux_load); 
      debug_load += aux_load;
      /* Convert and add to total. */
      load += convert2kg(aux_load);

      /* Select first cell 10. */
      GPIO_WRITE_PIN(LOAD_SELECT_0_BASE,LOAD_SELECT_0_MASK, 0x00);
      GPIO_WRITE_PIN(LOAD_SELECT_1_BASE,LOAD_SELECT_1_MASK, 0xFF);
      /* Read the analog input. */
      aux_load = adc_zoul.value(ZOUL_SENSORS_ADC1);
      PRINTF("FC2231: Load cell 2: %u,\n", aux_load);
      debug_load += aux_load;
      /* Convert and add to total. */
      load += convert2kg(aux_load);

      /* Select first cell 11. */
      GPIO_WRITE_PIN(LOAD_SELECT_0_BASE,LOAD_SELECT_0_MASK, 0xFF);
      GPIO_WRITE_PIN(LOAD_SELECT_1_BASE,LOAD_SELECT_1_MASK, 0xFF);
      /* Read the analog input. */
      aux_load = adc_zoul.value(ZOUL_SENSORS_ADC1);
      PRINTF("FC2231: Load cell 3: %u\n", aux_load); 
      debug_load += aux_load;
      /* Convert and add to total. */
      load += convert2kg(aux_load);
      
      
      //DEBUG print the total sensor value
      PRINTF("FC2231: Load cell total: %u\n", debug_load); 
      PRINTF("FC2231: Total weight: %u\n" , convert2kg(debug_load));

 

      *val = load;

      return FC2231_SUCCESS; 
}
/*---------------------------------------------------------------------------*/
static int
value(int type)
{
  uint16_t val = FC2231_ERROR;

  if(!enabled) {
    PRINTF("FC2231: Sensor not started.\n");
  }

  if(type == FC2231_WEIGHT){
    if(fc2315_read(&val) == FC2231_SUCCESS){
      return val;;
    } else {
      PRINTF("FC2231: Could not read sensor.\n");
    }
  }

  return val;
}
/*---------------------------------------------------------------------------*/
static int
configure(int type, int value)
{
  switch(type) {
  case FC2231_ACTIVE:
    if(value == 1) {  
      /* Setting the multiplexer's select pins as controlled outputs*/
      GPIO_SOFTWARE_CONTROL(LOAD_SELECT_0_BASE,LOAD_SELECT_0_MASK);
      GPIO_SET_OUTPUT(LOAD_SELECT_0_BASE,LOAD_SELECT_0_MASK);
      GPIO_SOFTWARE_CONTROL(LOAD_SELECT_1_BASE,LOAD_SELECT_1_MASK);
      GPIO_SET_OUTPUT(LOAD_SELECT_1_BASE,LOAD_SELECT_1_MASK);
      GPIO_SOFTWARE_CONTROL(LOAD_ENABLE_BASE,LOAD_ENABLE_MASK);
      GPIO_SET_OUTPUT(LOAD_ENABLE_BASE,LOAD_ENABLE_MASK);

      /* Power the load-cells. */
      GPIO_WRITE_PIN(LOAD_ENABLE_BASE,LOAD_ENABLE_MASK, 0x00);
      //clock_delay_usec(1000);

      /* Activate the analog sensors of zoul. */
      adc_zoul.configure(SENSORS_HW_INIT, ZOUL_SENSORS_ADC_ALL);

      enabled = 1;
      return FC2231_SUCCESS;
    } else if(value == FC2231_WAKE) {
      /* Deactivate sensor to save energy. */
      GPIO_WRITE_PIN(LOAD_ENABLE_BASE,LOAD_ENABLE_MASK, 0x00);
    }  else if(value == FC2231_SLEEP) {
      /* Deactivate sensor to save energy. */
      GPIO_WRITE_PIN(LOAD_ENABLE_BASE,LOAD_ENABLE_MASK, 0xFF);
    }

  default:
    return FC2231_ERROR;
  }

  return FC2231_ERROR;
}
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(fc2231, "FC2231 sensor", value, configure, status);
/*---------------------------------------------------------------------------*/
/** @} */
