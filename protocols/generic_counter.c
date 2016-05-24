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
#include <math.h>

#include "../../core/pilight.h"
#include "../../core/common.h"
#include "../../core/dso.h"
#include "../../core/log.h"
#include "../protocol.h"
#include "../../core/binary.h"
#include "../../core/gc.h"
#include "generic_counter.h"

static void createMessage(int id, int count) {
	generic_counter->message = json_mkobject();
	json_append_member(generic_counter->message, "id", json_mknumber(id, 0));
	json_append_member(generic_counter->message, "count", json_mknumber(count, 0));
}

static int createCode(JsonNode *code) {
	int id = -1;
	int count = 0;
	double itmp = 0;

	if(json_find_number(code, "id", &itmp) != 0) {
		logprintf(LOG_ERR, "generic_counter is missing an \"id\"");
		return EXIT_FAILURE;
	}
	id = (int)round(itmp);
	
	if(json_find_number(code, "count", &itmp) != 0) {
		logprintf(LOG_ERR, "generic_counter is missing a \"count\"");
		return EXIT_FAILURE;
	}
	count = (int)round(itmp);
	createMessage(id, count);
	return EXIT_SUCCESS;

}

static void printHelp(void) {
	printf("\t -i --id=id\t\t\tcontrol a device with this id\n");
	printf("\t -c --count=count\t\tpreset this counter\n");
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void genericCounterInit(void) {

	protocol_register(&generic_counter);
	protocol_set_id(generic_counter, "generic_counter");
	protocol_device_add(generic_counter, "generic_counter", "Generic Counter");
	generic_counter->devtype = API;

	options_add(&generic_counter->options, 'i', "id", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^([0-9]{1,})$");
	options_add(&generic_counter->options, 'c', "count", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, NULL);

	generic_counter->printHelp=&printHelp;
	generic_counter->createCode=&createCode;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "generic_counter";
	module->version = "0.1";
	module->reqversion = "6.0";
	module->reqcommit = "84";
}

void init(void) {
	genericCounterInit();
}
#endif
