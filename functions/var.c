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
#include "var.h"


static int run(struct rules_t *obj, struct JsonNode *arguments, char **ret, enum origin_t origin) {
	struct JsonNode *childs = json_first_child(arguments);
	struct devices_t *dev = NULL;
	struct devices_settings_t *opt = NULL;
	struct protocols_t *protocol = NULL;
	char *p = *ret, *varstore = NULL, *varname = NULL, *vartype = NULL, *token1 = NULL, *token2 = NULL, *name = NULL, *value = NULL;
	char *device = NULL, *option = NULL, *varstring = NULL, *varstring_copy = NULL, *defval = "?";
	int pos = 0, has_option = 0;
	if(childs == NULL) {
		logprintf(LOG_ERR, "VAR requires at least two parameters e.g. VAR(mydevice.myoption, varname)");
		return -1;
	}
	varstore = childs->string_; //first child must be device option holding list of name/value pairs
    
	//option ophalen van device 

	varstring = MALLOC(strlen(varstore)+1);
	if(!varstring) {
	logprintf(LOG_ERR, "out of memory");
	exit(EXIT_FAILURE);
	}
	
	strcpy(varstring, varstore);
	device = strtok(varstring, ".");
	if(strlen(device) > 0) {
		option = strtok(NULL, ".");
		if(option != NULL && strlen(option) > 0) {
			if(devices_get(device, &dev) == 0) {
				if(origin == RULE) {
					event_cache_device(obj, device);
				}
				protocol = dev->protocols;
				opt = dev->settings;
				while(opt) {
					if(strcmp(opt->name, option) == 0) {
						varstring = opt->values->string_;
						has_option = 1;
					}
					opt = opt->next;
				}
				if(has_option == 0) {
					logprintf(LOG_ERR, "VAR device \"%s\" has no \"%s\" option", device, option);
					return -1;
				}
			} else {
				logprintf(LOG_ERR, "VAR device \"%s\" doesn't exist in config", device);
				return -1;
			}		
		} else {
				logprintf(LOG_ERR, "VAR first parameter must be a device option, e.g. \"mydevice.myoption\"");
				return -1;	
		}
	}
	
	childs = childs->next;
	if(childs == NULL) {
		logprintf(LOG_ERR, "VAR requires at least two parameters e.g. VAR(mydevice.myoption, varname)");
		return -1;
	}
	varname = childs->string_; //second child is name of variable to lookup
 
	childs = childs->next;
	if(childs != NULL) { 
		//third parameter is default value to be returned if variable isn't found (optional).
		defval = childs->string_;
	}		
 
	name = MALLOC(strlen(varstring)+1);
	if(!name) {
		logprintf(LOG_ERR, "out of memory");
		exit(EXIT_FAILURE);
	}
	varstring_copy = MALLOC(strlen(varstring)+1);
	if(!varstring_copy) {
		logprintf(LOG_ERR, "out of memory");
		exit(EXIT_FAILURE);
	}
	//strtok modifies the source string, so make a writable copy
	strcpy(varstring_copy, varstring); 
	token1 = strtok(varstring_copy, "&");
	if (token1 != NULL && strlen(token1) > 2) {
		pos = strcspn(token1, "=");
	}
	else {
		pos = 0;
	}
	strncpy(name, token1, pos);
	name[pos] = 0;
	
	if (strcmp(name, varname) == 0) {
		strncpy(p, token1+pos+1, strlen(token1));
		FREE(name);
		FREE(varstring_copy);
		return 0;
	}
	while(token1 = strtok(NULL, "&")) {
		if (token1 != NULL && strlen(token1) > 2) {
		pos = strcspn(token1, "=");
		}
		else {
			pos = 0;
		}
		pos = strcspn(token1, "=");
		strncpy(name, token1, pos);
		name[pos] = 0;
		
		if (strcmp(name, varname) == 0) {
			strncpy(p, token1+pos+1, strlen(token1));
			FREE(name);
			FREE(varstring_copy);
			return 0;
		}
	}
	logprintf(LOG_NOTICE, "VAR: No match");
	//return default value
	strcpy(p, defval);
	FREE(name);
	FREE(varstring_copy);
	return 0;
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void functionVarInit(void) {
	event_function_register(&function_var, "VAR");

	function_var->run = &run;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "var";
	module->version = "0.1";
	module->reqversion = "6.0";
	module->reqcommit = "94";
}

void init(void) {
	functionVarInit();
}
#endif
