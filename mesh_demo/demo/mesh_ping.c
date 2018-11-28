#include "mem.h"
#include "osapi.h"
#include "c_types.h"
#include "mesh_ping.h"
#include "osapi.h"
#include "os_type.h"

int timerPing;

void ICACHE_FLASH_ATTR
ping_timer_init()
{
    static os_timer_t ping_timer;
    os_timer_disarm(&ping_timer);
    os_timer_setfn(&ping_timer, (os_timer_func_t *)ping_timer_bhfunc, NULL);
    os_timer_arm(&ping_timer, 1, true);
}

void ICACHE_FLASH_ATTR 
ping_timer_bhfunc ()
{
    timerPing++;
}

void ICACHE_FLASH_ATTR 
reset_ping_timer_bhfunc ()
{
    timerPing = 0;
}

unsigned int ICACHE_FLASH_ATTR 
read_ping_timer()
{
    return timerPing;
}


