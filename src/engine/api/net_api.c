#include "minisphere.h"
#include "api/net_api.h"

#include "api.h"
#include "sockets.h"

static duk_ret_t js_new_Server               (duk_context* ctx);
static duk_ret_t js_Server_finalize          (duk_context* ctx);
static duk_ret_t js_Server_close             (duk_context* ctx);
static duk_ret_t js_Server_accept            (duk_context* ctx);
static duk_ret_t js_new_Socket               (duk_context* ctx);
static duk_ret_t js_Socket_finalize          (duk_context* ctx);
static duk_ret_t js_Socket_get_bytesReceived (duk_context* ctx);
static duk_ret_t js_Socket_get_connected     (duk_context* ctx);
static duk_ret_t js_Socket_get_remoteAddress (duk_context* ctx);
static duk_ret_t js_Socket_get_remotePort    (duk_context* ctx);
static duk_ret_t js_Socket_close             (duk_context* ctx);
static duk_ret_t js_Socket_pipe              (duk_context* ctx);
static duk_ret_t js_Socket_read              (duk_context* ctx);
static duk_ret_t js_Socket_readString        (duk_context* ctx);
static duk_ret_t js_Socket_unpipe            (duk_context* ctx);
static duk_ret_t js_Socket_write             (duk_context* ctx);

duk_ret_t
dukopen_net(duk_context* ctx)
{
	initialize_sockets();
	
	register_api_type(ctx, "Server", js_Server_finalize);
	register_api_method(ctx, "Server", "accept", js_Server_accept);
	register_api_method(ctx, "Server", "close", js_Server_close);

	register_api_type(ctx, "Socket", js_Socket_finalize);
	register_api_prop(ctx, "Socket", "bytesReceived", js_Socket_get_bytesReceived, NULL);
	register_api_prop(ctx, "Socket", "connected", js_Socket_get_connected, NULL);
	register_api_prop(ctx, "Socket", "remoteAddress", js_Socket_get_remoteAddress, NULL);
	register_api_prop(ctx, "Socket", "remotePort", js_Socket_get_remotePort, NULL);
	register_api_method(ctx, "Socket", "close", js_Socket_close);
	register_api_method(ctx, "Socket", "pipe", js_Socket_pipe);
	register_api_method(ctx, "Socket", "read", js_Socket_read);
	register_api_method(ctx, "Socket", "readString", js_Socket_readString);
	register_api_method(ctx, "Socket", "unpipe", js_Socket_unpipe);
	register_api_method(ctx, "Socket", "write", js_Socket_write);

	duk_function_list_entry functions[] = {
		{ "Server", js_new_Server, DUK_VARARGS },
		{ "Socket", js_new_Socket, DUK_VARARGS },
		{ NULL, NULL, 0 }
	};
	duk_push_object(ctx);
	duk_put_function_list(ctx, -1, functions);
	return 1;
}

duk_ret_t
dukclose_net(duk_context* ctx)
{
	shutdown_sockets();
	return 0;
}

static duk_ret_t
js_new_Server(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	int port = duk_require_int(ctx, 0);
	int max_backlog = n_args >= 2 ? duk_require_int(ctx, 1) : 16;

	socket_t* socket;

	if (max_backlog <= 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "Server(): backlog size must be greater than zero (got: %i)", max_backlog);
	if (socket = listen_on_port(NULL, port, 1024, max_backlog))
		duk_push_sphere_obj(ctx, "Server", socket);
	else
		duk_push_null(ctx);
	return 1;
}

static duk_ret_t
js_Server_finalize(duk_context* ctx)
{
	socket_t*   socket;

	socket = duk_require_sphere_obj(ctx, 0, "Server");
	free_socket(socket);
	return 0;
}

static duk_ret_t
js_Server_accept(duk_context* ctx)
{
	socket_t* new_socket;
	socket_t* socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "Server");
	duk_pop(ctx);
	if (socket == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Server:accept(): socket has been closed");
	new_socket = accept_next_socket(socket);
	if (new_socket)
		duk_push_sphere_obj(ctx, "Socket", new_socket);
	else
		duk_push_null(ctx);
	return 1;
}

static duk_ret_t
js_Server_close(duk_context* ctx)
{
	socket_t*   socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "Server");
	duk_push_null(ctx); duk_put_prop_string(ctx, -2, "\xFF" "udata");
	duk_pop(ctx);
	if (socket != NULL)
		free_socket(socket);
	return 0;
}

static duk_ret_t
js_new_Socket(duk_context* ctx)
{
	const char* hostname = duk_require_string(ctx, 0);
	int port = duk_require_int(ctx, 1);

	socket_t*   socket;

	if ((socket = connect_to_host(hostname, port, 1024)) != NULL)
		duk_push_sphere_obj(ctx, "Socket", socket);
	else
		duk_push_null(ctx);
	return 1;
}

static duk_ret_t
js_Socket_finalize(duk_context* ctx)
{
	socket_t* socket;

	socket = duk_require_sphere_obj(ctx, 0, "Socket");
	free_socket(socket);
	return 0;
}

static duk_ret_t
js_Socket_get_bytesReceived(duk_context* ctx)
{
	// get Socket:bytesReceived
	// Returns the number of bytes in the socket's receive buffer.

	socket_t* socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "Socket");
	
	if (socket == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "socket is closed");
	duk_push_number(ctx, peek_socket(socket));
	return 1;
}

static duk_ret_t
js_Socket_get_connected(duk_context* ctx)
{
	socket_t* socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "Socket");
	duk_pop(ctx);
	if (socket != NULL)
		duk_push_boolean(ctx, is_socket_live(socket));
	else
		duk_push_false(ctx);
	return 1;
}

static duk_ret_t
js_Socket_get_remoteAddress(duk_context* ctx)
{
	socket_t* socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "Socket");
	duk_pop(ctx);
	if (socket == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "socket is closed");
	if (!is_socket_live(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "socket is not connected");
	duk_push_string(ctx, get_socket_host(socket));
	return 1;
}

static duk_ret_t
js_Socket_get_remotePort(duk_context* ctx)
{
	socket_t* socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "Socket");
	duk_pop(ctx);
	if (socket == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "socket is closed");
	if (!is_socket_live(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "socket is not connected");
	duk_push_int(ctx, get_socket_port(socket));
	return 1;
}

static duk_ret_t
js_Socket_close(duk_context* ctx)
{
	// Socket:close();
	// Closes the socket, after which no further I/O may be performed
	// with it.

	socket_t* socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "Socket");
	duk_push_null(ctx); duk_put_prop_string(ctx, -2, "\xFF" "udata");
	duk_pop(ctx);
	if (socket != NULL)
		free_socket(socket);
	return 0;
}

static duk_ret_t
js_Socket_pipe(duk_context* ctx)
{
	// Socket:pipe(destSocket);
	// Pipes all data received by a socket into another socket.
	// Arguments:
	//     destSocket: The Socket into which to pipe received data.

	socket_t* dest_socket;
	socket_t* socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "Socket");
	duk_pop(ctx);
	dest_socket = duk_require_sphere_obj(ctx, 0, "Socket");
	if (socket == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "socket is closed");
	if (dest_socket == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "destination socket is closed");
	pipe_socket(socket, dest_socket);

	// return destination socket (enables pipe chaining)
	duk_dup(ctx, 0);
	return 1;
}

static duk_ret_t
js_Socket_unpipe(duk_context* ctx)
{
	// Socket:unpipe();
	// Undoes a previous call to pipe(), reverting the socket to normal
	// behavior.

	socket_t* socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "Socket");
	duk_pop(ctx);
	if (socket == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:pipe(): socket has been closed");
	pipe_socket(socket, NULL);
	return 0;
}

static duk_ret_t
js_Socket_read(duk_context* ctx)
{
	// Socket:read(numBytes);
	// Reads from a socket, returning the data in an ArrayBuffer.
	// Arguments:
	//     numBytes: The maximum number of bytes to read.

	void*      buffer;
	size_t     bytes_read;
	duk_size_t num_bytes;
	socket_t*  socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "Socket");
	duk_pop(ctx);
	num_bytes = duk_require_uint(ctx, 0);
	if (socket == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:read(): socket has been closed");
	if (!is_socket_live(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:read(): socket is not connected");
	buffer = duk_push_fixed_buffer(ctx, num_bytes);
	bytes_read = read_socket(socket, buffer, num_bytes);
	duk_push_buffer_object(ctx, -1, 0, bytes_read, DUK_BUFOBJ_ARRAYBUFFER);
	return 1;
}

static duk_ret_t
js_Socket_readString(duk_context* ctx)
{
	// Socket:read(numBytes);
	// Reads data from a socket and returns it as a UTF-8 string.
	// Arguments:
	//     numBytes: The maximum number of bytes to read.

	uint8_t*  buffer;
	size_t    num_bytes;
	socket_t* socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "Socket");
	duk_pop(ctx);
	num_bytes = duk_require_uint(ctx, 0);
	if (socket == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:readString(): socket has been closed");
	if (!is_socket_live(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:readString(): socket is not connected");
	read_socket(socket, buffer = malloc(num_bytes), num_bytes);
	duk_push_lstring(ctx, (char*)buffer, num_bytes);
	free(buffer);
	return 1;
}

static duk_ret_t
js_Socket_write(duk_context* ctx)
{
	// Socket:write(data);
	// Writes data into the socket.
	// Arguments:
	//     data: The data to write; this can be either a JS string or an
	//           ArrayBuffer.

	const uint8_t* payload;
	socket_t*      socket;
	duk_size_t     write_size;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "Socket");
	duk_pop(ctx);
	if (duk_is_string(ctx, 0))
		payload = (uint8_t*)duk_get_lstring(ctx, 0, &write_size);
	else
		payload = duk_require_buffer_data(ctx, 0, &write_size);
	if (socket == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:write(): socket has been closed");
	if (!is_socket_live(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:write(): socket is not connected");
	write_socket(socket, payload, write_size);
	return 0;
}
