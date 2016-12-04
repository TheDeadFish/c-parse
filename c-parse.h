#ifndef _C_PARSE_H_
#define _C_PARSE_H_

enum {
	// string blocks
	CTOK_BAD = -2, CTOK_EOF, CTOK_WSPC, 
	CTOK_MACRO, CTOK_ENDM, CTOK_NAME, 
	CTOK_NUM, CTOK_STR, CTOK_CHR,

	// operators
	CTOK_AT, CTOK_COMMA, CTOK_LBR, CTOK_RBR,
	CTOK_LCBR, CTOK_RCBR, CPP_LQBR, CTOK_RQBR,
	CTOK_SEMCOL, CTOK_QMRK, CTOK_DOT, CTOK_COLON,
	CTOK_ADD, CTOK_SUB, CTOK_AND, CTOK_OR, CTOK_XOR,
	CTOK_NOT, CTOK_EQU, CTOK_NEQ, CTOK_LTH, CTOK_GTH,
	CTOK_MUL, CTOK_DIV, CTOK_MOD,
	
	// derived operators
	CTOK_HASH, CTOK_SHL, CTOK_SHR, CTOK_DREF, 
	CTOK_DREF_PM, CTOK_REF_PM, CTOK_DOT3,
	CTOK_SCOPE, CTOK_INC, CTOK_DEC,
	CTOK_LAND, CTOK_LOR, CTOK_MODEQU,
};

extern const char ctokStr[][4] ;

struct cParse
{
	enum { FLAG_STRCOMBINE = 1 };

	struct Token { char* str; int vl; 
		char value() { return vl & 255; }
		bool nspc() { return !(~vl&768); }
		bool wspc() { return vl & 512; }
		cstr cStr() { return {str, vl>>10}; }
		void setEnd(char* ep) { vl
			|= (PTRDIFF(ep,str) << 10); }	
		char getWs(char value);
		cstr getStr(void); 
		void as(int index) { str 
			+= index; vl -= index << 9; }
	};
	
	// high level parsing
	struct Parse_t : xRngPtr<Token>{ 
		using xRngPtr<Token>::xRngPtr;
		
		Parse_t getArg();
		bool getArgs(xVector<Parse_t>& args);
		cstr getCall(xVector<Parse_t>& args);
		cstr nTerm(void);
		byte valAt(int i = 0) { return chk(data+i)
			? data->value() : CTOK_EOF; }
	};

	// load/init initerface
	cParse() : base(0), newLineList(0), tokBase(0) {}
	~cParse() { this->free(); } void free(void);
	void reset(void) { state = {base, 0}; }
	bool load(const char* name);
	char* load2(const char* name, int flags);
	
	// parsing interface
	Token get(int flags = 0); 
	std::pair<int,int> getLine(char* str);
	Parse_t parse(int flags = 0);
	
	// file info
	Token* tokBase; Parse_t tokLst;
	char* base;	int* newLineList;
	int nNewListList; struct State {
		char* curPos; char inMacro; }; 
	State state;
};

#endif
