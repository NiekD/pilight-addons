# VAR function

The VAR function can be used together with the webswitch protocol (see the protocols folder), to get a named value from the "response" option if it is in query string format (name=value pairs separated by "&"). The type of the value returned by the VAR function can be either a number, or a string, depending on the value.

Example:

Let's assume that the "response" of our webswitch device called "mywebswitch" is "a=1&b=two"
To use the value of "a" in a rule we can do:
```
IF ... VAR(mywebswitch.response, a) == 1 THEN ...
```
 
We need to use ==, because the value of "a" is numeric.

To use the value of "b":

```
IF .... VAR(mywebswitch.response, b) IS two THEN ...
```

Here we need to use IS (or ISNOT) because the value of "b" is a string.

The VAR function takes an optional third parameter that will at as default if the name searched for doesn't exist or is not part of a name=value pair. Otherwise the string "?" will be returned which will cause an error when used with an arithmetic operator!

So, if c is supposed to have a numerc value but doesn't exist, an error will occur if you do:
```
 IF ... VAR(mywebswitch.response, c) != 0 THEN ...
```

but not if you do:
```
 IF ... VAR(mywebswitch.response, c, 0) != 0 THEN ...
```
Of course you should choose a default value that normally will not be retuned.
