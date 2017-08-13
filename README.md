 = HOW IT WORKS =
 - Use the DTMF lib to detect DTMF codes received from a HAM radio (baofeng UV-B2 or any appliance that can output signal with DTMF through a jack)
 - The DTMF lib has been modified to return wheter a signal without DTMF has been detected (source https://forum.arduino.cc/index.php?topic=121540.0)
 - if signal is detected then UP() the relay
 - if DTMF code is '0', down() (and sleep for 2 seconds to avoid to up the relay imediately because of signal presence)
 - if DTMF code is '*', lower brightness
 - if DTMF code is '#', higher brightness
 - for any other code, just up() the relay
 - Each time the relay is up(), a 5 minutes timer is reset
 - After 5 minutes being UP the relay is shutdown
 
= SCHEMATICS =
== SPEAKER SCHEMATICS ==
Ref: https://i.stack.imgur.com/XZddX.jpg
    ┌=sleeve=|=ring=|=tip=
    │
    ├[SPRK-]-|-ring-|-[SPRK+]-
    |



[VCC=5V+]------┐
               │
             [R1=1k]
               │
SPK+ ----------┼----[ARDUINO A0]
               │
             [R2=1k]
               │
SPK- ----------┤
               │
[GND]----------┘
             
            
== RELAY SCHEMATICS ==

[VBAT+]----------┐
                 │+
               [LED]
                 │-
[IRF540: DRAIN]--┘
                 -[IFR540: GATE]┬-[R=1k]-----[ARDUINO D11]
[IRF540: SOURCE]-┐              │
                 │             [R=10k]
[VBAT-]----------┤              │
[GND]------------┴--------------┘

 
== ARDUINO SCHEMATICS ==

[ARDUINO GND]---------[VBAT-]
[ARDUINO VIN]---------[VBAT+]
[ARDUINO A0]----------[SPK+]
[ARDUINO D11]---------[R=1k]--[IFR540: GATE]
