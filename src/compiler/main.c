#include "cell.h"

#include "command.h"
#include "project.h"

static void print_cell_quote (void);

int
main(int argc, char* argv[])
{
	command_t*   command;
	path_t*      path;
	project_t*   project;
	
	if (!(command = command_parse(argc, argv)))
		return EXIT_FAILURE;

	path = path_new("./");
	project = project_open(path);

	switch(command_action(command)) {
	case ACTION_INIT:
		if (project == NULL) {
			project = project_new(path, "Untitled");
			project_save(project);
			printf("cell: create game manifest `%s`\n", path_cstr(path));
		}
		else {
			printf("cell: game.s2gm already exists here, nothing to do\n");
		}
		break;
	case ACTION_EXPLODE:
		print_cell_quote();
		break;
	case ACTION_TITLE:
		project_set_title(project, "maggie ate it");
		project_save(project);
		break;
	default:
		printf("cell: internal error, Cell probably blew up\n");
		break;
	}
	
	project_free(project);
	return EXIT_SUCCESS;
}

static void
print_cell_quote(void)
{
	srand((unsigned int)time(NULL));

	// yes, these are entirely necessary. :o)
	static const char* const MESSAGES[] =
	{
		"This is the end for you!",
		"Very soon, I am going to explode. And when I do...",
		"Careful now! I wouldn't attack me if I were you...",
		"I'm quite volatile, and the slightest jolt could set me off!",
		"One minute left! There's nothing you can do now!",
		"If only you'd finished me off a little bit sooner...",
		"Ten more seconds, and the Earth will be gone!",
		"Let's just call our little match a draw, shall we?",
		"Watch out! You might make me explode!",
	};

	printf("Cell seems to be going through some sort of transformation...\n");
	printf("He's pumping himself up like a balloon!\n\n");
	printf("    Cell says:\n    \"%s\"\n", MESSAGES[rand() % (sizeof MESSAGES / sizeof(const char*))]);
}
