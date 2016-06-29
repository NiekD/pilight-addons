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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../../core/threads.h"
#include "../action.h"
#include "../../core/options.h"
#include "../../config/devices.h"
#include "../../core/log.h"
#include "../../core/dso.h"
#include "../../core/pilight.h"
#include "../../core/http.h"
#include "../../core/common.h"
#include "http.h"

static char* strip(char* input) {
	int loop;
	char *output = (char*) malloc (strlen(input) + 1);
	char *dest = output;
	
	if (output)
	{
	for (loop=0; loop<strlen(input); loop++)
		if (input[loop] != ' ')
		*dest++ = input[loop];
		*dest = '\0';
	}
	return output;
}

static int checkArguments(struct rules_actions_t *obj) {
	struct JsonNode *jget = NULL;
	struct JsonNode *jpost = NULL;
	struct JsonNode *jparam = NULL;
	struct JsonNode *jresult = NULL;
	struct JsonNode *jvalues = NULL;
	struct JsonNode *jchild = NULL;
	double nr1 = 0.0, nr2 = 0.0, nr3 = 0.0, nr4 = 0.0;
	int nrvalues = 0;

	jpost = json_find_member(obj->parsedargs, "POST");
	jget = json_find_member(obj->parsedargs, "GET");
	jparam = json_find_member(obj->parsedargs, "PARAM");
	jresult = json_find_member(obj->parsedargs, "RESULT");

	if(jpost == NULL && jget == NULL) {
		logprintf(LOG_ERR, "http action is missing a \"GET\" or \"POST\"");
		return -1;
	}
	if(jpost != NULL && jget != NULL) {
		logprintf(LOG_ERR, "http action must contain either a \"GET\" or a \"POST\"");
		return -1;
	}	
	nrvalues = 0;
	if(jpost != NULL) {
		json_find_number(jpost, "order", &nr1);
		if((jvalues = json_find_member(jpost, "value")) != NULL) {
	
			jchild = json_first_child(jvalues);
			while(jchild) {
				nrvalues++;
				jchild = jchild->next;
			}
		}
		if(nrvalues != 1) {
			logprintf(LOG_ERR, "http action \"POST\" only takes one argument");
			return -1;
		}
	}
	nrvalues = 0;
	if(jget != NULL) {
		json_find_number(jget, "order", &nr2);
		if((jvalues = json_find_member(jget, "value")) != NULL) {
	
			jchild = json_first_child(jvalues);
			while(jchild) {
				nrvalues++;
				jchild = jchild->next;
			}
		}
		if(nrvalues != 1) {
			logprintf(LOG_ERR, "http action \"GET\" only takes one argument");
			return -1;
		}	
	}
	nrvalues = 0;
	if(jparam != NULL) {
		json_find_number(jparam, "order", &nr3);
		if((jvalues = json_find_member(jparam, "value")) != NULL) {
			jchild = json_first_child(jvalues);
			while(jchild) {
				nrvalues++;
				jchild = jchild->next;
			}
		}
		if(nrvalues != 1) {
			logprintf(LOG_ERR, "http action \"PARAM\" only takes one argument");
			return -1;
		}	
	}
	nrvalues = 0;
	if(jresult != NULL) {
		json_find_number(jresult, "order", &nr4);
		if((jvalues = json_find_member(jresult, "value")) != NULL) {
			jchild = json_first_child(jvalues);
			while(jchild) {
				nrvalues++;
				jchild = jchild->next;
			}
			if(nrvalues != 1) {
				logprintf(LOG_ERR, "http action \"RESULT\" only takes one argument");
				return -1;
			}
			jchild = json_first_child(jvalues);
				
			struct devices_t *dev = NULL;
			if(devices_get(jchild->string_, &dev) == 0) {
				struct protocols_t *protocols = dev->protocols;
				int match = 0;
				while(protocols) {
					if(strcmp(protocols->name, "generic_label") == 0) {
						match = 1;
						break;
					}
					protocols = protocols->next;
				}
				if(match == 0) {
					logprintf(LOG_ERR, "the http action only works with the generic_label device");
					return -1;
				}
				} else {
				logprintf(LOG_ERR, "http action: device \"%s\" doesn't exist", jchild->string_);
				return -1;
			}			
	
		}
		if(nr1 == 1 && nr3 > 0 && nr4 > 0 && (nr3 != 2 || nr4 != 3)) {
			logprintf(LOG_ERR, "http actions are formatted as \"http POST ... PARAM ... RESULT ...\"");
			return -1;
		}
		if(nr1 == 1 && nr3 > 0 && nr3 != 2 && nr4 == 0) {
			logprintf(LOG_ERR, "http actions are formatted as \"http POST ... PARAM ...\"");
			return -1;
		}
		if(nr1 == 1 && nr4 > 0 && nr4 != 2 && nr3 == 0) {
			logprintf(LOG_ERR, "http actions are formatted as \"http POST ... RESULT ...\"");
			return -1;
		}
		if(nr2 == 1 && nr3 > 0 && nr4 > 0 && (nr3 != 2 || nr4 != 3)) {
			logprintf(LOG_ERR, "http actions are formatted as \"http GET ... PARAM ... RESULT ...\"");
			return -1;
		}
		if(nr2 == 1 && nr3 > 0 && nr3 != 2 && nr4 == 0) {
			logprintf(LOG_ERR, "http actions are formatted as \"http GET ... PARAM ...\"");
			return -1;
		}
		if(nr2 == 1 && nr4 > 0 && nr4 != 2 && nr3 == 0) {
			logprintf(LOG_ERR, "http actions are formatted as \"http GET ... RESULT ...\"");
			return -1;
		}
	} 
	return 0;
}

static void *thread(void *param) {
	struct event_action_thread_t *pth = (struct event_action_thread_t *)param;
	struct JsonNode *arguments = pth->obj->parsedargs;
	struct JsonNode *jpost = NULL;
	struct JsonNode *jget = NULL;
	struct JsonNode *jparam = NULL;
	struct JsonNode *jresult = NULL;
	struct JsonNode *jvalues1 = NULL;
	struct JsonNode *jvalues2 = NULL;
	struct JsonNode *jvalues3 = NULL;
	struct JsonNode *jvalues4 = NULL;
	struct JsonNode *jobj = NULL;
	struct JsonNode *jval1 = NULL;
	struct JsonNode *jval2 = NULL;
	struct JsonNode *jval3 = NULL;
	struct JsonNode *jval4 = NULL;

	char url[1024], typebuf[70];
	char *data = NULL, *parameters = NULL, *result = NULL, *tp = typebuf, *response = NULL;
	int ret = 0, size = 0, free_parameters = 0, free_result = 0;

	event_action_started(pth);

	jpost = json_find_member(arguments, "POST");
	jget = json_find_member(arguments, "GET");
	jparam = json_find_member(arguments, "PARAM");
	jresult = json_find_member(arguments, "RESULT");

	if(jpost != NULL) {
		if((jvalues1 = json_find_member(jpost, "value")) != NULL) {
			jval1 = json_find_element(jvalues1, 0);
		}
	}
	if(jget != NULL) {
		if((jvalues2 = json_find_member(jget, "value")) != NULL) {
			jval2 = json_find_element(jvalues2, 0);
		}
	}
	if(jparam != NULL) {
		if((jvalues3 = json_find_member(jparam, "value")) != NULL) {
			jval3 = json_find_element(jvalues3, 0);
		}
		if(jval3 != NULL && jval3->tag == JSON_STRING) {
			parameters = MALLOC(strlen(jval3->string_) + 1);
			if(!parameters) {
				fprintf(stderr, "out of memory\n");
				exit(EXIT_FAILURE);
			}
	//		strcpy(parameters, jval3->string_); 
			parameters = strip(jval3->string_);
			free_parameters = 1;
		}
	}
	if(jresult != NULL) {
		if((jvalues4 = json_find_member(jresult, "value")) != NULL){
			jval4 = json_find_element(jvalues4, 0);
		}
		if(jval4 != NULL && jval4->tag == JSON_STRING) {
			result = MALLOC(strlen(jval4->string_) + 1);
			if(!result) {
				fprintf(stderr, "out of memory\n");
				exit(EXIT_FAILURE);
			}
			strcpy(result, jval4->string_); 
			free_result = 1;
		}
	}
	response = MALLOC(1025);
	if(!response) {
		fprintf(stderr, "out of memory\n");
		exit(EXIT_FAILURE);
	}
	data = NULL;

	if(jval1 != NULL && jval1->tag == JSON_STRING) { //POST REQUEST
		if(parameters != NULL) {
			data = http_post_content(strip(jval1->string_), &tp, &ret, &size, "application/x-www-form-urlencoded", parameters);
		} else {
			data = http_post_content(strip(jval1->string_), &tp, &ret, &size, "application/x-www-form-urlencoded", parameters);
		}
	} else {
		if(jval2 != NULL && jval2->tag == JSON_STRING) { //GET REQUEST
			if(parameters != NULL) {
				sprintf(url, "%s?%s", strip(jval2->string_), parameters); 
			} else {
				strcpy(url, strip(jval2->string_)); 
			}
			data = http_get_content(url, &tp, &ret, &size);
		} 
	}
	logprintf(LOG_DEBUG, "http action calling %s", url);
	
	if(pilight.control != NULL && result != 0) {
		jobj = json_mkobject();
		if(ret == 200) {
			logprintf(LOG_DEBUG, "http action succeeded, received %s", data);
			if(size > 290) {
				logprintf(LOG_NOTICE, "http action response size %i is too big (limit is 290), response truncated", size);
				data[291] = '\0';
				logprintf(LOG_DEBUG, "truncated response: %s", data);
			}
			strcpy(response, data);
		} else {
			logprintf(LOG_NOTICE, "http action request to %s failed (error %d)", url, ret);
			sprintf(response, "HTTP ERROR %i", ret);
		}				
		json_append_member(jobj, "label", json_mkstring(response));
		pilight.control(pth->device, NULL, json_first_child(jobj), ACTION);
		json_delete(jobj);
	}

		
	
	if(response != NULL)
		FREE(response);
	if(data != NULL) {
		FREE(data);
	}
	if(free_parameters == 1) {
		FREE(parameters);
	}	
	if(free_result == 1) {
		FREE(result);
	}
	event_action_stopped(pth);
	return (void *)NULL;
}

static int run(struct rules_actions_t *obj) {
	struct JsonNode *jresult = NULL;
	struct JsonNode *jbvalues = NULL;
	struct JsonNode *jbchild = NULL;

	if((jresult = json_find_member(obj->parsedargs, "RESULT")) != NULL ) {
	
			if((jbvalues = json_find_member(jresult, "value")) != NULL) {
			jbchild = json_first_child(jbvalues);
			while(jbchild) {
				if(jbchild->tag == JSON_STRING) {
					struct devices_t *dev = NULL;
					if(devices_get(jbchild->string_, &dev) == 0) {
						event_action_thread_start(dev, action_http->name, thread, obj);
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
void actionHttpInit(void) {
	event_action_register(&action_http, "http");

	options_add(&action_http->options, 'a', "POST", OPTION_OPT_VALUE, DEVICES_OPTIONAL, JSON_STRING, NULL, NULL);
	options_add(&action_http->options, 'b', "GET", OPTION_OPT_VALUE, DEVICES_OPTIONAL, JSON_STRING, NULL, NULL);
	options_add(&action_http->options, 'c', "PARAM", OPTION_OPT_VALUE, DEVICES_OPTIONAL, JSON_STRING, NULL, NULL);
	options_add(&action_http->options, 'd', "RESULT", OPTION_OPT_VALUE, DEVICES_OPTIONAL, JSON_STRING, NULL, NULL);

	action_http->run = &run;
	action_http->checkArguments = &checkArguments;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "http";
	module->version = "0.1";
	module->reqversion = "5.0";
	module->reqcommit = "87";
}

void init(void) {
	actionHttpInit();
}
#endif
