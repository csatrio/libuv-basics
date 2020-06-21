#include "uv.h"

#define print(arg) printf("%s\n", arg);
#define debug(data) fprintf(stderr, "%s\n", data);
#define print_error(code) \
    if (code != 0)        \
        fprintf(stderr, "%s: %s\n", uv_err_name(code), uv_strerror(code));

short FLAG_WRITE = 1;
short FLAG_DISCONNECT = 2;

typedef struct
{
    uv_tcp_t *stream;
    uv_async_t async_handle;
    short flag;
    void *attachment;
    char *write_data;
    size_t write_data_len;
} connection_t;

typedef struct
{
  uv_write_t req;
  uv_buf_t buf;
  connection_t *connection;
} write_req_t;

typedef connection_t *connection;
typedef void (*init_cb_t)();
typedef void (*on_connect_cb_t)(connection _connection, int status);
typedef void (*on_read_cb_t)(connection _connection, ssize_t nread, const uv_buf_t *buf);
typedef void (*after_write_cb_t)(connection _connection, int status);
typedef void (*cleanup_attachment_cb_t)(void *attachment);
typedef void (*on_error_cb_t)(const char *error_name, const char *error_message, const char *at);
typedef void (*allocation_cb_t)(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);

int run_server(const char *ip, int port,
               init_cb_t init_cb,
               on_connect_cb_t on_connect_cb,
               on_read_cb_t on_read_cb,
               after_write_cb_t after_write_cb,
               cleanup_attachment_cb_t cleanup_attachment_cb,
               on_error_cb_t _on_error_cb,
               allocation_cb_t _allocation_cb);
void disconnect(connection _connection);
void write_simple(connection _connection, char *data);
void write(connection _connection, char *data, size_t len);