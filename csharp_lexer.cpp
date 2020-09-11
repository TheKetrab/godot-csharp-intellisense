#include "csharp_lexer.h"

#include "core/map.h"
const char *CSharpLexer::token_names[TK_MAX] = {

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
	"global", "group", "in", "into", "join",
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

void CSharpLexer::clear() {
	tokens = new List<TokenData>();

	line = 0;
	column = 0;

	code_pos = 0;
	comment_mode = false;
	
}

void CSharpLexer::tokenize() {
	clear();
	_tokenize();
}

#define GETCHAR(ofs) ((ofs + code_pos) >= len ? 0 : _code[ofs+code_pos])
#define INCPOS(ammount) { code_pos += ammount; column += ammount; } // warning - no line skipping



void CSharpLexer::_tokenize() {

	std::cout << "_tokenize" << std::endl;
	std::cout << "len is: " << len << std::endl;
	// ASCII PRINT
	for (int i = 0; i < len; i++) {
		std::cout << _code[i] << " ";
	}


	if (code_pos >= len) {
		_make_token(TK_EOF);
		return;
	}

	while (true) {

		std::cout << "code_pos is: " << code_pos << std::endl;
		switch (GETCHAR(0)) {

			// ----- ----- -----
			// SPECIAL CASES
			case 0: {
				_make_token(TK_EOF);
				return;
			}
			case '@': {
				std::cout << "@" << std::endl;
				// TODO READ WORD
				INCPOS(1);
				break;
			}
			case 0xFFFF: {
				std::cout << "CURSOR" << std::endl;
				_make_token(TK_CURSOR);
				INCPOS(1);
				break;
			}
			case '/': {
				if (GETCHAR(1) == '/') {
					INCPOS(2);
					skip_untill_newline();
					break;
				} else if (GETCHAR(1) == '*') {
					INCPOS(2);
					skip_untill_str("*/");
					break;
				}
			}
			case '\\': {
				// TODO
				break;
			}
			case '$': {
				// TODO
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

				CharType c = GETCHAR(0); // first sign of the word

				if (_is_whitespace(c)) {
					skip_whitespace();
				}

				// indentifier or keyword (a-z, A-Z, _)
				else if (_is_text_char(c) && !_is_number(c)) { 

					String word = "";
					Token type=Token(TK_EMPTY);

					if (read_word(word, type)) {
						if (type == TK_IDENTIFIER)
							_make_identifier(word);
						else
							_make_token(type);
					} else {
						// TODO error
					}
				}

				// integer or real literal (0-9, .)
				else if (_is_number(c) || (c == '.' && _is_number(GETCHAR(1)))) {

					std::cout << "NUMBER" << std::endl;

					String number = "";
					Token type = Token(TK_EMPTY);
					if (c == '.') type = Token(TK_LT_REAL);

					if (read_number(number, type)) {
						_make_token(type, number);
					} else {
						skip_untill_whitespace();
					}

				}

				// other character - probably operator or punctuator
				else {
					Token type = Token(TK_EMPTY);
					if (read_special_char(type)) {
						_make_token(type);
					} else {
						// TODO unknown char ... error?
					}
				}



			}
		}


		


	}
	
}



void CSharpLexer::skip_untill_newline() {

	std::cout << "skip_until_newline()" << std::endl;

	CharType c;
	while ((c = GETCHAR(0)) != '\n')
		INCPOS(1);

	INCPOS(1);
	column = 0;
	line++;

}

void CSharpLexer::skip_whitespace() {

	CharType c;
	while ((c = GETCHAR(0)) > 0 && _is_whitespace(c))
		INCPOS(1);
}

void CSharpLexer::skip_untill_whitespace() {

	CharType c;
	while ((c = GETCHAR(0)) > 0 && !_is_whitespace(c))
		INCPOS(1);
}

void CSharpLexer::skip_untill_str(String str) {

	std::cout << "skip_until_str()" << std::endl;

	CharType c;
	while ((c = GETCHAR(0)) > 0 && !_code.substr(code_pos).begins_with(str))
		INCPOS(1);
}

void CSharpLexer::set_code(const String &code) {
	this->_code = String(code);
	this->len = code.length();
}

// returns true if read successful
bool CSharpLexer::read_word(String &word, Token &type) {

	CharType c;
	while ((c = GETCHAR(0)) > 0 && _is_text_char(c)) {
		INCPOS(1); word += c;
	}

	std::cout << "word is: " << word.ascii().get_data() << std::endl;

	_is_keyword(word, type);
	return true;
}

static CharType ToUpper(CharType c) {
	if (c >= 'a' && c <= 'z')
		return c - 32;
	return c;
}


bool CSharpLexer::read_number(String& number, Token& type) {

	std::cout << "read_number()" << std::endl;

	bool has_suffix = false;
	bool hex_mode = false;
	bool bin_mode = false;
	bool real_mode = false;
	bool suffix_mode = false;
	bool has_exponent = false;
	bool has_dot = false;

	// it is real for sure, if it begins from '.'
	if (type == TK_LT_REAL) real_mode = true;

	// check if it is hex notation
	if (GETCHAR(0) == '0' && (GETCHAR(1) == 'x' || GETCHAR(1) == 'X')) {
		hex_mode = true; number += '0' + GETCHAR(1); INCPOS(2);
	}

	// check if it is C#7 bin notation
	if (GETCHAR(0) == '0' && (GETCHAR(1) == 'b' || GETCHAR(1) == 'B')) {
		bin_mode = true; number += '0' + GETCHAR(1); INCPOS(2);
	}


	CharType c;

	// PARSING INTEGER
	// parses integer until it realises it is real or possible suffix
	while ((c = GETCHAR(0)) > 0 && !suffix_mode && !real_mode) {
		std::cout << "PARSING INTEGER" << std::endl;
		// skipwith this notation
		if (c == '_') { INCPOS(1); continue; }

		if (bin_mode) {
			if (_is_bin(c)) { number += c;	INCPOS(1); }
			else { suffix_mode = true; } // sth strange... maybe suffix?
		}

		else if (hex_mode) {
			if (_is_hex(c)) { number += c; INCPOS(1); }
			else { suffix_mode = true; } // sth strange... maybe suffix?
		}

		else {
			if (c == 'e' || c == 'E' || c == '.') { real_mode = true; }
			else if (_is_number(c)) { number += c; INCPOS(1); }
			else { suffix_mode = true; } // sth strange... maybe suffix?
		}
	}


	// PARSING REAL
	while ((c = GETCHAR(0)) > 0 && !suffix_mode && real_mode) {
		std::cout << "PARSING REAL" << std::endl;

		// skipwith this notation
		if (c == '_') { INCPOS(1); continue; }

		if (c == '.' && !has_dot) {

			// another MUST be a digit
			if (_is_number(GETCHAR(1))) {
				number += '.' + GETCHAR(1);
				has_dot = true;
				INCPOS(2);
			} else {
				return false; // error
			}
			
		}

		// exponent part => sign? decimal_digit+
		else if ((c == 'e' || c == 'E') && !has_exponent) {

			if (GETCHAR(1) == '+' && _is_number(GETCHAR(2))) {
				has_exponent = true; number += c + GETCHAR(1) + GETCHAR(2); INCPOS(3); 
			} else if (GETCHAR(1) == '-' && _is_number(GETCHAR(2))) {
				has_exponent = true; number += c + GETCHAR(1) + GETCHAR(2); INCPOS(3);
			} else if (_is_number(GETCHAR(1))) {
				has_exponent = true; number += c + GETCHAR(1); INCPOS(2);
			} else {
				return false; // error
			}			
		}

		else {
			if (_is_number(c)) { number += c; INCPOS(1); }
			else { suffix_mode = true; } // sth strange... maybe suffix?
		}
	}

	// INTEGER TYPE SUFIX
	// 'U' | 'u' | 'L' | 'l' | 'UL' | 'Ul' | 'uL' | 'ul' | 'LU' | 'Lu' | 'lU' | 'lu'
	if (suffix_mode && !real_mode) {
		if ((ToUpper(GETCHAR(0)) == 'U' && ToUpper(GETCHAR(1)) == 'L')
		 ||	(ToUpper(GETCHAR(0)) == 'L' && ToUpper(GETCHAR(1)) == 'U')) {
			number += GETCHAR(0) + GETCHAR(1);
			INCPOS(2);
			has_suffix = true;
		} else if (ToUpper(GETCHAR(0)) == 'U'
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
	if (real_mode) type = TK_LT_REAL;
	else type = TK_LT_INTEGER;

	return true; // success!
}

// try to read operator or punctuator
bool CSharpLexer::read_special_char(Token& type) {

	switch (GETCHAR(0)) {

		case '{': type = Token(TK_CURLY_BRACKET_OPEN);						INCPOS(1); break;
		case '}': type = Token(TK_CURLY_BRACKET_CLOSE);						INCPOS(1); break;
		case '[': type = Token(TK_BRACKET_CLOSE);							INCPOS(1); break;
		case ']': type = Token(TK_BRACKET_CLOSE);							INCPOS(1); break;
		case '(': type = Token(TK_PARENTHESIS_OPEN);						INCPOS(1); break;
		case ')': type = Token(TK_PARENTHESIS_CLOSE);						INCPOS(1); break;
		case '.': type = Token(TK_PERIOD);									INCPOS(1); break;
		case ',': type = Token(TK_COMMA);									INCPOS(1); break;
		case ':': {
			if (GETCHAR(1) == ':') {	type = Token(TK_OP_COLON_DOUBLE);	INCPOS(2); }
			else {						type = Token(TK_COLON);				INCPOS(1); }
			break;
		}
		case ';': type = Token(TK_SEMICOLON);								INCPOS(1); break;
		case '+': {
			if (GETCHAR(1) == '+') {		type = Token(TK_OP_INCR);		INCPOS(2); }
			else if (GETCHAR(1) == '=') {	type = Token(TK_OP_ASSIGN_ADD); INCPOS(2); }
			else {							type = Token(TK_OP_ADD);		INCPOS(1); }
			break;
		}
		case '-': {
			if (GETCHAR(1) == '-') {		type = Token(TK_OP_DECR);			INCPOS(2); }
			else if (GETCHAR(1) == '=') {	type = Token(TK_OP_ASSIGN_SUB);		INCPOS(2); }
			else if (GETCHAR(1) == '>') {	type = Token(TK_OP_ARROW_FORWARD);	INCPOS(2); }
			else {							type = Token(TK_OP_SUB);			INCPOS(1); }
			break;
		}
		case '*': {
			if (GETCHAR(1) == '=') {	type = Token(TK_OP_ASSIGN_MUL);	INCPOS(2); }
			else {						type = Token(TK_OP_MUL);		INCPOS(1); }
			break;
		}
		case '/': {
			if (GETCHAR(1) == '=') {	type = Token(TK_OP_ASSIGN_DIV);	INCPOS(2); }
			else {						type = Token(TK_OP_DIV);		INCPOS(1); }
			break;
		}
		case '%': {
			if (GETCHAR(1) == '=') {	type = Token(TK_OP_ASSIGN_MOD);	INCPOS(2); }
			else {						type = Token(TK_OP_MOD);		INCPOS(1); }
			break;
		}
		case '&': {
			if (GETCHAR(1) == '&') {		type = Token(TK_OP_AND);			INCPOS(2); }
			else if (GETCHAR(1) == '=') {	type = Token(TK_OP_ASSIGN_BIT_AND); INCPOS(2); }
			else {							type = Token(TK_OP_BIT_AND);		INCPOS(1); }
			break;
		}
		case '|': {
			if (GETCHAR(1) == '|') {		type = Token(TK_OP_OR);				INCPOS(2); }
			else if (GETCHAR(1) == '=') {	type = Token(TK_OP_ASSIGN_BIT_OR);	INCPOS(2); }
			else {							type = Token(TK_OP_BIT_OR);			INCPOS(1); }
			break;
		}
		case '^': {
			if (GETCHAR(1) == '=') {	type = Token(TK_OP_ASSIGN_BIT_XOR);	INCPOS(2); }
			else {						type = Token(TK_OP_XOR);			INCPOS(1); }
			break;
		}
		case '!': {
			if (GETCHAR(1) == '=') {	type = Token(TK_OP_NOTEQUAL);	INCPOS(2); }
			else {						type = Token(TK_OP_NOT);		INCPOS(1); }
			break;
		}
		case '~': type = Token(TK_OP_INVERT);							INCPOS(1); break;
		case '=': {
			if (GETCHAR(1) == '=') {		type = Token(TK_OP_EQUAL);	INCPOS(2); }
			else if (GETCHAR(1) == '>') {	type = Token(TK_OP_LAMBDA);	INCPOS(2); }
			else {							type = Token(TK_OP_ASSIGN);	INCPOS(1); }
			break;
		}
		case '<': {
			if (GETCHAR(1) == '=') {							type = Token(TK_OP_LESS_EQUAL);			INCPOS(2); }
			else if (GETCHAR(1) == '<' && GETCHAR(2) == '=') {	type = Token(TK_OP_ASSIGN_LEFT_SHIFT);	INCPOS(3); }
			else if (GETCHAR(1) == '<') {						type = Token(TK_OP_LEFT_SHIFT);			INCPOS(2); }
			else {												type = Token(TK_OP_LESS);				INCPOS(1); }
			break;
		}
		case '>': {
			if (GETCHAR(1) == '=') {							type = Token(TK_OP_GREATER_EQUAL);		INCPOS(2); }
			else if (GETCHAR(1) == '>' && GETCHAR(2) == '=') {	type = Token(TK_OP_ASSIGN_RIGHT_SHIFT);	INCPOS(3); }
			else if (GETCHAR(1) == '>') {						type = Token(TK_OP_RIGHT_SHIFT);		INCPOS(2); }
			else {												type = Token(TK_OP_GREATER);			INCPOS(1); }
			break;
		}
		case '?': {
			if (GETCHAR(1) == '?') {	type = Token(TK_OP_QUESTION_MARK_DOUBLE);	INCPOS(2); }
			else {						type = Token(TK_OP_QUESTION_MARK);			INCPOS(1); }
			break;
		}
		default:
			type = Token(TK_ERROR);
			return false; // not operator		
	}

	return true;
}





void CSharpLexer::_make_token(Token p_type, String data) {
	TokenData td = { p_type, data, line, column };
	tokens->push_back(td);
}

void CSharpLexer::_make_identifier(const StringName& p_identifier) {
	_make_token(TK_IDENTIFIER, p_identifier);
}

void CSharpLexer::_make_constant(const int& p_constant) {
	//_make_token(TK_CONSTANT, String::num_int64(p_constant));
}


bool CSharpLexer::_is_keyword(const String &word, Token &type) const {

	// TODO zrobic drzewo Trie, do szybszego sprawdzania
	for (int i = RESERVED_KEYWORDS_BEGIN; i <= RESERVED_KEYWORDS_END; i++)
		if (word == token_names[i]) {
			type = Token(i);
			return true;
		}

	for (int i = CONTEXT_KEYWORDS_BEGIN; i <= CONTEXT_KEYWORDS_END; i++)
		if (word == token_names[i]) {
			type = Token(i);
			return true;
		}


	type = Token(TK_IDENTIFIER);
	return false;
}

bool CSharpLexer::_is_text_char(CharType c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}


bool CSharpLexer::_is_number(CharType c) {

	return (c >= '0' && c <= '9');
}

bool CSharpLexer::_is_hex(CharType c) {

	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

bool CSharpLexer::_is_bin(CharType c) {

	return (c == '0' || c == '1');
}

bool CSharpLexer::_is_whitespace(CharType c) {
	
	return (c == ' ' || c == '\t' || c == '\f');
}



#include <iostream>
void CSharpLexer::print_tokens() {

	int n = tokens->size();
	for (int i=0; i<n; i++) {

		TokenData td = (*tokens)[i];

		// token name
		std::cout << token_names[td.type];

		// print details
		switch (td.type) {
			case TK_LT_INTEGER:
			case TK_LT_REAL:
			case TK_IDENTIFIER: {
				std::cout << "(" << td.data.ascii().get_data() << ")";
				break;
			}
			default: break;
		}

		// separator
		std::cout << " ";
	}

}
