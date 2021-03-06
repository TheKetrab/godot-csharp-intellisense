#include <iostream>
#include "csharp_lexer.h"
#include "csharp_utils.h"

using namespace std;
using CST = CSharpLexer::Token;

#define GETCHAR(ofs) ((ofs + pos) >= len ? 0 : code[ofs+pos])
#define INCPOS(ammount) { pos += ammount; column += ammount; } // warning - no line skipping

set<string> CSharpLexer::keywords = set<string> ();
const char* CSharpLexer::token_names[(int)CST::TK_MAX] = {

	"Empty",

	// identifier
	"Identifier",

	// reserved-keyword
	"abstract", "as", "base", "bool", "break",
	"byte", "case", "catch", "char", "checked",
	"class", "const", "continue", "decimal", "default",
	"delegate", "do", "double", "else", "enum",
	"event", "explicit", "extern", "false", "finally",
	"fixed", "float", "for", "foreach", "goto",
	"if", "implicit", "in", "int", "interface",
	"internal", "is", "lock", "long", "namespace",
	"new", "null", "object", "operator", "out",
	"override", "params", "private", "protected", "public",
	"readonly", "ref", "return", "sbyte", "sealed",
	"short", "sizeof", "stackalloc", "static", "string",
	"struct", "switch", "this", "throw", "true",
	"try", "typeof", "uint", "ulong", "unchecked",
	"unsafe", "ushort", "using", "virtual", "void",
	"volatile", "while",

	// context-keyword
	"add", "ascending", "async", "await", "by",
	"descending", "dynamic", "equals", "from", "get",
	"global", "group",       "into", "join",
	"let", "nameof", "on", "orderby", "partial",
	"remove", "select", "set", "value", "var",
	"when", "where", "yield",

	// literal
	"integer", "real", "char", "string", "iterpolated",

	// punctuator
	"{", "}", "[", "]", "(",
	")", ".", ",", ":", ";",

	// operator
	"+", "-", "*", "/", "%",
	"&", "|", "^", "!", "~",
	"=", "<", ">", "?", "??",
	"::", "++", "--", "&&", "||",
	"->", "==", "!=", "<=", ">=",
	"+=", "-=", "*=", "/=", "%=",
	"&=", "|=", "^=", "<<", "<<=",
	"=>", ">>", ">>=",

	// misc
	"ERROR",
	"EOF",
	"CURSOR"
};


#include <iostream>

void CSharpLexer::clear_state()
{
	tokens.clear();

	pos = 0;
	line = 0;
	column = 0;

	verbatim_mode = false;
	interpolated_mode = false;
	possible_generic = false;
	force_generic_close = false;

	depth = 0;
}

CSharpLexer::Token CSharpLexer::_get_last_token() const
{
	int n = tokens.size();
	if (n == 0) return CST::TK_EMPTY;
	return tokens[n-1].type;
}

void CSharpLexer::tokenize()
{
	clear_state();
	_tokenize();
}

CSharpLexer::CSharpLexer(string code)
{
	clear_state();
	this->code = code;
	this->len = code.size();
}


vector<CSharpLexer::TokenData> CSharpLexer::get_tokens() const
{
	return tokens;
}

set<string> CSharpLexer::get_identifiers() const
{
	return identifiers;
}


void CSharpLexer::_tokenize()
{
	if (pos >= len) {
		_make_token(CST::TK_EOF);
		return;
	}

	while (true) {

		verbatim_mode = false;
		if (force_generic_close) {

			if (GETCHAR(0) == '>') {
				_make_token(CST::TK_OP_GREATER);
				INCPOS(1);
				continue;
			}
			else {
				// generic type closed - stop
				force_generic_close = false;
			}

		}

		if (possible_generic) {

			// if one of the following tokens, disable possible generic mode
			Token last_token = _get_last_token();
			switch (last_token) {
			case CST::TK_IDENTIFIER:        // part of name
			case CST::TK_PERIOD:            // typename.class...
			case CST::TK_OP_LESS:           // < deeper generic eg. List<List<T>>
			case CST::TK_COMMA:             // separate generic args eg. Dict<int,int>
			case CST::TK_OP_COLON_DOUBLE:   // namespace::typename
				break; // it's fine
			default: {
				possible_generic = false;
			}
			}

		}
		
		// CURSOR !!! ??? !!!
		if (GETCHAR(0) == 0xFFFF || GETCHAR(0) == -1) {
			std::cout << "CURSOR" << std::endl;
			_make_token(CST::TK_CURSOR);
			INCPOS(1);
			continue;
		}

		// cout << "code pos is: " << code_pos << " -> " << (char)GETCHAR(0) << endl;
		switch (GETCHAR(0)) {

			// ----- ----- -----
			// SPECIAL CASES
			case 0: {
				_make_token(CST::TK_EOF);
				return;
			}
			case '@': {
				std::cout << "@" << std::endl;
				verbatim_mode = true;
				INCPOS(1);
				break;
			}
			case '#': {
				_skip_until_newline();
				// TODO readuntil newline and add token DIRECTIVE, what if multiline directive?
				break;
			}
			case '\'': {
				INCPOS(1); // skip ' char
				string char_literal = _read_char_literal();
				_make_token(CST::TK_LT_CHAR, char_literal);
				INCPOS(1); // skip ' char
				break;
			}
			case '\"': {
				INCPOS(1); // skip " char
				string string_literal = _read_string_literal();
				if (interpolated_mode) {
					_make_token(CST::TK_LT_INTERPOLATED, string_literal);
					interpolated_mode = false;
				} else {
					_make_token(CST::TK_LT_STRING, string_literal);
				}
				INCPOS(1); // skip " char
				break;
			}
			case 0xFFFF: {
				_make_token(CST::TK_CURSOR);
				INCPOS(1);
				break;
			}
			case '/': {
				// linear comment
				if (GETCHAR(1) == '/') {
					INCPOS(2);
					_skip_until_newline();
					break;
				}
				// block comment
				else if (GETCHAR(1) == '*') {
					INCPOS(2);
					_skip_until("*/");
					break;
				}
				else {
					goto typicalcases; // div or div assign operator
				}
			}
			case '\\': {
				// TODO
				break;
			}
			case '$': {
				INCPOS(1); // skip '$'
				if (GETCHAR(0) != '"') _make_token(CST::TK_EMPTY);
				else interpolated_mode = true;
				break;
			}
			case '\r': {
				if (GETCHAR(1) == '\n') INCPOS(2) // CRLF
				else					INCPOS(1) // CR

					column = 0;
				line++;
				break;
			}
			case '\n': { // LF
				INCPOS(1);
				column = 0;
				line++;
				break;
			}

			// ----- ----- -----
			// TYPICAL CASES
			default: {

				typicalcases:
				char c = GETCHAR(0); // first sign of the word

				if (is_whitespace(c)) {
					_skip_whitespace();
				}

				// indentifier or keyword (a-z, A-Z, _)
				else if (is_text_char(c) && !is_number(c)) {

					string word = "";
					Token type = CST::TK_EMPTY;

					if (_read_word(word, type)) {
						if (type == CST::TK_IDENTIFIER)
							_make_identifier(word);
						else
							_make_token(type);
					}
					else {
						_make_token(CST::TK_ERROR);
						_skip_until_whitespace();
					}
				}

				// integer or real literal (0-9, .)
				else if (is_number(c) || (c == '.' && is_number(GETCHAR(1)))) {

					string number = "";
					Token type = CST::TK_EMPTY;
					if (c == '.') type = CST::TK_LT_REAL;

					if (_read_number(number, type)) {
						_make_token(type, number);
					}
					else {
						_make_token(CST::TK_ERROR);
						_skip_until_whitespace();
					}

				}

				// other character - probably operator or punctuator
				else {

					Token type = Token(CST::TK_EMPTY);
					if (_read_special_char(type)) {

						if (type == CST::TK_CURLY_BRACKET_OPEN) {
							_make_token(type);
							depth++;
						}
						else if (type == CST::TK_CURLY_BRACKET_CLOSE) {
							depth--;
							_make_token(type);
						}
						else {
							_make_token(type);
						}

					}
					else {
						_make_token(CST::TK_ERROR);
					}
				}
			}
		}

	}

}


string CSharpLexer::_skip_until_newline()
{
	char c; string res = "";
	while ((c = GETCHAR(0)) != '\n') {
		res += c; INCPOS(1);
	}

	INCPOS(1);
	column = 0;
	line++;

	return res;
}

void CSharpLexer::_skip_whitespace()
{
	char c;
	while ((c = GETCHAR(0)) > 0 && is_whitespace(c))
		INCPOS(1);
}

string CSharpLexer::_skip_until_whitespace()
{
	char c; string res = "";
	while ((c = GETCHAR(0)) > 0 && !is_whitespace(c)) {
		res += c; INCPOS(1);
	}

	return res;
}

// returns skipped string
string CSharpLexer::_skip_until(string str)
{
	char c; string res = "";
	while ((c = GETCHAR(0)) > 0 && !code.substr(pos).rfind(str, 0) == 0) { // begins with
		res += c; INCPOS(1);
	}

	return res;
}

string CSharpLexer::_read_char_literal()
{
	string res = "";
	if (GETCHAR(0) == '\\') { res += GETCHAR(0) + GETCHAR(1);	INCPOS(2); }
	else { res += GETCHAR(0); 				INCPOS(1); }

	return res;
}

string CSharpLexer::_read_string_in_brackets()
{
	int depth = 0;
	char c; string res;

	while ((c = GETCHAR(0)) > 0) {

		res += c;
		INCPOS(1);

		if (c == '{') depth++;
		if (c == '}') depth--; 

		if (depth == 0) return res;
	}

	return res;
}

string CSharpLexer::_read_string_literal()
{
	char c; string res = "";
	while ((c = GETCHAR(0)) > 0) {

		if (interpolated_mode) {
			if (c == '{') {
				res += _read_string_in_brackets();
				continue;
			}
		}

		if (verbatim_mode) {
			if (c == '"' && GETCHAR(1) == '"') { res += "\"\""; INCPOS(2); }
			else if (c == '"') { break; }
			else { res += c;	  INCPOS(1); }
		}
		else {
			if (c == '\\' && GETCHAR(1) == '\"') { res += "\\\""; INCPOS(2); }
			else if (c == '\"') { break; }
			else { res += c;	  INCPOS(1); }
		}
	}

	return res;

}

// returns true if read successful
bool CSharpLexer::_read_word(string& word, Token& type)
{
	char c;
	while ((c = GETCHAR(0)) > 0 && is_text_char(c)) {
		INCPOS(1); word += c;
	}

	// TODO if verbatim nothing is keyword
	_is_keyword(word, type);
	return true;
}

static char ToUpper(char c)
{
	if (c >= 'a' && c <= 'z')
		return c - 32;
	return c;
}

bool CSharpLexer::_read_number(string& number, Token& type)
{
	bool has_suffix = false;
	bool hex_mode = false;
	bool bin_mode = false;
	bool real_mode = false;
	bool suffix_mode = false;
	bool has_exponent = false;
	bool has_dot = false;

	// it is real for sure, if it begins from '.'
	if (type == CST::TK_LT_REAL) real_mode = true;

	// check if it is hex notation
	if (GETCHAR(0) == '0' && (GETCHAR(1) == 'x' || GETCHAR(1) == 'X')) {
		hex_mode = true; number += '0' + GETCHAR(1); INCPOS(2);
	}

	// check if it is C#7 bin notation
	if (GETCHAR(0) == '0' && (GETCHAR(1) == 'b' || GETCHAR(1) == 'B')) {
		bin_mode = true; number += '0' + GETCHAR(1); INCPOS(2);
	}

	char c;

	// PARSING INTEGER
	// parses integer until it realises it is real or possible suffix
	while ((c = GETCHAR(0)) > 0 && !suffix_mode && !real_mode) {
		
		// skipwith this notation
		if (c == '_') { INCPOS(1); continue; }

		if (bin_mode) {
			if (is_bin(c)) { number += c;	INCPOS(1); }
			else { suffix_mode = true; } // sth strange... maybe suffix?
		}

		else if (hex_mode) {
			if (is_hex(c)) { number += c; INCPOS(1); }
			else { suffix_mode = true; } // sth strange... maybe suffix?
		}

		else {
			if (c == 'e' || c == 'E' || c == '.') { real_mode = true; }
			else if (is_number(c)) { number += c; INCPOS(1); }
			else { suffix_mode = true; } // sth strange... maybe suffix?
		}
	}

	// PARSING REAL
	while ((c = GETCHAR(0)) > 0 && !suffix_mode && real_mode) {

		// skipwith this notation
		if (c == '_') { INCPOS(1); continue; }

		if (c == '.' && !has_dot) {

			// another MUST be a digit
			if (is_number(GETCHAR(1))) {
				number += '.';
				number += GETCHAR(1);
				has_dot = true;
				INCPOS(2);
			}
			else {
				return false; // error
			}

		}

		// exponent part => sign? decimal_digit+
		else if ((c == 'e' || c == 'E') && !has_exponent) {

			if (GETCHAR(1) == '+' && is_number(GETCHAR(2))) {
				has_exponent = true; number += c + GETCHAR(1) + GETCHAR(2); INCPOS(3);
			}
			else if (GETCHAR(1) == '-' && is_number(GETCHAR(2))) {
				has_exponent = true; number += c + GETCHAR(1) + GETCHAR(2); INCPOS(3);
			}
			else if (is_number(GETCHAR(1))) {
				has_exponent = true; number += c + GETCHAR(1); INCPOS(2);
			}
			else {
				return false; // error
			}
		}

		else {
			if (is_number(c)) { number += c; INCPOS(1); }
			else { suffix_mode = true; } // sth strange... maybe suffix?
		}
	}

	// INTEGER TYPE SUFIX
	// 'U' | 'u' | 'L' | 'l' | 'UL' | 'Ul' | 'uL' | 'ul' | 'LU' | 'Lu' | 'lU' | 'lu'
	if (suffix_mode && !real_mode) {
		if ((ToUpper(GETCHAR(0)) == 'U' && ToUpper(GETCHAR(1)) == 'L')
			|| (ToUpper(GETCHAR(0)) == 'L' && ToUpper(GETCHAR(1)) == 'U')) {
			number += GETCHAR(0) + GETCHAR(1);
			INCPOS(2);
			has_suffix = true;
		}
		else if (ToUpper(GETCHAR(0)) == 'U'
			|| ToUpper(GETCHAR(1)) == 'L') {
			number += GETCHAR(0);
			INCPOS(1);
			has_suffix = true;
		}
	}

	// REAL TYPE SUFFIX
	// 'F' | 'f' | 'D' | 'd' | 'M' | 'm'
	else if (suffix_mode && !has_suffix) {
		if (ToUpper(GETCHAR(0)) == 'F'
			|| ToUpper(GETCHAR(0)) == 'D'
			|| ToUpper(GETCHAR(0)) == 'M') {
			number += GETCHAR(0); INCPOS(1);
		}

	}

	// CHECK IF IT IS END ! (if not, error, read until end??? and return false TODO)
	// TODO

	// PREPARE TO RETURN
	if (real_mode) type = CST::TK_LT_REAL;
	else type = CST::TK_LT_INTEGER;

	return true; // success!
}

// try to read operator or punctuator
bool CSharpLexer::_read_special_char(Token& type)
{
	switch (GETCHAR(0)) {

	case '{': type = CST::TK_CURLY_BRACKET_OPEN;						INCPOS(1); break;
	case '}': type = CST::TK_CURLY_BRACKET_CLOSE;						INCPOS(1); break;
	case '[': type = CST::TK_BRACKET_OPEN;								INCPOS(1); break;
	case ']': type = CST::TK_BRACKET_CLOSE;								INCPOS(1); break;
	case '(': type = CST::TK_PARENTHESIS_OPEN;							INCPOS(1); break;
	case ')': type = CST::TK_PARENTHESIS_CLOSE;							INCPOS(1); break;
	case '.': type = CST::TK_PERIOD;									INCPOS(1); break;
	case ',': type = CST::TK_COMMA;										INCPOS(1); break;
	case ':': {
		if (GETCHAR(1) == ':') { type = CST::TK_OP_COLON_DOUBLE;		INCPOS(2); }
		else { type = CST::TK_COLON;									INCPOS(1); }
		break;
	}
	case ';': type = CST::TK_SEMICOLON;									INCPOS(1); break;
	case '+': {
		if (GETCHAR(1) == '+') { type = CST::TK_OP_INCR;				INCPOS(2); }
		else if (GETCHAR(1) == '=') { type = CST::TK_OP_ASSIGN_ADD;		INCPOS(2); }
		else { type = CST::TK_OP_ADD;									INCPOS(1); }
		break;
	}
	case '-': {
		if (GETCHAR(1) == '-') { type = CST::TK_OP_DECR;				INCPOS(2); }
		else if (GETCHAR(1) == '=') { type = CST::TK_OP_ASSIGN_SUB;		INCPOS(2); }
		else if (GETCHAR(1) == '>') { type = CST::TK_OP_ARROW_FORWARD;	INCPOS(2); }
		else { type = CST::TK_OP_SUB;									INCPOS(1); }
		break;
	}
	case '*': {
		if (GETCHAR(1) == '=') { type = CST::TK_OP_ASSIGN_MUL;			INCPOS(2); }
		else { type = CST::TK_OP_MUL;									INCPOS(1); }
		break;
	}
	case '/': {
		if (GETCHAR(1) == '=') { type = CST::TK_OP_ASSIGN_DIV;			INCPOS(2); }
		else { type = CST::TK_OP_DIV;									INCPOS(1); }
		break;
	}
	case '%': {
		if (GETCHAR(1) == '=') { type = CST::TK_OP_ASSIGN_MOD;			INCPOS(2); }
		else { type = CST::TK_OP_MOD;									INCPOS(1); }
		break;
	}
	case '&': {
		if (GETCHAR(1) == '&') { type = CST::TK_OP_AND;					INCPOS(2); }
		else if (GETCHAR(1) == '=') { type = CST::TK_OP_ASSIGN_BIT_AND; INCPOS(2); }
		else { type = CST::TK_OP_BIT_AND;								INCPOS(1); }
		break;
	}
	case '|': {
		if (GETCHAR(1) == '|') { type = CST::TK_OP_OR;					INCPOS(2); }
		else if (GETCHAR(1) == '=') { type = CST::TK_OP_ASSIGN_BIT_OR;	INCPOS(2); }
		else { type = CST::TK_OP_BIT_OR;								INCPOS(1); }
		break;
	}
	case '^': {
		if (GETCHAR(1) == '=') { type = CST::TK_OP_ASSIGN_BIT_XOR;		INCPOS(2); }
		else if (GETCHAR(1) == '|') { type = CST::TK_CURSOR; INCPOS(2); } // TODO wywalic po testach ( cursor = ^| )
		else { type = CST::TK_OP_XOR;									INCPOS(1); }
		break;
	}
	case '!': {
		if (GETCHAR(1) == '=') { type = CST::TK_OP_NOTEQUAL;			INCPOS(2); }
		else { type = CST::TK_OP_NOT;									INCPOS(1); }
		break;
	}
	case '~': type = CST::TK_OP_INVERT;									INCPOS(1); break;
	case '=': {
		if (GETCHAR(1) == '=') { type = CST::TK_OP_EQUAL;				INCPOS(2); }
		else if (GETCHAR(1) == '>') { type = CST::TK_OP_LAMBDA;			INCPOS(2); }
		else { type = CST::TK_OP_ASSIGN;								INCPOS(1); }
		break;
	}
	case '<': {
		if (GETCHAR(1) == '=') { type = CST::TK_OP_LESS_EQUAL;			INCPOS(2); }
		else if (GETCHAR(1) == '<' && GETCHAR(2) == '=') { type = CST::TK_OP_ASSIGN_LEFT_SHIFT;	INCPOS(3); }
		else if (GETCHAR(1) == '<') { type = CST::TK_OP_LEFT_SHIFT;		INCPOS(2); }
		else { 
			if (_get_last_token() == CST::TK_IDENTIFIER) {
				possible_generic = true;
			}
			type = CST::TK_OP_LESS;
			INCPOS(1);
		}
		break;
	}
	case '>': {
		if (GETCHAR(1) == '=') { type = CST::TK_OP_GREATER_EQUAL;		INCPOS(2); }
		else if (GETCHAR(1) == '>' && GETCHAR(2) == '=') { type = CST::TK_OP_ASSIGN_RIGHT_SHIFT;	INCPOS(3); }
		else if (GETCHAR(1) == '>' && GETCHAR(2) == '>') {
			// >>> - it must be a closure of a generic type: parse separately TK(>) TK(>) ... TK(>)
			force_generic_close = true; type = CST::TK_OP_GREATER;		INCPOS(1); break; 
		}
		else if (GETCHAR(1) == '>') { 
			
			if (possible_generic) {
				// for sure generic ! TODO
				type = CST::TK_OP_GREATER; INCPOS(1);
			}
			else {
				type = CST::TK_OP_RIGHT_SHIFT;	INCPOS(2);
			}

		}

		else { type = CST::TK_OP_GREATER;								INCPOS(1); }
		break;
	}
	case '?': {
		if (GETCHAR(1) == '?') { type = CST::TK_OP_QUESTION_MARK_DOUBLE;	INCPOS(2); }
		else { type = CST::TK_OP_QUESTION_MARK;								INCPOS(1); }
		break;
	}
	default:
		type = CST::TK_ERROR;
		return false; // not operator		
	}

	return true;
}


void CSharpLexer::_make_token(const Token p_type)
{
	_make_token(p_type, token_names[(int)p_type]);
}

void CSharpLexer::_make_token(const Token p_type, const string& data)
{
	TokenData td = { p_type, data, line, column };
	td.depth = this->depth;
	tokens.push_back(td);
}

void CSharpLexer::_make_identifier(const string& identifier)
{
	this->identifiers.insert(identifier);
	_make_token(CST::TK_IDENTIFIER, identifier);
}

bool CSharpLexer::_is_keyword(const string& word, Token& type) const
{
	for (int i = (int)RESERVED_KEYWORDS_BEGIN; i <= (int)RESERVED_KEYWORDS_END; i++)
		if (word == token_names[i]) {
			type = Token(i);
			return true;
		}

	for (int i = (int)CONTEXT_KEYWORDS_BEGIN; i <= (int)CONTEXT_KEYWORDS_END; i++)
		if (word == token_names[i]) {
			type = Token(i);
			return true;
		}


	type = CST::TK_IDENTIFIER;
	return false;
}

bool CSharpLexer::is_context_keyword(const Token& type)
{	
	return (int)type >= (int)CSharpLexer::CONTEXT_KEYWORDS_BEGIN
		&& (int)type <= (int)CSharpLexer::CONTEXT_KEYWORDS_END;
}

bool CSharpLexer::is_operator(const Token& type)
{
	return (int)type >= (int)CSharpLexer::OPS_BEGIN
		&& (int)type <= (int)CSharpLexer::OPS_END;
}

bool CSharpLexer::is_assignment_operator(Token& type)
{
	switch (type) {

	case CST::TK_OP_ASSIGN:
	case CST::TK_OP_ASSIGN_ADD:
	case CST::TK_OP_ASSIGN_SUB:
	case CST::TK_OP_ASSIGN_MUL: 
	case CST::TK_OP_ASSIGN_DIV:
	case CST::TK_OP_ASSIGN_MOD:
	case CST::TK_OP_ASSIGN_BIT_AND:
	case CST::TK_OP_ASSIGN_BIT_OR:
	case CST::TK_OP_ASSIGN_BIT_XOR:
	case CST::TK_OP_ASSIGN_LEFT_SHIFT:
	case CST::TK_OP_ASSIGN_RIGHT_SHIFT:
		return true;
	default: return false;

	}
}

bool CSharpLexer::is_text_char(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

bool CSharpLexer::is_number(char c)
{
	return (c >= '0' && c <= '9');
}

bool CSharpLexer::is_hex(char c)
{
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

bool CSharpLexer::is_bin(char c)
{
	return (c == '0' || c == '1');
}

bool CSharpLexer::is_whitespace(char c)
{
	return (c == ' ' || c == '\t' || c == '\f');
}

void CSharpLexer::print_tokens() const
{
	cout << "Printing tokens..." << endl;

	int n = tokens.size();
	int i = 0;
	while (i < n) {
		cout << "(" << i << ") --> ";
		for (int j = 0; j < 10 && i < n; j++) {

			TokenData td = tokens[i];

			// token name
			cout << token_names[(int)td.type];

			// print details
			switch (td.type) {
			case CST::TK_LT_INTEGER:
			case CST::TK_LT_REAL:
			case CST::TK_LT_CHAR:
			case CST::TK_LT_STRING:
			case CST::TK_LT_INTERPOLATED:
			case CST::TK_IDENTIFIER: {
				cout << "(" << td.data << ")";
				break;
			}
			default: break;
			}

			// separator
			cout << " ";
			i++;
		}
		cout << endl;
	}

	cout << "Printing tokens is done" << endl;
}

set<string> CSharpLexer::get_keywords()
{	
	if (CSharpLexer::keywords.size() < 0) { // INIT

		for (int i = (int)CSharpLexer::RESERVED_KEYWORDS_BEGIN;
			i <= (int)CSharpLexer::RESERVED_KEYWORDS_END; i++)
				keywords.insert(CSharpLexer::token_names[i]);

		for (int i = (int)CSharpLexer::CONTEXT_KEYWORDS_BEGIN;
			i <= (int)CSharpLexer::CONTEXT_KEYWORDS_END; i++)
				keywords.insert(CSharpLexer::token_names[i]);
	}

	return keywords;
}

string CSharpLexer::TokenData::to_string(bool typed) const
{
	if (type == CST::TK_IDENTIFIER) {
		return this->data;
	}

	if (typed) {

		switch (type) {

		case CST::TK_LT_INTEGER: {
			bool is_unsigned = (contains(data, 'u') || contains(data, 'U')) ? true : false;
			bool is_long = (contains(data, 'l') || contains(data, 'L')) ? true : false;

			string res;
			if (is_unsigned) res += "u";

			if (is_long) res += "long";
			else res += "int";

			return res;
		}

		case CST::TK_LT_REAL: {
			if (contains(data, 'f') || contains(data, 'F')) return "float";
			if (contains(data, 'm') || contains(data, 'M')) return "decimal";

			return "double";
		}

		case CST::TK_LT_CHAR:
			return "char";

		case CST::TK_LT_STRING:
		case CST::TK_LT_INTERPOLATED:
			return "string";

		case CST::TK_KW_TRUE:
		case CST::TK_KW_FALSE:
			return "bool";
		}
	}

	return token_names[(int)this->type];

}
