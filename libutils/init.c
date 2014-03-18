/* Twine: Configuration defaults
 *
 * Author: Mo McRoberts <mo.mcroberts@bbc.co.uk>
 *
 * Copyright (c) 2014 BBC
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "p_libutils.h"

const char *utils_progname = "(none)";
int utils_is_daemon = 0;

int
utils_init(int argc, char **argv, int daemon)
{
	const char *t;

	utils_is_daemon = daemon;
	if(argc && argv[0])
	{
		t = strrchr(argv[0], '/');
		if(t)
		{
			utils_progname = t + 1;
		}
		else
		{
			utils_progname = argv[0];
		}
	}
	log_set_ident(utils_progname);
	/* Log to stderr until configuration is loaded */
	log_set_stderr(1);
	log_set_level(LOG_NOTICE);
	if(!daemon)
	{
		log_set_syslog(0);
	}
	return 0;
}
