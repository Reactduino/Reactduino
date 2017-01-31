# Reactduino

An asynchronous programming library for the Arduino platform.

## Usage

### Example 1: Serial Echo Program

This application will write what it reads back to the serial port.

```cpp
#include <Arduino.h>
#include <Reactduino.h>

void React::setup(void)
{
  Serial.begin(9600);

  react.onAvailable(Serial, [] (void) {
    Serial.write(Serial.read());
  });
}
```

This program is equivalent to:

```cpp
#include <Arduino.h>
#include <Reactduino.h>

void onSerialData(void)
{
  Serial.write(Serial.read());
}

void React::setup(void)
{
  Serial.begin(9600);

  react.onAvailable(Serial, onSerialData);
}
```


### Example 2: Timer Example

This application will write data 'Hello' to the serial port every 10 seconds.

```cpp
#include <Arduino.h>
#include <Reactduino.h>

void React::setup(void)
{
  Serial.begin(9600);

  react.repeat(10 * 1000, [] (void) {
    Serial.println("Hello");
  });
 
  // Use react.delay() for a one off timer
}
```

This program is equivalent to:

```cpp
#include <Arduino.h>
#include <Reactduino.h>

void onInterval(void)
{
  Serial.println("World");
}

void React::setup(void)
{
  Serial.begin(9600);

  react.repeat(10 * 1000, onInterval);
  // Use react.delay() for a one off timer
}
```
