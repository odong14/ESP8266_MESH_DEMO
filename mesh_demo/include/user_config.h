#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__
#include "c_types.h"

#define user_procTaskPrio        0
#define user_procTaskQueueLen    1

#define sleepms(x) os_delay_us(x*1000);

// Device start address 0x50 to 0x57
// 0  b  1  0  1  0  0  A1  A0  R/W
//      MSB                     LSB
#define DEVICEADDRESS 0x50

// Set to a higher number if you want to start at a higher address.
#define MIN_ADDRESS 0x00
// Maximum address (inclusive) in address space.
#define MAX_ADDRESS 0x0000FFFF


#define MESH_DEMO_PRINT  ets_printf
#define MESH_DEMO_STRLEN ets_strlen
#define MESH_DEMO_MEMCPY ets_memcpy
#define MESH_DEMO_MEMSET ets_memset
#define MESH_DEMO_FREE   os_free

static const uint16_t server_port = 7000;                  /*PORT USED BY USER IOT SERVER FOR MESH SERVICE*/  
static const uint8_t server_ip[4] = {192, 168, 43, 33};      /*USER IOT SERVER IP ADDRESS*/
static const uint32_t UART_BAUD_RATIO = 74880;             /*UART BAUD RATIO*/
static const uint8_t MESH_GROUP_ID[6] = {0x18,0xfe,0x34,0x00,0x00,0x50};  /*MESH_GROUP_ID & MESH_SSID_PREFIX REPRESENTS ONE MESH NETWORK*/
//static const uint8_t MESH_ROUTER_BSSID[6] = {0xF0, 0xB4, 0x29, 0x2C, 0x7C, 0x72}; /*MAC OF ROUTER*/

/*
 * please change MESH_ROUTER_SSID and MESH_ROUTER_PASSWD according to your router
 */
// #define MESH_ROUTER_SSID     "Meisya"      /*THE ROUTER SSID*/
// #define MESH_ROUTER_PASSWD   "onggang16"    /*THE ROUTER PASSWORD*/
#define MESH_ROUTER_SSID     "Android"      /*THE ROUTER SSID*/
#define MESH_ROUTER_PASSWD   "bentarya"    /*THE ROUTER PASSWORD*/
#define MESH_SSID_PREFIX     "ESP_MESH"    /*SET THE DEFAULT MESH SSID PREFIX;THE FINAL SSID OF SOFTAP WOULD BE "MESH_SSID_PREFIX_X_YYYYYY"*/
#define MESH_AUTH            AUTH_WPA2_PSK  /*AUTH_MODE OF SOFTAP FOR EACH MESH NODE*/
#define MESH_PASSWD          "123123123"    /*SET PASSWORD OF SOFTAP FOR EACH MESH NODE*/
#define MESH_MAX_HOP         (4)            /*MAX_HOPS OF MESH NETWORK*/

#endif

