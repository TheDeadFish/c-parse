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
	CTOK_SCOPE, CTOK_INC, CTOK_DEC, CTOK_LAND,
	CTOK_LOR, CTOK_DOLAR, CTOK_MODEQU,
};

enum {
	CPP_DEFINE, CPP_INCLUDE, CPP_UNDEF, CPP_IFDEF,
	CPP_IFNDEF, CPP_IF, CPP_ELIF, CPP_ELSE, CPP_ENDIF,
	CPP_ERROR, CPP_PRAGMA, CPP_LINE, CPP_OTHER };
#define CPP_RNG_IF(x) inRng(x, CPP_IFDEF, CPP_IFNDEF)
#define CPP_CASE_IF case CPP_IFDEF ... CPP_IFNDEF
#define CPP_CASE_EL case CPP_ELSE: case CPP_ELIF
#define CPP_CASE_EI case CPP_ENDIF

struct cParse
{
	enum { FLAG_STRCOMBINE = 1 };

	struct Token { char* str; int vl; 
		char value() const { return vl & 255; }
		bool nspc() const { return !(~vl&768); }
		bool wspc() const { return vl & 256; }
		cstr cStr() const { return {str, vl>>10}; }
		void setEnd(char* ep) { vl
			|= (PTRDIFF(ep,str) << 10); }	
		char getWs(char value);
		cstr getStr(void); 
		void as(int index) { str 
			+= index; vl -= index << 10; }
		int cppType();
		
		int compar(const Token& that);
	};
	
	// high level parsing
	struct Parse_t : xRngPtr<Token>{ 
		using xRngPtr<Token>::xRngPtr;
		Parse_t cppBlock();
		int compar(Parse_t& that);
		
		
		// 
		byte print(FILE* fp, byte pt = 0);
		cstr text(void); cstr nTerm(void);
		byte valAt(int i = 0) { return chk(data+i)
			? data->value() : CTOK_EOF; }

		// high level parsing
		Parse_t getArg(byte tok = CTOK_RBR);
		bool getArgs(xVector<Parse_t>& args);
		cstr getCall(xVector<Parse_t>& args);
		
		Parse_t tok(int token);
		Token* chr(int token);
		Parse_t splitL(int token); 
		Parse_t splitR(int token); 
		
		
		
		
		
		
		
		
		
		
	};
	
	// preprocessor helper
	struct Block_t : xArray <Block_t>
	{
		Block_t() : tok{} {}
		Block_t(cParse::Parse_t lst) : tok(lst) {}
		bool init(cParse::Parse_t lst);
		cParse::Parse_t init_(cParse::Parse_t& lst);
		cParse::Parse_t tok;
		
		int cppType(int idx) { return (idx < len) ? 
			data[idx].cppType() : -1; }
		int cppType() { return tok->cppType(); }
		
		void kill(bool wd) { tok = {0,0}; if(wd) Clear(); }
		void kill(int idx, bool wd) { data[idx].kill(wd); }
		void branch(int idx, bool taken);
		
		
		
		
	};

	// load/init initerface
	cParse() { ZINIT; }
	~cParse() { this->free(); } void free(void);
	void reset(void) { state = {base, 0}; }
	bool load(const char* name);
	char* load2(const char* name, int flags);
	void load_(void* data);
	char* load2_(void* data, int flags);
	
	// parsing interface
	Token get(int flags = 0); 
	std::pair<int,int> getLine(char* str);
	Parse_t parse(int flags = 0);
	
	// file info
	Token* tokBase; Parse_t tokLst;
	char* base;	xarray<char*> lineList;
	struct State { char* curPos; char inMacro; }; 
	State state; bool mustDelete;
};

#endif
