/*
	Copyright (C) 2013 - 2016 CurlyMo & Niek

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "../action.h"
#include "../events.h"
#include "../../core/options.h"
#include "../../config/devices.h"
#include "../../core/log.h"
#include "../../core/dso.h"
#include "../../core/pilight.h"
#include "count.h"


static int checkArguments(struct rules_actions_t *obj) {
	struct JsonNode *jdevice = NULL;
	struct JsonNode *jup = NULL;
	struct JsonNode *jdown = NULL;
	struct JsonNode *jto = NULL;
	struct JsonNode *javalues = NULL;
	struct JsonNode *jbvalues = NULL;
	struct JsonNode *jcvalues = NULL;
	struct JsonNode *jachild = NULL;
	struct JsonNode *jbchild = NULL;
	struct JsonNode *jcchild = NULL;
	double nr1 = 0.0, nr2 = 0.0, nr3 = 0.0, nr4 = 0.0;
	int nrvalues = 0;

	jdevice = json_find_member(obj->parsedargs, "DEVICE");
	jup = json_find_member(obj->parsedargs, "UP");
	jdown = json_find_member(obj->parsedargs, "DOWN");
	jto = json_find_member(obj->parsedargs, "TO");

	if(jdevice == NULL) {
		logprintf(LOG_ERR, "count action is missing a \"DEVICE\"");
		return -1;
	}

	json_find_number(jdevice, "order", &nr1);

	if(jup != NULL) {
		json_find_number(jup, "order", &nr2);
		nrvalues = 0;
		if((javalues = json_find_member(jup, "value")) != NULL) {
			jachild = json_first_child(javalues);
			while(jachild) {
				nrvalues++;
				if(jachild->tag != JSON_NUMBER || jachild->number_ < 1) {
				logprintf(LOG_ERR, "count action \"UP\" requires a positive number");
				return -1;
			}	
				jachild = jachild->next;
			}
		}
		if((int)nrvalues != 1) {
			logprintf(LOG_ERR, "count action \"UP\" only takes one argument");
			return -1;
		}
	
	}
	if(jdown != NULL) {
		json_find_number(jdown, "order", &nr3);
		nrvalues = 0;
		if((jbvalues = json_find_member(jdown, "value")) != NULL) {
			jbchild = json_first_child(jbvalues);
			while(jbchild) {
				nrvalues++;
				if(jbchild->tag != JSON_NUMBER || jbchild->number_ < 1) {
				logprintf(LOG_ERR, "count action \"DOWN\" requires a positive number");
				return -1;
			}	
				jbchild = jbchild->next;
			}
		}
		if((int)nrvalues != 1) {
			logprintf(LOG_ERR, "count action \"DOWN\" only takes one argument");
			return -1;
		}
	}

	if(jto != NULL) {
		json_find_number(jto, "order", &nr4);
		nrvalues = 0;
		if((jcvalues = json_find_member(jto, "value")) != NULL) {
			jcchild = json_first_child(jcvalues);
			while(jcchild) {
				nrvalues++;
			if(jcchild->tag != JSON_NUMBER) {
				logprintf(LOG_ERR, "count action \"TO\" requires a number");
				return -1;
			}	
				jcchild = jcchild->next;
			}
		}
		if((int)nrvalues != 1) {
			logprintf(LOG_ERR, "count action \"TO\" only takes one argument");
			return -1;
		}
	}

	if(nr2 == 0 && nr3 == 0 && nr4 == 0) {
		logprintf(LOG_ERR, "count action requires two arguments");
		return -1;
	}
	if(nr2 > 0 && nr2 != 2) {
			logprintf(LOG_ERR, "count actions are formatted as \"count DEVICE ... UP ...\"");
			return -1;
	}
	if(nr3 > 0 && nr3 != 2) {
			logprintf(LOG_ERR, "count actions are formatted as \"count DEVICE ... DOWN ...\"");
			return -1;
	}
	if(nr4 > 0 && nr4 != 2) {
			logprintf(LOG_ERR, "count actions are formatted as \"count DEVICE ... TO ...\"");
			return -1;
	}

	if((jbvalues = json_find_member(jdevice, "value")) != NULL) {
		jbchild = json_first_child(jbvalues);
		struct devices_t *dev = NULL;
		if(devices_get(jbchild->string_, &dev) == 0) {
			struct protocols_t *protocols = dev->protocols;
			int match = 0;
			while(protocols) {
				if(strcmp(protocols->name, "generic_counter") == 0) {
					match = 1;
					break;
				}
				protocols = protocols->next;
			}
			if(match == 0) {
				logprintf(LOG_ERR, "the count action only works with the generic_counter device");
				return -1;
			}
		} else {
			logprintf(LOG_ERR, "device \"%s\" doesn't exist", jbchild->string_);
			return -1;
		}
	} else {
	
		return -1;
	}
	return 0;
}

static void *thread(void *param) {
	struct event_action_thread_t *pth = (struct event_action_thread_t *)param;
	struct JsonNode *json = pth->obj->parsedargs;
	struct JsonNode *jup = NULL;
	struct JsonNode *jdown = NULL;
	struct JsonNode *jto = NULL;
	struct JsonNode *jtemp = NULL;
	struct JsonNode *javalues = NULL;

	struct JsonNode *jvalues = NULL;
	int count = 0;

	event_action_started(pth);

	/* Get current counter value*/
	struct devices_t *tmp = pth->device;
	int match1 = 0;
	while(tmp) {
		struct devices_settings_t *opt = tmp->settings;
		while(opt) {
			if(strcmp(opt->name, "count") == 0) {
				if(opt->values->type == JSON_NUMBER) {
				count = opt->values->number_;
				match1 = 1;
				}
			}
			opt = opt->next;
		}
		if(match1 == 1) {
			break;
		}
		tmp = tmp->next;
	}
	if(match1 == 0) {
		logprintf(LOG_NOTICE, "could not retrieve count of \"%s\"", pth->device->id);
	}
	if ((jtemp = json_find_member(json, "UP")) != NULL) {
		if((jtemp = json_find_member(jtemp, "value")) != NULL) {
			jtemp = json_first_child(jtemp);
			if(jtemp->tag == JSON_NUMBER) {
				count = count + jtemp->number_;
			}
		}
	}
	else if ((jtemp = json_find_member(json, "DOWN")) != NULL) {
		if((jtemp = json_find_member(jtemp, "value")) != NULL) {
			jtemp = json_first_child(jtemp);
			if(jtemp->tag == JSON_NUMBER) {
				count = count - jtemp->number_;
			}
		}
	}
	else if ((jtemp = json_find_member(json, "TO")) != NULL) {
		if((jtemp = json_find_member(jtemp, "value")) != NULL) {
			jtemp = json_first_child(jtemp);
			if(jtemp->tag == JSON_NUMBER) {
				count = jtemp->number_;
			}
		}
	}

	
	logprintf(LOG_DEBUG, ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> %d", count);

	if(pilight.control != NULL) {
		jvalues = json_mkobject();
		json_append_member(jvalues, "count", json_mknumber(count,0));
		pilight.control(pth->device, NULL, json_first_child(jvalues), ACTION);
		json_delete(jvalues);
		
	}

	event_action_stopped(pth);

	return (void *)NULL;
}

static int run(struct rules_actions_t *obj) {
	struct JsonNode *jdevice = NULL;
	struct JsonNode *jtemp = NULL;
	struct JsonNode *jbvalues = NULL;
	struct JsonNode *jbchild = NULL;

	if((jdevice = json_find_member(obj->parsedargs, "DEVICE")) != NULL && (
		(jtemp = json_find_member(obj->parsedargs, "UP")) != NULL ||
		(jtemp = json_find_member(obj->parsedargs, "DOWN")) != NULL ||
		(jtemp = json_find_member(obj->parsedargs, "TO")) != NULL )) {
	
			if((jbvalues = json_find_member(jdevice, "value")) != NULL) {
			jbchild = json_first_child(jbvalues);
			while(jbchild) {
				if(jbchild->tag == JSON_STRING) {
					struct devices_t *dev = NULL;
					if(devices_get(jbchild->string_, &dev) == 0) {
						event_action_thread_start(dev, action_count->name, thread, obj);
					}
				}
				jbchild = jbchild->next;
			}
		}
	}
	return 0;
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void actionCountInit(void) {
	event_action_register(&action_count, "count");

	options_add(&action_count->options, 'a', "DEVICE", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);
	options_add(&action_count->options, 'b', "UP", OPTION_OPT_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, NULL);
	options_add(&action_count->options, 'c', "DOWN", OPTION_OPT_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, NULL);
	options_add(&action_count->options, 'd', "TO", OPTION_OPT_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, NULL);

	action_count->run = &run;
	action_count->checkArguments = &checkArguments;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "count";
	module->version = "0.1";
	module->reqversion = "6.0";
	module->reqcommit = "152";
}

void init(void) {
	actionCountInit();
}
#endif