#ifndef __MAIN_H_
#define __MAIN_H_

#define F_CPU 		8000000UL
#define F_TICK 		1000      // требуемая частота срабатывания прерывания системного таймера в герцах
#define TIMER_TICK  F_CPU/F_TICK-1

void Sys_Init(void);

#endif
