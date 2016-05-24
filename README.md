# pilight-addons
This repository contains some addons for pilight 7.

Two protocols:

* webswitch, which can send HTTP GET/POST requests and return the response
* wunderful, an extended version of the wunderground protocol

A function:

* LOOKUP, which can fetch individual values from a query string (name=value pairs separated by "&"). Usefull in combination with the webswitch protocol and for translation/conversion of device variables.
 
An operator:

* ISNOT, returns true if two strings are not equal (so the opposite of IS)
