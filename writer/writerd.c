/* Twine: Writer daemon
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

#include "p_writerd.h"

static void writerd_usage(void);
static int writerd_init(int argc, char **argv);
static int writerd_process_args(int argc, char **argv);
static int writerd_plugin_config_cb(const char *key, const char *value);
static void writerd_signal(int sig);

int
main(int argc, char **argv)
{
	pid_t child;
	int detach;

	if(writerd_init(argc, argv))
	{
		return 1;
	}
	detach = config_get_bool(TWINE_APP_NAME ":detach", 1);
	signal(SIGHUP, SIG_IGN);
	if(detach)
	{
		signal(SIGINT, SIG_IGN);
	}
	else
	{
		signal(SIGINT, writerd_signal);
	}
	signal(SIGALRM, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGUSR1, SIG_IGN);
	signal(SIGUSR2, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGTERM, writerd_signal);
	if(detach)
	{
		child = utils_daemon(TWINE_APP_NAME ":pidfile", LOCALSTATEDIR "/run/twine-writerd.pid");
		if(child < 0)
		{
			return 1;
		}
		if(child > 0)
		{
			return 0;
		}
	}
	/* Execute the runloop */
	if(writerd_runloop())
	{
		return 1;
	}
	return 0;
}

static int
writerd_init(int argc, char **argv)
{
	struct twine_configfn_struct configfn;
		
	/* Initialise libtwine */
	twine_init_(log_vprintf);
	configfn.config_get = config_get;
	configfn.config_geta = config_geta;
	configfn.config_get_int = config_get_int;
	configfn.config_get_bool = config_get_bool;
	configfn.config_get_all = config_get_all;
	twine_config_init_(&configfn);
	
	/* Initialise the utilities library */
	if(utils_init(argc, argv, 1))
	{
		return -1;
	}
	/* Apply default configuration */
	if(config_init(utils_config_defaults))
	{
		return -1;
	}
	/* Process command-line arguments */
	if(writerd_process_args(argc, argv))
	{
		return -1;
	}
	/* Load the configuration file */
	if(config_load(NULL))
	{
		return -1;
	}
	/* Update logging configuration to use configuration file */
	log_set_use_config(1);

	/* Initialise libCURL */
	curl_global_init(CURL_GLOBAL_ALL);
	/* Set up the SPARQL interface */
	if(writerd_sparql_init())
	{
		return -1;
	}
	/* Set up the AMQP interface */
	if(utils_proton_init_recv(TWINE_APP_NAME ":amqp-uri"))
	{
		return -1;
	}
	/* Load plug-ins */
	config_get_all(TWINE_APP_NAME, "module", writerd_plugin_config_cb);
	return 0;
}

static void
writerd_usage(void)
{
	fprintf(stderr, "Usage: %s [OPTIONS]\n"
			"\n"
			"OPTIONS is one or more of:\n"
			"  -h                   Print this notice and exit\n"
			"  -f                   Don't detach and run in the background\n"
			"  -d                   Enable debug output to standard error\n"
			"  -c FILE              Specify path to configuration file\n\n",
			utils_progname);
}

static int
writerd_process_args(int argc, char **argv)
{
	int c;

	while((c = getopt(argc, argv, "hfc:d")) != -1)
	{
		switch(c)
		{
		case 'h':
			writerd_usage();
			exit(0);
		case 'f':
			config_set(TWINE_APP_NAME ":detach", "0");
			break;
		case 'c':
			config_set("global:configFile", optarg);
			break;
		case 'd':
			config_set("log:level", "debug");
			config_set("log:stderr", "1");
			config_set("sparql:debug", "1");
			config_set(TWINE_APP_NAME ":detach", "0");
			break;
		default:
			writerd_usage();
			return -1;
		}
	}
	if(optind != argc)
	{
		/* There should not be any remaining command-line arguments */
		writerd_usage();
		return -1;
	}
	return 0;
}

static int
writerd_plugin_config_cb(const char *key, const char *value)
{
	(void) key;

	twine_plugin_load_(value);
	return 0;
}

static void
writerd_signal(int signo)
{
	(void) signo;
	
	writerd_exit();
}
