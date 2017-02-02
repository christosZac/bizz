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
#include "lib/sensors.h"
/*----------------------------------------------------------------------------*/
#ifndef AM2315_H_
#define AM2315_H_
/*----------------------------------------------------------------------------*/
#define AM2315_ADDR				0x5c
#define AM2315_HUM_ADDR 		0x00
#define AM2315_TEMP_ADDR		0x02
#define AM2315_USER_REG			0x08
/*----------------------------------------------------------------------------*/
#define AM2315_READ_CODE 		0x03
#define AM2315_VAL_LENGTH		2
#define AM2315_RESPONSE_LENGTH 	6
#define AM2315_REQUEST_LENGTH 	3
/*----------------------------------------------------------------------------*/
#define AM2315_ACTIVE			SENSORS_ACTIVE
#define AM2315_SUCCESS			0
#define AM2315_ERROR			-1
/*----------------------------------------------------------------------------*/
#define AM2315_VAL_HUM          0
#define AM2315_VAL_TEMP	        1
/*----------------------------------------------------------------------------*/
extern const struct sensors_sensor am2315;
/*----------------------------------------------------------------------------*/
#endif /* ifndef AM2315_H_ */
/**
 * @}
 */
