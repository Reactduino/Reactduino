#ifndef REACTDUINO_H_
#define REACTDUINO_H_

#include <Arduino.h>
#include <stdint.h>

#define INVALID_REACTION -1

#ifndef REACTDUINO_MAX_REACTIONS
#define REACTDUINO_MAX_REACTIONS 50
#endif

typedef void (*react_callback) (void);
typedef int32_t reaction;

#define REACTION_FLAG_ALLOCATED 0x80
#define REACTION_FLAG_ENABLED 0x40
#define REACTION_TYPE_MASK 0x3F

#define REACTION_TYPE_DELAY 0
#define REACTION_TYPE_REPEAT 1
#define REACTION_TYPE_STREAM 2
#define REACTION_TYPE_INTERRUPT 3
#define REACTION_TYPE_TICK 4

#define REACTION_TYPE(x) ((x) & REACTION_TYPE_MASK)

typedef struct reaction_entry_t_ {
    uint8_t flags;
    void *ptr;
    uint32_t param1, param2;
    react_callback cb;
} reaction_entry_t;

class Reactduino
{
public:
    Reactduino(react_callback cb);
    void setup(void);
    void tick(void);

    // Public API
    reaction delay(uint32_t t, react_callback cb);
    reaction repeat(uint32_t t, react_callback cb);
    reaction onAvailable(Stream *stream, react_callback cb);
    reaction onInterrupt(uint8_t number, react_callback cb, int mode);
    reaction onPinRising(uint8_t pin, react_callback cb);
    reaction onPinFalling(uint8_t pin, react_callback cb);
    reaction onPinChange(uint8_t pin, react_callback cb);
    reaction onTick(react_callback cb);

    void enable(reaction r);
    void disable(reaction r);
    void free(reaction r);

private:
    react_callback _setup;
    reaction_entry_t _table[REACTDUINO_MAX_REACTIONS];
    reaction _top = 0;

    reaction alloc(uint8_t type, react_callback cb);
};

extern Reactduino app;

#endif
