#include "ReactduinoISR.h"

#define ISR_NUM 10

int8_t top = 0;
bool occupation[ISR_NUM];
volatile bool states[ISR_NUM];

react_callback isr_table[ISR_NUM] = {
    [] () { states[0] = true; },
    [] () { states[1] = true; },
    [] () { states[2] = true; },
    [] () { states[3] = true; },
    [] () { states[4] = true; },
    [] () { states[5] = true; },
    [] () { states[6] = true; },
    [] () { states[7] = true; },
    [] () { states[8] = true; },
    [] () { states[9] = true; }
};

int8_t react_isr_alloc(void)
{
    int8_t isr;

    for (isr = 0; isr < ISR_NUM; isr++) {
        if (isr >= top || !occupation[isr]) {
            occupation[isr] = true;
            states[isr] = false;

            if (isr >= top) {
                top = isr + 1;
            }

            return isr;
        }
    }

    return INVALID_ISR;
}

react_callback react_isr_get(int8_t isr)
{
    return isr_table[isr];
}

bool react_isr_check(int8_t isr)
{
    bool state;

    state = states[isr];
    states[isr] = false;

    return state;
}

void react_isr_free(int8_t isr)
{
    if (isr == INVALID_ISR) {
        return;
    }

    occupation[isr] = false;
}
