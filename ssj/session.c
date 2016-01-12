#include "ssj.h"
#include "session.h"

#include "remote.h"

struct session
{
	remote_t*    remote;
	bool         is_paused;
	unsigned int req_id;
	unsigned int rep_id;
};

session_t*
new_session(const char* hostname, int port)
{
	session_t* sess;

	sess = calloc(1, sizeof(session_t));
	if (!(sess->remote = connect_remote(hostname, port)))
		goto on_error;
	return sess;

on_error:
	free(sess);
	return NULL;
}