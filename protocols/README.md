# webswitch protocol

The webswitch protocol is similar to the program protocol, but instead of starting and stopping a local program, it performs a HTTP GET or POST request. The config of this protocol looks like this:

``` 
              "bewatering": {
                        "protocol": [ "webswitch" ],
                        "id": [{
                                "id": 1
                        }],
                        "method": "GET",
                        "on_uri": "http://192.168.1.13/gardena.php",
                        "off_uri": "http://192.168.1.13/gardena.php",
                        "on_query": "c=on",
                        "off_query": "c=off",
                        "on_success": "status=normaal&bewatering=aan",
                        "off_success": "status=normaal&bewatering=uit",
                        "err_response": "status=httperror",
                        "response": "bewatering=uit&status=normaal&bodem=nat&flow=uit",
                        "state": "stopped"
                },

```
*All options shown above are mandatory, except "err_response".*

The webswitch is a pending switch. It can be in one of three states: "Stopped", "Pending" and "Running" (same as program protocol). If the webswitch is activated when it is in the "Stopped" state it goes to the "Pending" state and an HTTP(S) request is sent to the URI defined as "on_uri" together with the query string defined as "on_query". The request method used is determined by the "method" option and must be either "GET" ot "POST". The webswitch remains in the Pending state until a response is received or a time-out occurs. 

If the response received matches the "on_succes" string, the switch moves to the "Running" state, otherwise it reverts to the "Stopped" state.

If the connection fails (returncode is not 200),  "response" is set to "\*CONNECTION FAILED\*". This default string can be replaced by a string of your own choice by adding the optional "err_response" option to the webswitch config. 

If the response is in the form of a query string (one or more name=value pairs separated by "&", like in the config example shown above), you can let the webswitch check one or more of the values in it, otherwise it will check the whole string. This is done by entering name=value pairs for "on_success" and "off_success" as shown in the config example above. Only if all these name=value pairs are present in the result (the order is irrelevant) the webswitch goes to the new state.
If you want to use one or more of the values in your rules, you can use the VAR function to select them by name.

Switching from "Running" to "Stopped" works the same way using the corresponding uri -query and success values.

# wunderful protocol

The wunderful protocol is an extended version of the wunderground protocol. It provides a lot of data of the current weather, the forecast for the next hour and weather alerts. Just like the wunderground protocol, you need an api-key to let the protocol get access to the weather underground api.

```
                "ws": {
                        "protocol": [ "wunderful" ],
                        "id": [{
                                "api": "yourapikey",
                                "location": "heemstede",
                                "country": "nl"
                        }],
                        "weather": "Partly Cloudy",
                        "weatherfc": "Mostly Sunny",
                        "humidity": 55,
                        "temperature": 16,
                        "bft": 3,
                        "gustbft": 0,
                        "degrees": 190,
                        "dir": "S",
                        "preciphour": 0,
                        "precipday": 0,
                        "pressure": 1008,
                        "rain": 0,
                        "city": "Heemstede",
                        "stationid": "EHAM",
                        "fcttime": 17.00,
                        "obstime": 15.55,
                        "dirfc": "W",
                        "tempfc": 14,
                        "bftfc": 2,
                        "degreesfc": 279,
                        "rainfc": 0,
                        "precipfc": 0,
                        "pressurefc": 1007,
                        "popfc": 0,
                        "humidityfc": 63,
                        "alarmtext": "None",
                        "leveltext": "None",
                        "alarmlevel": 0,
                        "alarmtype": 0,
                        "alarmstart": 18.00,
                        "alarmend": 19.59,
                        "update": 0,
                        "poll-interval": 300
                },

```

Options with names ending with "fc" contain forecast data.

# generic_counter protocol
The generic_counter protocol is a simple protocol to be used by rules to count events. It can be manipulated with the count action. The config of this protocol looks like this:

```
                "counter1": {
                        "protocol": [ "generic_counter" ],
                        "id": [{
                                "id": 1
                        }],
                        "count": 1
                },
```

