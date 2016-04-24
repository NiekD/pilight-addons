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
#include <errno.h>
#include <fcntl.h>
#include <math.h>

#define DFLT_ERR_RSP "*CONNECTION FAILED*"

#ifndef _WIN32
	#ifdef __mips__
		#define __USE_UNIX98
	#endif
	#include <sys/wait.h>
#endif
#include <pthread.h>

#include "../../core/threads.h"
#include "../../core/pilight.h"
#include "../../core/common.h"
#include "../../core/dso.h"
#include "../../core/log.h"
#include "../protocol.h"
#include "../../core/binary.h"
#include "../../core/json.h"
#include "../../core/gc.h"
#include "../../core/http.h"
#include "webswitch.h"

#ifndef _WIN32
static int free_successcode = 0;
static int free_contype = 0;
static int free_post = 0;
static unsigned short loop = 1;
static unsigned short threads = 0;

static pthread_mutex_t lock;
static pthread_mutexattr_t attr;

typedef struct settings_t {
	double id;
	char *method;
	char *on_uri;
	char *off_uri;
	char *on_query;
	char *off_query;
	char *on_success;
	char *off_success;
	char *err_response;
	char *response;
	int wait;
	int currentstate;
	int laststate;
	int newstate;
	pthread_t pth;
	int hasthread;
	protocol_threads_t *thread;
	struct settings_t *next;
} settings_t;

static struct settings_t *settings = NULL;

static void *thread(void *param) {
	struct protocol_threads_t *pnode = (struct protocol_threads_t *)param;
	struct JsonNode *json = (struct JsonNode *)pnode->param;
	struct JsonNode *jid = NULL;
	struct JsonNode *jchild = NULL;
	char *state = NULL, *onquery = NULL, *offquery = NULL, *onsuccess = NULL, *offsuccess = NULL;
	char *errresponse = NULL, *onuri = NULL, *offuri = NULL, *response = NULL, *method = NULL;
	int interval = 1, nrloops = 0;
	double itmp = 0;

	threads++;

	json_find_string(json, "method", &method);
	json_find_string(json, "on_uri", &onuri);
	json_find_string(json, "off_uri", &offuri);
	json_find_string(json, "on_query", &onquery);
	json_find_string(json, "off_query", &offquery);
	json_find_string(json, "on_success", &onsuccess);
	json_find_string(json, "off_success", &offsuccess);
	json_find_string(json, "err_response", &errresponse);
	json_find_string(json, "response", &response);
	json_find_string(json, "state", &state);

	struct settings_t *lnode = MALLOC(sizeof(struct settings_t));
	lnode->wait = 0;
	lnode->hasthread = 0;
	memset(&lnode->pth, '\0', sizeof(pthread_t));

	if(strcmp(state, "running") == 0) {
		lnode->newstate = 1;
	} else {
		lnode->newstate = 0;
	}
	
	lnode->id = 0;
	if((jid = json_find_member(json, "id"))) {
		jchild = json_first_child(jid);
		if(json_find_number(jchild, "id", &itmp) == 0)
			lnode->id = (int)round(itmp);
	}

	if(strcmp(method, "GET") != 0 && strcmp(method, "POST") != 0) {
		logprintf(LOG_ERR, "Webswitch %i: method must be either \"GET\" or \"POST\"", lnode->id);
		exit(EXIT_FAILURE);
	}
	
	
	if(method != NULL && strlen(method) > 0) {
		if((lnode->method = MALLOC(strlen(method)+1)) == NULL) {
			fprintf(stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}
		strcpy(lnode->method, method);
	} else {
		lnode->method = NULL;
	}
	
	if(onuri != NULL && strlen(onuri) > 0) {
		if((lnode->on_uri = MALLOC(strlen(onuri)+1)) == NULL) {
			fprintf(stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}
		strcpy(lnode->on_uri, onuri);
	} else {
		lnode->on_uri = NULL;
	}
	
	if(offuri != NULL && strlen(offuri) > 0) {
		if((lnode->off_uri = MALLOC(strlen(offuri)+1)) == NULL) {
			fprintf(stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}
		strcpy(lnode->off_uri, offuri);
	} else {
		lnode->off_uri = NULL;
	}
	
	if(onquery != NULL && strlen(onquery) > 0) {
		if((lnode->on_query = MALLOC(strlen(onquery)+1)) == NULL) {
			fprintf(stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}
		strcpy(lnode->on_query, onquery);
	} else {
		lnode->on_query = NULL;
	}

	if(offquery != NULL && strlen(offquery) > 0) {
		if((lnode->off_query = MALLOC(strlen(offquery)+1)) == NULL) {
			fprintf(stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}
		strcpy(lnode->off_query, offquery);
	} else {
		lnode->off_query = NULL;
	}

	if(onsuccess != NULL && strlen(onsuccess) > 0) {
		if((lnode->on_success = MALLOC(strlen(onsuccess)+1)) == NULL) {
			fprintf(stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}
		strcpy(lnode->on_success, onsuccess);
	} else {
		lnode->on_success = NULL;
	}
	
	if(offsuccess != NULL && strlen(offsuccess) > 0) {
		if((lnode->off_success = MALLOC(strlen(offsuccess)+1)) == NULL) {
			fprintf(stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}
		strcpy(lnode->off_success, offsuccess);
	} else {
		lnode->off_success = NULL;
	}

	if(errresponse != NULL && strlen(errresponse) > 0) {
		if((lnode->err_response = MALLOC(strlen(errresponse)+1)) == NULL) {
			fprintf(stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}
		strcpy(lnode->err_response, errresponse);
	} else {
		lnode->err_response = NULL;
	}

	if(response != NULL && strlen(response) > 0) {
		if((lnode->response = MALLOC(strlen(response)+1)) == NULL) {
		//if((lnode->response = MALLOC(BUFFER_SIZE)) == NULL) {
		fprintf(stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}
		strcpy(lnode->response, response);
	} else {
		lnode->response = NULL;
	}

	lnode->thread = pnode;
	lnode->laststate = -1;

	lnode->next = settings;
	settings = lnode;

	if(json_find_number(json, "poll-interval", &itmp) == 0)
		interval = (int)round(itmp);

	while(loop) {

	if(protocol_thread_wait(pnode, interval, &nrloops) == ETIMEDOUT) {
			pthread_mutex_lock(&lock);
			if(lnode->wait == 0) {
				struct JsonNode *message = json_mkobject();

				JsonNode *code = json_mkobject();
				json_append_member(code, "id", json_mknumber(lnode->id, 0));
				json_append_member(code, "response", json_mkstring(lnode->response));
				if(lnode->newstate == 1) {
					lnode->currentstate = 1;
					json_append_member(code, "state", json_mkstring("running"));
				} else {
					lnode->currentstate = 0;
					json_append_member(code, "state", json_mkstring("stopped"));
				}
				json_append_member(message, "message", code);
				json_append_member(message, "origin", json_mkstring("receiver"));
				json_append_member(message, "protocol", json_mkstring(webswitch->id));

				if(lnode->currentstate != lnode->laststate) {
					lnode->laststate = lnode->currentstate;

					if(pilight.broadcast != NULL) {
						pilight.broadcast(webswitch->id, message, PROTOCOL);
					}
				}
				json_delete(message);
				message = NULL;
			}
			pthread_mutex_unlock(&lock);
		}
	}
	pthread_mutex_unlock(&lock);

	threads--;
	return (void *)NULL;
}

static struct threadqueue_t *initDev(JsonNode *jdevice) {

	loop = 1;
	int temp =  0;
	char *output = json_stringify(jdevice, NULL);
	JsonNode *json = json_decode(output);
	json_free(output);

	struct protocol_threads_t *node = protocol_thread_init(webswitch, json);
	return threads_register("webswitch", &thread, (void *)node, 0);
}

static void *execute(void *param) {
	struct settings_t *p = (struct settings_t *)param;
	char *data = NULL, url[1024], *contype = NULL, *post = NULL;
	char typebuf[255], *tp = typebuf, *successcode, *token;
	int ret = 0, size = 0, meth = 0;
	unsigned int i = 0, match = 0;
	
	if(strcmp(p->method, "POST") == 0) {
		contype = MALLOC(40);
		if(!contype) {
			fprintf(stderr, "out of memory\n");

		}
		free_contype = 1;
		strcpy(contype, "application/x-www-form-urlencoded");
		//strcpy(contype, "text/plain");

		post = MALLOC(BUFFER_SIZE);
		if(!post) {
			fprintf(stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}
		meth = 1;
	}
	if(p->laststate == 0) { 
		//was off, so try to switch on
		if(meth == 0) {
			sprintf(url, "%s?%s", p->on_uri, p->on_query);
		}
		else {
			strcpy(url, p->on_uri);
			strcpy(post, p->on_query);
		}
		successcode = MALLOC(strlen(p->on_success) + 1);
		if(!successcode) {
			fprintf(stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}
		strcpy(successcode, p->on_success); 
		free_successcode = 1;
	}
	else { 
		//was on, so try to switch off
		if(meth == 0) {
			//GET method
			sprintf(url, "%s?%s", p->off_uri, p->off_query);
		}
		else {
			//POST method
			strcpy(url, p->off_uri);
			strcpy(post, p->off_query);
		}
		successcode = MALLOC(strlen(p->off_success) + 1);
		if(!successcode) {
			fprintf(stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}
		strcpy(successcode, p->off_success); 
		free_successcode = 1;
	}
	logprintf(LOG_DEBUG, "Webswitch: Calling %s", url);
	data = NULL;
	if (meth == 0) {
		logprintf(LOG_DEBUG, "Webswitch: Method GET");
		data = http_get_content(url, &tp, &ret, &size);
	}
	else {
		data = http_post_content(url, &tp, &ret, &size, contype, post);
	}
	
	logprintf(LOG_DEBUG, "Webswitch: Returncode: %i, Type: %s, Data: %s, Size: %i", ret, tp, data, size);

	if(ret == 200) {
	if(data == NULL || strlen(data) == 0) {
		logprintf(LOG_NOTICE, "Webswitch: %s didn't return success code. Expected \"%s\", got nothing", url, successcode);
		if (data == NULL) {
			strcpy(p->response, "null");
		} else {
			strcpy(p->response, data);
		}
	} else {
			if((p->response = MALLOC(strlen(data)+1)) == NULL) {
				fprintf(stderr, "out of memory\n");
				exit(EXIT_FAILURE);
			}
			match = 0;
			strcpy(p->response, data);
			token = strtok(successcode, "&");
			if(token != NULL && strlen(token) > 0) {
				if(strstr(data, token) != NULL) {
					match = 1;
					while ((token = strtok(NULL, "&")) && match == 1) {
						if(strlen(token) > 0) {
							if(strstr(data, token) == NULL) {
								match = 0;
							}
						}
					}
				}
				else {
					//one or more parameters didn't match
					logprintf(LOG_NOTICE, "Webswitch: %s didn't return success code. Expected \"%s\", got \"%s\"", url, successcode, data);
				}
			}
			else { 
				//response doesn't contain parameter list, so compare full response with expected response
				if(strstr(data, successcode) == NULL) {
					// No parameterlist, no match 
					logprintf(LOG_NOTICE, "Webswitch: %s didn't return success code. Expected \"%s\", got \"%s\"", url, successcode, data);
				}
				else {
					// No parameter list, entire expected response matched
					match = 1;
				}	
			}
		}
	} else {
		if(p->err_response != NULL) {
			strcpy(p->response, p->err_response);
		}
		else {
			strcpy(p->response, DFLT_ERR_RSP);	
		}
	}
	if (match == 1) {
		//full match so switch to new state
		if(p->laststate == 0) {
			p->newstate = 1; 
		}
		else {
			p->newstate = 0;						
		}
	}
	else {
		//no match, so return to last state
		p->newstate=p->laststate;
	}
	if(free_contype) {
		FREE(contype);
	}
	if(free_post) {
		FREE(post);
	}

	p->wait = 0;
	memset(&p->pth, '\0', sizeof(pthread_t));
	p->hasthread = 0;
	p->laststate = -1;

	pthread_mutex_unlock(&p->thread->mutex);
	pthread_cond_signal(&p->thread->cond);

	return NULL;
}

static int createCode(JsonNode *code) {
	double itmp = 0;
	int state = -1;
	double id = 0;
	int free_response = 0;
	char *response = NULL;
	
	if(json_find_number(code, "id", &id) == 0) {
		struct settings_t *tmp = settings;
		while(tmp) {
			if(tmp->id == id) {
				if(tmp->wait == 0) {
					if(tmp->method != NULL && tmp->on_uri != NULL && tmp->off_uri != NULL
						&& tmp->on_success != NULL && tmp->off_success != NULL && tmp->response != NULL ) {

						if(json_find_number(code, "running", &itmp) == 0)
							state = 1;
						else if(json_find_number(code, "stopped", &itmp) == 0)
							state = 0;

						if(state > -1) {
							webswitch->message = json_mkobject();
							JsonNode *code1 = json_mkobject();
							json_append_member(code1, "id", json_mknumber(id, 0));

							if(state == 1) {
								json_append_member(code1, "state", json_mkstring("running"));
							} else {
								json_append_member(code1, "state", json_mkstring("stopped"));
							}

							json_append_member(webswitch->message, "message", code1);
							json_append_member(webswitch->message, "origin", json_mkstring("receiver"));
							json_append_member(webswitch->message, "protocol", json_mkstring(webswitch->id));
							if(pilight.broadcast != NULL) {
								pilight.broadcast(webswitch->id, webswitch->message, PROTOCOL);
							}
							json_delete(webswitch->message);
							webswitch->message = NULL;
						}

						tmp->wait = 1;

						threads_create(&tmp->pth, NULL, execute, (void *)tmp);

						tmp->hasthread = 1;
						pthread_detach(tmp->pth);

						webswitch->message = json_mkobject();
						json_append_member(webswitch->message, "id", json_mknumber(id, 0));
						json_append_member(webswitch->message, "state", json_mkstring("pending"));

					} else {
						logprintf(LOG_NOTICE, "webswitch \"%i\" cannot operate due to missing parameters", tmp->id);
					}
				} else {
					logprintf(LOG_NOTICE, "please wait for webswitch \"%i\" to finish it's state change", tmp->id);
				}
				break;
			}
			tmp = tmp->next;
		}
	} else {
		return EXIT_FAILURE;
	}
	if(free_response) {
		FREE(response);
	}
	return EXIT_SUCCESS;
}

static void threadGC(void) {
	loop = 0;
	protocol_thread_stop(webswitch);
	while(threads > 0) {
		usleep(10);
	}
	protocol_thread_free(webswitch);

	struct settings_t *tmp;
	while(settings) {
		tmp = settings;
		if(tmp->method) FREE(tmp->method);
		if(tmp->on_uri) FREE(tmp->on_uri);
		if(tmp->off_uri) FREE(tmp->off_uri);
		if(tmp->on_query) FREE(tmp->on_query);
		if(tmp->off_query) FREE(tmp->off_query);
		if(tmp->on_success) FREE(tmp->on_success);
		if(tmp->off_success) FREE(tmp->off_success);
		if(tmp->err_response) FREE(tmp->err_response);
		if(tmp->response) FREE(tmp->response);
		if(tmp->hasthread > 0) pthread_cancel(tmp->pth);
		settings = settings->next;
		FREE(tmp);
	}
	if(settings != NULL) {
		FREE(settings);
	}
}

static void printHelp(void) {
	printf("\t -t --running\t\t\tsend on command\n");
	printf("\t -f --stopped\t\t\tsend off command\n");
	printf("\t -i --id=id\t\t\tid of the webswitch\n");
}
#endif

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void webswitchInit(void) {
#ifndef _WIN32
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&lock, &attr);
#endif

	protocol_register(&webswitch);
	protocol_set_id(webswitch, "webswitch");
	protocol_device_add(webswitch, "webswitch", "Control remote web service");
	webswitch->devtype = PENDINGSW;
	webswitch->hwtype = API;
	webswitch->multipleId = 0;

	options_add(&webswitch->options, 'i', "id", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^([0-9]{1,})$");
	options_add(&webswitch->options, 'm', "method", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);
	options_add(&webswitch->options, 'a', "on_uri", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);
	options_add(&webswitch->options, 'b', "off_uri", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);
	options_add(&webswitch->options, 'c', "on_query", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);
	options_add(&webswitch->options, 'd', "off_query", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);
	options_add(&webswitch->options, 'e', "on_success", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);
	options_add(&webswitch->options, 'g', "off_success", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);
	options_add(&webswitch->options, 'h', "err_response", OPTION_HAS_VALUE, DEVICES_SETTING, JSON_STRING, "", NULL);
	options_add(&webswitch->options, 'r', "response", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);
	options_add(&webswitch->options, 't', "running", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&webswitch->options, 'p', "pending", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&webswitch->options, 'f', "stopped", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);

	options_add(&webswitch->options, 0, "readonly", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)0, "^[10]{1}$");
	options_add(&webswitch->options, 0, "confirm", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)0, "^[10]{1}$");
	options_add(&webswitch->options, 0, "poll-interval", OPTION_HAS_VALUE, DEVICES_SETTING, JSON_NUMBER, (void *)1, "[0-9]");

#ifndef _WIN32
	webswitch->createCode=&createCode;
	webswitch->printHelp=&printHelp;
	webswitch->initDev=&initDev;
	webswitch->threadGC=&threadGC;
#endif

}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "webswitch";
	module->version = "1.0";
	module->reqversion = "6.0";
	module->reqcommit = "84";
}

void init(void) {
	webswitchInit();
}
#endif
