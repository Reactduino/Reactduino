#include <Arduino.h>
#include <string.h>

#include "Reactduino.h"

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

    for (r = 0; r < _top; r++) {
        if (!(_table[r].flags & REACTION_FLAG_ALLOCATED) || !(_table[r].flags & REACTION_FLAG_ENABLED)) {
            continue;
        }

        switch (_table[r].flags & REACTION_TYPE_MASK) {
            case REACTION_TYPE_DELAY: {
                uint32_t elapsed;

                elapsed = millis() - _table[r].param1;

                if (elapsed >= _table[r].param2) {
                    free(r);
                    _table[r].cb();
                }

                break;
            }

            case REACTION_TYPE_REPEAT: {
                uint32_t elapsed;

                elapsed = millis() - _table[r].param1;

                if (elapsed >= _table[r].param2) {
                    _table[r].param1 = millis();
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
    // TODO: Complete with isr's
    return INVALID_REACTION;
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

void Reactduino::enable(reaction r)
{
    if (r == INVALID_REACTION) {
        return;
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

    _table[r].flags &= ~REACTION_FLAG_ALLOCATED;
}

reaction Reactduino::alloc(uint8_t type, react_callback cb)
{
    reaction r;

    for (r = 0; r < REACTDUINO_MAX_REACTIONS; r++) {
        if (r >= _top || !(_table[r].flags & REACTION_FLAG_ALLOCATED)) {
            _table[r].flags = REACTION_FLAG_ALLOCATED | REACTION_FLAG_ENABLED | (type & REACTION_TYPE_MASK);
            _table[r].cb = cb;

            if (r >= _top) {
                _top = r + 1;
            }

            return r;
        }
    }

    return INVALID_REACTION;
}
