#include "seasocks/printflogger.h"
#include "seasocks/server.h"
#include <boost/shared_ptr.hpp>

using namespace SeaSocks;

int main(int argc, const char* argv[]) {
	if (argc != 3) {
		std::cerr << "Usage: serve ROOT PORT" << std::endl;
		return 1;
	}

	boost::shared_ptr<Logger> logger(new PrintfLogger(Logger::Level::INFO));
	Server(logger).serve(argv[1], atoi(argv[2]));
	return 0;
}
