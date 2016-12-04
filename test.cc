#include <stdshit.h>
#include "c-parse.h"
const char progName[] = "test";

void token_out(cParse::Parse_t lst)
{
	byte prevTok = 0;
	for(auto tok : lst) {
		if(tok.value() < 0) break;
		char ch = tok.getWs(prevTok);
		prevTok = tok.value();
		if(ch) printf("%c", ch);
		printf("%.*s",tok.getStr().prn());
	}
}

int main(int argc, char* argv[])
{
	cParse cp;  char* err = cp.load2(
		"c-parse.cc", cp.FLAG_STRCOMBINE);
	if(err != NULL) {
		printf("failed to load at line: %d,%d\n", 
			cp.getLine(err)); return 0; }
	token_out(cp.tokLst);
}
