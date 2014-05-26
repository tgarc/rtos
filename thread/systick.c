#include "driverlib/SysTick.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "inc/lm3s8962.h"
#include "inc/hw_ints.h"
#include "util/macros.h"

/*
 * resetPeriodUs should be between 1 and 1000
 */
void SysTickInit(ULONG resetPeriodUs) {
	UINT status = StartCritical();
	ULONG resetValue = ((resetPeriodUs*1000)/20) - 1;
	NVIC_ST_CTRL_R = 0; // disable SysTick during setup
	NVIC_ST_RELOAD_R = resetValue; // 1ms period
	NVIC_ST_CURRENT_R = 0; // any write to current clears it
	//NVIC_SYS_PRI3_R = (NVIC_SYS_PRI3_R&0x00FFFFFF)|0xE0000000; // priority 7
	IntPrioritySet(FAULT_SYSTICK, 7);
	// enable SysTick with core clock and interrupts
	NVIC_ST_CTRL_R = NVIC_ST_CTRL_ENABLE+NVIC_ST_CTRL_CLK_SRC+NVIC_ST_CTRL_INTEN;

	EndCritical(status);
}

ULONG SysTickGet(void) {
	return NVIC_ST_CURRENT_R;
}

ULONG SysTickGetReload(void) {
	return NVIC_ST_RELOAD_R;
}

ULONG SysTickGetPeriodUs(void) {
	return (1 + NVIC_ST_RELOAD_R)*20;
}
