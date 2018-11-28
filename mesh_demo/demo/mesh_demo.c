/******************************************************************************
 * Copyright 2013-2014 Espressif Systems
 *
 * FileName: mesh_demo.c
 *
 * Description: mesh demo
 *
 * Modification history:
 *     2015/12/4, v1.0 create this file.
*******************************************************************************/
#include "mem.h"
#include "mesh.h"
#include "osapi.h"
#include "c_types.h"
#include "espconn.h"
#include "user_config.h"

#define MESH_PRINT  ets_printf
#define MESH_STRLEN ets_strlen
#define MESH_MEMCPY ets_memcpy
#define MESH_MEMSET ets_memset
#define MESH_FREE   os_free
#define ROOT        1

static esp_tcp ser_tcp;
static struct espconn ser_conn;

void esp_mesh_test();
void mesh_enable_cb(int8_t res);
void esp_mesh_con_cb(void *);
void esp_recv_entrance(void *, char *, uint16_t);

void esp_recv_entrance(void *arg, char *pdata, uint16_t len)
{
    uint8_t *usr_data = NULL;
    uint16_t usr_data_len = 0;
    uint8_t *src = NULL, *dst = NULL;
    enum mesh_usr_proto_type proto;
    uint8_t *resp = "{\"rsp_key\":\"rsp_key_value\"}";
    struct mesh_header_format *header = (struct mesh_header_format *)pdata;

    MESH_PRINT("recv entrance\n");
    
    if (!pdata)
        return;

    if (!espconn_mesh_get_usr_data_proto(header, &proto))
        return;
    if (!espconn_mesh_get_usr_data(header, &usr_data, &usr_data_len)) 
        return;

    MESH_PRINT("user data, len:%d, content:%s\n", &usr_data_len, usr_data);
    /* 
     * process packet
     * call packet_parser(...)
     * general packet_parser demo
     */

#if 0
     packet_parser[proto]->handler(arg, usr_data, usr_data_len);
#endif

    /*
     * then build response packet,
     * and give resonse to controller
     * the following code is response demo
     */
#if 0
    if (!espconn_mesh_get_src_addr(header, &src) || !espconn_mesh_get_dst_addr(header, &dst)) {
        MESH_PRINT("get addr fail\n");
        return;
    }

    header = (struct mesh_header_format *)espconn_mesh_create_packet(
                            src,   // destiny address
                            dst,   // source address
                            false, // not p2p packet
                            true,  // piggyback congest request
                            M_PROTO_JSON,  // packe with JSON format
                            MESH_STRLEN(resp),  // data length
                            false, // no option
                            0,     // option len
                            false, // no frag
                            0,     // frag type, this packet doesn't use frag
                            false, // more frag
                            0,     // frag index
                            0);    // frag length
    if (!header) {
        MESH_PRINT("create packet fail\n");
        return;
    }

    if (!espconn_mesh_set_usr_data(header, resp, MESH_STRLEN(resp))) {
        MESH_PRINT("set user data fail\n");
        MESH_FREE(header);
        return;
    }

    if (espconn_mesh_sent(&ser_conn, (uint8_t *)header, header->len)) {
        MESH_PRINT("mesh sent fail\n");
        MESH_FREE(header);
        return;
    }

    MESH_FREE(header);
#endif
}

void ICACHE_FLASH_ATTR
mesh_json_bcast()
{
    char buf[32];
    uint8_t src[6];
    uint8_t dst[6];
    struct mesh_header_format *header = NULL;

    if (!wifi_get_macaddr(STATION_IF, src)) {
        MESH_PRINT("bcast get sta mac fail\n");
        return;
    }

    os_memset(buf, 0, sizeof(buf));

    os_sprintf(buf, "%s", "{\"bcast\":\"");
    os_sprintf(buf + os_strlen(buf), MACSTR, MAC2STR(src));
    os_sprintf(buf + os_strlen(buf), "%s", "\"}\r\n");

    os_memset(dst, 0, sizeof(dst));  // use bcast to get all the devices working in mesh from root.
    header = (struct mesh_header_format *)espconn_mesh_create_packet(
                            dst,     // destiny address (bcast)
                            src,     // source address
                            false,   // not p2p packet
                            true,    // piggyback congest request
                            M_PROTO_JSON,   // packe with JSON format
                            os_strlen(buf), // data length
                            false,   // no option
                            0,       // option total len
                            false,   // no frag
                            0,       // frag type, this packet doesn't use frag
                            false,   // more frag
                            0,       // frag index
                            0);      // frag length
    if (!header) {
        MESH_PRINT("bcast create packet fail\n");
        return;
    }

    if (!espconn_mesh_set_usr_data(header, buf, os_strlen(buf))) {
        MESH_PRINT("bcast set user data fail\n");
        MESH_FREE(header);
        return;
    }

    if (espconn_mesh_sent(&ser_conn, (uint8_t *)header, header->len)) {
        MESH_PRINT("bcast mesh is busy\n");
        MESH_FREE(header);
        espconn_mesh_connect(&ser_conn);
        return;
    }

    MESH_FREE(header);
}

void ICACHE_FLASH_ATTR
mesh_json_bcast_init()
{
    static os_timer_t bcast_timer;
    os_timer_disarm(&bcast_timer);
    os_timer_setfn(&bcast_timer, (os_timer_func_t *)mesh_json_bcast, NULL);
    os_timer_arm(&bcast_timer, 18000, true);
}

void esp_mesh_con_cb(void *arg)
{
    static os_timer_t tst_timer;
    struct espconn *server = (struct espconn *)arg;

    MESH_PRINT("esp_mesh_con_cb\n");

    if (server != &ser_conn) {
        MESH_PRINT("con_cb, para err\n");
        return;
    }

    os_timer_disarm(&tst_timer);
    os_timer_setfn(&tst_timer, (os_timer_func_t *)esp_mesh_test, NULL);
    os_timer_arm(&tst_timer, 5000, true);
}

void mesh_enable_cb(int8_t res)
{
	MESH_PRINT("mesh_enable_cb\n");

    if (res == MESH_OP_FAILURE) {
	    MESH_PRINT("enable mesh fail\n");
        return;
    }

    /*
     * try to estable user virtual connect
     * user can to use the virtual connect to sent packet to any node, server or mobile.
     * if you want to sent packet to one node in mesh, please build p2p packet
     * if you want to sent packet to server/mobile, please build normal packet (uincast packet)
     * if you want to sent bcast/mcast packet, please build bcast/mcast packet
     */
    MESH_MEMSET(&ser_conn, 0 ,sizeof(ser_conn));
    MESH_MEMSET(&ser_tcp, 0, sizeof(ser_tcp));

    MESH_MEMCPY(ser_tcp.remote_ip, server_ip, sizeof(server_ip));
    ser_tcp.remote_port = server_port;
    ser_tcp.local_port = espconn_port();
    ser_conn.proto.tcp = &ser_tcp;

    if (espconn_regist_connectcb(&ser_conn, esp_mesh_con_cb)) {
        MESH_PRINT("regist con_cb err\n");
        espconn_mesh_disable(NULL);
        return;
    }

    if (espconn_regist_recvcb(&ser_conn, esp_recv_entrance)) {
        MESH_PRINT("regist recv_cb err\n");
        espconn_mesh_disable(NULL);
        return;
    }

    /*
     * regist the other callback
     * sent_cb, reconnect_cb, disconnect_cb
     * if you donn't need the above cb, you donn't need to register them.
     */

    if (espconn_mesh_connect(&ser_conn)) {
        MESH_PRINT("connect err\n");
        espconn_mesh_disable(NULL);
        return;
    }
    
    mesh_json_bcast_init();
}

void esp_mesh_test()
{
    char buf[32];
    uint8_t src[6];
    uint8_t dst[6];
    struct mesh_header_format *header = NULL;

    /*
     * the mesh data can be any content
     * it can be string(json/http), or binary(MQTT).
     */

    if (!wifi_get_macaddr(STATION_IF, src)) {
        MESH_PRINT("get sta mac fail\n");
        return;
    }

    // char *tst_data = "{\"req_key\":\"device2\"}\r\n";
    // uint8_t tst_data[] = {'a', 'b', 'c', 'd'};}
    // uint8_t tst_data[] = {0x01, 0x02, 0x03, 0x04, 0x00};
    os_memset(buf, 0, sizeof(buf));

    os_sprintf(buf, "%s", "{\"from\":\"");
    os_sprintf(buf + os_strlen(buf), MACSTR, MAC2STR(src));
    os_sprintf(buf + os_strlen(buf), "%s", "\"}\r\n");

    MESH_MEMCPY(dst, server_ip, sizeof(server_ip));
    MESH_MEMCPY(dst + sizeof(server_ip), &server_port, sizeof(server_port));

    header = (struct mesh_header_format *)espconn_mesh_create_packet(
                            dst,   // destiny address
                            src,   // source address
                            false, // not p2p packet
                            true,  // piggyback congest request
                            M_PROTO_JSON,  // packe with JSON format
                            MESH_STRLEN(buf),  // data length
                            false, // no option
                            0,     // option len
                            false, // no frag
                            0,     // frag type, this packet doesn't use frag
                            false, // more frag
                            0,     // frag index
                            0);    // frag length
    if (!header) {
        MESH_PRINT("create packet fail\n");
        return;
    }

    if (!espconn_mesh_set_usr_data(header, buf, MESH_STRLEN(buf))) {
        MESH_PRINT("set user data fail\n");
        MESH_FREE(header);
        return;
    }

    if (espconn_mesh_sent(&ser_conn, (uint8_t *)header, header->len)) {
        MESH_PRINT("mesh sent fail\n");
        MESH_FREE(header);
        /*
         * if fail, we re-enable mesh
         */
	    espconn_mesh_enable(mesh_enable_cb, MESH_ONLINE);
        return;
    }

    MESH_FREE(header);
}

bool esp_mesh_init()
{
    struct station_config config;

    // print version of mesh
    espconn_mesh_print_ver();

    /*
     * set the AP password of mesh node
     */
    if (!espconn_mesh_encrypt_init(MESH_AUTH, MESH_PASSWD, MESH_STRLEN(MESH_PASSWD))) {
        MESH_PRINT("set pw fail\n");
        return false;
    }

    /*
     * if you want set max_hop > 4
     * please make the heap is avaliable
     * mac_route_table_size = (4^max_hop - 1)/3 * 6
     */
    if (!espconn_mesh_set_max_hops(MESH_MAX_HOP)) {
        MESH_PRINT("fail, max_hop:%d\n", espconn_mesh_get_max_hops());
        return false;
    }

    /*
     * mesh_ssid_prefix
     * mesh_group_id and mesh_ssid_prefix represent mesh network
     */
    if (!espconn_mesh_set_ssid_prefix(MESH_SSID_PREFIX, MESH_STRLEN(MESH_SSID_PREFIX))) {
        MESH_PRINT("set prefix fail\n");
        return false;
    }

    /*
     * mesh_group_id
     * mesh_group_id and mesh_ssid_prefix represent mesh network
     */
    if (!espconn_mesh_group_id_init((uint8_t *)MESH_GROUP_ID, sizeof(MESH_GROUP_ID))) {
        MESH_PRINT("set grp id fail\n");
        return false;
    }

    if (!espconn_mesh_server_init((struct ip_addr *)server_ip, server_port)) {
        MESH_PRINT("server_init fail\n");
        return false;
    }

    /*
     * please change MESH_ROUTER_SSID and MESH_ROUTER_PASSWD according to your router
     */
    MESH_MEMSET(&config, 0, sizeof(config));
    MESH_MEMCPY(config.ssid, MESH_ROUTER_SSID, MESH_STRLEN(MESH_ROUTER_SSID));
    MESH_MEMCPY(config.password, MESH_ROUTER_PASSWD, MESH_STRLEN(MESH_ROUTER_PASSWD));
    /*
     * you can use esp-touch(smart configure) to sent information about router AP to mesh node
     * if you donn't use esp-touch, you should use espconn_mesh_set_router to set router for mesh node
     */
    if (!espconn_mesh_set_router(&config)) {
        MESH_PRINT("set_router fail\n");
        return false;
    }
    
    return true;
}

void user_rf_pre_init(void){}
void user_pre_init(void){}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void user_init(void)
{
    if (!esp_mesh_init())
        return;

    user_devicefind_init();

    /*
     * enable mesh
     * after enable mesh, you should wait for the mesh_enable_cb to be triggered.
     */
	espconn_mesh_enable(mesh_enable_cb, MESH_ONLINE);
}

