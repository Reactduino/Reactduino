#include <Arduino.h>
#include <string.h>

#include "Reactduino.h"
#include "ReactduinoISR.h"


typedef union {
    uint32_t as_uint32;
    struct {
        uint8_t pin;
        uint8_t state;
    } detail;
} input_change_specs_t;


void setup(void)
{
    app.setup();
}

void loop(void)
{
    app.tick();
    yield();
}

Reactduino::Reactduino(react_callback cb) : _setup(cb)
{
}

void Reactduino::setup(void)
{
    _setup();
}

void Reactduino::tick(void)
{
    reaction r;
    uint32_t now = millis();    // better caching the current time, since it is used in
                                // several places. This way we both optimize by calling millis()
                                // only once, and we ensure having a consistent time when managing
                                // iall the reactions.

    for (r = 0; r < _top; r++) {
        if (!(_table[r].flags & REACTION_FLAG_ALLOCATED) || !(_table[r].flags & REACTION_FLAG_ENABLED)) {
            continue;
        }

        switch (REACTION_TYPE(_table[r].flags)) {
            case REACTION_TYPE_DELAY: {
                uint32_t elapsed;

                elapsed = now - _table[r].param1;

                if (elapsed >= _table[r].param2) {
                    free(r);
                    _table[r].cb();
                }

                break;
            }

            case REACTION_TYPE_REPEAT: {
                uint32_t elapsed;

                elapsed = now - _table[r].param1;

                if (elapsed >= _table[r].param2) {
                    _table[r].param1 = now;
                    _table[r].cb();
                }

                break;
            }

            case REACTION_TYPE_STREAM: {
                Stream *stream;

                stream = (Stream *) _table[r].ptr;

                if (stream->available()) {
                    _table[r].cb();
                }

                break;
            }

            case REACTION_TYPE_INTERRUPT: {
                if (react_isr_check(_table[r].param2)) {
                    _table[r].cb();
                }

                break;
            }

            case REACTION_TYPE_INPUT_CHANGE: {
                input_change_specs_t specs;
                specs.as_uint32 = _table[r].param1;

                uint8_t new_state = digitalRead(specs.detail.pin);
                uint8_t last_state = (uint8_t)_table[r].param2;

                if (new_state != last_state) {
                    if (specs.detail.state == INPUT_STATE_ANY || new_state == specs.detail.state){
                        _table[r].cb();
                    }
                    _table[r].param2 = new_state;
                }

                break;
            }  

            case REACTION_TYPE_TICK: {
                _table[r].cb();

                break;
            }
        }
    }
}

reaction Reactduino::delay(uint32_t t, react_callback cb)
{
    reaction r;

    r = alloc(REACTION_TYPE_DELAY, cb);

    if (r == INVALID_REACTION) {
        return INVALID_REACTION;
    }

    _table[r].param1 = millis();
    _table[r].param2 = t;

    return r;
}

reaction Reactduino::repeat(uint32_t t, react_callback cb)
{
    reaction r;

    r = alloc(REACTION_TYPE_REPEAT, cb);

    if (r == INVALID_REACTION) {
        return INVALID_REACTION;
    }

    _table[r].param1 = millis();
    _table[r].param2 = t;

    return r;
}

reaction Reactduino::onAvailable(Stream *stream, react_callback cb)
{
    reaction r;

    r = alloc(REACTION_TYPE_STREAM, cb);

    if (r == INVALID_REACTION) {
        return INVALID_REACTION;
    }

    _table[r].ptr = stream;

    return r;
}

reaction Reactduino::onInterrupt(uint8_t number, react_callback cb, int mode)
{
    reaction r;
    int8_t isr;

    // Obtain and use ISR to handle the interrupt, see: ReactduinoISR.c/.h

    isr = react_isr_alloc();

    if (isr == INVALID_ISR) {
        return INVALID_REACTION;
    }

    r = alloc(REACTION_TYPE_INTERRUPT, cb);

    if (r == INVALID_REACTION) {
        react_isr_free(isr);
        return INVALID_REACTION;
    }

    _table[r].param1 = isr;
    _table[r].param2 = number;

    attachInterrupt(number, react_isr_get(isr), mode);

    return r;
}

reaction Reactduino::onInputChange(uint8_t pin, react_callback cb, int state)
{
    reaction r;

    r = alloc(REACTION_TYPE_INPUT_CHANGE, cb);

    if (r == INVALID_REACTION) {
        return INVALID_REACTION;
    }

    input_change_specs_t specs;
    specs.detail.pin = pin;
    specs.detail.state = state;

    _table[r].param1 = specs.as_uint32;   
    _table[r].param2 = INPUT_STATE_UNSET;   // param2 is used to store the last state

    return r;
}

reaction Reactduino::onPinRising(uint8_t pin, react_callback cb)
{
    return onInterrupt(digitalPinToInterrupt(pin), cb, RISING);
}

reaction Reactduino::onPinFalling(uint8_t pin, react_callback cb)
{
    return onInterrupt(digitalPinToInterrupt(pin), cb, FALLING);
}

reaction Reactduino::onPinChange(uint8_t pin, react_callback cb)
{
    return onInterrupt(digitalPinToInterrupt(pin), cb, CHANGE);
}

reaction Reactduino::onPinRisingNoInt(uint8_t pin, react_callback cb)
{
    return onInputChange(pin, cb, INPUT_STATE_HIGH);
}

reaction Reactduino::onPinFallingNoInt(uint8_t pin, react_callback cb)
{
    return onInputChange(pin, cb, INPUT_STATE_LOW);
}

reaction Reactduino::onPinChangeNoInt(uint8_t pin, react_callback cb)
{
    return onInputChange(pin, cb, INPUT_STATE_ANY);
}

reaction Reactduino::onTick(react_callback cb)
{
    return alloc(REACTION_TYPE_TICK, cb);
}

void Reactduino::enable(reaction r)
{
    if (r == INVALID_REACTION) {
        return;
    }

    // Clear any interrupts whilst reaction was disabled
    if (REACTION_TYPE(_table[r].flags) == REACTION_TYPE_INTERRUPT) {
        react_isr_check(_table[r].param1);
    }

    _table[r].flags |= REACTION_FLAG_ENABLED;
}

void Reactduino::disable(reaction r)
{
    if (r == INVALID_REACTION) {
        return;
    }

    _table[r].flags &= ~REACTION_FLAG_ENABLED;
}

void Reactduino::free(reaction r)
{
    if (r == INVALID_REACTION) {
        return;
    }

    // Disable any interrupts and free the ISR for reallocation
    if (REACTION_TYPE(_table[r].flags) == REACTION_TYPE_INTERRUPT) {
        detachInterrupt(_table[r].param2);
        react_isr_free(_table[r].param1);
    }

    _table[r].flags &= ~REACTION_FLAG_ALLOCATED;

    // Move the top of the stack pointer down if we free from the top
    if (_top == r + 1) {
        _top--;
    }
}

reaction Reactduino::alloc(uint8_t type, react_callback cb)
{
    reaction r;

    for (r = 0; r < REACTDUINO_MAX_REACTIONS; r++) {
        // If we're at the top of the stak or the allocated flag isn't set
        if (r >= _top || !(_table[r].flags & REACTION_FLAG_ALLOCATED)) {
            // Reaction is allocated, enabled and of the provided type
            _table[r].flags = REACTION_FLAG_ALLOCATED | REACTION_FLAG_ENABLED | (type & REACTION_TYPE_MASK);
            _table[r].cb = cb;

            // Move the stack pointer up if we add to the top
            if (r >= _top) {
                _top = r + 1;
            }

            return r;
        }
    }

    return INVALID_REACTION;
}
