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
#include "gpio.h"

#include "ets_sys.h"
#include "i2c.h"
#include "bh1750.h"
//#include "driver/uart.h"
#include "os_type.h"
#include "user_interface.h"
#include "user_config.h"

#include "dht22.h" 
#include "gpio16.h"

#define MESH_PRINT  ets_printf
#define MESH_STRLEN ets_strlen
#define MESH_MEMCPY ets_memcpy
#define MESH_MEMSET ets_memset
#define MESH_FREE   os_free
#define ROOT        0

static esp_tcp ser_tcp;
static struct espconn ser_conn;

void esp_mesh();
void mesh_enable_cb(int8_t res);
void esp_mesh_con_cb(void *);
void esp_recv_entrance(void *, char *, uint16_t);
void pwm_initialize();
void pwm_set(int value);
void sensor_init();
void sensor_cb();
void mesh_bcast(char buf[]);
void timer_init();
void timer_cb();
void artificial_proccess(char* _Keterangan,char* _Data);

int flag = 0;
int cnt = 0;

int setpoint = 500;
int range = 20;
int brigthnessincrement = 20;
int brigthnessdecrement = 20;
int lastbrigthness = 0;
int brigthness = 0;
uint32 io_info[][3] = {{PERIPHS_IO_MUX_GPIO5_U,FUNC_GPIO5,5}};// PWM setup led
u32 duty[1] = {0};
int last_value = 0;
uint8 data[7];
int value = 0;
char RecvData[100];
unsigned int timerPing = 0;

//DHT22
extern uint8_t pin_num[GPIO_PIN_NUM];
DHT_Sensor sensor;

void ICACHE_FLASH_ATTR esp_recv_entrance(void *arg, char *pdata, uint16_t len)
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
    if(!ROOT){
        if(flag==0){
            flag=1;
            int i = 0;
            bool kamu = false;
            char tst_data[104];  
            char Keterangan[20];
            char Data[100];
            for (i = 0; i < 104; i++){tst_data[i] = '\0';}
            for (i = 0; i < 20; i++){Keterangan[i] = '\0';}
            for (i = 0; i < 100; i++){Data[i] = '\0';}
            for (i = 0; i < len; i++)
            {
                if (usr_data[i] == '{' && usr_data[i+1] == '"')
                {
                    kamu =  true;
                }
                if (kamu)
                {
                    os_sprintf (tst_data, "%s%c",tst_data, usr_data[i]);
                }
            }
            uint8 Mark[4] = {0,0,0,0};
            uint8 count = 0;
            for ( i = 0; i < strlen(tst_data); i++)
            {
                if (tst_data[i] == '"')
                {
                    Mark[count] = i;
                    count++;
                }

            }
            count = 0;
            for (i = Mark[0]; i < Mark[1]-1; i++)
            {
                Keterangan[count++] = tst_data[i+1];
            }
            count = 0;
            for (i = Mark[2]; i < Mark[3]-1; i++)
            {
                Data[count++] = tst_data[i+1];
            }
            artificial_proccess(Keterangan,Data);
            MESH_PRINT("Paket:%s",tst_data);
            MESH_PRINT("Keterangan:%s\n",Keterangan);
            MESH_PRINT("Data:%s\n\n",Data);
            mesh_bcast(usr_data);
        }
    }
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

void ICACHE_FLASH_ATTR artificial_proccess(char* _Keterangan,char* _Data)
{

    if (_Keterangan[0] == 'C' && _Keterangan[1] == 'D')pwm_set(atoi(_Data));
   
}

void ICACHE_FLASH_ATTR mesh_bcast(char buf[])
{
    uint8_t src[6];
    uint8_t dst[6];
    struct mesh_header_format *header = NULL;

    if (!wifi_get_macaddr(STATION_IF, src)) {
        MESH_PRINT("bcast get sta mac fail\n");
        return;
    }

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

void ICACHE_FLASH_ATTR esp_mesh_con_cb(void *arg)
{
    static os_timer_t tst_timer;
    struct espconn *server = (struct espconn *)arg;

    MESH_PRINT("esp_mesh_con_cb\n");

    if (server != &ser_conn) {
        MESH_PRINT("con_cb, para err\n");
        return;
    }
    
    timer_init();
    pwm_initialize();
    if(ROOT){
        sensor_init();
    } 

    os_timer_disarm(&tst_timer);
    os_timer_setfn(&tst_timer, (os_timer_func_t *)esp_mesh, NULL);
    os_timer_arm(&tst_timer, 5000, true);
}
void ICACHE_FLASH_ATTR timer_init()
{
    static os_timer_t timer_timer;
    os_timer_disarm(&timer_timer);
    os_timer_setfn(&timer_timer, (os_timer_func_t *)timer_cb, NULL);
    os_timer_arm(&timer_timer, 1000, true);
}

void ICACHE_FLASH_ATTR timer_cb()
{
    if(flag==1){
        cnt++;
        if(cnt>5){
            cnt=0;
            flag=0;
        }
    }
}

void ICACHE_FLASH_ATTR sensor_init()
{
    sensor.pin = 5;
	sensor.type = DHT11;
 	MESH_DEMO_PRINT("DHT22 init on GPIO%d\r\n", pin_num[sensor.pin]);
	DHTInit(&sensor);
    if (BH1750Init()) {
    	MESH_DEMO_PRINT("BH1750 init done.\r\n");
    }
    else {
        MESH_DEMO_PRINT("BH1750 init error.\r\n");
        return;
    }
    static os_timer_t sensor_timer;
    os_timer_disarm(&sensor_timer);
    os_timer_setfn(&sensor_timer, (os_timer_func_t *)sensor_cb, NULL);
    os_timer_arm(&sensor_timer, 10000, true);
}

void ICACHE_FLASH_ATTR sensor_cb(){
    //baca
    MESH_PRINT("BACA SENSOR \r\n"); 
    // baca dht
	static uint8_t i;
	DHT_Sensor_Data data;
	uint8_t pin;
	pin = pin_num[sensor.pin];
	if (DHTRead(&sensor, &data))
	{
	    char d1[20];DHTFloat2String(d1, data.temperature);
	    char d2[20];DHTFloat2String(d2, data.humidity);
	    char Send_Paket[64];
	    os_sprintf (Send_Paket, "{\"TM\":\"%s\"}",d1);
	    MESH_DEMO_PRINT("Temperature: %s *C\r\n", d1);
	    MESH_DEMO_PRINT("Humidity: %s %%\r\n", d2);
	} else {
	    MESH_DEMO_PRINT("Failed to read temperature and humidity sensor on GPIO%d\n", pin);
	}
	int datalux;
    char temp[80];
    MESH_DEMO_PRINT("Get light...\r\n");
    datalux = GetLight();
    if(datalux == -1){
    	MESH_DEMO_PRINT("Error get light value\r\n");
        os_sprintf(temp,"{\"CD\":\"%d\"}", 96);
		mesh_bcast(temp);
    }
    else
    {
        pwm_set(datalux);
    	os_sprintf(temp,"{\"CD\":\"%d\"}", datalux);
		mesh_bcast(temp);
    }
}

void ICACHE_FLASH_ATTR pwm_initialize()
{
    //inisialisai PWM
    MESH_PRINT("PWM INIT DONE\r\n");
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U,FUNC_GPIO5);
	gpio_init();
    pwm_init(1000, duty,1,io_info);
}

void ICACHE_FLASH_ATTR pwm_set(int value)
{
    MESH_PRINT("PWM SET\r\n");
    if (value>(setpoint - range) && value <(setpoint + range))
	{
		brigthness=lastbrigthness;
	}
	else if (value>(setpoint + range))
	{
	MESH_DEMO_PRINT("lux tinggi\r\n");
		//brigthness= lastbrigthness + brigthnessincrement;
		last_value = last_value - brigthnessdecrement;
		if(last_value < 0){
			last_value = 0;
		}else if(last_value > 100){
			last_value = 100;
		}
		pwm_set_duty(last_value * 222, 0);  
		pwm_start();
		//brigthness=lastbrigthness;
	}
	else if (value<(setpoint - range))
	{
	MESH_DEMO_PRINT("lux rendah\r\n");
		//brigthness= lastbrigthness - brigthnessdecrement;
		last_value = last_value + brigthnessincrement;
		if(last_value < 0){
			last_value = 0;
		}else if(last_value > 100){
			last_value = 100;
		}
		pwm_set_duty(last_value * 222, 0);  
		pwm_start();
	}
	//os_sprintf ("{\"value\":\"%d\"}",brigthness);
	MESH_DEMO_PRINT("brigthness:%d\r\n",last_value);
	MESH_DEMO_PRINT("lux       :%d\r\n",value);
}


void ICACHE_FLASH_ATTR mesh_enable_cb(int8_t res)
{
	MESH_PRINT("mesh_enable_cb\n");
    MESH_PRINT("I am ROOT\r\n");

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
    
}

void ICACHE_FLASH_ATTR esp_mesh()
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

bool ICACHE_FLASH_ATTR esp_mesh_init()
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
void ICACHE_FLASH_ATTR user_init(void)
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

