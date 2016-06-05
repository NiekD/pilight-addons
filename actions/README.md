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
# file action
The file action can be used to write lines of text to files. 
Both file name and the text to be written may contain device values and functions. 
The file action has a MODE parameter."MODE new" tells the action to create a new file (and possibly overwrite an existing file) and write the text to it. "MODE append" is meant to be used if the text must be appended to an existing file. If the  file doesn't exist yet, the file will be created.

## Usage
```
IF ... THEN file TO /home/pi/mylogs/mylog1.log MODE new TEXT My line of text
```
This will create a new file with the text "My line of text"
or a more advanced example
```
IF ... file TO /home/pi/mylogs/mylog DATE_FORMAT(dt, %Y%m) .log MODE append TEXT DATE_FORMAT(dt, %Y-%m-%d %H:%M:%S) Switch state switch.state
```
This is a typical logging example, with a new file created every month and log lines with a date/time prefix.

Note the spaces before and after the functions and device values. These are required for proper processing by the eventing system, but ALL spaces in the resulting file name are removed by the file action.
