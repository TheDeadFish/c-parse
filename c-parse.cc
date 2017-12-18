#include "stdshit.h"
#include "c-parse.h"

template <int N> struct Str8 { char len; char str[N];
	constexpr Str8(cch* in) : len(0), str{} {
		for(; in[len]; len++) str[len] = in[len]; } 
	cstr cStr() { return {str, len}; }
	bool cmp(cstr s) { if(len != s.slen) return false;
		for(int i = 0; i < len; i++) if(toLower(s[i])
			!= str[i]) return false; return true; }
};
	
Str8<3> ctokStr[] = {"", "", "", "", "", "", "", "@",
	",", "(", ")", "{", "}", "[", "]", ";", "?", ".", ":", "+",
	"-", "&", "|", "^", "~", "=", "!", "<", ">", "*", "/", "%",
	"#", "<<", ">>", "->", "->*", ".*", "...", "::", "++", "--",
	"&&", "||", "+=", "-=", "&=", "|=", "^=", "~=", "==", "!=",
	"<=", ">=", "*=", "/=", "%=", "<<=", ">>=" };

Str8<7> cppStr[] = { "define", "include", "undef",
	"ifdef", "ifndef", "if", "elif", "else", "endif",
	"error", "pragma", "line" };

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
	-2,-2,-2,-2,-2,-2,0,58,5,1,0,61,117,6,9,10,61,115,8,116,17,62,4,4,4,4,4,4,
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
	if(nspc()) return ' ';
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
	cstr str = ctokStr[value()].cStr();
	return str.slen ? str : cStr();
}

int cParse::Token::cppType(void)
{
	if(value() != CTOK_MACRO) return -1;
	cstr str = cStr().right(1);
	int type = 0;
	for(; type < ARRAYSIZE(cppStr); type++)
		if(cppStr[type].cmp(str)) break;
	return type;
}

int cParse::Token::compar(const Token& that)
{
	int diff = (vl&511) - (that.vl&511);
	if((diff)||(u8(value())
		>= CTOK_AT)) return diff;
	cstr x = that.cStr(); cstr y = cStr();
	return cStr().cmp(that.cStr());
}

void cParse::free(void)
{
	if(mustDelete)::free(base);
	::free(newLineList);
	::free(tokBase); ZINIT;
}

void cParse::load_(void* data)
{
	this->free();
	base = (char*)data;	
	char *src = base, *dst = base;
	for(;;) { char ch = RDI(src); switch(ch) {
		case '\r': if(*src == '\n') src++; ch = '\n';
		case '\n': if(dst[-1] == '\\') { dst--;
			xNextAlloc(newLineList,	nNewListList)
			= dst-base;	continue; } }
		WRI(dst, ch); if(!ch) break; }
	this->reset();
}

bool cParse::load(const char* name)
{
	this->free();
	auto file = loadFile(name, 1);
	if(!file.data) return false;
	load_(file.data);
	mustDelete = true;
	return true;
}

char* cParse::load2_(void* data, int flags)
{
	load_(data); auto tmp = parse(flags);
	if(!tmp.end_) { return (char*)tmp.data; }
	tokBase = tmp; tokLst = tmp; return NULL;
}

char* cParse::load2(const char* name, int flags)
{
	this->free();
	auto file = loadFile(name, 1);
	if(!file.data) return (char*)1;
	return load2_(file.data, flags);
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
NEXT_TOKEN2: Token token{curPos, ti}; 
	if(state.inMacro > 0) { if(state.inMacro < 5)
		state.inMacro++; token.vl |= 0x200; }
	VARFIX(token.vl);
	if(ti < 0) { if(state.inMacro) { state.inMacro = 0;
		token.vl = CTOK_ENDM; } return token;
	} token.vl &= -97; curPos++;

	switch(token.value())
	{
	case CTOK_LBR: if(state.inMacro == 4)
		state.inMacro = ~state.inMacro; break;
	case CTOK_RBR: if(state.inMacro < 0)
		state.inMacro = ~state.inMacro; break;
	case CTOK_WSPC: 
		if(ch == '$') { token.vl += CTOK_DOLAR; return token; }
		for(;; curPos++) { if((state.inMacro)&&(ch == '\n')) {
		state.inMacro = 0; token.vl = CTOK_ENDM; return token; }
		ti = cTokTab[ch = *curPos]; if(ti != CTOK_WSPC) {
		ti |= 0x100; goto NEXT_TOKEN2; } }

	case CTOK_DIV:
		if(*curPos == '/'){ curPos++; while(!is_one_of(
		*curPos, '\0', '\n')) curPos++; goto NEXT_TOKEN; }
		ei(*curPos == '*'){ while(*(++curPos) && !((RW(
			curPos) == 0x2F2A)&&(curPos += 2, 1)));
			goto NEXT_TOKEN; } break;
	
	case CTOK_MACRO:
		if(state.inMacro) { HASH: token.vl += 
			CTOK_HASH-CTOK_MACRO; return token; }
		for(char* pos = token.str; pos > base;) { 
			pos--; if(u8(*pos) > ' ') goto HASH;
			if(*pos == '\n') break; }
		while(is_one_of(cTokTab[u8(*curPos)]
			, CTOK_NAME, CTOK_NUM)) curPos++;
		state.inMacro = 1; token.setEnd(curPos); 
		return token;
	
	case CTOK_NUM: ch = '.'; if(0) { 
	case CTOK_NAME: ch = '$'; }
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

byte cParse::Parse_t::
	print(FILE* fp, byte prevTok)
{
	for(auto tok : *this) {
		if(tok.value() < 0) break;
		char ch = tok.getWs(prevTok);
		prevTok = tok.value();
		if(ch) fprintf(fp, "%c", ch);
		fprintf(fp, "%.*s",tok.getStr().prn());
	} return prevTok;
}

cstr cParse::Parse_t::text(void)
{
	if(chk() == false) return {0,0};
	int len = l().getStr().slen;
	return {f().str, l().str+len };
}

cstr cParse::Parse_t::nTerm(void)
{
	cstr str = text(); if(str) 
		*str.end() = 0; return str;
}

cParse::Parse_t cParse::Parse_t::cppBlock()
{
	Token* base = data; Token* cp = data;
	if(chk(cp)) { if(cp++->value() == CTOK_MACRO) {
		while(chk(cp) && (cp++->value() != CTOK_ENDM));
	} else {
		while(chk(cp) && (cp->value() != CTOK_MACRO)) cp++;
	}} data = cp; return {base, data};
}

int cParse::Parse_t::compar(Parse_t& that)
{
	int diff = count() - that.count();
	if(!diff) for(int i : Range(0, count())) {
		diff = (*this)[i].compar(that[i]);
		if(diff) break; } return diff;
}


struct BraceStack {
	byte stack[256]; 
	uint stackPos = 0;
	bool push(byte value);
	bool pop(byte value);
};

bool BraceStack::push(byte value)
{
	if(!is_one_of(value, CTOK_LBR,
	CTOK_LCBR, CPP_LQBR)) return false;
	if(stackPos >= 256) stackPos = -1;
	else { stack[stackPos] = value;
	stackPos++; } return true;
}

bool BraceStack::pop(byte value)
{
	if(!is_one_of(value, CTOK_RBR,
	CTOK_RCBR, CTOK_RQBR)) return false;
	if((!stackPos)||(stack[--stackPos] != 
	(value-1))){stackPos = -1; } return true;
}

cParse::Parse_t cParse::Parse_t::getArg()
{
	BraceStack stack; Token* base = data;
	for(int bracketLevel = 0; chk2();) {
	byte value = fi().value();
	if(!stack.push(value)) { if((!stack.stackPos)
	&&(is_one_of(value, CTOK_COMMA, CTOK_RBR))) return 
	{base, data-1}; stack.pop(value); }} return {0,0};
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

cParse::Parse_t cParse::Parse_t::tok(int token)
{
	while(chk() && (f().value() == token)) fi();
	auto* base = data;
	while(chk() && (f().value() != token)) fi();
	if(base == data) return {0,0}; return {base, data};
}

cParse::Token* cParse::Parse_t::chr(int token)
{
	for(auto* curPos = data; chk(curPos); curPos++)
		if(curPos->value() == token) return curPos;
	return NULL;
}

cParse::Parse_t cParse::Parse_t::splitL(int token)
{
	if(auto* ptr = chr(token)) { SCOPE_EXIT(data=ptr);
		return {data, ptr}; } return {0,0};
}

cParse::Parse_t cParse::Parse_t::splitR(int token)
{
	if(auto* ptr = chr(token)) { SCOPE_EXIT(data=ptr+1);
		return {data, ptr+1}; } return {0,0};
}

bool cParse::Block_t::init(cParse::Parse_t lst)
{
	return init_(lst);
}

cParse::Parse_t cParse::Block_t::
	init_(cParse::Parse_t& lst)
{
	bool ifState = false;
	while(1) { auto block = lst.cppBlock();
	NEXT_BLOCK:	if(!block.chk()) { if(!ifState)
		block = {0,0}; return block; }
		
		switch(block->cppType()) 
		{
		CPP_CASE_EI:
			if(!ifState) return block;
			ifState = false; break;
		CPP_CASE_EL:
			if(!ifState) return block;
		CPP_CASE_IF: {
			auto& chld = push_back(block);
			block = chld.init_(lst);
			ifState = true;
			goto NEXT_BLOCK; }
		}
		
		push_back(block);
	}
}

void cParse::Block_t::
	branch(int idx, bool taken)
{
	// locate bounds of if-endif
	int ifend = idx; while(data[ifend].
		tok->cppType() != CPP_ENDIF) ifend++;
	int ifbeg = idx; while(!CPP_RNG_IF(
		data[ifbeg].tok->cppType())) ifbeg--;
	
	// handle taken
	assert(taken == true);
	for(int i = ifbeg; i <= ifend; i++) {
		this->kill(i, i != idx); }
}
