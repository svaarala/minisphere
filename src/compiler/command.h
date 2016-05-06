#ifndef CELL__COMMAND_H__INCLUDED
#define CELL__COMMAND_H__INCLUDED

typedef struct command command_t;

typedef enum action
{
	ACTION_NONE,
	ACTION_EXPLODE,
	ACTION_INIT,
	ACTION_AUTHOR,
	ACTION_TITLE,
} action_t;

command_t* command_parse  (int argc, char* argv[]);
void       command_free   (command_t* cmd);
action_t   command_action (const command_t* cmd);

#endif // CELL__COMMAND_H__INCLUDED
