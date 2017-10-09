# pilight-addons
This repository contains some addons for pilight 7.

### WEBSWITCH, WUNDERFUL, HTTP and LOOKUP ARE NO LONGER COMPATIBLE WITH PILIGHT DEVELOPMENT VERSION 7.

CURRENTLY NEW COMPATIBLE VERSIONS ARE BEING TESTED IN DAILY PRACTICE.
THE NEW COMPATIBLE CODE OF THESE MODULES WILL BE PUBLISHED HERE SOON.


Three protocols:

* webswitch, which can send HTTP GET/POST requests and return the response **(see above)**
* wunderful, an extended version of the wunderground protocol **(see above)**
* generic_counter, a counter that can be manipulated in rules with  the count action.

Three actions:

* count, to be used in rules to update a generic_counter device (increment, decrement or set to a specific value).
* write, enables you to write lines of text to files from your rules.
* http, enables you to call remote services via HTTP(S) GET or POST **(see above)**.

One function:

* LOOKUP, which can fetch individual values from a query string (key=value pairs separated by "&"). Usefull in combination with the webswitch protocol and for translation/conversion of device variables **(see above)** .
 
More details and examples of usage can be found in each of the subfolders of this repository.
 
## Installation
All addons can be built as modules to be installed in the appropriate pilight folders. If you are on Raspberry PI, the easiest way to do that is by using the installation shell script you can find in this folder. This script will compile and install the selected addons. If you don't need some of the installed modules installed by the script you can simply delete them afterwards.

