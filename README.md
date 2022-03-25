Simple Arduino based CAN interface with display for Eletek FlatPack2 PSU's.

Original code and credit goes to https://github.com/The6P4C/Flatpack2

Slimmed and added voltage and display control, still need to add alarm and error codes to display

Requires Adafruit SSD1306 library
ande mcp_can library

CAN board
https://www.amazon.com/gp/product/B087Z9KBXR/ref=ppx_yo_dt_b_asin_title_o09_s00?ie=UTF8&psc=1

Display
https://www.amazon.com/gp/product/B08TLXYKS6/ref=ppx_yo_dt_b_asin_title_o01_s01?ie=UTF8&psc=1

To set voltage connect via serial monitor; 115200 baud and carriage return.  Send S followed by voltage in centivolts, eg. S4850 would be 48.50 volts.
