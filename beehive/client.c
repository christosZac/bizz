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
#include "dev/leds.h"
#include "dev/fc2231.h"
#include "dev/button-sensor.h"

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
#define SERVER_NODE(ipaddr)   uip_ip6addr(ipaddr, 0xfd00, 0, 0, 0, 0, 0, 0, 1) 
#define LOCAL_PORT      UIP_HTONS(COAP_DEFAULT_PORT + 1)
#define REMOTE_PORT     UIP_HTONS(8181)
/* Time factor for sending data to the server. This will be used for
   initializing the timer and it multiplies a time unit. */
#define TOGGLE_INTERVAL 10
/* Path info. */
#define URL_PATH "/target"
/* Credentials obtained from the dashboard. */
#define DEVICE_ID "1f7140cb-7ece-4294-8f23-8a0da0736f35"
#define USER_TOKEN "Bearer zfwlgFZdVFSqJeIOOcJ5mVQCGIWNTryzhnOzJuiAJsmqNGr0Iq33dcX8fuw72s92"

PROCESS(er_client, "CoAP Client Workshop");
AUTOSTART_PROCESSES(&er_client);

uip_ipaddr_t server_ipaddr;

/* Event timer structure. */
static struct etimer et;
static struct etimer sensor_et;
static struct etimer aux_et;

/* Global variables for reading the sensor. */
static uint16_t rh = 0;
static uint16_t temperature = 0;
static uint16_t load = 0;

static uint16_t tare = 0;



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

  leds_init();


#ifdef AM2315
  /* Activate the sensor. */
  SENSORS_ACTIVATE(am2315);
#else 
  /* Activate the default sensor. */
  SENSORS_ACTIVATE(hih6130);
#endif
  
  SENSORS_ACTIVATE(fc2231);

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
          /* Introducing a 10ms delay to procceed reading. */
          //etimer_set(&sensor_et, CLOCK_SECOND/100);
          //PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&sensor_et));
          /* Reading temperature sensor. */
          temperature = am2315.value(AM2315_VAL_TEMP) / 10;
#endif

      
      fc2231.configure(SENSORS_ACTIVE, FC2231_WAKE);
      /* Wait for sensors to settle. */
      etimer_set(&sensor_et, CLOCK_SECOND);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&sensor_et));
      /* Read sensor. */
      printf("\ntare: %u\n" , tare);
      load = (fc2231.value(FC2231_WEIGHT) - tare);
      /* Avoid having negative (invalid) values. */
      if(load & 0x8000 ){
        load = 0;
      }
      /* Deactivate to save energy. */
      fc2231.configure(SENSORS_ACTIVE, FC2231_SLEEP);
   
      printf("%d, %d, %u", rh, temperature, load);

      /* ---------- Sending load reading ------------ */
      /* Prepare request, TID is set by COAP_BLOCKING_REQUEST(). */
      coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
      coap_set_header_uri_path(request, URL_PATH);
      /* Message buffer for temperature. */
      char load_msg[REST_MAX_CHUNK_SIZE];
      /* Setting the buffer with the temperature value. */
      snprintf((char *)load_msg,
               REST_MAX_CHUNK_SIZE,
               "%s,%s,%s,%05u", DEVICE_ID, USER_TOKEN,
               "load", load);
      /* Set the CoAP payload using the message buffer above. */
      coap_set_payload(request,
                       (uint8_t *)load_msg,
                       sizeof(DEVICE_ID)
                       +
                       sizeof(USER_TOKEN)
                       +
                       sizeof("load") 
                       + 
                       5);

      /* Debugging messages. */
      PRINT6ADDR(&server_ipaddr);
      PRINTF(" : %u\n", UIP_HTONS(REMOTE_PORT));
      /*  Sends the CoAP client request. */
      //COAP_BLOCKING_REQUEST(&server_ipaddr, REMOTE_PORT, request,
        //                    client_chunk_handler);

      /* ---------- Sending temperature reading ------------ */
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
      //COAP_BLOCKING_REQUEST(&server_ipaddr, REMOTE_PORT, request,
        //                    client_chunk_handler);


      /* ---------- Sending humidity reading ------------ */
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
      //COAP_BLOCKING_REQUEST(&server_ipaddr, REMOTE_PORT, request,
        //                    client_chunk_handler);

      printf("\n--Done--\n");
      /*  Resetting the timer. Start counting until we reach
          TOGGLE_INTERVAL seconds again. */
      etimer_reset(&et);
    } else if(ev == sensors_event) {
        /* If the button is pressed, tare! */
        if(data == &button_sensor) {
          if(button_sensor.value(BUTTON_SENSOR_VALUE_TYPE_LEVEL) ==
                BUTTON_SENSOR_PRESSED_LEVEL) {
            printf("Taring load cells...");
            fc2231.configure(SENSORS_ACTIVE, FC2231_WAKE);
            etimer_set(&sensor_et, CLOCK_SECOND);
            PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&sensor_et));
            tare = fc2231.value(FC2231_WEIGHT);
            fc2231.configure(SENSORS_ACTIVE, FC2231_SLEEP);
            leds_on(LEDS_RED);
            printf("... Taring completed.");
          } else {
            leds_off(LEDS_RED);
          }
        }
    }

  }

  PROCESS_END();
}
