#include <stdbool.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include "matrix.h"
#include "action.h"
#include "suspend.h"
#include "timer.h"
#ifdef PROTOCOL_LUFA
#include "lufa.h"
#endif
#include "recore.h"


void watchdog_on(void) {
    wdt_enable(WDTO_1S);
}

#define wdt_suspend_enable(value)   \
__asm__ __volatile__ (  \
    "in __tmp_reg__,__SREG__" "\n\t"    \
    "cli" "\n\t"    \
    "wdr" "\n\t"    \
    "sts %0,%1" "\n\t"  \
    "out __SREG__,__tmp_reg__" "\n\t"   \
    "sts %0,%2" "\n\t" \
    : /* no outputs */  \
    : "M" (_SFR_MEM_ADDR(_WD_CONTROL_REG)), \
    "r" (_BV(_WD_CHANGE_BIT) | _BV(WDE)), \
    "r" ((uint8_t) ((value & 0x08 ? _WD_PS3_MASK : 0x00) | \
        _BV(WDE)| _BV(WDIE) | (value & 0x07)) ) \
    : "r0"  \
)

static uint8_t wdt_timeout = 0;
static void power_down(uint8_t wdto)
{
    if (USB_DeviceState == DEVICE_STATE_Configured) return;
    wdt_timeout = wdto;
    wdt_suspend_enable(wdto);

    suspend_power_down_action();

#ifdef SOFTPWM_LED_ENABLE
    softpwm_disable();
#endif

#ifdef USE_SLEEP_MODE_STANDBY
    set_sleep_mode(SLEEP_MODE_STANDBY);
#else
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
#endif
    sleep_enable();
    sei();
    sleep_cpu();
    sleep_disable();
}

void suspend_power_down(void)
{
    power_down(WDTO_30MS);
}

// run immediately after wakeup
void suspend_wakeup_init(void)
{
    /* If clear_keyboard(), the first key pressed when waking up may be missed.*/
    //clear_keyboard();
    suspend_wakeup_init_action();
}

ISR(WDT_vect)
{
    if (wdt_timeout == WDTO_30MS) { timer_count += 30 + 2; }  
}



