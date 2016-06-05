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
The file function has a MODE parameter. MODE new tells the function to create a new file (and possibly overwrite an existing file) and write the text to it. MODE append is meant to be used if the text must be appended to an existing file. If the  file doesn't exist yet, the file will be created.

## Usage
```
IF ... THEN file TO /home/pi/mylogs/mylog1.log MODE append TEXT My line of text
 or
IF ... file TO /home/pi/mylogs /mylog DATE_FORMAT(dt, %Y%m) .log MODE append TEXT DATE_FORMAT(dt, %Y-%m-%d %H:%M:%S) Switch state switch.state
```
