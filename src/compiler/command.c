#include "cell.h"
#include "command.h"

struct command
{
	action_t action;
};

static void print_banner (bool want_copyright, bool want_deps);
static void print_usage  (void);

static const char* const ACTION_NAMES[] =
{
	"explode",
	"init",
	"author",
	"title",
};

command_t*
command_parse(int argc, char* argv[])
{
	command_t* cmd;
	int        num_actions;
	
	int i;

	cmd = calloc(1, sizeof(command_t));
	num_actions = sizeof(ACTION_NAMES) / sizeof(ACTION_NAMES[0]);

	if (argc >= 2) {
		for (i = 0; i < num_actions; ++i) {
			if (strcmp(argv[1], ACTION_NAMES[i]) == 0)
				cmd->action = (action_t)(i + 1);
		}
		if (cmd->action == ACTION_NONE) {
			printf("cell: unrecognized Cell command `%s`\n", argv[1]);
			goto on_error;
		}
	}
	else {
		print_usage();
		return NULL;
	}

	return cmd;

on_error:
	free(cmd);
	return NULL;
}

void
command_free(command_t* cmd)
{
	free(cmd);
}

action_t
command_action(const command_t* cmd)
{
	return cmd->action;
}

static void
print_banner(bool want_copyright, bool want_deps)
{
	printf("Cell %s Sphere Project Prep Tool (%s)\n", VERSION_NAME, sizeof(void*) == 8 ? "x64" : "x86");
	if (want_copyright) {
		printf("a programmable build engine for Sphere games\n");
		printf("(c) 2015-2016 Fat Cerberus\n");
	}
	if (want_deps) {
		printf("\n");
		printf("   Duktape: v%ld.%ld.%ld\n", DUK_VERSION / 10000, DUK_VERSION / 100 % 100, DUK_VERSION % 100);
		printf("      zlib: v%s\n", zlibVersion());
	}
}

static void
print_usage(void)
{
	print_banner(true, false);
	printf("\n");
	printf("USAGE:\n");
	printf("   cell -b <out-dir> [options] [target]\n");
	printf("   cell -p <out-file> [options] [target]\n");
	printf("\n");
	printf("OPTIONS:\n");
	printf("       --in-dir    Set the input directory. Without this option, Cell will   \n");
	printf("                   look for sources in the current working directory.        \n");
	printf("   -b, --build     Build an unpackaged Sphere game distribution.             \n");
	printf("   -p, --package   Build a Sphere package (.spk).                            \n");
	printf("   -d, --debug     Include a debugging map in the compiled game which maps   \n");
	printf("                   compiled assets to their corresponding source files.      \n");
	printf("       --version   Print the version number of Cell and its dependencies.    \n");
	printf("       --help      Print this help text.                                     \n");
}
