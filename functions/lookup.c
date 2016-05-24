/*
	Copyright (C) 2016 CurlyMo & Niek

	This file is part of pilight.
	pilight is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License as published by the Free Software
	Foundation, either version 3 of the License, or (at your option) any later
	version.

	pilight is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with pilight. If not, see	<http://www.gnu.org/licenses/>
*/
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#ifndef __USE_XOPEN
	#define __USE_XOPEN
#endif

#include "../function.h"
#include "../events.h"
#include "../../config/devices.h"
#include "../../core/options.h"
#include "../../core/log.h"
#include "../../core/dso.h"
#include "../../core/pilight.h"
#include "../../core/common.h"
#include "lookup.h"

static void trm(char **c) {
	char *tmp = *c;
	int i  = 0;
	while(isspace(tmp[i])) {
		i++;
	}
	tmp = tmp + i;
	i = strlen(tmp) - 1;
	while(isspace(tmp[i])) {
		i--;
	}
	tmp[i+1] = '\0';
	strcpy(*c, tmp);
	return;
}

static int run(struct rules_t *obj, struct JsonNode *arguments, char **ret, enum origin_t origin) {
	struct JsonNode *childs = json_first_child(arguments);
	struct devices_t *dev = NULL;
	struct devices_settings_t *opt = NULL;
	char *p = *ret, *haystack = NULL, *haystack_copy = NULL, *needle = NULL, *token1 = NULL, *key = NULL;
	char *device = NULL, *option = NULL, *param1 = NULL, *param2 = NULL, *defval = NULL;
	int pos = 0, has_option = 0, match = 0;
	

	

	
	if(childs == NULL) {
		logprintf(LOG_ERR, "LOOKUP requires at least two parameters e.g. LOOKUP(mydevice.myoption, needle)");
		return -1;
	}
	
	//first parameter must be a string, or a device option containing name=value pairs(haystack)
	if(!isNumeric(childs->string_)) {
		logprintf(LOG_ERR, "LOOKUP first parameter must be a string");
		return -1;
	}
	param1 = MALLOC(strlen(childs->string_)+1);
	if(!param1) {
		logprintf(LOG_ERR, "out of memory");
		exit(EXIT_FAILURE);
	}
	
	strcpy(param1, childs->string_);
	device = strtok(param1, ".");
	if(strlen(device) > 0) {
		//possibly device option
		option = strtok(NULL, ".");
		if(option != NULL && strlen(option) > 0) {
			if(devices_get(device, &dev) == 0) {
				if(origin == RULE) {
					event_cache_device(obj, device);
				}
				opt = dev->settings;
				while(opt) {
					if(strcmp(opt->name, option) == 0) {
						if(opt->values->type != JSON_STRING) {
							logprintf(LOG_ERR, "LOOKUP option \"option\" of device \"%s\" isn't a string", option, device);
							return -1;
						}
						haystack = opt->values->string_; //use option value
						has_option = 1;

					}
					opt = opt->next;
				}
			} 
		} 
	}
	if(has_option == 0) {
		haystack = childs->string_; //no option value found, restore haystack to original
	}

	childs = childs->next;
	if(childs == NULL) {
		logprintf(LOG_ERR, "LOOKUP requires at least two parameters e.g. LOOKUP(mydevice.myoption, needle)");
		return -1;
	}

	//second parameter is name of variable or device value to lookup (needle)
	has_option = 0;
	param2 = MALLOC(strlen(childs->string_)+1);
	if(!param2) {
		logprintf(LOG_ERR, "out of memory");
		exit(EXIT_FAILURE);
	}

	strcpy(param2, childs->string_);

	device = strtok(param2, ".");
	if(strlen(device) > 0) {
		//possibly device option
		option = strtok(NULL, ".");
		if(option != NULL && strlen(option) > 0) {
			if(devices_get(device, &dev) == 0) {
				if(origin == RULE) {
					event_cache_device(obj, device);
				}
				opt = dev->settings;
				while(opt) {
					if(strcmp(opt->name, option) == 0) {
						needle = opt->values->string_; //use option value
						has_option = 1;
					}
					opt = opt->next;
				}
			}
		}
	}
	if(has_option == 0) {
		needle = childs->string_; //no option value found, restore needle to original
	}
	trm(&needle);
	
	defval = MALLOC(BUFFER_SIZE);
	if(!defval) {
		logprintf(LOG_ERR, "out of memory");
		exit(EXIT_FAILURE);
	}
	strcpy(defval, "?");

	childs = childs->next;
	if(childs != NULL) {
		//third parameter is default value to be returned if variable isn't found (optional).
		if(strcmp(childs->string_, "*") == 0) {
			strcpy(defval, needle);
		} else {
			if (strcmp(childs->string_, "$") == 0) {			
				strcpy(defval, haystack);
			} else {
				strcpy(defval, childs->string_);
			}
		}
	}

	//strtok modifies the source string (haystack), so make a copy
	haystack_copy = MALLOC(strlen(haystack)+1);
	if(!haystack_copy) {
		logprintf(LOG_ERR, "out of memory");
		exit(EXIT_FAILURE);
	}

	strcpy(haystack_copy, haystack); 

	token1 = strtok(haystack_copy, "&");
	if (token1 != NULL && strlen(token1) > 2) {
		pos = strcspn(token1, "=");
	}
	else {
		pos = 0;
	}

	key = MALLOC(strlen(haystack)+1);
	if(!key) {
		logprintf(LOG_ERR, "out of memory");
		exit(EXIT_FAILURE);
	}
	strncpy(key, token1, pos);
	key[pos] = '\0';
	trm(&key);


	match = 0;
	if (strcmp(key, needle) == 0) {
		strncpy(p, token1+pos+1, strlen(token1));
		trm(&p);
		match = 1;

	} else {
		while(token1 = strtok(NULL, "&")) {
			if (token1 != NULL && strlen(token1) > 2) {
				pos = strcspn(token1, "=");
			}
			else {
				pos = 0;
			}
			strncpy(key, token1, pos);
			key[pos] = '\0';
			trm(&key);

			if (strcmp(key, needle) == 0) {
				strncpy(p, token1+pos+1, strlen(token1));
				trm(&p);
				match = 1;
				break;
			}
		}
	}
	if(match == 0) {
		logprintf(LOG_NOTICE, "LOOKUP: No match for \"%s\", returning \"%s\"", needle, defval);
		//return default value
		strcpy(p, defval);
	}

	FREE(param1);
	FREE(param2);
	FREE(haystack_copy);
	FREE(key);
	FREE(defval);
	return 0;
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void functionLookupInit(void) {
	event_function_register(&function_lookup, "LOOKUP");

	function_lookup->run = &run;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "lookup";
	module->version = "0.1";
	module->reqversion = "6.0";
	module->reqcommit = "94";
}

void init(void) {
	functionLookupInit();
}
#endif
