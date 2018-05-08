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
    uint32_t now = millis();    

    for (r = 0; r < _top; r++) {
        reaction_entry_t& r_entry = _table[r];

        if (!(r_entry.flags & REACTION_FLAG_ALLOCATED) || !(r_entry.flags & REACTION_FLAG_ENABLED)) {
            continue;
        }

        switch (REACTION_TYPE(r_entry.flags)) {
            case REACTION_TYPE_DELAY: {
                uint32_t elapsed;

                elapsed = now - r_entry.param1;

                if (elapsed >= r_entry.param2) {
                    free(r);
                    r_entry.cb();
                }

                break;
            }

            case REACTION_TYPE_REPEAT: {
                uint32_t elapsed;

                elapsed = now - r_entry.param1;

                if (elapsed >= r_entry.param2) {
                    r_entry.param1 = now;
                    r_entry.cb();
                }

                break;
            }

            case REACTION_TYPE_STREAM: {
                Stream *stream;

                stream = (Stream *) r_entry.ptr;

                if (stream->available()) {
                    r_entry.cb();
                }

                break;
            }

            case REACTION_TYPE_INTERRUPT: {
                if (react_isr_check(r_entry.param2)) {
                    r_entry.cb();
                }

                break;
            }

            case REACTION_TYPE_INPUT_CHANGE: {
                input_change_specs_t specs;
                uint8_t new_state, last_state;
                
                specs.as_uint32 = r_entry.param1;
                new_state = digitalRead(specs.detail.pin);
                last_state = (uint8_t)r_entry.param2;

                if (new_state != last_state) {
                    if (specs.detail.state == INPUT_STATE_ANY || new_state == specs.detail.state){
                        r_entry.cb();
                    }
                    r_entry.param2 = new_state;
                }

                break;
            }  

            case REACTION_TYPE_TICK: {
                r_entry.cb();

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
    input_change_specs_t specs;

    r = alloc(REACTION_TYPE_INPUT_CHANGE, cb);

    if (r == INVALID_REACTION) {
        return INVALID_REACTION;
    }

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
