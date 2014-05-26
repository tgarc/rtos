#ifndef SYSTICK_H
#define SYSTICK_H

void SysTickInit(ULONG resetUs);
ULONG SysTickGet(void);
ULONG SysTickGetPeriodUs(void);
void SysTickElapsed(void);
ULONG SysTickGetReload(void);

#endif
