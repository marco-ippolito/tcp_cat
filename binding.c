#include <assert.h>
#include <bare.h>
#include <js.h>
#include <stdlib.h>
#include <uv.h>

typedef struct {
  char *data;
  js_deferred_t *deferred;
  js_env_t *env;
} request_data;

typedef utf8_t tcp_cat_host_t[38 + 1]; // 4*8 + 7 ipv6
typedef utf8_t tcp_cat_data_t[65536];  // 64KB for simplicity

static void
allocator (uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
  buf->base = malloc(suggested_size);
  buf->len = suggested_size;
}

static void
on_close (uv_handle_t *handle) {
  free(handle);
}

static void
on_read (uv_stream_t *tcp, ssize_t nread, const uv_buf_t *buf) {

  if (nread >= 0) {
    int err;
    request_data *request_data = tcp->data;

    js_handle_scope_t *scope;
    err = js_open_handle_scope(request_data->env, &scope);
    assert(err == 0);

    js_value_t *result;
    err = js_create_string_utf8(request_data->env, (utf8_t *) buf->base, nread, &result);
    assert(err == 0);

    err = js_resolve_deferred(request_data->env, request_data->deferred, result);
    assert(err == 0);
    uv_close((uv_handle_t *) tcp, on_close);
  }
  free(buf->base);
}

static void
on_write_end (uv_write_t *req, int status) {
  assert(status == 0);
  req->handle->data = req->data;
  uv_read_start(req->handle, allocator, on_read);
}

static void
on_connect (uv_connect_t *connection, int status) {
  assert(status == 0);

  uv_stream_t *stream = connection->handle;

  request_data *request_data = connection->data;

  char *body = (char *) request_data->data;

  uv_buf_t wrbuf = uv_buf_init(body, sizeof(body));
  wrbuf.len = strlen(body);
  wrbuf.base = body;

  uv_write_t *req = malloc(sizeof(uv_write_t));
  req->data = request_data;
  uv_write(req, stream, &wrbuf, 1, on_write_end);
}

static js_value_t *
tcp_cat (js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 3;
  js_value_t *argv[3];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);

  assert(argc == 3);

  tcp_cat_host_t host;
  err = js_get_value_string_utf8(env, argv[0], host, sizeof(tcp_cat_host_t), NULL);
  assert(err == 0);

  int32_t port;
  err = js_get_value_int32(env, argv[1], &port);
  assert(err == 0);

  tcp_cat_data_t data;
  err = js_get_value_string_utf8(env, argv[2], data, sizeof(tcp_cat_data_t), NULL);
  assert(err == 0);

  uv_loop_t *loop;
  js_get_env_loop(env, &loop);

  uv_tcp_t *socket = malloc(sizeof(uv_tcp_t));
  err = uv_tcp_init(loop, socket);
  assert(err == 0);

  struct sockaddr_in dest;
  err = uv_ip4_addr((char *) host, port, &dest);
  assert(err == 0);

  js_deferred_t *deferred;
  js_value_t *promise;
  err = js_create_promise(env, &deferred, &promise);
  assert(err == 0);

  uv_connect_t *connection = malloc(sizeof(uv_connect_t));
  request_data *rq_data = malloc(sizeof(request_data));
  rq_data->deferred = deferred;       // store callback
  rq_data->data = (char *) data;      // store request body
  rq_data->env = env;                 // store env

  connection->data = rq_data;

  err = uv_tcp_connect(connection, socket, (const struct sockaddr *) &dest, on_connect);
  assert(err == 0);
  return promise;
}

static js_value_t *
init (js_env_t *env, js_value_t *exports) {
  int err;

  {
    js_value_t *fn;
    err = js_create_function(env, "tcpCat", -1, tcp_cat, NULL, &fn);
    assert(err == 0);

    err = js_set_named_property(env, exports, "tcpCat", fn);
    assert(err == 0);
  }

  return exports;
}

BARE_MODULE(tcp_cat, init)
