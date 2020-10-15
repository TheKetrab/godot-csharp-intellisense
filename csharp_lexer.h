// KETRAB
#ifndef CSHARP_LEXER_H
#define CSHARP_LEXER_H

#include <cstdlib>
#include <vector>
#include <iostream>
#include <string>
using namespace std;
// lekser to maszynka ktora sobie jedzie po tekscie i tworzy liste tokenow

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
		std::string data;
		int line;
		int column;
	};

	vector<TokenData> tokens;

	static const Token RESERVED_KEYWORDS_BEGIN = Token::TK_KW_ABSTRACT;
	static const Token RESERVED_KEYWORDS_END = Token::TK_KW_WHILE;
	static const Token CONTEXT_KEYWORDS_BEGIN = Token::TK_KW_ADD;
	static const Token CONTEXT_KEYWORDS_END = Token::TK_KW_YIELD;
	static const Token OPS_BEGIN = Token::TK_OP_ADD;
	static const Token OPS_END = Token::TK_OP_ASSIGN_RIGHT_SHIFT;


public:
	static const char* token_names[(int)Token::TK_MAX];

public:
	void tokenize(); // start tokenize procedure
	void clear(); // clear state of current object (prepare to tokenize again)
	void print_tokens();
	void set_code(const std::string &code);

	static bool _is_text_char(char c);
	static bool _is_number(char c);
	static bool _is_hex(char c);
	static bool _is_bin(char c);
	static bool _is_whitespace(char c);


private:
	string _code;  // code to analyze
	int len;      // length of the code

	int code_pos; // i-ta pozycja w stringu _code
	int line;
	int column;

	bool comment_mode;
	bool verbatim_mode;


public:
	CSharpLexer();

	void _tokenize();


	void _make_token(Token p_type, string data = "");
	//void _make_newline(int p_indentation = 0, int p_tabs = 0);
	void _make_identifier(const string& p_identifier);
	void _make_constant(const int& p_constant);
	//void _make_type(const Variant::Type &p_type);
	//void _make_error(const String &p_error);

	string skip_until_newline();
	void skip_whitespace();
	string skip_until(string str); // na przyklad dla skip do konca komentarza blokowego
	string skip_until_whitespace();
	string read_char_literal();
	string read_string_literal();

	bool read_word(string& word, Token& type); // read until identifier character
	bool read_number(string& number, Token& type);
	bool read_special_char(Token& type);
	bool _is_keyword(const string& word, Token& type) const;
	static bool is_operator(Token& type);
};


#endif // CSHARP_LEXER_H
