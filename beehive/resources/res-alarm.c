/*
 * Copyright (c) 2013, Institute for Pervasive Computing, ETH Zurich
 *           (c) 2016, relayr GmbH
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
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 */

/**
 * \file
 *      6LoWPAN workshop CoAP observable resource.
 * \author
 *      Christos Zachiotis <christos@relayr.io>
 *      Antonio P. P. Almeida <appa@perusio.net>
 */

#include <string.h>
#include "rest-engine.h"
#include "er-coap.h"
#include "dev/hih6130.h"

/* Value of humidity that triggers an update of the observable resource. */
#define HUMIDITY_DELTA 50

/* GET request handler. */
static void res_get_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
/* Recurring handler that  checks if the resource value changed. */
static void res_periodic_handler(void);
/* Setup the periodic resource for humidity. */
PERIODIC_RESOURCE(res_alarm,
                  "title=\"Humidity Alert\";obs",
                  res_get_handler,
                  NULL,
                  NULL,
                  NULL,
                  3 * CLOCK_SECOND,
                  res_periodic_handler);

/*
 * Use local resource state that is accessed by res_get_handler() and
   altered by res_periodic_handler() or PUT or POST.
 */
static uint16_t rh = 0;

static void
res_get_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  /*
   * For minimal complexity, request query and options should be ignored for GET on observable resources.
   * Otherwise the requests must be stored with the observer list and passed by REST.notify_subscribers().
   * This would be a TODO in the corresponding files in contiki/apps/erbium/!
   */
  REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
  /* Set Max-Age header (CoAP option). */
  REST.set_header_max_age(response, res_alarm.periodic->period / CLOCK_SECOND);
  /* Set the payload. */
  REST.set_response_payload(response, buffer,
                            snprintf((char *)buffer,
                                     preferred_size,
                                     "Warning! Humidity is now %u percent",
                                     rh));
  /* The REST.subscription_handler() will be called for observable resources by the REST framework. */
}
/*
 * Additionally, a handler function named [resource name]
 * _handler must be implemented for each PERIODIC_RESOURCE.
 * It will be called by the REST manager process with the defined period.
 */
static void
res_periodic_handler()
{
  /* Periodically sampling the sensor and check humidity. */
  if(hih6130.configure(HIH6130_MEASUREMENT_REQUEST, 0) >= 0) {
    if(hih6130.configure(HIH6130_SENSOR_READ, 0) >= 0) {
      rh = hih6130.value(HIH6130_VAL_HUMIDITY) / 1000;
    }
  }
  /* Usually a condition is defined under with subscribers are
     notified, e.g., large enough delta in sensor reading. */
  if(rh > HUMIDITY_DELTA) {
    /* Notify the registered observers which will trigger the
       res_get_handler to create the response. */
    REST.notify_subscribers(&res_alarm);
  }
}
