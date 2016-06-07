/*
	Copyright (C) 2013 - 2015 CurlyMo

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
#include <ctype.h>

#include "../../core/threads.h"
#include "../action.h"
#include "../../core/options.h"
#include "../../config/devices.h"
#include "../../core/log.h"
#include "../../core/dso.h"
#include "../../core/pilight.h"
#include "../../core/http.h"
#include "file.h"

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

	struct JsonNode *jto = NULL;
	struct JsonNode *jtext = NULL;
	struct JsonNode *jmode = NULL;
	struct JsonNode *jvalues = NULL;
	struct JsonNode *jval = NULL;

	struct JsonNode *jchild = NULL;
	int nrvalues = 0;

	jto = json_find_member(obj->parsedargs, "TO");
	jtext = json_find_member(obj->parsedargs, "TEXT");
	jmode = json_find_member(obj->parsedargs, "MODE");

	if(jto == NULL) {
		logprintf(LOG_ERR, "file action is missing a \"TO\"");
		return -1;
	}
	if(jmode == NULL) {
		logprintf(LOG_ERR, "file action is missing a \"MODE\"");
		return -1;
	}
	if(jtext == NULL) {
		logprintf(LOG_ERR, "file action is missing a \"TEXT\"");
		return -1;
	}
	
	nrvalues = 0;
	if((jvalues = json_find_member(jto, "value")) != NULL) {
		jchild = json_first_child(jvalues);
		while(jchild) {
			nrvalues++;
			jchild = jchild->next;
		}
	}
	if(nrvalues != 1) {
		logprintf(LOG_ERR, "file action \"TO\" only takes one argument");
		return -1;
	}

	nrvalues = 0;
	if((jvalues = json_find_member(jmode, "value")) != NULL) {
		jchild = json_first_child(jvalues);
		while(jchild) {
			nrvalues++;
			jchild = jchild->next;
		}
	}
	if(nrvalues != 1) {
		logprintf(LOG_ERR, "file action \"MODE\" only takes one argument");
		return -1;
	}
	jval = json_find_element(jvalues, 0);
		
	if(jval->tag != JSON_STRING || ((strcmp(jval->string_, "new") != 0) && (strcmp(jval->string_, "append") != 0))) {
		logprintf(LOG_ERR, "file action \"MODE\" argument must be either \"new\" or \"append\"");
		return -1;
	}

	nrvalues = 0;
	if((jvalues = json_find_member(jtext, "value")) != NULL) {
		jchild = json_first_child(jvalues);
		while(jchild) {
			nrvalues++;
			jchild = jchild->next;
		}
	}
	if(nrvalues != 1) {
		logprintf(LOG_ERR, "file action \"TEXT\" only takes one argument");
		return -1;
	}
	return 0;
}

static void *thread(void *param) {
	struct rules_actions_t *pth = (struct rules_actions_t *)param;
	// struct rules_t *obj = pth->obj;
	struct JsonNode *arguments = pth->parsedargs;
	struct JsonNode *jto = NULL;
	struct JsonNode *jmode = NULL;
	struct JsonNode *jtext = NULL;
	struct JsonNode *jvalues1 = NULL;
	struct JsonNode *jvalues2 = NULL;
	struct JsonNode *jvalues3 = NULL;
	struct JsonNode *jval1 = NULL;
	struct JsonNode *jval2 = NULL;
	struct JsonNode *jval3 = NULL;
	char *mode = "a", *file = NULL;
	FILE * fp;
	action_file->nrthreads++;

	jto = json_find_member(arguments, "TO");
	jmode = json_find_member(arguments, "MODE");
	jtext = json_find_member(arguments, "TEXT");

	if(jto != NULL && jtext != NULL && jmode != NULL) {
		jvalues1 = json_find_member(jto, "value");
		jvalues2 = json_find_member(jmode, "value");
		jvalues3 = json_find_member(jtext, "value");
		if(jvalues1 != NULL && jvalues2 != NULL && jvalues3 != NULL) {
			jval1 = json_find_element(jvalues1, 0);
			jval2 = json_find_element(jvalues2, 0);
			jval3 = json_find_element(jvalues3, 0);
			if(jval1 != NULL && jval2 != NULL  && jval3 != NULL &&
			 jval1->tag == JSON_STRING && jval2->tag == JSON_STRING && jval3->tag == JSON_STRING) {
				file = MALLOC(strlen(jval1->string_) + 1);
				if(!file) {
					logprintf(LOG_ERR, "out of memory");
					exit(EXIT_FAILURE);
				}
				//remove spaces from file name
				file = strip(jval1->string_);

				mode = MALLOC(2);
				if(!mode) {
					logprintf(LOG_ERR, "out of memory");
					exit(EXIT_FAILURE);
				}
		 
				if(strcmp(jval2->string_, "new") == 0) {
					strcpy(mode, "w");
				} else if(strcmp(jval2->string_, "append") == 0) {
					strcpy(mode, "a");						 
				} else {
				logprintf(LOG_NOTICE, "file action \"MODE\" must be either \"new\" or \"append\"");
				}
				if(mode != NULL && (fp = fopen(file, mode)) != NULL){
					if(fprintf(fp, "%s%s", jval3->string_,"\n") > 0) {
						logprintf(LOG_DEBUG, "file action to file \"%s\" with text \"%s\" succeeded", file, jval3->string_);
					}
					fclose(fp);
				} else {
					logprintf(LOG_NOTICE, "file action couldn't open \"%s\" with mode \"%s\"", jval1->string_, jval2->string_);			
				}
				FREE(file);
				FREE(mode);
			} else {
				logprintf(LOG_NOTICE, "file action couldn't operate due to missing or bad parameters");
			}
		}
	}

	action_file->nrthreads--;

	return (void *)NULL;
}

static int run(struct rules_actions_t *obj) {
	pthread_t pth;
	threads_create(&pth, NULL, thread, (void *)obj);
	pthread_detach(pth);
	return 0;
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void actionFileInit(void) {
	event_action_register(&action_file, "file");

	options_add(&action_file->options, 'a', "TO", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);
	options_add(&action_file->options, 'b', "MODE", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);
	options_add(&action_file->options, 'c', "TEXT", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);

	action_file->run = &run;
	action_file->checkArguments = &checkArguments;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "file";
	module->version = "0.1";
	module->reqversion = "6.0";
	module->reqcommit = "87";
}

void init(void) {
	actionFileInit();
}
#endif
