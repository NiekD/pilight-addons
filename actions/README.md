# count action
The count action is to be used with generic_counter devices (see protocols folder).

## Usage
```
IF ... THEN count DEVICE counter1 UP 1
 or
IF ... THEN count DEVICE counter1 DOWN 2
 or
IF ... THEN count DEVICE counter1 TO 20
```
# write action
The write action can be used to write lines of text to files. 
Both file name and the text to be written may contain device values and functions. 
The write action has a MODE parameter. "MODE new" tells the action to create a new file (and eventually overwrite an existing file with the same name) and write the text to it. "MODE append" is meant to be used if the text must be appended to an existing file. If the  file doesn't exist yet, the file will be created. The location for the file must be an existing folder allowing write access.

## Usage
```
IF ... THEN write TO /home/pi/mylogs/myfile.txt MODE new TEXT My line of text
```
This will create a new file "myfile.txt" with the text "My line of text" as content.

A more advanced example:
```
IF ... write TO /home/pi/mylogs/mylog DATE_FORMAT(dt, %Y%m) .log MODE append TEXT DATE_FORMAT(dt, %Y-%m-%d %H:%M:%S) Switch state: switch.state
```
This is a typical logging example, with a new file created every month and log lines with a date/time prefix.

Note the spaces before and after the functions and device values. These are required for proper processing by the eventing system, but ALL spaces in the resulting file name are removed by the file action.

# http action
The http action can be used in pilight rules to send HTTP POST or GET requests, either with or without parameters. The result of the request can optionally be stored in a generic label and can then be displayed in the gui, and/or can be used in other rules.

If you want to make other rules act on the result of the http(s) request, you can add a trigger that tells that the request is finished.
The trigger is an ordinary generic_switch that is automatically switched to off when the http action starts and to on when it finishes.  

## Usage
```
IF ... THEN http GET|POST <url> [PARAM <parameters>] [RESULT <label device>] [TRIGGER <generic_switch device>]
```
GET or POST  with url are mandatory, PARAM, RESULT and TRIGGER are optional.
Url and parameters can be strings or device values or combinations of both.

RESULT must be a generic_label device and TRIGGER  must be a generic_switch device.

*N.B. A trailing slash (/) is required for the url if it refers to a path as shown in the examples below.*

Some examples

```
IF ... THEN http GET http://192.168.2.10/test.cgi

IF ... THEN http POST https://192.168.2.10/ 

IF ... THEN http GET http://192.168.2.10/ PARAM command=start&reply=yes

IF ... THEN http GET http://192.168.2.10/ RESULT mylabel

IF ... THEN http GET http://192.168.2.10/ PARAM c= mysensor.state RESULT mylabel

IF ... THEN http GET http://192.168.2.10/ PARAM c= mysensor.state RESULT mylabel TRIGGER mygeneric_switch
IF mygeneric_switch.state == on THEN ...
IF mygeneric_switch.state == on AND mylabel.label == OK THEN ...
```
Note the space between the "=" sign and the device variable name in the last example. This space is required for the eventing system to recognize the device variable (or function). 
The action will automaticly remove all spaces from both url and parameters before the request is sent. If you need to retain certain spaces in the parameters you can replace each one of them with %20

