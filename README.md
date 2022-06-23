# Dorm Lights

This Arduino project uses an ESP8266 to control a ceiling-mounted WS2811 LED strip. It hosts a webserver that provides controls for HSV values and switching between animations.

## Config

You'll note that the `dorm_lights.ino` file includes a `config.h` file that is missing from the repository. This file should only exist in the root of the local repo. It should include the following code:

`#define STASSID "networkname"`  
`#define STAPSK  "password"`

where `networkname` is your WiFi network name and `password` is your WiFi password.
