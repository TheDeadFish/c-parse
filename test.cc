#include <stdshit.h>
#include "c-parse.h"
const char progName[] = "test";

int main(int argc, char* argv[])
{
	cParse cp;  char* err = cp.load2(
		"test-in.cc", 0);
	cp.tokLst.print(stdout);
}
