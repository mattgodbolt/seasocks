#include <event.h>
#include <evhttp.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <string>

const int port = 9090;
const char* serverRoot = "./src/web";

void handle_moo(evhttp_request *req, void *data) {
	evbuffer *out = evbuffer_new();

	evbuffer_add_printf(out, "Moo!");

	evhttp_send_reply(req, HTTP_OK, "OK", out);
	evbuffer_free(out);
}

void handle_static(evhttp_request *req, void *data) {
	std::string pathName(serverRoot);
	pathName.append(req->uri);
	if (pathName[pathName.length()-1] == '/') {
		pathName.append("index.html");
	}

	int fd = ::open(pathName.c_str(), O_RDONLY);
	evbuffer *out = evbuffer_new();
	struct stat fileStat;
	if (fd == -1 || fstat(fd, &fileStat) == -1) {
		evbuffer_add_printf(out, "Can't open %s (%d)", pathName.c_str(), errno);
		evhttp_send_reply(req, HTTP_NOTFOUND, "Not found", out);
		evbuffer_free(out);
		return;
	}
	int bytesRead = evbuffer_read(out, fd, static_cast<int>(fileStat.st_size));
	::close(fd);
	evhttp_send_reply(req, HTTP_OK, "OK", out);
	evbuffer_free(out);
}

int main(int argc, const char* argv[]) {
	event_base* ev_base = event_base_new();
	evhttp* ev_http = evhttp_new(ev_base);

	if (evhttp_bind_socket(ev_http, "0.0.0.0", port) == -1) {
		printf("fail: evhttp_bind_socket\n");
		return -1;
	}

	evhttp_set_cb(ev_http, "/moo", handle_moo, 0);
	evhttp_set_gencb(ev_http, handle_static, 0);

	printf("Listening on http://localhost:%d/\n", port);

	return event_base_dispatch(ev_base);;
}
