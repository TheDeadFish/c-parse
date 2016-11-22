#include "stdshit.h"
#include "c-parse.h"

const char ctokStr[][4] = {" ", "", "", "", "", "", "", "@",
	",", "(", ")", "{", "}", "[", "]", ";", "?", ".", ":", "+",
	"-", "&", "|", "^", "~", "=", "!", "<", ">", "*", "/", "%",
	"#", "<<", ">>", "->", "->*", ".*", "...", "::", "++", "--",
	"&&", "||", "+=", "-=", "&=", "|=", "^=", "~=", "==", "!=",
	"<=", ">=", "*=", "/=", "%=", "<<=", ">>=" };

REGCALL(1) std::pair<cch*,
	int> strLitType(cch* str)
{
	WORD ch = RW(str); str++;
	if(ch == 0x224C) return {str+1, 2};
	ei(u8(ch) == '"') return {str, 1};
	return {str, 0};
}

static const char cTokTab[] = {
	-1,-2,-2,-2,-2,-2,-2,-2,-2,0,0,-2,-2,0,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,
	-2,-2,-2,-2,-2,-2,0,58,5,1,-2,61,117,6,9,10,61,115,8,116,17,62,4,4,4,4,4,4,
	4,4,4,4,82,15,123,57,124,16,7,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
	3,3,3,3,13,-2,14,55,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
	3,11,118,12,56,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
	3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
	3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
	3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3 };

char cParse::Token::getWs(char prev)
{
	if((prev == CTOK_ENDM)||((value() == 
	CTOK_MACRO)&&(prev > 0))) return '\n';
	if(macro()) { return wspc() ? ' ' : '\0'; }
	if( is_one_of(value(), CTOK_NAME, CTOK_NUM)
	&& is_one_of(prev, CTOK_NAME, CTOK_NUM) )
		return ' ';
	if(is_one_of(prev, CTOK_LTH, CTOK_GTH) && 
	is_one_of(value(), prev, prev+(CTOK_SHL-CTOK_LTH)))
		return ' ';
	return '\0';
}

cstr cParse::Token::getStr(void)
{
	cch* str = ctokStr[value()];
	if(*str == '\0') return cStr(); 
	return {str, 1 + !!str[1] + !!str[2]};
}

void cParse::free(void)
{
	::free(base); ::free(newLineList);
	::free(tokBase); ZINIT;
}

bool cParse::load(const char* name)
{
	this->free();
	auto file = loadFile(name, 1);
	if(!file.data) return false;
	base = (char*)file.data;	
	char *src = base, *dst = base;
	for(;;) { char ch = RDI(src); switch(ch) {
		case '\r': if(*src == '\n') src++; ch = '\n';
		case '\n': if(dst[-1] == '\\') { dst--;
			xNextAlloc(newLineList,	nNewListList)
			= dst-base;	continue; } }
		WRI(dst, ch); if(!ch) break; }
	this->reset();
	return true;
}

char* cParse::load2(const char* name, int flags)
{
	if(!load(name)) return (char*)1;
	auto tmp = parse(flags);
	if(!tmp.end_) { return (char*)tmp.data; }
	tokBase = tmp; tokLst = tmp; return NULL;
}

std::pair<int,int> cParse::getLine(char* str)
{
	// initialize variables
	if(base == NULL) return {0,0};
	int* newLinePos = newLineList;
	int* newLineEnd = newLineList+nNewListList;
	char* curPos = base; int curLine = 0; 
	char* curLinePtr; char* newLineVal; 
	goto LOOP_BEGIN;
	
	for(;;) { // restore deleted new-lines
	while(curPos == newLineVal) {LOOP_BEGIN:  
		curLine++; curLinePtr = curPos;
		newLineVal = 0;	if(newLinePos < newLineEnd) {
			newLineVal = base + *newLinePos++; }}
	if((curPos == str)||(*curPos == '\0'))
		return {curLine, curPos-curLinePtr+1};
	if(*curPos++ == '\n') {
		curLine++; curLinePtr = curPos; }
	}
}

cParse::Token cParse::get(int flags)
{
	char* curPos = state.curPos; SCOPE_EXIT(state.curPos = curPos);
NEXT_TOKEN: char ch = *curPos; int ti = cTokTab[ch];
NEXT_TOKEN2: Token token{curPos, ti | 
	(state.inMacro << 8) }; VARFIX(token.vl);
	if(ti < 0) { if(state.inMacro) { state.inMacro = 0;
		token.vl = CTOK_ENDM; } return token;
	} token.vl &= -97; curPos++;

	switch(token.value())
	{
	case CTOK_WSPC: 
		for(;; curPos++) { if((state.inMacro)&&(ch == '\n')) {
		state.inMacro = 0; token.vl = CTOK_ENDM; return token; }
		ti = cTokTab[ch = *curPos]; if(ti != CTOK_WSPC) {
		ti |= 0x200; goto NEXT_TOKEN2; } }

	case CTOK_DIV:
		if(*curPos == '/'){ curPos++; while(!is_one_of(
		*curPos, '\0', '\n')) curPos++; goto NEXT_TOKEN; }
		ei(*curPos == '*'){ while(*(++curPos) && !((RW(
			curPos) == 0x2F2A)&&(curPos += 2, 1)));
			goto NEXT_TOKEN; } break;
	
	case CTOK_MACRO:
		if(state.inMacro) { token.vl += 
			CTOK_HASH-CTOK_MACRO; return token; }
		while(u8(*curPos) > ' ') curPos++;
		state.inMacro = 1; token.setEnd(curPos); 
		return token;
	
	case CTOK_NUM: ch = '.';
	case CTOK_NAME:
		for(;(*curPos == ch)||is_one_of(cTokTab[(*curPos)]
			&31, CTOK_NAME, CTOK_NUM); curPos++);
		 if(*curPos == '"') { auto type = strLitType(
			token.str);	if(type.second != 0) { ch =
				'"'; curPos += 1; goto WIDESTR; } }
		token.setEnd(curPos); return token;
		
	case CTOK_STR: case CTOK_CHR:
		#define CPNT_RDCH2_ if(!(ch2 = *curPos)) { \
			token.vl = CTOK_BAD; return token; } curPos++;
		WIDESTR: // get length of string
		{ char* pe = NULL; NEXTSTR: char* sb = curPos;
		for(byte ch2;;) { CPNT_RDCH2_; if(ch2 ==
		'\\') { CPNT_RDCH2_; } ei(ch2 == ch) break; }
		if(pe == NULL) pe = curPos; else { pe--;
			while(sb < curPos) WRI(pe, RDI(sb)); }
			
		// locate next string
		if(flags & FLAG_STRCOMBINE) {
			byte ch3 = !state.inMacro ? '\n' : ' ';
			for(byte ch2;; curPos++) { ch2 = *curPos;
				if(ch2 == '"') { curPos++; goto NEXTSTR; }
				if(!is_one_of(ch2, ' ', '\t', ch3)) break; }
			
				
				
				
			//while(pe < curPos) WRI(pe, ' ');
		} token.setEnd(pe); return token; }
		
	case CTOK_SUB:
		if(*curPos == '>') { asm("lea 2(%1), %0"
		: "=r"(curPos) : "r"(token.str)); token.vl
		+= CTOK_DREF-CTOK_SUB; VARFIX(token.vl);
		if(*curPos == '*') { curPos++; token.vl +=
		CTOK_DREF_PM-CTOK_DREF; } return token; } break;
		
	case CTOK_DOT:
		if((curPos[0] == '.')&&(curPos[1] == '.')) {
		curPos += 2; token.vl += CTOK_DOT3-CTOK_DOT;
		return token; } break;
	}
	
	// basic operators
	if((ti & 0x40)&&(*curPos == ch)) { curPos++;
		if(is_one_of(ch, '<', '>')) { token.vl 
			+= CTOK_SHL-CTOK_LTH; goto IS_SHIFT; }
		token.vl += CTOK_SCOPE-CTOK_COLON;
	} ei(ti & 0x20) { IS_SHIFT:	if(*curPos == '=') {
		curPos++; token.vl += CTOK_MODEQU-CTOK_ADD; }
	} return token;
}

cParse::Parse_t cParse::parse(int flags)
{
	Token* token = NULL; int count = 0; reset(); 
	while(1) { auto tok = get(flags);
		xNextAlloc(token, count) = tok;
		if(tok.value() == CTOK_EOF) break;
		if(tok.value() < CTOK_EOF) { ::free(token);
			return {(Token*)tok.str, (Token*)0}; }
	} return {token, count-1};
}

#define REF_PTR(t, n1, n2) t n1 = n2; SCOPE_EXIT(n2 = n1);

cstr cParse::Parse_t::nTerm(void)
{
	if(chk() == false) return NULL;
	char *base = data->str, *end = end_->str;
	while((end > base)&&(u8(end[-1]) <= ' '))
		end--; *end = 0; return {base, end};
}

cParse::Parse_t cParse::Parse_t::getArg()
{
	Token* base = data;
	for(int bracketLevel = 0; chk2();)
	switch(fi().value()) {
	case CTOK_LBR: bracketLevel++; break;	
	case CTOK_RBR: bracketLevel--;
	case CTOK_COMMA: if(bracketLevel > 0) break;
		return {base, data-1};
	} return {0,0};
}

bool cParse::Parse_t::getArgs(
	xVector<Parse_t>& args)
{
	args.setCount(0);
	if(chk() && fi().value() == CTOK_LBR)
	while(1) { auto arg = getArg();
		if(arg == NULL) break;
		args.push_back(arg);
		if(arg.end()->value() == CTOK_RBR)
			return true; 
	} return false;
}

cstr cParse::Parse_t::getCall(
	xVector<Parse_t>& args)
{
	if(!chk()) return {0, 0};
	if(f().value() != CTOK_NAME)	
		return {(char*)data->str, 0};
	cstr ret = fi().cStr();
	if(!getArgs(args))
		return {(char*)data->str, 0};
	return ret;
}
