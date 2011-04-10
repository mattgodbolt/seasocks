#include "tinytest.h"

void test_placeholder() {
	ASSERT("TODO: Some real tests", true);
}

int main(int argc, const char* argv[]) {
	RUN(test_placeholder);
	return TEST_REPORT();
}
 
