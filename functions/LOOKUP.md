# LOOKUP function

The LOOKUP function is intended to fetch values from lists of key=value pairs separated by "&" such as 
```
key1=value1&key2=value2&......
```
Such a list can be a fixed string, or a device value. Although the biggest advantage of the LOOKUP function is when used with webswitch devices returning several parameters, it can be quite useful to simplify your rules. 
Let's say we want to display the state of one of our devices in a label. As an example, we could have in our config:
```
                 "mylabel": {
                        "protocol": [ "generic_label" ],
                        "id": [{
                                "id": 1
                        }],
                        "label": ""
                        "color": "black"
                    "mylamp": {
                        "protocol": [ "kaku_switch" ],
                        "id": [{
                                "id": 12345678,
                                "unit": 0
                        }],
                        "state": "off"
                },
```
We want to show the state of mylamp translated to our own language in mylabel.
Of course we could create separate rules for different states to achieve that, but using LOOKUP, it can be done in one rule:
```
"IF mylamp.state IS on OR mylamp.state IS off THEN label DEVICE mylabel TO LOOKUP(on=aan&off=uit, mylamp.state)";
```
We can use (dummy) LABEL devices to store such lists when they are accessed by multiple rules. Simply  add a label to your config like:
```
                "translate": {
                        "protocol": [ "generic_label" ],
                        "id": [{
                                "id": 2
                        }],
                        "label": "on=aan&off=uit&running=aan&stopped=uit&Red=Rood&Orange=Oranje&Yellow=Geel"
                        "color": "black"
                },
```
This also gives us a simple way of storing variables dynamically by writing key=value pairs to a label device:
IF ... THEN label DEVICE mylabel TO state=on&
The LOOKUP function takes an optional third parameter. This parameter can be a string, a number, or a single asterisk (*).   The string or number provided will be returned if the key searched for doesn't exist.  If an asterisk is entered, the key itself will be returned if the key is not present in the list.
Without the third parameter, the string "?" will be returned if the key is not found.
The type of the value returned by the LOOKUP function can be either a number, or a string, depending on its value.
Example:
Let's assume that the value of label1.label "a=1&b=two"
To use the value of "a" in a rule we can do:
```
IF ... LOOKUP(mydevice.option, a) == 1 THEN ...
```
We need to use ==, because the value of "a" is numeric.
To use the value of "b":
```
IF .... LOOKUP(mydevice.option, b) IS two THEN ...
```
Here we need to use IS (or ISNOT) because the value of "b" is a string.
If you are using the result of the LOOKUP function with an operator, it is advised to set a value of the type matching the operator as third parameter. 
So, if c is supposed to have a numeric value but doesn't exist, an error will occur if you do:
```
 IF ... LOOKUP(mydevice.option, c) != 0 THEN ...
```
but not if you do:
```
 IF ... LOOKUP(mydevice.option, c, 0) != 0 THEN ...
```
In genaral you should choose a default value that normally will not be returned.
