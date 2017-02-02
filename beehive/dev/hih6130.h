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
#include "lib/sensors.h"
/*----------------------------------------------------------------------------*/
#ifndef HIH6130_H_
#define HIH6130_H_
/*----------------------------------------------------------------------------*/
#define HIH6130_SUCCESS                 0
#define HIH6130_ERROR                   -1
/*----------------------------------------------------------------------------*/
#define HIH6130_ACTIVE                  SENSORS_ACTIVE
#define HIH6130_MEASUREMENT_REQUEST     1
#define HIH6130_SENSOR_READ             2
/*----------------------------------------------------------------------------*/
#define HIH6130_VAL_TEMP                0
#define HIH6130_VAL_HUMIDITY            1
/*----------------------------------------------------------------------------*/
extern const struct sensors_sensor hih6130;
/*----------------------------------------------------------------------------*/
#endif /* ifndef HIH6130_H_ */
/**
 * @}
 */
