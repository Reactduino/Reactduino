#ifndef REACTDUINO_ISR_H_
#define REACTDUINO_ISR_H_

#include "Reactduino.h"

#define INVALID_ISR -1

int8_t react_isr_alloc(void);
react_callback react_isr_get(int8_t isr);
bool react_isr_check(int8_t isr);
void react_isr_free(int8_t isr);

#endif
