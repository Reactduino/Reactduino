# Reactduino

An asynchronous programming library for the Arduino platform.

## Usage

### Example 1: Serial Echo Program

This application will write what it reads back to the serial port.

```c
#include <Arduino.h>
#include <Reactduino.h>

Reactduino app;

void onSerialData(void)
{
  Serial.write(Serial.read());
}

void setup(void)
{
  Serial.begin(9600);

  app.onAvailable(Serial, onSerialData);
}

void loop(void)
{
  app.tick();
}
```


### Example 2: Timer Example

This application will write data 'Hello' to the serial port every 10 seconds.

```c
#include <Arduino.h>
#include <Reactduino.h>

Reactduino app;

void onInterval(void)
{
  Serial.println("World");
}

void setup(void)
{
  Serial.begin(9600);

  app.onInterval(10 * 1000, onInterval);
  // Use app.onTimer() for a one off timer
}

void loop(void)
{
  app.tick();
}
```
