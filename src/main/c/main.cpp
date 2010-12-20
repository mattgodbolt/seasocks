#include "server.h"

using namespace SeaSocks;

int main(int argc, const char* argv[]) {
	Server server;
	server.serve(9090);
	return 0;
}

