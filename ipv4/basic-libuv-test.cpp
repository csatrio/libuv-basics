#include "basic-libuv.c"

static void _init()
{
    //print("init");
}

static void _on_connect(connection c, int status)
{
    //print("on_connect_child_callback");
}

static void _on_read(connection c, ssize_t nread, const uv_buf_t *buf)
{
    //printf("on_read, buf_pointer:%p\n", buf->base);
    write(c, buf->base, nread);
}

static void _after_write(connection c, int status)
{
    //printf("after_write[%d], pointer:%p\n", status, c->write_data);
    free(c->write_data);
}

static void _cleanup_attachment(void *attachment)
{
    //print("cleanup_attachment");
}

static void _on_error(const char *error_name, const char *error_message, const char *at)
{
    printf("%s: %s -- at:%s\n", error_name, error_message, at);
}

int main()
{
    printf("Starting server at ip %s, and port %d\n", "0.0.0.0", 8000);
    run_server("0.0.0.0", 8000, _init, _on_connect, _on_read, _after_write, _cleanup_attachment, _on_error, NULL);
}