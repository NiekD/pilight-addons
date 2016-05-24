# pilight-addons
This repository contains some addons for pilight 7.

Three protocols:

* webswitch, which can send HTTP GET/POST requests and return the response
* wunderful, an extended version of the wunderground protocol
* generic_counter, a counter that can be manipulated in rules with  the count action (in progress).

One action:

* count (in progress), to be used in rules to update a generic_counter device (increment, decrement or set to a specific value).

One function:

* LOOKUP, which can fetch individual values from a query string (name=value pairs separated by "&"). Usefull in combination with the webswitch protocol and for translation/conversion of device variables.
 
One operator:

* ISNOT, returns true if two strings are not equal (so the opposite of IS)
