# Arduino_ESP8266_SSL

This client library makes it simple for ESP8266/NodeMCU devices to 
store time series data using the online service [pushdata.io](https://pushdata.io).

## Installation

The simplest way to install the library is by using the Arduino IDE Library Manager - see [this guide](https://www.arduino.cc/en/Guide/Libraries) for more information, or the library manager of [PlatformIO](https://platformio.org). In both cases you can search for "pushdata" and click to install the library so your IDE has access to it.

## Usage

```c++
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "Pushdata_ESP8266_SSL.h"

Pushdata_ESP8266_SSL pd;

char ssid[] = "MyWiFiNetwork";
char passwd[] = "MyWiFiPassword";

void setup() {
  // Using nerdy_gnu123@example.com account email - check it out at https://pushdata.io/nerdy_gnu123@example.com
  pd.setEmail("nerdy_gnu123@example.com");
  pd.addWiFi(ssid, passwd);
}

void loop() {
  // Call monitorWiFi() in every loop iteration to make sure the WiFi
  // connection is functional (reconnect if we were disconnected)
  if (pd.monitorWiFi() != WL_CONNECTED) 
    return;
  // Store the value 4711 once every minute. 
  // Note that we do not configure a name for the time series here - the Pushdata library will then use 
  // the ethernet MAC address of the NodeMCU as the time series name. You can change the primary name 
  // to something else in the Pushdata UI, but the MAC address name will always remain, and function as 
  // an alias name (so your device can continue sending data to it without any kind of reconfiguration)
  if (millis() % 60000 == 0) {
    pd.send(4711);
  }
}
```

Check out the [examples](https://github.com/pushdata-io/Arduino_ESP8266_SSL/tree/master/examples) folder also, for more information on how to use the library.

## Auto-naming

This library, and pushdata.io, try very hard to make the lives of IoT hackers simpler. One feature we think may be really useful is the *auto-naming*, which means that you can compile one single Arduino flash image and upload it to many NodeMCU sensor devices, then just switch them on, one at a time, and each one will send their sensor data to a separate timeseries on pushdata.io.

This is possible because the NodeMCUs have a WiFi chip with a unique ethernet MAC address, which is used as the time series name when the user has not specified a name to store the data under. The library will then construct a timeseries name with a special format: `---MACADDRESS__`

Where *MACADDRESS* is the hexadecimal, colon-separated ethernet MAC address. The actual name used can look like this: `---4e:cc:56:b2:67:f9__`

Pushdata.io recognizes this name format, and treats it differently than other timeseries with regular names. It will display these timeseries prominently in the web UI, and prompt the user to change their name to something more human-friendly. But the real magic happens when the user does change the name of the time series. 

### The problem with changing names

Normally, changing the name of a timeseries when you have clients out there sending data using that particular timeseries name is not a good idea. If you have a timeseries called "x" and an Arduino wind sensor installed on the top of Mount Everest that uploads wind data regularly to the "x" timeseries, it is perhaps hard to get up there and reflash the Arduino just because you decided that "x" wasn't a good name for your timeseries. If you don't reflash it, however, and rename "x" to "MountEverestWind", you will have your old wind data in the "MountEverestWind" timeseries, but new sensor readings will be uploaded to "x" still. As "x" was renamed, it will not exist on pushdata.io, so the first time your sensor uploads something, pushdata.io will create "x" again automatically. So you end up with two timeseries: one with the old data and a new name, and one with the old name and new data. Often, this is not what you want.

### The solution

So, what pushdata.io actually does, when encountering a timeseries name in the above format (containing a MAC address), is to rename the timeseries, but remember the old name as an __alias__ name for the same timeseries. This means that a client can send data to it using either the old or the new name.

This functionality makes installation of multiple sensors easier, as it allows you to flash all your sensors with the exact same image, then just start them up one at a time and rename each timeseries in the pushdata.io UI as soon as you see new sensor data coming in.
