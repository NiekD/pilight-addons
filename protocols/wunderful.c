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

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <regex.h>
#include <sys/stat.h>
#ifndef _WIN32
	#ifdef __mips__
		#define __USE_UNIX98
	#endif
#endif
#include <pthread.h>

#include "../../core/threads.h"
#include "../../core/pilight.h"
#include "../../core/common.h"
#include "../../core/dso.h"
#include "../../core/datetime.h" // Full path because we also have a datetime protocol
#include "../../core/log.h"
#include "../../core/http.h"
#include "../protocol.h"
#include "../../core/binary.h"
#include "../../core/json.h"
#include "../../core/gc.h"
#include "../../core/strptime.h"
#include "wunderful.h"

#define INTERVAL	300

typedef struct settings_t {
	char *api;
	char *country;
	char *location;
	time_t update;
	protocol_threads_t *thread;
	struct settings_t *next;
} settings_t;

static pthread_mutex_t lock;
static pthread_mutexattr_t attr;

static struct settings_t *settings;
static unsigned short loop = 1;
static unsigned short threads = 0;


static int get_number(JsonNode *object, const char *name, double *out) {
	JsonNode *node = json_find_member(object, name);
	if (node && node->tag == JSON_NUMBER) {
		*out = node->number_;
		return 0;
	} else if(node && node->tag == JSON_STRING) {
			*out = atof(node->string_);
			return 0;
	}
	return 1;
}
static void *thread(void *param) {
	struct protocol_threads_t *thread = (struct protocol_threads_t *)param;
	struct JsonNode *json = (struct JsonNode *)thread->param;
	struct JsonNode *jid = NULL;
	struct JsonNode *jchild = NULL;
	struct JsonNode *jchild1 = NULL;
	struct JsonNode *node = NULL;
	struct settings_t *wnode = MALLOC(sizeof(struct settings_t));
	struct tm alarmtime;
	struct tm exptime;	
	struct tm obstime;

	int interval = 86400, ointerval = 86400, event = 0;
	int firstrun = 1, nrloops = 0, timeout = 0;

	char url[1024];
	char *tz = NULL;
	char *data = NULL;
	char typebuf[255], *tp = typebuf;
	char *stmp = NULL, *winddir = "X", *stationid = "X", *obsdt = "X", *weather = "X", *city = "NA";
	char *weatherfc = "X", *winddirfc = "X";
	char *alarm_descr = "NA", *level_descr = "NA", *alarm_date = "X", *alarm_exp = "X", *winddir2 = NULL;
	double alarm_type = 0, alarm_level = 0;
	double latitude = 0, longitude = 0;
	double temp = 0, itmp = -1, winddegrees = 0, windspeed = 0, windgust = 0, preciph = 0, precipd = 0, rain = 0, estimated = 0;
	double tempfc = 0, windspeedfc = 0, winddegreesfc = 0, rainfc = 0, popfc = 0, precipfc = 0, fcttime = 0, humifc = 0, pressure = 0, pressurefc = 0;
	int humi = 0, ret = 0, size = 0, reti = 0, utc_offset = 0, dst = 0, has_current = 0, has_forecast = 0, has_alert = 0;
	regex_t regex;
	
	JsonNode *jdata = NULL;
	JsonNode *jobs = NULL;
	JsonNode *jtemp = NULL;
	JsonNode *jtemp2 = NULL;
	JsonNode *jtemp3 = NULL;
	JsonNode *jfchour = NULL;	
	JsonNode *jestim = NULL;
	
	memset(&typebuf, '\0', 255);

	if(wnode == NULL) {
		logprintf(LOG_ERR, "out of memory");
		exit(EXIT_FAILURE);
	}

	threads++;

	int has_country = 0, has_api = 0, has_location = 0;
	if((jid = json_find_member(json, "id"))) {
		jchild = json_first_child(jid);
		while(jchild) {
			has_country = 0, has_api = 0, has_location = 0;
			jchild1 = json_first_child(jchild);

			while(jchild1) {
				if(strcmp(jchild1->key, "api") == 0) {
					has_api = 1;
					wnode->api = MALLOC(strlen(jchild1->string_)+1);
					if(!wnode->api) {
						logprintf(LOG_ERR, "out of memory");
						exit(EXIT_FAILURE);
					}
					strcpy(wnode->api, jchild1->string_);
				}
				if(strcmp(jchild1->key, "location") == 0) {
					has_location = 1;
					wnode->location = MALLOC(strlen(jchild1->string_)+1);
					if(!wnode->location) {
						logprintf(LOG_ERR, "out of memory");
						exit(EXIT_FAILURE);
					}
					strcpy(wnode->location, jchild1->string_);
				}
				if(strcmp(jchild1->key, "country") == 0) {
					has_country = 1;
					wnode->country = MALLOC(strlen(jchild1->string_)+1);
					if(!wnode->country) {
						logprintf(LOG_ERR, "out of memory");
						exit(EXIT_FAILURE);
					}
					strcpy(wnode->country, jchild1->string_);
				}
				jchild1 = jchild1->next;
			}
			if(has_country == 1 && has_api == 1 && has_location == 1) {
				wnode->thread = thread;
				wnode->next = settings;
				settings = wnode;
			} else {
				if(has_country == 1) {
					FREE(wnode->country);
				}
				if(has_location == 1) {
					FREE(wnode->location);
				}
				if(has_api == 1) {
					FREE(wnode->api);
				}
				FREE(wnode);
				wnode = NULL;
			}
			jchild = jchild->next;
		}
	}

	if(!wnode) {
		return 0;
	}

	if(json_find_number(json, "poll-interval", &itmp) == 0)
		interval = (int)round(itmp);
	ointerval = interval;

	while(loop) {
		event = protocol_thread_wait(thread, INTERVAL, &nrloops);
		pthread_mutex_lock(&lock);
		if(loop == 0) {
			break;
		}
		timeout += INTERVAL;
		if(timeout >= interval || event != ETIMEDOUT || firstrun == 1) {
			timeout = 0;
			interval = ointerval;
			data = NULL;
			has_current = 0;
			has_forecast = 0;
			has_alert = 0;
			ret = 0, size = 0;
			sprintf(url, "http://api.wunderground.com/api/%s/conditions/hourly/alerts/q/%s/%s.json", wnode->api, wnode->country, wnode->location);
			logprintf(LOG_INFO, "Calling %s", url);
			data = http_get_content(url, &tp, &ret, &size);
			logprintf(LOG_INFO, "Got response %i from wunderground", ret);
			if(ret == 200) {
				if(strstr(typebuf, "application/json") != NULL) {
					if(json_validate(data) == true) {
						if((jdata = json_decode(data)) != NULL) {
							if((jobs = json_find_member(jdata, "current_observation")) != NULL) {
								has_current = 1;
								if(get_number(jobs, "temp_c", &temp) != 0) {
									logprintf(LOG_NOTICE,"api.wunderground.com json has no temp_c key");
								} else if((node = json_find_member(jobs, "display_location")) == NULL) {
									logprintf(LOG_NOTICE,"api.wunderground.com json has no display_location key");
								} else 	if (json_find_string(node, "city", &city) != 0) {		
									logprintf(LOG_NOTICE,"api.wunderground.com json has no city key");
								} else 	if (get_number(node, "latitude", &latitude) != 0) {		
									logprintf(LOG_NOTICE,"api.wunderground.com json has no latitude key");
								} else if(get_number(node, "longitude", &longitude) != 0) {
									logprintf(LOG_NOTICE,"api.wunderground.com json has no longitude key");
								} else if(json_find_string(jobs, "relative_humidity", &stmp) != 0) {
									logprintf(LOG_NOTICE,"api.wunderground.com json has no relative_humidity key");
								} else if(json_find_string(jobs, "wind_dir", &winddir) != 0) {
									logprintf(LOG_NOTICE,"api.wunderground.com json has no wind_dir key");
								} else if(get_number(jobs, "wind_degrees", &winddegrees) != 0) {
									logprintf(LOG_NOTICE,"api.wunderground.com json has no wind_degrees key");
								} else if(get_number(jobs, "wind_kph", &windspeed) != 0) {
									logprintf(LOG_NOTICE,"api.wunderground.com json has no wind_kph key");
								} else if(get_number(jobs, "wind_gust_kph", &windgust) != 0) {
									logprintf(LOG_NOTICE,"api.wunderground.com json has no wind_gust_kph key");
								} else if(get_number(jobs, "precip_1hr_metric", &preciph) != 0) {
									logprintf(LOG_NOTICE,"api.wunderground.com json has no precip_1hr_metric key");
								} else if(get_number(jobs, "precip_today_metric", &precipd) != 0) {
									logprintf(LOG_NOTICE,"api.wunderground.com json has no precip_1hr_metric key");
								} else if(get_number(jobs, "pressure_mb", &pressure) != 0) {
									logprintf(LOG_NOTICE,"api.wunderground.com json has no precip_1hr_metric key");
								} else if(json_find_string(jobs, "observation_time_rfc822", &obsdt) != 0) {
									logprintf(LOG_NOTICE,"api.wunderground.com json has no observation_time_rfc822 key");
								} else if(json_find_string(jobs, "weather", &weather) != 0) {
									logprintf(LOG_NOTICE,"api.wunderground.com json has no weather key");
								} else if(json_find_string(jobs, "station_id", &stationid) != 0) {
									logprintf(LOG_NOTICE,"api.wunderground.com json has no station_id key");
								}
								if ((jestim = json_find_member(jobs, "estimated")) != 0 && get_number(jestim, "estimated", &estimated) == 0) {
									logprintf(LOG_NOTICE, "Estimated current weather");
									strcpy(stationid, "ESTIMATED");
								}
								sscanf(stmp, "%d%%", &humi);
								logprintf(LOG_INFO, "Station ID: %s, Obstime: %s, Lat: %f, Lon: %f", stationid, obsdt, latitude, longitude);	
								if (strptime(obsdt, "%a, %d %b %Y %H:%M:%S", &obstime) == 0) {
									logprintf(LOG_NOTICE, "api.wunderground.com invalid observation time");
								}
								winddir2 = MALLOC(strlen(winddir)+1);
								if(!winddir2) {
									logprintf(LOG_ERR, "out of memory");
									exit(EXIT_FAILURE);
								}
								strcpy(winddir2, winddir);

								str_replace("North", "N", &winddir2);
								str_replace("East", "E", &winddir2);
								str_replace("South", "S", &winddir2);
								str_replace("West", "W", &winddir2);

#if !defined(__FreeBSD__) && !defined(_WIN32)
								size_t maxGroups = 7;
								regmatch_t groupArray[maxGroups];

								reti = regcomp(&regex, "([R|r]ain)|([D|d]rizzle)|([T|t]hunder)|([S|s]hower)", REG_EXTENDED);
								if(reti) {
									logprintf(LOG_ERR, "could not compile regex for wunderground rain");
								}
								reti = regexec(&regex, weather, maxGroups, groupArray, 0);
								if(reti == REG_NOMATCH || reti != 0) {
									logprintf(LOG_INFO, "no rain now");
									rain = 0;
								} else {
									logprintf(LOG_INFO, "it is raining now");
									rain = 1;															
								}
								regfree(&regex);
#endif
							} else {
								logprintf(LOG_NOTICE, "api.wunderground.com json has no current_observation key");
							}

							if((jtemp = json_find_member(jdata, "hourly_forecast")) != NULL) {
								if((jtemp2 = json_first_child(jtemp)) != NULL) {
									has_forecast = 1;
									if(get_number(jtemp2, "pop", &popfc) != 0) {
										logprintf(LOG_NOTICE, "api.wunderground.com json has no pop key");
									} else if((jtemp3 = json_find_member(jtemp2, "FCTTIME")) == NULL || get_number(jtemp3, "hour", &fcttime) != 0) {
										logprintf(LOG_NOTICE, "api.wunderground.com json has no hour key");
									} else if(json_find_string(jtemp2, "wx", &weatherfc) != 0) {
										logprintf(LOG_NOTICE, "api.wunderground.com json has no wx key");
									} else if((jfchour = json_find_member(jtemp2, "temp")) == NULL || get_number(jfchour, "metric", &tempfc) != 0) {
										logprintf(LOG_NOTICE, "api.wunderground.com json has no temp key");
									} else if((jfchour = json_find_member(jtemp2, "wspd")) == NULL || get_number(jfchour, "metric", &windspeedfc) != 0) {
										logprintf(LOG_NOTICE, "api.wunderground.com json has no wspd key");
									} else if((jfchour = json_find_member(jtemp2, "wdir")) == NULL) {
										logprintf(LOG_NOTICE, "api.wunderground.com json has no wdir key");
									} else if(json_find_string(jfchour, "dir", &winddirfc) != 0) {
										logprintf(LOG_NOTICE, "api.wunderground.com json has no dir key");
									} else if(get_number(jfchour, "degrees", &winddegreesfc) != 0) {
										logprintf(LOG_NOTICE, "api.wunderground.com json has no degrees key");
									} else if(get_number(jtemp2, "humidity", &humifc) != 0) {
										logprintf(LOG_NOTICE, "api.wunderground.com json has no humidity key");
									} else if((jfchour = json_find_member(jtemp2, "qpf")) == NULL || get_number(jfchour, "metric", &precipfc) != 0) {
										logprintf(LOG_NOTICE, "api.wunderground.com json has no qpf key");
									} else if((jfchour = json_find_member(jtemp2, "mslp")) == NULL || get_number(jfchour, "metric", &pressurefc) != 0) {
										logprintf(LOG_NOTICE, "api.wunderground.com json has no mslp key");
									}

			#if !defined(__FreeBSD__) && !defined(_WIN32)
									size_t maxGroups = 7;
									regmatch_t groupArray[maxGroups];

									reti = regcomp(&regex, "([R|r]ain)|([D|d]rizzle)|([T|t]hunder)|([S|s]hower)", REG_EXTENDED);
									if(reti) {
										logprintf(LOG_ERR, "could not compile regex for wunderground rain");
									}
									reti = regexec(&regex, weatherfc, maxGroups, groupArray, 0);
									if(reti == REG_NOMATCH || reti != 0) {
										logprintf(LOG_INFO, "no rain expected");
										rainfc = 0;
									} else {
										logprintf(LOG_INFO, "rain expected next hour");
										rainfc = 1;
									}
									regfree(&regex);

#endif

								} else {
									logprintf(LOG_NOTICE, "api.wunderground.com json has no forecast data");
								}
							} else {

								logprintf(LOG_NOTICE, "api.wunderground.com json has no hourly_forecast key");
							}

							if((jtemp = json_find_member(jdata, "alerts")) != NULL) {
								if((jtemp2 = json_first_child(jtemp)) != NULL) {
									has_alert = 1;
									if(json_find_string(jtemp2, "wtype_meteoalarm_name", &alarm_descr) != 0) {
										logprintf(LOG_NOTICE, "api.wunderground.com json has no description key");
									} else if(get_number(jtemp2, "wtype_meteoalarm", &alarm_type) != 0) {
										logprintf(LOG_NOTICE, "api.wunderground.com json has no wtype_meteoalarm key");
									} else if(get_number(jtemp2, "level_meteoalarm", &alarm_level) != 0) {
										logprintf(LOG_NOTICE, "api.wunderground.com json has no level_meteoalarm key");
									} else if(json_find_string(jtemp2, "level_meteoalarm_name", &level_descr) != 0) {
										logprintf(LOG_NOTICE, "api.wunderground.com json has no level_meteoalarm_name key");								
									} else if(json_find_string(jtemp2, "date", &alarm_date) != 0) {
										logprintf(LOG_NOTICE, "api.wunderground.com json has no date key");
									} else if(json_find_string(jtemp2, "expires", &alarm_exp) != 0) {
										logprintf(LOG_NOTICE, "api.wunderground.com json has no expires key");
									}

									logprintf(LOG_INFO, "Alarm text: %s, Level: %s, Alarm time: %s, Alarm Expires: %s", alarm_descr, level_descr, alarm_date, alarm_exp);	
									if((tz = coord2tz(longitude, latitude)) == NULL) {
										logprintf(LOG_INFO, "wunderful could not determine timezone");
										tz = "UTC";
									} else {
										logprintf(LOG_INFO, "display location %.6f:%.6f seems to be in timezone: %s", longitude, latitude, tz);
									}
									utc_offset = tzoffset("UTC", tz);

									logprintf(LOG_DEBUG, "Time offset: %d", utc_offset);

									time_t timestamp;

									timestamp = time(NULL);
									dst = isdst(timestamp, tz);
									logprintf(LOG_DEBUG, "DST: %d", dst);

									if (strptime(alarm_date, "%Y-%m-%d %H:%M:%S GMT", &alarmtime) == 0) { //UTC time!
										logprintf(LOG_NOTICE, "api.wunderground.com invalid alarm time");
									} else {

										alarmtime.tm_hour += utc_offset;
										alarmtime.tm_hour += dst;
										if(alarmtime.tm_hour > 23) {
											alarmtime.tm_hour -= 24;
										}

										//logprintf(LOG_DEBUG, "UTC: %d DST: %d", alarmtime.tm_hour, dst);

									}
									if (strptime(alarm_exp, "%Y-%m-%d %H:%M:%S GMT", &exptime) == 0) { //2012-08-21 16:00:00 GMT"
										logprintf(LOG_NOTICE, "api.wunderground.com invalid expiration time");
									} else {
										exptime.tm_hour += utc_offset;
										exptime.tm_hour += dst;
										if(exptime.tm_hour > 23) {
											exptime.tm_hour -= 24;
										}
									}

								} else {
									logprintf(LOG_INFO, "api.wunderground.com json has no alert data");
								}
							} else {
								logprintf(LOG_NOTICE, "api.wunderground.com json has no alerts key");		
							}

						if(has_current == 1 || has_forecast == 1) {
							wunderful->message = json_mkobject();
							JsonNode *code = json_mkobject();

							json_append_member(code, "api", json_mkstring(wnode->api));
							json_append_member(code, "location", json_mkstring(wnode->location));
							json_append_member(code, "country", json_mkstring(wnode->country));

							if(has_current == 1) {
								json_append_member(code, "temperature", json_mknumber((double)round(temp), 0));
								json_append_member(code, "humidity", json_mknumber((double)humi, 0));
								json_append_member(code, "bft", json_mknumber(round(pow((windspeed/3.01), (2.0/3.0))), 0));
								json_append_member(code, "gustbft", json_mknumber(round(pow((windgust/3.01), (2.0/3.0))), 0));
								json_append_member(code, "degrees", json_mknumber(winddegrees, 0));
								json_append_member(code, "dir", json_mkstring(winddir2));
								json_append_member(code, "preciphour", json_mknumber(round(preciph), 0));
								json_append_member(code, "precipday", json_mknumber(round(precipd), 0));
								json_append_member(code, "pressure", json_mknumber(pressure, 0));
								json_append_member(code, "rain", json_mknumber(rain, 0));
								json_append_member(code, "stationid", json_mkstring(stationid));
								json_append_member(code, "city", json_mkstring(city));
								json_append_member(code, "obstime", json_mknumber((double)obstime.tm_hour + ((double)obstime.tm_min / 100), 2));
								json_append_member(code, "weather", json_mkstring(weather));
								FREE(winddir2);
							}
							if(has_forecast == 1) {
								json_append_member(code, "weatherfc", json_mkstring(weatherfc));
								json_append_member(code, "dirfc", json_mkstring(winddirfc));
								json_append_member(code, "tempfc", json_mknumber(tempfc, 0));
								json_append_member(code, "speedfc", json_mknumber(windspeedfc, 0));
								json_append_member(code, "degreesfc", json_mknumber(winddegreesfc, 0));
								json_append_member(code, "bftfc", json_mknumber(round(pow((windspeedfc/3.01), (2.0/3.0))), 0));
								json_append_member(code, "rainfc", json_mknumber(rainfc, 0));
								json_append_member(code, "precipfc", json_mknumber(precipfc, 0));
								json_append_member(code, "pressurefc", json_mknumber(pressurefc, 0));
								json_append_member(code, "popfc", json_mknumber(popfc, 0));
								json_append_member(code, "humidityfc", json_mknumber(humifc, 0));
								json_append_member(code, "fcttime", json_mknumber(fcttime, 2));

							}
							if(has_alert == 1) {
								json_append_member(code, "alarmtype", json_mknumber(alarm_type, 0));
								json_append_member(code, "alarmlevel", json_mknumber(alarm_level, 0));
								json_append_member(code, "leveltext", json_mkstring(level_descr));
								json_append_member(code, "alarmtext", json_mkstring(alarm_descr));
								json_append_member(code, "alarmstart", json_mknumber((double)alarmtime.tm_hour + ((double)alarmtime.tm_min / 100), 2));
								json_append_member(code, "alarmend", json_mknumber((double)exptime.tm_hour + ((double)exptime.tm_min / 100), 2));

							}
							else{	
								json_append_member(code, "alarmtype", json_mknumber(0, 0));
								json_append_member(code, "alarmlevel", json_mknumber(0, 0));
								json_append_member(code, "leveltext", json_mkstring("None"));
								json_append_member(code, "alarmtext", json_mkstring("None"));
							}

							json_append_member(code, "update", json_mknumber(0, 0));

							json_append_member(wunderful->message, "message", code);
							json_append_member(wunderful->message, "origin", json_mkstring("receiver"));
							json_append_member(wunderful->message, "protocol", json_mkstring(wunderful->id));

							if(pilight.broadcast != NULL) {
								pilight.broadcast(wunderful->id, wunderful->message, PROTOCOL);
							}
							json_delete(wunderful->message);
							wunderful->message = NULL;

							wnode->update = time(NULL);
							json_delete(jdata);
						}

						} else {
							logprintf(LOG_NOTICE, "api.wunderground.com json could not be parsed");
						}
					} else {
						logprintf(LOG_NOTICE, "api.wunderground.com response was not in a valid json format");
					}
				} else {
					logprintf(LOG_NOTICE, "api.wunderground.com response was invalid");
				}
			} else {
				logprintf(LOG_NOTICE, "could not reach api.wundergrond.com");
			}

			if(data != NULL) {
				FREE(data);
				data = NULL;
			}

		} else {
			wunderful->message = json_mkobject();
			JsonNode *code = json_mkobject();
			json_append_member(code, "api", json_mkstring(wnode->api));
			json_append_member(code, "location", json_mkstring(wnode->location));
			json_append_member(code, "country", json_mkstring(wnode->country));
			json_append_member(code, "update", json_mknumber(1, 0));
			json_append_member(wunderful->message, "message", code);
			json_append_member(wunderful->message, "origin", json_mkstring("receiver"));
			json_append_member(wunderful->message, "protocol", json_mkstring(wunderful->id));
			if(pilight.broadcast != NULL) {
				pilight.broadcast(wunderful->id, wunderful->message, PROTOCOL);
			}
			json_delete(wunderful->message);
			wunderful->message = NULL;
		}
		firstrun = 0;
		pthread_mutex_unlock(&lock);
	}
	pthread_mutex_unlock(&lock);

	threads--;
	return (void *)NULL;
}

static struct threadqueue_t *initDev(JsonNode *jdevice) {
	loop = 1;
	char *output = json_stringify(jdevice, NULL);
	JsonNode *json = json_decode(output);
	json_free(output);

	struct protocol_threads_t *node = protocol_thread_init(wunderful, json);
	return threads_register("wunderful", &thread, (void *)node, 0);
}

static int checkValues(JsonNode *code) {
	double interval = INTERVAL;

	json_find_number(code, "poll-interval", &interval);

	if((int)round(interval) < INTERVAL) {
		logprintf(LOG_ERR, "wunderful poll-interval cannot be lower than %d", INTERVAL);
		return 1;
	}

	return 0;
}

static int createCode(JsonNode *code) {
	struct settings_t *wtmp = settings;
	char *country = NULL;
	char *location = NULL;
	char *api = NULL;
	double itmp = 0;
	time_t currenttime = 0;

	if(json_find_number(code, "min-interval", &itmp) == 0) {
		logprintf(LOG_ERR, "you can't override the min-interval setting");
		return EXIT_FAILURE;
	}

	if(json_find_string(code, "country", &country) == 0 &&
	  json_find_string(code, "location", &location) == 0 &&
	  json_find_string(code, "api", &api) == 0 &&
	  json_find_number(code, "update", &itmp) == 0) {

		while(wtmp) {
			if(strcmp(wtmp->country, country) == 0
				&& strcmp(wtmp->location, location) == 0
				&& strcmp(wtmp->api, api) == 0) {
				if((currenttime-wtmp->update) > INTERVAL) {
					pthread_mutex_unlock(&wtmp->thread->mutex);
					pthread_cond_signal(&wtmp->thread->cond);
					wtmp->update = time(NULL);
				}
			}
			wtmp = wtmp->next;
		}
	} else {
		logprintf(LOG_ERR, "wunderful: insufficient number of arguments");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

static void threadGC(void) {
	loop = 0;
	protocol_thread_stop(wunderful);
	while(threads > 0) {
		usleep(10);
	}
	protocol_thread_free(wunderful);

	struct settings_t *wtmp = NULL;
	while(settings) {
		wtmp = settings;
		FREE(settings->api);
		FREE(settings->country);
		FREE(settings->location);
		settings = settings->next;
		FREE(wtmp);
	}
	if(settings != NULL) {
		FREE(settings);
	}
}

static void printHelp(void) {
	printf("\t -c --country=country\t\tupdate an entry with this country\n");
	printf("\t -l --location=location\t\tupdate an entry with this location\n");
	printf("\t -a --api=api\t\t\tupdate an entry with this api code\n");
	printf("\t -u --update\t\t\tupdate the defined weather entry\n");
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void wunderfulInit(void) {
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&lock, &attr);

	protocol_register(&wunderful);
	protocol_set_id(wunderful, "wunderful");
	protocol_device_add(wunderful, "wunderful", "Weatherstation based on Weaher Underground API");
	wunderful->devtype = WEATHER;
	wunderful->hwtype = API;
	wunderful->multipleId = 0;
	wunderful->masterOnly = 1;

	options_add(&wunderful->options, 't', "temperature", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, "^[0-9]{1,5}$");
	options_add(&wunderful->options, 'h', "humidity", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, "^[0-9]{1,5}$");
	options_add(&wunderful->options, 'a', "api", OPTION_HAS_VALUE, DEVICES_ID, JSON_STRING, NULL, "^[a-z0-9]+$");
	options_add(&wunderful->options, 'l', "location", OPTION_HAS_VALUE, DEVICES_ID, JSON_STRING, NULL, "^[a-z_]+$");
	options_add(&wunderful->options, 'c', "country", OPTION_HAS_VALUE, DEVICES_ID, JSON_STRING, NULL, "^[a-z]+$");
	options_add(&wunderful->options, 'b', "bft", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, NULL);
	options_add(&wunderful->options, 'f', "degrees", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, NULL);
	options_add(&wunderful->options, 'g', "dir", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);
	options_add(&wunderful->options, 'i', "gustbft", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, NULL);
	options_add(&wunderful->options, 'j', "preciphour", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, NULL);
	options_add(&wunderful->options, 'e', "precipday", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, NULL);
	options_add(&wunderful->options, 'k', "rain", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, NULL);
	options_add(&wunderful->options, 'p', "pressure", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, NULL);
	options_add(&wunderful->options, 'm', "weather", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);
	options_add(&wunderful->options, 'n', "obstime", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, NULL);
	options_add(&wunderful->options, 'o', "city", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);
	options_add(&wunderful->options, 'd', "weatherfc", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);
	options_add(&wunderful->options, 'q', "dirfc", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);
	options_add(&wunderful->options, 'r', "tempfc", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, NULL);
	options_add(&wunderful->options, 's', "bftfc", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, NULL);
	options_add(&wunderful->options, 'v', "degreesfc", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, NULL);
	options_add(&wunderful->options, 'w', "rainfc", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, NULL);
	options_add(&wunderful->options, 'x', "precipfc", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, NULL);
	options_add(&wunderful->options, '9', "pressurefc", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, NULL);
	options_add(&wunderful->options, 'y', "popfc", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, NULL);	
	options_add(&wunderful->options, 'z', "humidityfc", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, NULL);	
	options_add(&wunderful->options, '1', "fcttime", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, NULL);
	options_add(&wunderful->options, '2', "alarmtype", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, NULL);	
	options_add(&wunderful->options, '3', "alarmlevel", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, NULL);	
	options_add(&wunderful->options, '4', "alarmtext", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);
	options_add(&wunderful->options, '5', "alarmstart", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, NULL);	
	options_add(&wunderful->options, '6', "alarmend", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, NULL);	
	options_add(&wunderful->options, '7', "leveltext", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);	
	options_add(&wunderful->options, '8', "stationid", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);

	options_add(&wunderful->options, 'u', "update", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, NULL);

	// options_add(&wunderful->options, 0, "decimals", OPTION_HAS_VALUE, DEVICES_SETTING, JSON_NUMBER, (void *)2, "[0-9]");
	/*options_add(&wunderful->options, 0, "temperature-decimals", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)2, "[0-9]");
	options_add(&wunderful->options, 0, "humidity-decimals", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)2, "[0-9]");
	options_add(&wunderful->options, 0, "sunriseset-decimals", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)2, "[0-9]");
	options_add(&wunderful->options, 0, "show-humidity", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "^[10]{1}$");
	options_add(&wunderful->options, 0, "show-temperature", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "^[10]{1}$");
	options_add(&wunderful->options, 0, "show-sunriseset", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "^[10]{1}$");
	options_add(&wunderful->options, 0, "show-update", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "^[10]{1}$");
	*/
	options_add(&wunderful->options, 0, "poll-interval", OPTION_HAS_VALUE, DEVICES_SETTING, JSON_NUMBER, (void *)86400, "[0-9]");

	wunderful->createCode=&createCode;
	wunderful->initDev=&initDev;
	wunderful->checkValues=&checkValues;
	wunderful->threadGC=&threadGC;
	wunderful->printHelp=&printHelp;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "wunderful";
	module->version = "1.0";
	module->reqversion = "6.0";
	module->reqcommit = "84";
}

void init(void) {
	wunderfulInit();
}
#endif
