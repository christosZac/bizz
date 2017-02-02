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
 *      6LoWPAN workshop CoAP client.
 * \author
 *      Christos Zachiotis <christos@relayr.io>
 *      Antonio P. P. Almeida <appa@perusio.net>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contiki.h"
#include "contiki-net.h"
#include "er-coap-engine.h"
#include "dev/hih6130.h"
#include "dev/am2315.h"
#include "dev/adc-zoul.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINT6ADDR(addr) PRINTF("[%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x]", ((uint8_t *)addr)[0], ((uint8_t *)addr)[1], ((uint8_t *)addr)[2], ((uint8_t *)addr)[3], ((uint8_t *)addr)[4], ((uint8_t *)addr)[5], ((uint8_t *)addr)[6], ((uint8_t *)addr)[7], ((uint8_t *)addr)[8], ((uint8_t *)addr)[9], ((uint8_t *)addr)[10], ((uint8_t *)addr)[11], ((uint8_t *)addr)[12], ((uint8_t *)addr)[13], ((uint8_t *)addr)[14], ((uint8_t *)addr)[15])
#define PRINTLLADDR(lladdr) PRINTF("[%02x:%02x:%02x:%02x:%02x:%02x]", (lladdr)->addr[0], (lladdr)->addr[1], (lladdr)->addr[2], (lladdr)->addr[3], (lladdr)->addr[4], (lladdr)->addr[5])
#else
#define PRINTF(...)
#define PRINT6ADDR(addr)
#define PRINTLLADDR(addr)
#endif



/* Define the border router settings. */
#define SERVER_NODE(ipaddr)   uip_ip6addr(ipaddr, 0xfd00, 0, 0, 0, 0, 0, 0, 0x1)
#define LOCAL_PORT      UIP_HTONS(COAP_DEFAULT_PORT + 1)
#define REMOTE_PORT     UIP_HTONS(8181)
/* Time factor for sending data to the server. This will be used for
   initializing the timer and it multiplies a time unit. */
#define TOGGLE_INTERVAL 4
/* Path info. */
#define URL_PATH "/target"
/* Credentials obtained from the dashboard. */
#define DEVICE_ID "2a52eb8a-8721-46f6-babc-223fef0408c6"
#define USER_TOKEN "Bearer WnaSWEq9309fuODa4Vfwilr5IRSNfgZ6zi1yI5E9k0G2mbffgsLsVxQPuKt9IvCX"

PROCESS(er_client, "CoAP Client Workshop");
AUTOSTART_PROCESSES(&er_client);

uip_ipaddr_t server_ipaddr;

/* Arrays for interpolating voltage readings. (Taken from datasheet graphs). *
 * Vakues valid for 3V DCC                                                   */
static uint16_t  VoutArray[]  =  {0,  3,  11, 42, 120, 200, 348,  700, 830, 1400};
static uint16_t  LuxArray[]   =  {1,  3,   9, 27, 100, 154, 281,  500, 602, 1000};

/* Event timer structure. */
static struct etimer et;
static struct etimer sensor_et;

/* Global variables for reading the sensor. */
static uint16_t rh = 0;
static uint16_t temperature = 0;
static uint16_t lum = 0;

/****************************************************************************/
/***       Local Functions                                                ***/
/****************************************************************************/
//This code uses MultiMap implementation from http://playground.arduino.cc/Main/MultiMap
//Copyright and License by its original author

uint16_t multiMap(uint16_t mV, uint16_t * _in, uint16_t * _out, uint8_t size)
{
  // take care the value is within range
  // val = constrain(val, _in[0], _in[size-1]);
  if (mV <= _in[0]) return _out[0];
  if (mV >= _in[size-1]) return _out[size-1];

  // search right interval
  uint8_t pos = 1;  // _in[0] allready tested
  while(mV > _in[pos]) pos++;

  // this will handle all exact "points" in the _in array
  if (mV == _in[pos]) return _out[pos];

  // interpolate in the right segment for the rest
  return (mV - _in[pos-1]) * (_out[pos] - _out[pos-1]) / (_in[pos] - _in[pos-1]) + _out[pos-1];
}


/* This function is will be passed to COAP_BLOCKING_REQUEST() to handle responses. */
void
client_chunk_handler(void *response)
{
  const uint8_t *chunk;
  int len = coap_get_payload(response, &chunk);
  printf("|%.*s", len, (char *)chunk);
}
PROCESS_THREAD(er_client, ev, data)
{
  PROCESS_BEGIN();

#ifdef AM2315
  /* Activate the sensor. */
  SENSORS_ACTIVATE(am2315);
#else 
  /* Activate the default sensor. */
  SENSORS_ACTIVATE(hih6130);
#endif

  adc_zoul.configure(SENSORS_HW_INIT, ZOUL_SENSORS_ADC_ALL);

  /* Check what this is exactly! */
  static coap_packet_t request[1];      /* This way the packet can be treated as pointer as usual. */

  SERVER_NODE(&server_ipaddr);

  /* Receives all CoAP messages. */
  coap_init_engine();

  /* Set the event timer interval. */
  etimer_set(&et, TOGGLE_INTERVAL * CLOCK_SECOND);

  /* Infinite loop running the main logic. */
  while(1) {
    /* Wait on any event. IN this case the event will be the timer expiration: TOGGLE_INTERVAL seconds. */
    PROCESS_YIELD();
    /*  The timer fired up an event. Control is returned here. */
    if(etimer_expired(&et)) {
      printf("--Sending readings...--\n");

#ifdef HIH6130
      /* Setup the I2C bus communication for the sensor and request
         the value(s). */
      if(hih6130.configure(HIH6130_MEASUREMENT_REQUEST, 0) >= 0) {
        if(hih6130.configure(HIH6130_SENSOR_READ, 0) >= 0) {
          /* Quick & dirty rounding of the values: convert to integer. */
          rh = hih6130.value(HIH6130_VAL_HUMIDITY) / 1000;
          temperature = hih6130.value(HIH6130_VAL_TEMP) / 1000;
        }
      }
#endif

#ifdef AM2315
          /* Convert to integer. Sensor gives the reading x10 */
          rh = am2315.value(AM2315_VAL_HUM) / 10;
          /* Introducing a 100ms delay to procceed reading. */
          etimer_set(&sensor_et, CLOCK_SECOND/10);
          PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&sensor_et));
          /* Reading temperature sensor. */
          temperature = am2315.value(AM2315_VAL_TEMP) / 10;
#endif

      ;

      lum = multiMap(adc_zoul.value(ZOUL_SENSORS_ADC1)/10, VoutArray, LuxArray, 10);

      printf("%d, %d, %u", rh, temperature, lum);

      /* ---------- Sending lum reading ------------ */
      /* Prepare request, TID is set by COAP_BLOCKING_REQUEST(). */
      coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
      coap_set_header_uri_path(request, URL_PATH);
      /* Message buffer for temperature. */
      char lum_msg[REST_MAX_CHUNK_SIZE];
      /* Setting the buffer with the temperature value. */
      snprintf((char *)lum_msg,
               REST_MAX_CHUNK_SIZE,
               "%s,%s,%s,%04u", DEVICE_ID, USER_TOKEN,
               "luminosity", lum);
      /* Set the CoAP paylum using the message buffer above. */
      coap_set_payload(request,
                       (uint8_t *)lum_msg,
                       sizeof(DEVICE_ID)
                       +
                       sizeof(USER_TOKEN)
                       +
                       sizeof("luminosity") 
                       + 
                       4);

      /* Debugging messages. */
      PRINT6ADDR(&server_ipaddr);
      PRINTF(" : %u\n", UIP_HTONS(REMOTE_PORT));
      /*  Sends the CoAP client request. */
      COAP_BLOCKING_REQUEST(&server_ipaddr, REMOTE_PORT, request,
                            client_chunk_handler);

      /* Prepare request, TID is set by COAP_BLOCKING_REQUEST(). */
      coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
      coap_set_header_uri_path(request, URL_PATH);
      /* Message buffer for temperature. */
      char temp_msg[REST_MAX_CHUNK_SIZE];
      /* Setting the buffer with the temperature value. */
      snprintf((char *)temp_msg,
               REST_MAX_CHUNK_SIZE,
               "%s,%s,%s,%u", DEVICE_ID, USER_TOKEN,
               "temperature", temperature);
      /* Set the CoAP payload using the message buffer above. */
      coap_set_payload(request,
                       (uint8_t *)temp_msg,
                       sizeof(DEVICE_ID)
                       +
                       sizeof(USER_TOKEN)
                       +
                       sizeof("temperature") 
                       + 
                       2);

      /* Debugging messages. */
      PRINT6ADDR(&server_ipaddr);
      PRINTF(" : %u\n", UIP_HTONS(REMOTE_PORT));
      /*  Sends the CoAP client request. */
      COAP_BLOCKING_REQUEST(&server_ipaddr, REMOTE_PORT, request,
                            client_chunk_handler);

      /* Prepare request, TID is set by COAP_BLOCKING_REQUEST(). */
      coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
      coap_set_header_uri_path(request, URL_PATH);
      /* Message buffer for humidity. */
      char hum_msg[REST_MAX_CHUNK_SIZE];
      /* Setting the buffer to the humidity value. */
      snprintf((char *)hum_msg,
               REST_MAX_CHUNK_SIZE,
               "%s,%s,%s,%u", DEVICE_ID, USER_TOKEN, "humidity", rh);

      coap_set_payload(request, (uint8_t *)hum_msg,
                       sizeof(DEVICE_ID)
                       +
                       sizeof(USER_TOKEN)
                       +
                       sizeof("humidity")
                       +
                       2);
      /* Debugging messages. */
      PRINT6ADDR(&server_ipaddr);
      PRINTF(" : %u\n", UIP_HTONS(REMOTE_PORT));
      /* Sending the client request. */
      COAP_BLOCKING_REQUEST(&server_ipaddr, REMOTE_PORT, request,
                            client_chunk_handler);

      printf("\n--Done--\n");
      /*  Resetting the timer. Start counting until we reach
          TOGGLE_INTERVAL seconds again. */
      etimer_reset(&et);
    }
  }

  PROCESS_END();
}
