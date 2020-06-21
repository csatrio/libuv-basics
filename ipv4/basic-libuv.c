#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "basic-libuv.h"

static init_cb_t init_cb = NULL;
static on_connect_cb_t on_connect_cb = NULL;
static on_read_cb_t on_read_cb = NULL;
static after_write_cb_t after_write_cb = NULL;
static cleanup_attachment_cb_t cleanup_attachment_cb = NULL;
static on_error_cb_t on_error_cb = NULL;
static allocation_cb_t allocation_cb = NULL;
static uv_loop_t *LOOP = NULL;

static inline void check_error(int code, const char *at)
{
    if (code != 0 && on_error_cb != NULL)
        on_error_cb(uv_err_name(code), uv_strerror(code), at);
}

static inline void free_handle_data(uv_handle_t *handle)
{
    if (handle->data != NULL)
    {
        free(handle->data);
        handle->data = NULL;
    }
}

static inline void free_connection(connection c)
{
    if (cleanup_attachment_cb != NULL && c->attachment != NULL)
    {
        cleanup_attachment_cb(c->attachment);
        c->attachment = NULL;
    }
    uv_close((uv_handle_t *)&c->async_handle, free_handle_data);
}

static void after_shutdown(uv_shutdown_t *req, int status)
{
    connection c = (connection)req->data;
    free(req);
    free_connection(c);
}

static inline void after_close(uv_handle_t *handle)
{
    connection c = (connection)handle->data;
    free(handle);
    free_connection(c);
}

static inline void after_write(uv_write_t *response, int status)
{
    write_req_t *wr = (write_req_t *)response;
    if (after_write_cb != NULL)
    {
        check_error(status, "after_write");
        connection c = wr->connection;
        after_write_cb(c, status);
    }
    free(response);
}

static inline void after_read(uv_stream_t *handle,
                              ssize_t nread,
                              const uv_buf_t *buf)
{
    int r;
    uv_shutdown_t *req;
    connection c = (connection)handle->data;

    if (nread <= 0 && buf->base != NULL)
        free(buf->base);

    if (nread == 0)
        return;

    if (nread < 0)
    {
        uv_unref((uv_handle_t *)handle);
        req = (uv_shutdown_t *)malloc(sizeof(*req));
        req->data = c;

        r = uv_shutdown(req, handle, after_shutdown);
        check_error(nread, "after_read uv_shutdown");
        return;
    }
    if (on_read_cb != NULL)
    {
        on_read_cb(c, nread, buf);
    }
}

static inline void alloc_cb(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
    buf->base = (char *)malloc(suggested_size);
    buf->len = suggested_size;
}

static void async_handler(uv_async_t *handle)
{
    connection c = (connection)handle->data;

    if (c->flag == FLAG_WRITE)
    {
        write_req_t *wr = (write_req_t *)malloc(sizeof(write_req_t));
        wr->connection = c;
        wr->buf = uv_buf_init(c->write_data, c->write_data_len);
        wr->req.data = c;
        uv_write(&wr->req, (uv_stream_t *)c->stream, &wr->buf, 1, after_write);
    }
    else if (c->flag == FLAG_DISCONNECT)
    {
        uv_close((uv_handle_t *)c->stream, after_close);
    }
}

static void on_connection(uv_stream_t *server, int status)
{
    uv_tcp_t *stream;
    int r;

    stream = (uv_tcp_t *)malloc(sizeof(uv_tcp_t));

    r = uv_tcp_init(LOOP, stream);
    check_error(r, "on_connection uv_tcp_init");

    stream->data = server;

    r = uv_accept(server, (uv_stream_t *)stream);
    check_error(r, "on_connection uv_accept");

    size_t conn_size = sizeof(connection_t);
    connection c = (connection)malloc(conn_size);
    memset(c, 0, conn_size);
    uv_async_init(LOOP, &c->async_handle, async_handler);
    c->flag = FLAG_WRITE;
    c->async_handle.data = c;
    c->stream = stream;
    stream->data = c;
    if (on_connect_cb != NULL)
    {
        on_connect_cb(c, status);
        r = uv_read_start((uv_stream_t *)stream, allocation_cb, after_read);
        check_error(r, "on_connection read_start");
    }
}

int run_server(const char *ip, int port,
               init_cb_t _init_cb,
               on_connect_cb_t _on_connect_cb,
               on_read_cb_t _on_read_cb,
               after_write_cb_t _after_write_cb,
               cleanup_attachment_cb_t _cleanup_attachment_cb,
               on_error_cb_t _on_error_cb,
               allocation_cb_t _allocation_cb)
{
    LOOP = uv_default_loop();
    init_cb = _init_cb;
    on_connect_cb = _on_connect_cb;
    on_read_cb = _on_read_cb;
    after_write_cb = _after_write_cb;
    cleanup_attachment_cb = _cleanup_attachment_cb;
    on_error_cb = _on_error_cb;
    allocation_cb = _allocation_cb == NULL ? alloc_cb : _allocation_cb;

    uv_tcp_t *tcp_server;
    struct sockaddr_in addr;
    int r;

    r = uv_ip4_addr(ip, port, &addr);
    assert(r == 0);

    tcp_server = (uv_tcp_t *)malloc(sizeof(*tcp_server));

    r = uv_tcp_init(LOOP, tcp_server);
    assert(r == 0);

    r = uv_tcp_bind(tcp_server, (const struct sockaddr *)&addr, 0);
    check_error(r, "uv_tcp_bind");

    r = uv_listen((uv_stream_t *)tcp_server, SOMAXCONN, on_connection);
    check_error(r, "uv_listen");

    if (init_cb != NULL)
    {
        init_cb();
    }

    r = uv_run(LOOP, UV_RUN_DEFAULT);
    assert(r == 0);

    return r;
}

void disconnect(connection c)
{
    connection con = (connection)c->async_handle.data;
    con->flag = FLAG_DISCONNECT;
    uv_async_send(&c->async_handle);
}

void write_simple(connection c, char *data)
{
    size_t len = strlen(data);
    c->write_data = data;
    c->write_data_len = len;
    c->flag = FLAG_WRITE;
    uv_async_send(&c->async_handle);
}

void write(connection c, char *data, size_t len)
{
    if (c->flag != FLAG_DISCONNECT)
    {
        c->write_data = data;
        c->write_data_len = len;
        c->flag = FLAG_WRITE;
        uv_async_send(&c->async_handle);
    }
}