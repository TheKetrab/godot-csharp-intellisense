#ifndef CSHARP_LEXER_H
#define CSHARP_LEXER_H

#include <cstdlib>
#include <vector>
#include <iostream>
#include <string>
#include <stack>
#include <set>


using namespace std;

class CSharpLexer {

public:

	enum class Token {

		TK_EMPTY,

		// identifier
		TK_IDENTIFIER,

		// reserved-keyword
		TK_KW_ABSTRACT, TK_KW_AS, TK_KW_BASE, TK_KW_BOOL, TK_KW_BREAK,
		TK_KW_BYTE, TK_KW_CASE, TK_KW_CATCH, TK_KW_CHAR, TK_KW_CHECKED,
		TK_KW_CLASS, TK_KW_CONST, TK_KW_CONTINUE, TK_KW_DECIMAL, TK_KW_DEFAULT,
		TK_KW_DELEGATE, TK_KW_DO, TK_KW_DOUBLE, TK_KW_ELSE, TK_KW_ENUM,
		TK_KW_EVENT, TK_KW_EXPLICIT, TK_KW_EXTERN, TK_KW_FALSE, TK_KW_FINALLY,
		TK_KW_FIXED, TK_KW_FLOAT, TK_KW_FOR, TK_KW_FOREACH, TK_KW_GOTO,
		TK_KW_IF, TK_KW_IMPLICIT, TK_KW_IN, TK_KW_INT, TK_KW_INTERFACE,
		TK_KW_INTERNAL, TK_KW_IS, TK_KW_LOCK, TK_KW_LONG, TK_KW_NAMESPACE,
		TK_KW_NEW, TK_KW_NULL, TK_KW_OBJECT, TK_KW_OPERATOR, TK_KW_OUT,
		TK_KW_OVERRIDE, TK_KW_PARAMS, TK_KW_PRIVATE, TK_KW_PROTECTED, TK_KW_PUBLIC,
		TK_KW_READONLY, TK_KW_REF, TK_KW_RETURN, TK_KW_SBYTE, TK_KW_SEALED,
		TK_KW_SHORT, TK_KW_SIZEOF, TK_KW_STACKALLOC, TK_KW_STATIC, TK_KW_STRING,
		TK_KW_STRUCT, TK_KW_SWITCH, TK_KW_THIS, TK_KW_THROW, TK_KW_TRUE,
		TK_KW_TRY, TK_KW_TYPEOF, TK_KW_UINT, TK_KW_ULONG, TK_KW_UNCHECKED,
		TK_KW_UNSAFE, TK_KW_USHORT, TK_KW_USING, TK_KW_VIRTUAL, TK_KW_VOID,
		TK_KW_VOLATILE, TK_KW_WHILE,

		// context-keyword
		TK_KW_ADD, TK_KW_ASCENDING, TK_KW_ASYNC, TK_KW_AWAIT, TK_KW_BY,
		TK_KW_DESCENDING, TK_KW_DYNAMIC, TK_KW_EQUALS, TK_KW_FROM, TK_KW_GET,
		TK_KW_GLOBAL, TK_KW_GROUP, TK_KW_INTO, TK_KW_JOIN,
		TK_KW_LET, TK_KW_NAMEOF, TK_KW_ON, TK_KW_ORDERBY, TK_KW_PARTIAL,
		TK_KW_REMOVE, TK_KW_SELECT, TK_KW_SET, TK_KW_VALUE, TK_KW_VAR,
		TK_KW_WHEN, TK_KW_WHERE, TK_KW_YIELD,

		// literal
		TK_LT_INTEGER, TK_LT_REAL, TK_LT_CHAR, TK_LT_STRING, TK_LT_INTERPOLATED,

		// punctuator
		TK_CURLY_BRACKET_OPEN, TK_CURLY_BRACKET_CLOSE, TK_BRACKET_OPEN, TK_BRACKET_CLOSE, TK_PARENTHESIS_OPEN,
		TK_PARENTHESIS_CLOSE, TK_PERIOD, TK_COMMA, TK_COLON, TK_SEMICOLON,

		// operator
		TK_OP_ADD, TK_OP_SUB, TK_OP_MUL, TK_OP_DIV, TK_OP_MOD,
		TK_OP_BIT_AND, TK_OP_BIT_OR, TK_OP_XOR, TK_OP_NOT, TK_OP_INVERT,
		TK_OP_ASSIGN, TK_OP_LESS, TK_OP_GREATER, TK_OP_QUESTION_MARK, TK_OP_QUESTION_MARK_DOUBLE,
		TK_OP_COLON_DOUBLE, TK_OP_INCR, TK_OP_DECR, TK_OP_AND, TK_OP_OR,
		TK_OP_ARROW_FORWARD, TK_OP_EQUAL, TK_OP_NOTEQUAL, TK_OP_LESS_EQUAL, TK_OP_GREATER_EQUAL,
		TK_OP_ASSIGN_ADD, TK_OP_ASSIGN_SUB, TK_OP_ASSIGN_MUL, TK_OP_ASSIGN_DIV, TK_OP_ASSIGN_MOD,
		TK_OP_ASSIGN_BIT_AND, TK_OP_ASSIGN_BIT_OR, TK_OP_ASSIGN_BIT_XOR, TK_OP_LEFT_SHIFT, TK_OP_ASSIGN_LEFT_SHIFT,
		TK_OP_LAMBDA, TK_OP_RIGHT_SHIFT, TK_OP_ASSIGN_RIGHT_SHIFT,

		// misc
		TK_ERROR,
		TK_EOF,
		TK_CURSOR,
		TK_MAX
	};

	struct TokenData {
		Token type;
		string data;
		int line;
		int column;
		int depth;

		string to_string(bool typed = false) const;
	};

	static const char* token_names[(int)Token::TK_MAX];
	static set<string> get_keywords();
	static set<string> keywords;

private:

	set<string> identifiers;     // wszystkie mozliwe identyfikatory znalezione w tym pliku

	vector<TokenData> tokens;    // generated tokens
	string code;                 // code to analyze
	int len;                     // length of the code

	int pos;                     // current position in 'code' string
	int line;                    // real line of current position in code
	int column;                  // real column of current position in code

	bool verbatim_mode;
	bool interpolated_mode;
	bool possible_generic;
	bool force_generic_close;    // each '>' is interpreted as TK(>), don't care about GETCHAR(1) 

	int depth; // glebokosc w klamrach {} (dla funkcji 'wycofaj sie z bloku')
	

public:
	CSharpLexer(string code);
	~CSharpLexer() = default;

	void tokenize();           // start tokenize procedure
	void clear_state();        // clear state of current object (prepare to retokenize)
	void print_tokens() const;
	vector<TokenData> get_tokens() const;
	set<string> get_identifiers() const;

	static bool is_text_char(char c);     // a-z A-Z 0-9 _
	static bool is_number(char c);        // 0-9
	static bool is_hex(char c);           // 0-9 a-f A-F
	static bool is_bin(char c);           // 0 1
	static bool is_whitespace(char c);    // space, tab, newline
	static bool is_operator(const Token& type);
	static bool is_assignment_operator(Token& type);
	static bool is_context_keyword(const Token& type);

private:
	void _tokenize();
	void _make_token(const Token p_type);
	void _make_token(const Token p_type, const string &data);
	void _make_identifier(const string &identifier);

	void _skip_whitespace();
	string _skip_until_newline();
	string _skip_until(string str); // na przyklad dla skip do konca komentarza blokowego
	string _skip_until_whitespace();
	string _read_char_literal();
	string _read_string_literal();
	string _read_string_in_brackets();

	bool _read_word(string& word, Token& type); // read until identifier character
	bool _read_number(string& number, Token& type);
	bool _read_special_char(Token& type);
	bool _is_keyword(const string& word, Token& type) const;

	Token _get_last_token() const;

private:
	static const Token RESERVED_KEYWORDS_BEGIN = Token::TK_KW_ABSTRACT;
	static const Token RESERVED_KEYWORDS_END = Token::TK_KW_WHILE;
	static const Token CONTEXT_KEYWORDS_BEGIN = Token::TK_KW_ADD;
	static const Token CONTEXT_KEYWORDS_END = Token::TK_KW_YIELD;
	static const Token OPS_BEGIN = Token::TK_OP_ADD;
	static const Token OPS_END = Token::TK_OP_ASSIGN_RIGHT_SHIFT;

};


#endif // CSHARP_LEXER_H
