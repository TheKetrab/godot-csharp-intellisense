#include "csharp_parser.h"
#include "csharp_context.h"
#include "csharp_utils.h"

using CST = CSharpLexer::Token;
using CSM = CSharpParser::Modifier;
using namespace std;

#define GETTOKEN(ofs) ((ofs + pos) >= len ? CST::TK_ERROR : tokens[ofs + pos].type)
#define TOKENINFO(ofs) ((ofs + pos) >= len ? CSharpLexer::TokenData() : tokens[ofs + pos])
#define TOKENDATA(ofs) ((ofs + pos >= len ? "" : tokens[ofs + pos].data))
#define INCPOS(ammount) { pos += ammount; }

CSharpParser* CSharpParser::active_parser = nullptr;
const string CSharpParser::wldc = "?"; // Wild Cart Type

void CSharpParser::_found_cursor()
{
	cinfo.ctx_cursor = current;
	cinfo.cursor_column = TOKENINFO(0).column;
	cinfo.cursor_line = TOKENINFO(0).line;
	cinfo.completion_type = _deduce_completion_type();
	cinfo.ctx_namespace = cur_namespace;
	cinfo.ctx_class = cur_class;
	cinfo.ctx_method = cur_method;
	cinfo.ctx_block = cur_block;

	if (cur_expression == "" && cur_type == "")
		cinfo.completion_expression = prev_expression;
	else
		cinfo.completion_expression = cur_expression + cur_type;
}

#define CASEATYPICAL \
	case CST::TK_EOF:    error("EOF found"); \
	case CST::TK_ERROR:  error("Found error token."); \
	case CST::TK_EMPTY:  error("Empty token error."); \
	case CST::TK_CURSOR: _found_cursor(); INCPOS(1); break;

#define CASEMODIFIER \
		case CST::TK_KW_PUBLIC: \
		case CST::TK_KW_PROTECTED: \
		case CST::TK_KW_PRIVATE: \
		case CST::TK_KW_INTERNAL: \
		case CST::TK_KW_EXTERN: \
		case CST::TK_KW_ABSTRACT: \
		case CST::TK_KW_CONST: \
		case CST::TK_KW_OVERRIDE: \
		case CST::TK_KW_PARTIAL: \
		case CST::TK_KW_READONLY: \
		case CST::TK_KW_SEALED: \
		case CST::TK_KW_STATIC: \
		case CST::TK_KW_UNSAFE: \
		case CST::TK_KW_VIRTUAL: \
		case CST::TK_KW_VOLATILE: \
		case CST::TK_KW_ASYNC:

#define CASELITERAL case CST::TK_LT_INTEGER: \
		case CST::TK_LT_REAL: \
		case CST::TK_LT_CHAR: \
		case CST::TK_LT_STRING: \
		case CST::TK_LT_INTERPOLATED: \
		case CST::TK_KW_TRUE: \
		case CST::TK_KW_FALSE:

void CSharpParser::indentation(int n) {
	cout << string(n, ' ');
}

std::map<CST, CSM> CSharpParser::to_modifier = {
	{ CST::TK_KW_PUBLIC,		CSM::MOD_PUBLIC },
	{ CST::TK_KW_PROTECTED,		CSM::MOD_PROTECTED },
	{ CST::TK_KW_PRIVATE,		CSM::MOD_PRIVATE },
	{ CST::TK_KW_INTERNAL,		CSM::MOD_INTERNAL },
	{ CST::TK_KW_EXTERN,		CSM::MOD_EXTERN },
	{ CST::TK_KW_ABSTRACT,		CSM::MOD_ABSTRACT },
	{ CST::TK_KW_CONST,			CSM::MOD_CONST },
	{ CST::TK_KW_OVERRIDE,		CSM::MOD_OVERRIDE },
	{ CST::TK_KW_PARTIAL,		CSM::MOD_PARTIAL },
	{ CST::TK_KW_READONLY,		CSM::MOD_READONLY },
	{ CST::TK_KW_SEALED,		CSM::MOD_SEALED },
	{ CST::TK_KW_STATIC,		CSM::MOD_STATIC },
	{ CST::TK_KW_UNSAFE,		CSM::MOD_UNSAFE },
	{ CST::TK_KW_VIRTUAL,		CSM::MOD_VIRTUAL },
	{ CST::TK_KW_VOLATILE,		CSM::MOD_VOLATILE },
	{ CST::TK_KW_ASYNC,			CSM::MOD_ASYNC },
	{ CST::TK_KW_REF,           CSM::MOD_REF },
	{ CST::TK_KW_IN,            CSM::MOD_IN },
	{ CST::TK_KW_OUT,           CSM::MOD_OUT },
	{ CST::TK_KW_PARAMS,        CSM::MOD_PARAMS }
};

map<string, int> CSharpParser::base_type_category = {
	{ "bool",    1 }, { "Boolean", 1 }, { "System.Boolean", 1 },
	{ "byte",    2 }, { "Byte",    2 }, { "System.Byte",    2 },
	{ "sbyte",   3 }, { "SByte",   3 }, { "System.SByte",   3 },
	{ "char",    4 }, { "Char",    4 }, { "System.Char",    4 },
	{ "string",  5 }, { "String",  5 }, { "System.String",  5 },
	{ "decimal", 6 }, { "Decimal", 6 }, { "System.Decimal", 6 },
	{ "double",  7 }, { "Double",  7 }, { "System.Double",  7 },
	{ "float",   8 }, { "Single",  8 }, { "System.Single",  8 },
	{ "int",     9 }, { "Int32",   9 }, { "System.Int32",   9 },
	{ "long",   10 }, { "Int64",  10 }, { "System.Int64",  10 },
	{ "short",  11 }, { "Int16",  11 }, { "System.Int16",  11 },
	{ "uint",   12 }, { "UInt32", 12 }, { "System.UInt32", 12 },
	{ "ulong",  13 }, { "UInt64", 13 }, { "System.UInt64", 13 },
	{ "ushort", 14 }, { "UInt16", 14 }, { "System.UInt16", 14 },
	{ "object", 15 }, { "Object", 15 }, { "System.Object", 15 },
	{ "void",   16 }, { "Void",   16 }, { "System.Void",   16 }
};

int CSharpParser::get_base_type_category(const string& type)
{
	auto it = base_type_category.find(type);
	if (it == base_type_category.end())
		return -1;

	return it->second;
}


void CSharpParser::debug_info() const {
	//if (pos == 62) {
	//	int x = 1;
	//}
	//cout << "Token: " << (GETTOKEN(0) == CST::TK_IDENTIFIER ? TOKENDATA(0) : CSharpLexer::token_names[(int)GETTOKEN(0)]) << " Pos: " << pos << endl;
}

// ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** //
// ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** //
//                               ===========================================================                               //
//                                    ---------------- PARSER'S METHODS ---------------                                    //
//                               ===========================================================                               //
// ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** //
// ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** //


CSharpParser::FileNode* CSharpParser::parse()
{
	clear_state();
	this->tokens = tokens;
	FileNode* node = _parse_file();

	return node;
}

CSharpParser::~CSharpParser()
{

}

CSharpParser::CSharpParser(string code)
{
	CSharpParser::active_parser = this;
	clear_state();

	// lexical analyze
	CSharpLexer lexer(code);
	lexer.tokenize();

	this->tokens = lexer.get_tokens();
	this->len = this->tokens.size();
	this->identifiers = lexer.get_identifiers();
}

void CSharpParser::clear_state()
{
	cur_namespace    = nullptr;
	cur_class        = nullptr;
	cur_method       = nullptr;
	cur_block        = nullptr;
	current          = nullptr;
	cinfo.ctx_cursor = nullptr;
	pos              = 0;      
	
	modifiers        = 0;
	kw_value_allowed = false;

	cinfo.completion_type      = CompletionType::COMPLETION_NONE;
	cinfo.ctx_namespace = nullptr;
	cinfo.ctx_class     = nullptr;
	cinfo.ctx_method    = nullptr;
	cinfo.ctx_block     = nullptr;
}

void CSharpParser::_parse_modifiers()
{
	this->modifiers = 0;

	bool end = false;
	while (!end) {

		switch (GETTOKEN(0)) {

		CASEMODIFIER {
			modifiers |= (int)to_modifier[GETTOKEN(0)];
			pos++;
			break;
		}
		default: {
			end = true; // end of modifiers, sth other
		}
		}
	}

	// if no modifiers pub/pro/pri -> it is private!
	if ((modifiers & (int)Modifier::MOD_PPP) == 0) {
		modifiers |= (int)Modifier::MOD_PRIVATE;
	}
}

void CSharpParser::error(string msg) const
{
	//cout << "--> " << ((cinfo.ctx_cursor == nullptr) ? "" : "(cursor found) ") << "Error: " << msg << endl;
	throw CSharpParserException(msg);
}

void CSharpParser::_unexpeced_token_error() const
{
	string msg = "Unexpected token: ";
	msg += CSharpLexer::token_names[(int)(GETTOKEN(0))];
	error(msg);
}

// assures if actual token is as expected
// if current token is CURSOR, sets cursor and checks if the next token is expected by assertion token
// if assertion is false, then escapes (error)
void CSharpParser::_assert(CSharpLexer::Token tk)
{
	_is_actual_token(tk, true);
}

bool CSharpParser::_is_actual_token(CSharpLexer::Token tk, bool assert)
{
	CSharpLexer::Token token = GETTOKEN(0);

	switch (token) {

	case CST::TK_EOF:    error("EOF found");
	case CST::TK_ERROR:  error("Found error token.");
	case CST::TK_EMPTY:  error("Empty token error.");
	case CST::TK_CURSOR: _found_cursor(); INCPOS(1); return _is_actual_token(tk, assert);
	default: {

		if (token != tk) {
			if (assert) {
				string msg = "Assertion failed. Expected token was: ";
				msg += CSharpLexer::token_names[(int)tk];
				msg += " but current token is: ";
				msg += (token == CST::TK_IDENTIFIER) ? TOKENDATA(0) : CSharpLexer::token_names[(int)token];
				msg += " at pos: ";
				msg += to_string(pos);
				error(msg);
			}
			return false;
		}
	}
	}

	return true;
}

// parses attributes of function or class or, ...
// eg: [Atr1][Atr2]function
void CSharpParser::_parse_attributes()
{
	int bracket_depth = 0;
	string attribute = "";
	while (true) {

		switch (GETTOKEN(0)) {

		CASEATYPICAL

		case CST::TK_BRACKET_OPEN: {

				if (bracket_depth == 0) {
					attribute = "[ ";
				}

				bracket_depth++;
				INCPOS(1);
				break;
			}
		case CST::TK_BRACKET_CLOSE: {
			bracket_depth--;
			INCPOS(1);

			if (bracket_depth == 0) {
				attribute += "]";
				this->attributes.push_back(attribute);
				attribute = "";
			}
			break;
		}
		default: {
			if (bracket_depth == 0) return;
			else attribute += TOKENDATA(0) + " ";
			INCPOS(1);
		}
		}
	}

}

void CSharpParser::_apply_attributes(CSharpParser::Node* node)
{
	node->attributes = this->attributes; // copy
}

// deduces what kind of autocompletion is expected from current pos
CSharpParser::CompletionType CSharpParser::_deduce_completion_type()
{
	CompletionType res = CompletionType::COMPLETION_NONE;

	switch (GETTOKEN(-1)) {

		// member of class or object
	case CST::TK_PERIOD:
	case CST::TK_OP_ARROW_FORWARD:
	{
		res = CompletionType::COMPLETION_MEMBER;
		break;
	}

	// function arguments
	case CST::TK_PARENTHESIS_OPEN:
	case CST::TK_COMMA:
	{
		// completion_info_str => function name
		// completion_info_int => current argument number

		res = CompletionType::COMPLETION_CALL_ARGUMENTS;

		int parenthesis = 0;
		int cur_argument_number = 0;
		bool end = false;
		for (int i = -1; !end; i--) {

			switch (GETTOKEN(i)) {

			case CST::TK_ERROR: {
				res = CompletionType::COMPLETION_NONE;
				end = true;
				break;
			}
			case CST::TK_PARENTHESIS_OPEN: {
				if (parenthesis == 0) {
					end = true;
					//if (GETTOKEN(i - 1) == CST::TK_IDENTIFIER)
					//	cinfo.completion_info_str = TOKENDATA(i - 1);
					//else res = CompletionType::COMPLETION_NONE; // error
				}
				else parenthesis++;
				break;
			}
			case CST::TK_BRACKET_OPEN:
			case CST::TK_CURLY_BRACKET_OPEN: {
				if (parenthesis == 0) {
					// you are in multidimensional array or block context
					// example: array[1,2,    --> this is not function
					// example: obj = { 1, 2, --> this is not function
					res = CompletionType::COMPLETION_NONE;
					end = true;
				}
				else parenthesis++;
				break;
			}
			case CST::TK_PARENTHESIS_CLOSE:
			case CST::TK_BRACKET_CLOSE:
			case CST::TK_CURLY_BRACKET_CLOSE: {
				parenthesis--;
				break;
			}
			case CST::TK_COMMA: {
				// parenthesis must be 0! example: func(arg1,invocation(a1,a2,a3),a3);
				if (parenthesis == 0) cur_argument_number++;
				break;
			}
			}
		}

		cinfo.cur_arg = cur_argument_number;
		break;
	}

	// override function
	case CST::TK_KW_OVERRIDE: {
		res = CompletionType::COMPLETION_VIRTUAL_FUNC;
		break;
	}

	// uzupelnienie jakiejs nazwy (wszystkie identifiery i keywordsy)
	case CST::TK_IDENTIFIER: {

		// maybe this is the begining of member?
		if (GETTOKEN(-2) == CST::TK_PERIOD
			|| GETTOKEN(-2) == CST::TK_OP_ARROW_FORWARD)
		{
			res = CompletionType::COMPLETION_MEMBER;

		}

		// or just begining or a word
		else {
			res = CompletionType::COMPLETION_IDENTIFIER;
		}

		break;
	}

	case CST::TK_COLON: {

		if (GETTOKEN(-2) == CST::TK_KW_GOTO) {
			res = CompletionType::COMPLETION_LABEL;
		}

		break;
	}

	}


	return res;

}

void CSharpParser::_apply_modifiers(CSharpParser::Node* node)
{
	node->modifiers = this->modifiers;
}

// read file = read namespace (even 'no-namespace')
// global -> for file parsing (no name parsing and keyword namespace)
CSharpParser::NamespaceNode* CSharpParser::_parse_namespace(bool global = false)
{
	string name = "global";
	TD td = tokens[pos];
	if (!global) {

		try {
			// is namespace?
			_assert(CST::TK_KW_NAMESPACE);
			INCPOS(1); // skip 'namespace'

			// read name
			_assert(CST::TK_IDENTIFIER);

			name = "";
			while (true) {
				_assert(CST::TK_IDENTIFIER);
				name += TOKENDATA(0);
				INCPOS(1); // skip name
				if (_is_actual_token(CST::TK_PERIOD)) {
					name += ".";
					INCPOS(1);
				}
				else {
					break;
				}
			}
		}
		catch (CSharpParserException&) {
			_escape();
			return nullptr;
		}
	}

	NamespaceNode* node = new NamespaceNode(td);
	Node* prev_current = current;
	NamespaceNode* prev_namespace = cur_namespace;
	this->current = node;
	this->cur_namespace = node;
	node->name = name;
	root->node_shortcuts.insert({ node->fullname(),node });

	try {
		_assert(CST::TK_CURLY_BRACKET_OPEN);
		INCPOS(1); // skip '{'
		while (_parse_namespace_member(node));
	}
	catch (CSharpParserException&) {
		_escape();
		delete node;
		node = nullptr;
	}

	// restore state
	this->current = prev_current;
	this->cur_namespace = prev_namespace;
	return node;

}

CSharpParser::FileNode * CSharpParser::_parse_file()
{
	FileNode* node = new FileNode();
	this->root = node;
	this->current = node;

	try {
		while (_parse_using_directive(node));
		while (_parse_namespace_member(node));
	}
	catch (CSharpParserException&) {
		// probably found eof token
	}

	this->current = nullptr;
	return node;
}

// after this function pos will be ON the token at the same level (depth)
void CSharpParser::_skip_until_token(CSharpLexer::Token tk)
{
	while (true) {

		// if '}' then not found the token... this is error
		if (GETTOKEN(0) == CST::TK_CURLY_BRACKET_CLOSE) {
			_unexpeced_token_error();
		}

		if (GETTOKEN(0) == tk)
			return;
		else
			INCPOS(1);
	}

}

string CSharpParser::completion_type_name(CompletionType type)
{
	switch (type) {
	case CompletionType::COMPLETION_NONE:           return "none";
	case CompletionType::COMPLETION_TYPE:           return "type";
	case CompletionType::COMPLETION_MEMBER:         return "member";
	case CompletionType::COMPLETION_CALL_ARGUMENTS: return "argument";
	case CompletionType::COMPLETION_LABEL:          return "label";
	case CompletionType::COMPLETION_VIRTUAL_FUNC:   return "virtual func";
	case CompletionType::COMPLETION_ASSIGN:         return "assign";
	case CompletionType::COMPLETION_IDENTIFIER:     return "identifier";
	};

	return "";
}

vector<string> CSharpParser::_parse_generic_declaration() {

	_assert(CST::TK_OP_LESS);
	INCPOS(1); // skip '<'

	vector<string> generic_declarations;

	bool end = false;
	while (!end) {
		switch (GETTOKEN(0)) {

		CASEATYPICAL

		case CST::TK_IDENTIFIER: {
				generic_declarations.push_back(TOKENDATA(0));
				INCPOS(1); break;
			}
		case CST::TK_COMMA: { INCPOS(1); break; }
		case CST::TK_OP_GREATER: { INCPOS(1); end = true; break; }
		default: {
			_unexpeced_token_error();
		}
		}
	}

	return generic_declarations;

}

vector<string> CSharpParser::_parse_derived_and_implements(bool generic_context)
{
	_assert(CST::TK_COLON);
	INCPOS(1); // skip ':'

	vector<string> types;

	bool end = false;
	while (!end) {
		switch (GETTOKEN(0)) {

		CASEATYPICAL

		case CST::TK_IDENTIFIER: {
				string type = _parse_type();
				types.push_back(type);
				break;
			}
		case CST::TK_COMMA: { INCPOS(1); break; }
		case CST::TK_CURLY_BRACKET_OPEN: { end = true; break; }
		default: {
			if (generic_context && _is_actual_token(CST::TK_KW_WHERE)) {
				end = true; break;
			}
			// else
			_unexpeced_token_error();
		}
		}
	}

	return types;
}

// generic -> where T : C1, new()
string CSharpParser::_parse_constraints()
{
	_assert(CST::TK_KW_WHERE);

	string constraints;
	// TODO in the future
	_skip_until_token(CST::TK_CURLY_BRACKET_OPEN);

	return "";
}

CSharpParser::ClassNode* CSharpParser::_parse_class()
{
	TD td = tokens[pos];

	_assert(CST::TK_KW_CLASS);

	INCPOS(1); // skip 'class'

	_assert(CST::TK_IDENTIFIER);

	ClassNode* node = new ClassNode(td);
	Node* prev_current = current;
	ClassNode* prev_class = cur_class;
	this->current = node;
	this->cur_class = node;

	node->name = TOKENDATA(0);
	root->node_shortcuts.insert({ node->fullname(),node });

	INCPOS(1); // skip name

	_apply_attributes(node);
	_apply_modifiers(node);

	try {
		// generic
		if (_is_actual_token(CST::TK_OP_LESS)) {
			node->is_generic = true;
			node->generic_declarations = _parse_generic_declaration();
		}

		// derived and implements
		if (_is_actual_token(CST::TK_COLON)) {
			node->base_types = _parse_derived_and_implements(node->is_generic);
		}

		if (node->base_types.empty()) // object is last on class chain
			node->base_types.push_back("object");

		// constraints
		if (_is_actual_token(CST::TK_KW_WHERE)) {
			node->constraints = _parse_constraints();
		}

		_assert(CST::TK_CURLY_BRACKET_OPEN);
		INCPOS(1); // skip '{'
		while (_parse_class_member(node));

		if (!node->any_constructor_declared) {
			MethodNode* constructor = node->create_auto_generated_constructor();
			node->methods.push_back(constructor);
			node->any_constructor_declared = true;
		}
	}
	catch (CSharpParserException&) {
		_escape();
		delete node;
		node = nullptr;
	}


	// restore state
	this->current = prev_current;
	this->cur_class = prev_class;
	return node;
}

string CSharpParser::_parse_initialization_block()
{	
	_assert(CST::TK_CURLY_BRACKET_OPEN);

	INCPOS(1); // skip '{'

	string initialization_block = "{ ";
	bool end = false;
	while (!end) {

		switch (GETTOKEN(0)) {

		CASEATYPICAL

		CASELITERAL {
			initialization_block += TOKENDATA(0);
			INCPOS(1);
			break;
		}

		case CST::TK_IDENTIFIER: {
			initialization_block += TOKENDATA(0);
			INCPOS(1);
			break;
		}

		case CST::TK_OP_ASSIGN: {
			initialization_block += " = ";
			INCPOS(1);
			string val = _parse_expression();
			break;
		}

		case CST::TK_COMMA: {
			initialization_block += " , ";
			INCPOS(1);
			break;
		}

		case CST::TK_CURLY_BRACKET_CLOSE: {
			initialization_block += " }";
			INCPOS(1); // skip '}'
			end = true;
			break;
		}
		default: {
			_unexpeced_token_error();
			_escape();
			end = true;
		}

		}


	}
	return initialization_block;
}

string CSharpParser::_parse_new()
{
	// new constructor()       --> new X(a1,a2);
	// new type block          --> new Dict<int,string> { 1 = "one" }
	// new type[n] {?}         --> new int[5] { optional_block }
	// new { ... } (anonymous) --> new { x = 1, y = 2 }

	prev_expression = cur_expression;

	_assert(CST::TK_KW_NEW);

	INCPOS(1); // skip 'new'
	cur_expression = "new ";

	switch (GETTOKEN(0)) {

	CASEATYPICAL

	case CST::TK_CURLY_BRACKET_OPEN: {
		string initialization_block = _parse_initialization_block();
		cur_expression += initialization_block;
		break;
	}
	case CST::TK_CURLY_BRACKET_CLOSE: {
		// todo
		break;
	}

	CASEBASETYPE {
		string type = _parse_type(true);
		cur_expression += type;
		if (GETTOKEN(0) == CST::TK_CURLY_BRACKET_OPEN) {
			string initialization_block = _parse_initialization_block();
			cur_expression += " " + initialization_block;
		}
		break;
	}

	case CST::TK_IDENTIFIER: {
		string type = _parse_type(true);
		cur_expression += type;
		if (GETTOKEN(0) == CST::TK_CURLY_BRACKET_OPEN) {
			cur_expression += "{";
			string initialization_block = _parse_initialization_block();
			cur_expression += initialization_block;
			cur_expression += "}";
		} else if (GETTOKEN(0) == CST::TK_PARENTHESIS_OPEN) {
			_parse_method_invocation();
		}
		break;
	}

	default: {
		_unexpeced_token_error();
	}
	}

	string res = cur_expression;
	cur_expression = prev_expression;

	return res;
}

// parse expression -> (or assignment: x &= 10 is good as well)
string CSharpParser::_parse_expression(bool inside, CSharpLexer::Token opener)
{
	// https://docs.microsoft.com/en-us/dotnet/csharp/language-reference/language-specification/expressions
	// try untill ')' at the same depth or ',' or ';'

	// UWAGA - nie chcemy tu rzuca� wyj�tku, bo try-catch jest na zewn�trz tej funkcji
	// i usunie si� np VarNode: int x = "...error...", a powinni�my widzie� zmienn� x!
	// --> je�li co� dziwnego, to nale�y ustawi� end = true

	string p_expression = cur_expression;
	prev_expression = cur_expression;
	cur_expression = "";

	try {
		bool end = false;
		while (!end) {
			debug_info();
			switch (GETTOKEN(0)) {

				CASEATYPICAL

					CASEBASETYPE{
						string type = _parse_type();
						cur_expression += type;
						break;
				}

			case CST::TK_KW_AWAIT:
			case CST::TK_KW_BASE:
			case CST::TK_KW_AS:
			case CST::TK_KW_THIS:
			case CST::TK_KW_NAMEOF:
			case CST::TK_KW_NULL: {
				cur_expression += CSharpLexer::token_names[(int)GETTOKEN(0)];
				INCPOS(1);
				break;
			}
			case CST::TK_KW_NEW: {

				// new Int32(1) + 9 <-- this is expression
				cur_expression += _parse_new();
				break;
			}

			case CST::TK_COLON: {
				INCPOS(1); // ignore (TODO: add handling operator ? : )
				cur_expression += ":";
				break;
			}

			// end if
			case CST::TK_COMMA: {
				end = true;
				//if (!inside) end = true;
				//else { cur_expression += ","; INCPOS(1); }
				break;
			}
			case CST::TK_SEMICOLON: {
				if (!inside) end = true;
				else { end = true; }
				break;
			}
			case CST::TK_PARENTHESIS_CLOSE: {
				if (opener == CST::TK_PARENTHESIS_OPEN) end = true;
				else { cur_expression += ")"; INCPOS(1); }
				break;
			}

			// function invokation
			case CST::TK_PARENTHESIS_OPEN: {
				cur_expression += "(";
				INCPOS(1);
				while (true) {
					string e = prev_expression;
					cur_expression += _parse_expression(true, CST::TK_PARENTHESIS_OPEN); // inside
					prev_expression = e;
					if (_is_actual_token(CST::TK_PARENTHESIS_CLOSE)) break;
					else if (_is_actual_token(CST::TK_COMMA)) { cur_expression += ","; INCPOS(1); }
					else _unexpeced_token_error();
				}
				_assert(CST::TK_PARENTHESIS_CLOSE);
				cur_expression += ")";
				INCPOS(1);
				break;
			}

			case CST::TK_OP_LAMBDA: {
				// TODO: wrong solution! -> add handling '=>' ald lambda expressions
				cur_expression += " => ";
				INCPOS(1);
				string s = _parse_expression();
				cur_expression += s;
				break;
			}

			CASELITERAL{
				if (tokens[pos].type == CST::TK_LT_CHAR) {
					cur_expression += "'" + TOKENDATA(0) + "'";
				}
				else if (tokens[pos].type == CST::TK_LT_STRING) {
					cur_expression += "\"" + TOKENDATA(0) + "\"";
				}
				else {
					cur_expression += TOKENDATA(0);
				}
				INCPOS(1);
				break;
			}

			case CST::TK_IDENTIFIER: {

				// maybe it is a type (MyOwnType)
				// or array access (MyIdentifier[expression])
				int cur_pos = pos;
				string type = _parse_type(true);

				if (type == "") {
					// this is not a type, neither access to array
					// it can be eg method invocation -> add to res and decide in next iteration
					pos = cur_pos;
					cur_expression += TOKENDATA(0);
					INCPOS(1);
					break;
				}
				else {

					cur_expression += type;
					break;

				}
			}
			case CST::TK_PERIOD: { cur_expression += "."; INCPOS(1); break; }
			case CST::TK_BRACKET_OPEN: {
				cur_expression += "[";
				INCPOS(1);
				string e = prev_expression;
				cur_expression += _parse_expression(true, CST::TK_BRACKET_OPEN); // inside
				prev_expression = e;
				_assert(CST::TK_BRACKET_CLOSE);
				cur_expression += "]";
				INCPOS(1);
				break;
			}

			case CST::TK_BRACKET_CLOSE: {
				if (opener == CST::TK_BRACKET_OPEN) end = true;
				//else { cur_expression += ")"; INCPOS(1); }
				else { end = true; }
				break;
			}
			case CST::TK_CURLY_BRACKET_CLOSE: { // for initialization block: var x = new { a = EXPR };
				if (opener == CST::TK_CURLY_BRACKET_OPEN) end = true;
				//else { cur_expression += "}"; INCPOS(1); }
				else { end = true; }
				break;
			}
			case CST::TK_CURLY_BRACKET_OPEN: {
				BlockNode* node = _parse_block();
				cur_expression += "{ ... }";
				break;
			}

			default: {

				CSharpLexer::Token t = GETTOKEN(0);
				if (CSharpLexer::is_operator(t)) {
					cur_expression += CSharpLexer::token_names[(int)GETTOKEN(0)];
					INCPOS(1);

					if (CSharpLexer::is_assignment_operator(t)) {
						string expr = _parse_expression();
						cur_expression += expr;
					}

				}

				else if (this->kw_value_allowed && t == CST::TK_KW_VALUE) {
					cur_expression += "value";
					INCPOS(1);
					break;
				}

				else if (CSharpLexer::is_context_keyword(t)) {

					cur_expression += CSharpLexer::token_names[(int)t];
					INCPOS(1);
					break;
				}

				else {
					_unexpeced_token_error();
				}
			}


			}
		}

	}
	catch (CSharpParserException&) {
		// DO NOT ESCAPE
		//_escape();
		//delete variable;
		//variable = nullptr;
	}

	string res = cur_expression;
	cur_expression = p_expression;
	//cur_expression = prev_expr;
	prev_expression = "";
	return res;
	
}

CSharpParser::EnumNode* CSharpParser::_parse_enum()
{
	TD td = tokens[pos];
	_assert(CST::TK_KW_ENUM);

	INCPOS(1); // skip 'enum'

	// read name
	_assert(CST::TK_IDENTIFIER);

	EnumNode* node = new EnumNode(td);
	Node* prev_current = current;
	this->current = node;
	node->name = TOKENDATA(0);
	root->node_shortcuts.insert({ node->fullname(),node });

	INCPOS(1); // skip name

	_apply_attributes(node);
	_apply_modifiers(node);

	node->type = "int"; // default

	try {

		// enum type
		if (_is_actual_token(CST::TK_COLON)) {
			INCPOS(1); // skip ';'
			node->type = _parse_type();
		}

		// parse members
		bool end = false;
		while (!end) {

			switch (GETTOKEN(0)) {

			CASEATYPICAL

			case CST::TK_IDENTIFIER: {
					node->members.push_back(TOKENDATA(0));
					INCPOS(1);
					break;
				}
			case CST::TK_OP_ASSIGN: {
				INCPOS(1); // skip '='
				_parse_expression(); // don't care about values
				break;
			}
			case CST::TK_COMMA: {
				INCPOS(1); // skip ','
				break;
			}
			case CST::TK_CURLY_BRACKET_CLOSE: {
				INCPOS(1); // skip '}'
				end = true; break;
			}
			default: {
				_unexpeced_token_error();
			}
			}
		}
	}
	catch (CSharpParserException&) {
		_escape();
		delete node;
		node = nullptr;
	}

	// restore
	this->current = prev_current;
	return node;
}

CSharpParser::JumpNode* CSharpParser::_parse_jump()
{
	TD td = tokens[pos];
	JumpNode* node = new JumpNode(td);
	Node* prev_current = current;
	this->current = node;

	try {

		// type of jump
		switch (GETTOKEN(0)) {
		case CST::TK_KW_BREAK: node->jump_type = JumpNode::Type::BREAK; break;
		case CST::TK_KW_CONTINUE: node->jump_type = JumpNode::Type::CONTINUE; break;
		case CST::TK_KW_GOTO: node->jump_type = JumpNode::Type::GOTO; break;
		case CST::TK_KW_RETURN: node->jump_type = JumpNode::Type::RETURN; break;
		case CST::TK_KW_YIELD: node->jump_type = JumpNode::Type::YIELD; break;
		case CST::TK_KW_THROW: node->jump_type = JumpNode::Type::THROW; break;
		default: _unexpeced_token_error();
		}

		node->raw = CSharpLexer::token_names[(int)GETTOKEN(0)];
		INCPOS(1); // skip keyword

		bool end = false;
		while (!end) {
			switch (GETTOKEN(0)) {

			CASEATYPICAL

			case CST::TK_SEMICOLON: {
					node->raw += ";";
					end = true;
					INCPOS(1);
					break;
				}
			default: {
				node->raw +=
					(GETTOKEN(0) == CST::TK_IDENTIFIER ? TOKENDATA(0)
						: CSharpLexer::token_names[(int)GETTOKEN(0)]);
				INCPOS(1);
			}
			}
		}
	}
	catch (CSharpParserException&) {
		_escape();
		delete node;
		node = nullptr;
	}

	this->current = prev_current;
	return node;
	
}

CSharpParser::LoopNode* CSharpParser::_parse_loop()
{
	TD td = tokens[pos];
	LoopNode* node = new LoopNode(td);
	Node* prev_current = current;
	this->current = node;

	try {
		// type of loop
		switch (GETTOKEN(0)) {
		case CST::TK_KW_DO:      node->loop_type = LoopNode::Type::DO;      break;
		case CST::TK_KW_FOR:     node->loop_type = LoopNode::Type::FOR;     break;
		case CST::TK_KW_FOREACH: node->loop_type = LoopNode::Type::FOREACH; break;
		case CST::TK_KW_WHILE:   node->loop_type = LoopNode::Type::WHILE;   break;
		default: _unexpeced_token_error();
		}

		node->node_type = CSP::Node::Type::LOOP;

		INCPOS(1); // skip keyword

		// ----- ----- -----
		// PARSE BEFORE BLOCK

		// ----- FOR -----
		if (node->loop_type == LoopNode::Type::FOR) {

			INCPOS(1); // skip '('

			// init
			VarNode* variable = _parse_declaration();

			if (variable == nullptr) {
				_skip_until_token(CST::TK_SEMICOLON); // assignment or sth else
			}
			else {
				variable->parent = node;
				node->local_variable = variable;
			}

			INCPOS(1); // skip ';'

			// cond
			_skip_until_token(CST::TK_SEMICOLON);
			INCPOS(1); // skip ';'

			// iter
			_skip_until_token(CST::TK_PARENTHESIS_CLOSE);
			INCPOS(1); // skip ')'

		}

		// ----- FOREACH -----
		else if (node->loop_type == LoopNode::Type::FOREACH) {

			INCPOS(1); // skip '('

			// declaration
			VarNode* variable = _parse_declaration();
			if (variable == nullptr) {
				_skip_until_token(CST::TK_KW_IN);
			}
			else {
				variable->parent = node;
				node->local_variable = variable;
			}

			_assert(CST::TK_KW_IN);
			INCPOS(1); // skip 'in'

			string collection = _parse_expression(true,CST::TK_PARENTHESIS_OPEN);
			if (node->local_variable != nullptr)
				node->local_variable->value = collection;

			_assert(CST::TK_PARENTHESIS_CLOSE);
			INCPOS(1); // skip ')'
		}

		// ----- WHILE -----
		else if (node->loop_type == LoopNode::Type::WHILE) {

			// cond
			_skip_until_token(CST::TK_PARENTHESIS_CLOSE);
			INCPOS(1); // skip ')'

		}

		// ----- ----- -----
		// PARSE BLOCK
		node->body = _parse_statement();
		if (node->body != nullptr)
			node->body->parent = node; // bind

		// ----- ----- -----
		// PARSE AFTER BLOCK
		if (node->loop_type == LoopNode::Type::DO) {
			//todo skip while
		}

	}
	catch (CSharpParserException&) {
		_escape();
		delete node;
		node = nullptr;
	}

	// restore
	this->current = prev_current;
	return node;
}

CSharpParser::DelegateNode* CSharpParser::_parse_delegate()
{
	return nullptr; // TODO
}

std::string CSharpParser::_parse_type(bool array_constructor)
{
	string prev_cur_type = cur_type;
	cur_type = "";

	// base type: int/bool/... (nullable ?) [,,...,]
	// complex type: NAMESPACE::NAME1. ... .NAMEn<type1,type2,...>[,,...,]

	// note: 
	// array_constructor = false -> int[n] is NOT a type
	// array_constructor = true -> int[n] is a type

	// TODO add '::' handling (for namespaces)

	bool base_type = false;
	bool complex_type = false;
	bool generic_types_mode = false;
	bool array_mode = false;

	bool end = false;
	while (!end) {
		debug_info();

		if (array_mode) {
			if (!_is_actual_token(CST::TK_BRACKET_CLOSE)
				&& !_is_actual_token(CST::TK_COMMA))
			{
				end = true;
				cur_type = "";
				break;
			}
		}

		switch (GETTOKEN(0)) {

		CASEATYPICAL

		case CST::TK_OP_QUESTION_MARK: {
			// nullable type
			if (base_type) { cur_type += "?"; INCPOS(1); break; }
			else _unexpeced_token_error();
		}

		case CST::TK_OP_LESS: { // <

			generic_types_mode = true;
			cur_type += "<";
			INCPOS(1);
			cur_type += _parse_type(); // recursive
			break;
		}

		case CST::TK_OP_GREATER: { // >

			generic_types_mode = false;
			cur_type += ">";
			INCPOS(1);

			// this is end unless it is '['
			if (GETTOKEN(0) != CST::TK_BRACKET_OPEN) {
				end = true;
			}

			break;
		}

		case CST::TK_BRACKET_OPEN: { // [
			array_mode = true;
			cur_type += "[";
			INCPOS(1);
			if (array_constructor) { // eg: new int[5]
				string expr = _parse_expression(true,CSharpLexer::Token::TK_BRACKET_OPEN);
				cur_type += expr;
			}
			break;
		}
		case CST::TK_BRACKET_CLOSE: { // ]
			array_mode = false;
			cur_type += "]";
			INCPOS(1);

			// for sure this is end of type
			end = true; break;
		}

		case CST::TK_COMMA: { // ,
			if (generic_types_mode) {
				cur_type += ",";
				INCPOS(1);
				cur_type += _parse_type();
			}
			else if (array_mode) {
				cur_type += ",";
				INCPOS(1);
				if (array_constructor) { // eg: new int[5,6,7]
					string expr = _parse_expression(true, CSharpLexer::Token::TK_BRACKET_OPEN);
					cur_type += expr;
				}
			}
			else {
				_unexpeced_token_error();
				_escape();
				end = true;
			}
			break;
		}

		// base type?
		CASEBASETYPE {

			if (complex_type) {
				// error -> System.Reflexion.int ??????
				// vector<int> still allowed, cause int will be parsed recursively
				_unexpeced_token_error();
			}

			base_type = true;
			cur_type += CSharpLexer::token_names[(int)GETTOKEN(0)];
			INCPOS(1);

			// another has to be '?' either '[' - else it is finish
			if (GETTOKEN(0) != CST::TK_OP_QUESTION_MARK
				&& GETTOKEN(0) != CST::TK_BRACKET_OPEN)
			{
				end = true;
			}
			break;
		}

		// complex type
		case CST::TK_IDENTIFIER: {

			if (base_type) {
				_unexpeced_token_error();
			}

			complex_type = true;
			cur_type += TOKENDATA(0);
			INCPOS(1);

			// finish if
			if (_is_actual_token(CST::TK_PERIOD)) {
				cur_type += ".";
				INCPOS(1);
				break;
			}
			else if (_is_actual_token(CST::TK_OP_LESS)) {
				break; // don't skip!
			}
			else if (_is_actual_token(CST::TK_BRACKET_OPEN)) {
				break; // don't skip!
			}
			else { // none of '.', '<', '['
				end = true; break;
			}
			break;
		}
		default: {

			_unexpeced_token_error();
		}
		}
	}

	string res = cur_type;
	cur_type = prev_cur_type;
	return res;
}

bool CSharpParser::_parse_using_directive(FileNode* node)
{
	// TODO struct using directives node

	// using X;
	// using static X;
	// using X = Y;

	// is using?
	if (!_is_actual_token(CST::TK_KW_USING))
		return false; // no longer using
		
	INCPOS(1); // skip 'using'

	bool is_static = false;
	bool is_alias = false;

	if (_is_actual_token(CST::TK_KW_STATIC)) {
		INCPOS(1);
		is_static = true;
	}

	try {

		// read name or type
		string name_or_type = _parse_type();
		string refers_to;

		if (_is_actual_token(CST::TK_OP_ASSIGN)) {
			INCPOS(1);
			is_alias = true;
			refers_to = _parse_type();
		}

		_assert(CST::TK_SEMICOLON);
		INCPOS(1); // skip ';'
		node->using_directives.push_back(name_or_type);

	}
	catch (CSharpParserException&) {
		_skip_until_next_line(); // skip this using line
	}

	return true;
}

// returns information if should continue parsing member or if it is finish
bool CSharpParser::_parse_namespace_member(NamespaceNode* node)
{
	// parsing file is parsing namespace - if current token is eof (not '}')
	// then we can say it is end of 'parsing namespace'
	if (GETTOKEN(0) == CST::TK_EOF) {
		return false;
	}

	_parse_attributes();
	_apply_attributes(node);
	_parse_modifiers();

	debug_info();

	switch (GETTOKEN(0)) {

	CASEATYPICAL

	case CST::TK_KW_NAMESPACE: {
		NamespaceNode* member = _parse_namespace();
		if (member != nullptr) {
			node->namespaces.push_back(member);
			member->parent = node;
		}
		break;
	}
	case CST::TK_KW_CLASS: {
		ClassNode* member = _parse_class();
		if (member != nullptr) {
			node->classes.push_back(member);
			member->parent = node;
		}
		break;
	}
	case CST::TK_KW_STRUCT: {
		StructNode* member = _parse_struct();
		if (member != nullptr) {
			node->structures.push_back(member);
			member->parent = node;
		}
		break;
	}
	case CST::TK_KW_INTERFACE: {
		InterfaceNode* member = _parse_interface();
		if (member != nullptr) {
			node->interfaces.push_back(member);
			member->parent = node;
		}
		break;
	}
	case CST::TK_KW_ENUM: {
		EnumNode* member = _parse_enum();
		if (member != nullptr) {
			node->enums.push_back(member);
			member->parent = node;
		}
		break;
	}
	case CST::TK_KW_DELEGATE: {
		DelegateNode* member = _parse_delegate();
		if (member != nullptr) {
			node->delegates.push_back(member);
			member->parent = node;
		}
		break;
	}
	case CST::TK_CURLY_BRACKET_CLOSE: {

		INCPOS(1); // skip '}'
		return false;
	}

	default: {		
		_unexpeced_token_error();
	}
	}

	return true;

}

bool CSharpParser::_parse_class_member(ClassNode* node)
{
	debug_info();
	_parse_attributes();
	_parse_modifiers();

	switch (GETTOKEN(0)) {
	case CST::TK_KW_CLASS: {
		ClassNode* member = _parse_class();
		if (member != nullptr) {
			node->classes.push_back(member);
			member->parent = node;
		}
		break;
	}
	case CST::TK_KW_INTERFACE: {
		InterfaceNode* member = _parse_interface();
		if (member != nullptr) {
			node->interfaces.push_back(member);
			member->parent = node;
		}
		break;
	}
	case CST::TK_KW_STRUCT: {
		StructNode* member = _parse_struct();
		if (member != nullptr) {
			node->structures.push_back(member);
			member->parent = node;
		}
		break;
	}
	case CST::TK_KW_ENUM: {
		EnumNode* member = _parse_enum();
		if (member != nullptr) {
			node->enums.push_back(member);
			member->parent = node;
		}
		break;
	}
	case CST::TK_KW_DELEGATE: {
		DelegateNode* member = _parse_delegate();
		if (member != nullptr) {
			node->delegates.push_back(member);
			member->parent = node;
		}
		break;
	}
	case CST::TK_CURLY_BRACKET_CLOSE: {

		// TODO if depth ok - ok else error
		INCPOS(1);
		return false;
	}
	default: {

		TD td = tokens[pos];

		// field (var), property or method
		string type = _parse_type();

		// constructor?
		if (_is_actual_token(CST::TK_PARENTHESIS_OPEN)) {

			if (type != node->name) {
				_unexpeced_token_error();
				_escape();
				break;
			}

			this->modifiers |= (int)Modifier::MOD_CONSTRUCTOR;
			node->any_constructor_declared = true;
			MethodNode* member = _parse_method_declaration(type, type);
			if (member != nullptr) {
				node->methods.push_back(member);
				member->parent = node;
			}
			break;
		}

		// if not constructor here MUST be identifier
		_assert(CST::TK_IDENTIFIER);
		string name = TOKENDATA(0);
		INCPOS(1);

		// FIELD
		if (_is_actual_token(CST::TK_SEMICOLON)) {

			VarNode* member = new VarNode(name, type, td);
			_apply_modifiers(member); // is const?
			node->variables.push_back(member);
			member->parent = node;
			INCPOS(1); // skip ';'
		}

		// FIELD WITH ASSIGNMENT
		else if (_is_actual_token(CST::TK_OP_ASSIGN)) {
			
			VarNode* member = new VarNode(name, type, td);
			_apply_modifiers(member);
			node->variables.push_back(member);
			member->parent = node;
			INCPOS(1); // skip '='
			string expr = _parse_expression();
			_assert(CST::TK_SEMICOLON);
			INCPOS(1); // skip ';'
			member->value = expr;
			return member;
		}

		// PROPERTY
		else if (_is_actual_token(CST::TK_CURLY_BRACKET_OPEN)) {
			
			PropertyNode* member = _parse_property(name, type);
			if (member != nullptr) {
				node->properties.push_back(member);
				member->parent = node;
			}
			break;
		}

		// METHOD
		else if (_is_actual_token(CST::TK_PARENTHESIS_OPEN)) {

			MethodNode* member = _parse_method_declaration(name, type);
			if (member != nullptr) {
				node->methods.push_back(member);
				member->parent = node;
			}
			break;

		}

		else {
			_unexpeced_token_error();
		}

	}

	}

	return true;

}

bool CSharpParser::_parse_interface_member(InterfaceNode* node)
{
	_parse_attributes();
	// NOTE: there are no modifiers in interfaces

	switch (GETTOKEN(0)) {

	CASEATYPICAL

	case CST::TK_CURLY_BRACKET_CLOSE: {

		// TODO if depth ok - ok else error
		INCPOS(1);
		return false;
	}
	default: {

		// field (var), property or method
		string type = _parse_type();

		// expected name
		_assert(CST::TK_IDENTIFIER);

		string name = TOKENDATA(0);
		INCPOS(1); // skip name

		// PROPERTY
		if (_is_actual_token(CST::TK_CURLY_BRACKET_OPEN)) {

			PropertyNode* member = _parse_property(name, type);
			if (member != nullptr) {
				node->properties.push_back(member);
				member->parent = node;
			}
			break;
		}

		// METHOD
		else if (_is_actual_token(CST::TK_PARENTHESIS_OPEN)) {

			MethodNode* member = _parse_method_declaration(name, type, true);
			if (member != nullptr) {
				node->methods.push_back(member);
				member->parent = node;
			}
			break;

		}

		else {
			_unexpeced_token_error();
		}
	}

	}

	return true;
}

CSharpParser::VarNode* CSharpParser::_parse_declaration()
{
	// type name;
	// type name = x;
	// const type name = x;

	TD td = tokens[pos];
	VarNode* variable = new VarNode(td);
	Node* prev_current = current;
	this->current = variable;

	if (_is_actual_token(CST::TK_KW_CONST)) {
		variable->modifiers |= (int)CSM::MOD_CONST;
		INCPOS(1);
	}

	try {
		string type = _parse_type();
		variable->type = type;

		if (_is_actual_token(CST::TK_IDENTIFIER)) {
			variable->name = TOKENDATA(0);
			INCPOS(1);
		}
		else {
			delete variable;
			this->current = prev_current;
			return nullptr;
		}

		// int x;
		if (!_is_actual_token(CST::TK_OP_ASSIGN)) {
			this->current = prev_current;
			return variable;
		}
		// int x = expr;
		_assert(CST::TK_OP_ASSIGN);
		INCPOS(1); // skip '='
		string expression = _parse_expression();
		variable->value = expression;
	}
	catch (CSharpParserException&) {
		_escape();
		delete variable;
		variable = nullptr;
	}

	// restore
	this->current = prev_current;
	return variable;
}

CSharpParser::StructNode* CSharpParser::_parse_struct()
{
	// TODO
	return nullptr;
}

CSharpParser::InterfaceNode* CSharpParser::_parse_interface()
{
	TD td = tokens[pos];
	_assert(CST::TK_KW_INTERFACE);
	INCPOS(1); // skip 'interface'

	// read name
	_assert(CST::TK_IDENTIFIER);
	InterfaceNode* node = new InterfaceNode(td);
	Node* prev_current = current;
	this->current = node;

	node->name = TOKENDATA(0);
	root->node_shortcuts.insert({ node->fullname(),node });

	INCPOS(1); // skip name

	_apply_attributes(node);
	_apply_modifiers(node);

	try {
		// generic
		if (_is_actual_token(CST::TK_OP_LESS)) {
			node->is_generic = true;
			node->generic_declarations = _parse_generic_declaration();
		}

		// derived and implements
		if (_is_actual_token(CST::TK_COLON)) {
			node->base_types = _parse_derived_and_implements(node->is_generic);
		}

		if (node->base_types.empty()) // object is last on class chain
			node->base_types.push_back("object");

		// constraints
		if (_is_actual_token(CST::TK_KW_WHERE)) {
			node->constraints = _parse_constraints();
		}

		_assert(CST::TK_CURLY_BRACKET_OPEN);
		INCPOS(1); // skip '}'
		while (_parse_interface_member(node));
	}
	catch (CSharpParserException&) {
		_escape();
		delete node;
		node = nullptr;
	}

	// restore
	this->current = prev_current;
	return node;

}

CSharpParser::UsingNode* CSharpParser::_parse_using_statement()
{	
	TD td = tokens[pos];
	_assert(CST::TK_KW_USING);

	// using ( declaration or expression ) statement
	// using declaration // TODO: add this functionality (C# 8.0)

	UsingNode* node = new UsingNode(td);
	Node* prev_current = current;
	this->current = node;

	INCPOS(1); // skip 'using'
	node->raw = "using ";

	try {

		// USING DECLARATION VERSION
		if (!_is_actual_token(CST::TK_PARENTHESIS_OPEN)) {

			// TODO: doesn't work

			VarNode* variable = _parse_declaration();
			if (variable == nullptr) {
				return nullptr; // error
			}

			_assert(CST::TK_SEMICOLON);
			INCPOS(1); // skip ';'
			return node;

		}


		// NORMAL VERSION
		_assert(CST::TK_PARENTHESIS_OPEN);
		INCPOS(1); // skip '('
		node->raw += "(";

		// parse declaration or expression
		int cur_pos = pos;
		VarNode* variable = _parse_declaration();
		if (variable != nullptr) {
			// it was declaration
			node->local_variable = variable;
		}
		else {
			// it is expression
			pos = cur_pos; // reset and retry
			string expr = _parse_expression();
			node->raw += expr;
		}

		_assert(CST::TK_PARENTHESIS_CLOSE);
		INCPOS(1); // skip ')'
		node->raw += ")";

		StatementNode* statement = _parse_statement();
		node->body = statement;
		statement->parent = node;

	}
	catch (CSharpParserException&) {
		_escape();
		delete node;
		node = nullptr;
	}

	// restore
	this->current = prev_current;
	return node;
}


CSharpParser::ConditionNode* CSharpParser::_parse_if_statement()
{	
	// if ( expr ) statement else statement
	
	TD td = tokens[pos];
	
	_assert(CST::TK_KW_IF);
	INCPOS(1); // skip 'if'

	ConditionNode* node = new ConditionNode(td);
	Node* prev_current = current;
	this->current = node;

	node->raw = "if ";

	try {

		_assert(CST::TK_PARENTHESIS_OPEN);
		INCPOS(1); // skip '('
		node->raw += "(";

		string expression = _parse_expression(true,CSharpLexer::Token::TK_PARENTHESIS_OPEN);
		node->raw += expression;

		_assert(CST::TK_PARENTHESIS_CLOSE);
		INCPOS(1); // skip ')'
		node->raw += ") ";

		StatementNode* then = _parse_statement();
		if (then == nullptr)
			error("Failed to parse 'then' in IF statement.");

		then->parent = node;
		node->raw += then->raw;

		// optional else
		if (_is_actual_token(CST::TK_KW_ELSE)) {

			INCPOS(1); // skip 'else'
			node->raw += " else ";
			StatementNode* else_st = _parse_statement();
			if (else_st == nullptr)
				error("Failed to parse 'else' in IF statement.");

			else_st->parent = node;
			node->raw += else_st->raw;
		}
	}
	catch (CSharpParserException&) {
		_escape();
		delete node;
		node = nullptr;
	}

	// restore
	this->current = prev_current;
	return node;

}

CSharpParser::TryNode* CSharpParser::_parse_try_statement()
{
	// try { ... } (catch { ... } (when ...)? )* (finally { ... })?
	TD td = tokens[pos];

	_assert(CST::TK_KW_TRY);
	INCPOS(1); // skip 'try'

	TryNode* node = new TryNode(td);
	Node* prev_current = current;
	this->current = node;

	try {
		_assert(CST::TK_CURLY_BRACKET_OPEN);

		BlockNode* try_block = _parse_block();
		if (try_block == nullptr)
			error("Failed to parse block in TRY statement.");

		try_block->parent = node;

		while (_is_actual_token(CST::TK_KW_CATCH)) {

			INCPOS(1); // skip 'catch'
			_assert(CST::TK_PARENTHESIS_OPEN); INCPOS(1); // skip '('		
			_parse_expression(true,CST::TK_PARENTHESIS_OPEN); // ignore
			_assert(CST::TK_PARENTHESIS_CLOSE); INCPOS(1); // skip ')'

			// possible when
			if (_is_actual_token(CST::TK_KW_WHEN)) {

				INCPOS(1); // skip 'when'
				_assert(CST::TK_PARENTHESIS_OPEN); INCPOS(1); // skip '('
				_parse_expression(true,CST::TK_PARENTHESIS_OPEN); // ignore
				_assert(CST::TK_PARENTHESIS_CLOSE); INCPOS(1); // skip ')'
			}

			// block
			_assert(CST::TK_CURLY_BRACKET_OPEN);

			BlockNode* block = _parse_block();
			node->blocks.push_back(block);
			block->parent = node;
		}

		// possible finally
		if (_is_actual_token(CST::TK_KW_FINALLY)) {

			INCPOS(1); // skip 'finally'
			_assert(CST::TK_CURLY_BRACKET_OPEN);
			BlockNode* finally_block = _parse_block();
			node->blocks.push_back(finally_block);
			finally_block->parent = node;
		}
	}
	catch (CSharpParserException&) {
		_escape();
		delete node;
		node = nullptr;
	}

	this->current = prev_current;
	return node;
}

CSharpParser::ConditionNode* CSharpParser::_parse_switch_statement()
{	
	// switch (...) { (case ... (when ...)?)* default: ?}

	TD td = tokens[pos];
	_assert(CST::TK_KW_SWITCH);
	INCPOS(1); // skip 'switch'

	ConditionNode* node = new ConditionNode(td);
	Node* prev_current = current;
	this->current = node;

	try {
		_assert(CST::TK_PARENTHESIS_OPEN);
		INCPOS(1); // skip '('

		_skip_until_token(CST::TK_PARENTHESIS_CLOSE);
		INCPOS(1); // skip ')'

		_assert(CST::TK_CURLY_BRACKET_OPEN);

		_skip_until_token(CST::TK_CURLY_BRACKET_CLOSE);
		INCPOS(1); // skip '}'
	}
	catch (CSharpParserException&) {
		_escape();
		delete node;
		node = nullptr;
	}

	// restore
	this->current = prev_current;
	return node;
}

CSharpParser::StatementNode* CSharpParser::_parse_property_definition()
{
	TD td = tokens[pos];
	switch (GETTOKEN(0)) {

	CASEATYPICAL

	case CST::TK_OP_LAMBDA: {
		INCPOS(1); // skip '=>'
		return _parse_statement();
	}
	case CST::TK_SEMICOLON: {
		StatementNode* node = new StatementNode(td);
		node->raw = ";";
		debug_info();
		INCPOS(1);
		debug_info();
		return node; // empty statement
	}
	case CST::TK_CURLY_BRACKET_OPEN: {
		return _parse_block();
	}
	default: {
		_unexpeced_token_error();
		_escape();
	}

	}

	return nullptr;
}

CSharpParser::PropertyNode* CSharpParser::_parse_property(string name, string type)
{
	TD td = tokens[pos];
	_assert(CST::TK_CURLY_BRACKET_OPEN);
	INCPOS(1); // skip '{'

	PropertyNode* node = new PropertyNode(td);
	Node* prev_current = current;
	this->current = node;

	_apply_attributes(node);
	_apply_modifiers(node);

	node->name = name;
	node->type = type;
	root->node_shortcuts.insert({ node->fullname(),node });

	try {
		// we don't know what is declared first: get or set
		bool get_parsed = false;
		bool set_parsed = false;

		_parse_attributes();
		_parse_modifiers();

		bool end = false;
		while (!end) {

			switch (GETTOKEN(0)) {

			CASEATYPICAL

			case CST::TK_KW_GET: {
					if (get_parsed) {
						error("GET defined more then once while parsing property: " + name);
					}
					else {
						INCPOS(1); // skip 'get'
						node->get_modifiers = this->modifiers;
						node->get_statement = _parse_property_definition();
						get_parsed = true;
						if (!set_parsed) {
							// probably following definition of set
							_parse_attributes();
							_parse_modifiers();
						}
						break;
					}
				}
			case CST::TK_KW_SET: {
				if (set_parsed) {
					error("SET defined more then once while parsing property: " + name);
				}
				else {
					INCPOS(1); // skip 'set'
					node->set_modifiers = this->modifiers;
					this->kw_value_allowed = true;
					node->set_statement = _parse_property_definition();
					this->kw_value_allowed = false;
					set_parsed = true;
					if (!get_parsed) {
						// probably following definition of get
						_parse_attributes();
						_parse_modifiers();
					}
					break;
				}
			}
			case CST::TK_CURLY_BRACKET_CLOSE: {
				debug_info();
				INCPOS(1); // skip '}'

				// C# 6 - possible initialization
				if (GETTOKEN(0) == CST::TK_OP_ASSIGN) {
					INCPOS(1); // skip '='
					_parse_expression(); // ignore
					INCPOS(1); // skip ';'
				}

				this->current = prev_current;
				end = true; break;
			}
			default: {
				_unexpeced_token_error();
			}
			}
		}
	}
	catch (CSharpParserException&) {
		_escape();
		delete node;
		node = nullptr;
	}

	// restore
	this->current = prev_current;
	return node;
}


CSharpParser::StatementNode* CSharpParser::_parse_statement()
{
	try {

		TD td = tokens[pos];
		switch (GETTOKEN(0)) {

			CASEATYPICAL

		case CST::TK_CURLY_BRACKET_OPEN: {
				BlockNode* node = _parse_block();
				return node;
			}

		CASEBASETYPE {

			// static method from type, eg: int.TryParse("123",x);
			if (GETTOKEN(1) == CST::TK_PERIOD) {
				string expr = _parse_expression();
				StatementNode* n = new StatementNode(td);
				n->raw = expr;
				return n;
			}
			else { // declaration, eg: int x = 123;

				VarNode *variable = _parse_declaration();
				DeclarationNode* node = new DeclarationNode(td);

				// binding
				node->variable = variable;
				if (variable != nullptr)
					variable->parent = node;

				if (_is_actual_token(CST::TK_SEMICOLON)) {
					// finished normally
					INCPOS(1); // skip ';'
				}
				else if (_is_actual_token(CST::TK_CURLY_BRACKET_CLOSE)) {
					// finished probably with errors -> don't skip,
					// this is closing '}' for some outer block
					;
				}
				// TODO?? else: error ?

				return node;
			}
		}

		case CST::TK_KW_TRUE:
		case CST::TK_KW_FALSE:
		case CST::TK_KW_THIS:
		case CST::TK_KW_BASE: {

			StatementNode* node = new StatementNode(td);
			string expr = _parse_expression();
			_assert(CST::TK_SEMICOLON);
			INCPOS(1); // skip ';'
			node->raw = expr + ";";
			return node;

		}


		case CST::TK_IDENTIFIER: {

			// It can be:
			//  - declaration of custom type
			//  - expression (also function invocation)
			//  - assignment
			//  - label

			const int cur_pos = pos;

			// var declaration of custom type? (Foo x = new Foo();)
			VarNode* variable = _parse_declaration();
			if (variable != nullptr) {
				DeclarationNode* node = new DeclarationNode(td);
				variable->parent = node;
				node->variable = variable;
				INCPOS(1); // skip ';'
				return node;
			}

			pos = cur_pos; // reset
			StatementNode* node = new StatementNode(td);

			// label?
			if (_is_actual_token(CST::TK_COLON)) {
				node->raw += TOKENDATA(0) + ":";
				// todo add TOKENDATA(0) to labels set
				INCPOS(2);
				return node;
			}

			// assignment or expression?
			string expr = _parse_expression();
			_assert(CST::TK_SEMICOLON);
			INCPOS(1); // skip ';'
			node->raw = expr + ";";
			return node;

		}
		case CST::TK_KW_FOR:
		case CST::TK_KW_WHILE:
		case CST::TK_KW_FOREACH:
		case CST::TK_KW_DO: {
			LoopNode* node = _parse_loop();
			return node;
		}

		// CONDITION STATEMENT
		case CST::TK_KW_IF: {
			ConditionNode* node = _parse_if_statement();
			return node;
		}
		case CST::TK_KW_SWITCH: {
			ConditionNode* node = _parse_switch_statement();
			return node;
		}

		// JUMP STATEMENT
		case CST::TK_KW_BREAK:
		case CST::TK_KW_CONTINUE:
		case CST::TK_KW_GOTO:
		case CST::TK_KW_RETURN:
		case CST::TK_KW_YIELD:
		case CST::TK_KW_THROW:
		{
			JumpNode* node = _parse_jump();
			return node;
		}

		// TRY-CATCH-FINALLY
		case CST::TK_KW_TRY: {
			TryNode* node = _parse_try_statement();
			return node;
		}

		// USING STATEMENT
		case CST::TK_KW_USING: {
			UsingNode* node = _parse_using_statement();
			return node;
		}

		// OTHER
		case CST::TK_KW_AWAIT: {
			StatementNode* node = new StatementNode(td);
			node->raw += "await ";
			string expr = _parse_expression();
			node->raw += expr;
			_assert(CST::TK_SEMICOLON);
			INCPOS(1);
			node->raw += ";";
			return node;
		}


		case CST::TK_KW_FIXED:
		case CST::TK_KW_LOCK: {
			INCPOS(1);
			break;
		}
		case CST::TK_SEMICOLON: {
			StatementNode* node = new StatementNode(td);
			node->raw = ";";
			INCPOS(1);
			return node; // empty statement
		}
		default: {
			_unexpeced_token_error();
		}

		}

	}
	catch (const CSharpParserException&)
	{
		return nullptr;
	}

	return nullptr;
}

CSharpParser::MethodNode* CSharpParser::_parse_method_declaration(string name, string return_type, bool interface_context)
{
	TD td = tokens[pos-2];
	MethodNode* node = new MethodNode(td);
	Node* prev_current = current;
	MethodNode* prev_method = cur_method;
	this->current = node;
	this->cur_method = node;

	_apply_attributes(node);
	_apply_modifiers(node);

	node->name = name;
	node->return_type = return_type;

	try {
		_assert(CST::TK_PARENTHESIS_OPEN);
		INCPOS(1); // skip '('

		// ARGUMETNS
		VarNode* argument = new VarNode(td);
		bool end = false;
		while (!end) {
			debug_info();
			switch (GETTOKEN(0)) {
			case CST::TK_PARENTHESIS_CLOSE: {
				if (argument->name == "") delete argument; // no argument
				else node->arguments.push_back(argument);
				end = true; INCPOS(1); break;
			}
			case CST::TK_COMMA: {
				TD td = tokens[pos + 1];
				node->arguments.push_back(argument);
				argument = new VarNode(td);
				INCPOS(1);
				break;
			}
			case CST::TK_OP_ASSIGN: { // optional arguments (assigned by default)
				INCPOS(1);
				string val = _parse_expression();
				argument->value = val;
				break;
			}
			case CST::TK_KW_IN: {
				argument->modifiers |= (int)CSM::MOD_IN;
				INCPOS(1); // skip
				break;
			}
			case CST::TK_KW_OUT: {
				argument->modifiers |= (int)CSM::MOD_OUT;
				INCPOS(1); // skip
				break;
			}
			case CST::TK_KW_REF: {
				argument->modifiers |= (int)CSM::MOD_REF;
				INCPOS(1); // skip
				break;
			}
			case CST::TK_KW_PARAMS: {
				argument->modifiers |= (int)CSM::MOD_PARAMS;
				INCPOS(1); // skip
				break;
			}

			default: {

				string type = _parse_type();
				_assert(CST::TK_IDENTIFIER);
				string name = TOKENDATA(0);
				INCPOS(1);
				argument->type = type;
				argument->name = name;
			}
			}
		}

		// TODO GENERIC METHODS ( : where T is ... )
		debug_info();
		if (_is_actual_token(CST::TK_COLON)) {
			INCPOS(1); // skip ':'
			if (_is_actual_token(CST::TK_KW_BASE)) {
				INCPOS(1); // skip 'base'
				_parse_method_invocation();
			}
		}

		// ----- ----- -----
		//       BODY
		// ----- ----- -----

		if (interface_context) {
			_assert(CST::TK_SEMICOLON);
			INCPOS(1); // skip ';'
		}
		else { // class context
			BlockNode* body_node = _parse_block();
			if (body_node != nullptr)
			{
				// binding
				node->body = body_node;
				body_node->parent = node;
			}
		}

		// ADD SHORTCUT
		root->node_shortcuts.insert({ node->fullname(),node });

	}
	catch (CSharpParserException&) {
		_escape();
		delete node;
		node = nullptr;
	}

	// restore
	this->current = prev_current;
	this->cur_method = prev_method;
	return node;
}

// parse just invocation without name 
string CSharpParser::_parse_method_invocation()
{
	_assert(CST::TK_PARENTHESIS_OPEN);
	INCPOS(1);
	cur_expression += "(";

	// ARGUMETNS
	while (!_is_actual_token(CST::TK_PARENTHESIS_CLOSE)) {

		if (_is_actual_token(CST::TK_COMMA)) {
			cur_expression += " , ";
			INCPOS(1);
		}
		else {
			cur_expression += _parse_expression(true, CST::TK_PARENTHESIS_OPEN);
		}

	}

	cur_expression += ")";
	INCPOS(1);

	return ""; // TODO: should it return 'raw' statement?

}

CSharpParser::BlockNode* CSharpParser::_parse_block()
{	
	TD td = tokens[pos];
	_assert(CST::TK_CURLY_BRACKET_OPEN);
	INCPOS(1); // skip '{'

	BlockNode* node = new BlockNode(td);
	Node* prev_current = current;
	BlockNode* prev_block = cur_block;
	this->current = node;
	this->cur_block = node;

	bool error_found = false;
	StatementNode* statement = nullptr;
	while (!_is_actual_token(CST::TK_CURLY_BRACKET_CLOSE)) {

		statement = _parse_statement();
		if (statement == nullptr) {
			// failed to parse statement (possible error)
			_escape(); // go to '}'
			error_found = true;
			break; // skip loop
		}

		// bind
		statement->parent = node;
		node->statements.push_back(statement);
	}

	if (!error_found)
		INCPOS(1); // skip '}'

	// restore
	this->current = prev_current;
	this->cur_block = prev_block;
	return node;

}


// escape puts cursor behind '}'
void CSharpParser::_escape()
{
	if (cur_block != nullptr)          _skip_until_end_of_block();
	else if (cur_class != nullptr)     _skip_until_end_of_class();
	else if (cur_namespace != nullptr) _skip_until_end_of_namespace();
}

void CSharpParser::_skip_until_end_of_block()
{
	if (cur_block == nullptr) return;
	int dest_depth = cur_block->creator.depth;
	while (pos < len && tokens[pos].depth > dest_depth) pos++;
	pos++; // skip '}'
}

void CSharpParser::_skip_until_end_of_class()
{
	if (cur_class == nullptr) return;
	int dest_depth = cur_class->creator.depth;
	while (pos < len && tokens[pos].depth > dest_depth) pos++;
	pos++; // skip '}'
}

void CSharpParser::_skip_until_end_of_namespace()
{
	if (cur_namespace == nullptr) return;
	int dest_depth = cur_namespace->creator.depth;
	while (pos < len && tokens[pos].depth > dest_depth) pos++;
	pos++; // skip '}'
}

void CSharpParser::_skip_until_next_line()
{
	int cur_line = tokens[pos].line;
	while (pos < len && tokens[pos].line == cur_line) pos++;
}

bool CSharpParser::is_base_type(string type) {

	remove_array_type(type);

	string base_type[] = {
		"bool", "byte", "sbyte"
		"char", "string",
		"decimal", "double", "float",
		"int", "long", "short",
		"uint", "ulong", "ushort",
		"void", "object"
	};

	string base_type_object[] = {
		"Boolean", "Byte", "SByte"
		"Char", "String",
		"Decimal", "Double", "Single",
		"Int32", "Int64", "Int16",
		"UInt32", "UInt64", "UInt16",
		"Void", "Object"
	};

	for (string x : base_type)
		if (x == type)
			return true;

	for (string x : base_type_object)
		if (x == type)
			return true;

	return false;
}

int CSharpParser::compute_rank(const string& type)
{
	int n = type.size();
	if (n < 2) return 0;

	int rank = 0;
	if (type[n - 1] == ']') {
		for (int i = n - 2; i >= 0; i--) {
			rank++;
			if (type[i] == '[')
				break;
		}
	}

	return rank;
}

// removes array chars from type and returns the dimension (rank) of type
// eg.: int[,] -> int * 2
int CSharpParser::remove_array_type(string& array_type)
{
	int n = array_type.length();
	if (n < 2) return 0;

	int rank = compute_rank(array_type);
	if (rank > 0)
		array_type = array_type.substr(0, n-rank-1);

	return rank;
}

void CSharpParser::add_array_type(string & type, int rank)
{
	if (rank <= 0) return;

	type += "[";
	for (int i = 1; i < rank; i++)
		type += ",";

	type += "]";

}

// ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** //
// ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** //
//                               ===========================================================                               //
//                                    ----------------  NODES FUNCTIONS ---------------                                    //
//                               ===========================================================                               //
// ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** //
// ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** //

bool CSharpParser::Node::is_public()
{
	return modifiers & (int)Modifier::MOD_PUBLIC;
}

bool CSharpParser::Node::is_protected()
{
	return modifiers & (int)Modifier::MOD_PROTECTED;
}

bool CSharpParser::Node::is_private()
{
	return modifiers & (int)Modifier::MOD_PRIVATE;
}

bool CSharpParser::Node::is_static()
{
	return modifiers & (int)Modifier::MOD_STATIC;
}

void CSharpParser::Node::print_header(int indent, const vector<string> &v, string title) const
{
	indentation(indent);
	cout << title;
	for (string x : v)
		cout << " " << x;
	cout << endl;
}

CSharpParser::Node::Node(Type t, TD td)
{
	this->creator = td;
	this->node_type = t;
}

CSharpParser::Node::~Node()
{
	if (active_parser->cinfo.ctx_cursor == this) {
		active_parser->cinfo.ctx_cursor = nullptr;
		// deleting cursor allowed!
	}
	else if (active_parser->cinfo.ctx_block == this) {
		active_parser->cinfo.ctx_block = nullptr;
		active_parser->cinfo.error = 1;
	}
	else if (active_parser->cinfo.ctx_class == this) {
		active_parser->cinfo.ctx_class = nullptr;
		active_parser->cinfo.error = 1;
	}
	else if (active_parser->cinfo.ctx_namespace == this) {
		active_parser->cinfo.ctx_namespace = nullptr;
		active_parser->cinfo.error = 1;
	}

	else {

		// restore path from cursor to root if one node from path is deleted
		for (CSP::Node* temp = active_parser->cinfo.ctx_cursor;
			temp != nullptr && active_parser->cinfo.error == 0; temp = temp->parent) {

			if (temp == this)
				active_parser->cinfo.error = 1;

			//if (temp->parent == this) {
			//	if (temp->parent->parent != nullptr)
			//		temp->parent = temp->parent->parent;
			//	else
			//		temp->parent = nullptr;
			//}

		}
	}
}

CSharpParser::BlockNode::~BlockNode()
{
	FOREACH_DELETE(statements);
}

list<CSharpParser::VarNode*> CSharpParser::BlockNode::get_visible_vars(int visibility) const
{
	list<VarNode*> res;

	// all statements in current block are BEFORE cursor
	int n = statements.size();
	for (int i = n - 1; i >= 0; i--) {

		StatementNode* node = statements[i];
		// declaration -> add variable to visibles
		if (node->node_type == Type::DECLARATION && is_unique(res,node))
			res.push_back(((DeclarationNode*)node)->variable);

	}

	// all visible in parent
	if (parent != nullptr) {
		auto visible_vars = parent->get_visible_vars(visibility);
		MERGE_LISTS_COND(res, visible_vars, IS_UNIQUE(res));
	}

	return res;
}

list<CSharpParser::TypeNode*> CSharpParser::NamespaceNode::get_visible_types(int visibility) const
{
	list<TypeNode*> res;
	if (parent != nullptr) // private no longer visible in parent
		res = parent->get_visible_types(visibility & ~VIS_PRIVATE);

	MERGE_LISTS_COND(res, structures, VISIBILITY_COND);
	MERGE_LISTS_COND(res, classes, VISIBILITY_COND);
	MERGE_LISTS_COND(res, interfaces, VISIBILITY_COND);

	return res;
}

list<CSharpParser::Node*> CSharpParser::NamespaceNode::get_members(const string name, int visibility) const
{
	list<CSharpParser::Node*> res;

	MERGE_LISTS_COND(res, namespaces, (CHILD_COND && VISIBILITY_COND));
	MERGE_LISTS_COND(res, classes,    (CHILD_COND && VISIBILITY_COND));
	MERGE_LISTS_COND(res, structures, (CHILD_COND && VISIBILITY_COND));
	MERGE_LISTS_COND(res, interfaces, (CHILD_COND && VISIBILITY_COND));
	MERGE_LISTS_COND(res, enums,      (CHILD_COND && VISIBILITY_COND));
	MERGE_LISTS_COND(res, delegates,  (CHILD_COND && VISIBILITY_COND));

	return res;
}

// default
list<CSharpParser::MethodNode*> CSharpParser::Node::get_visible_methods(int visibility) const
{
	if (parent != nullptr)
		return parent->get_visible_methods(visibility);

	return list<MethodNode*>();
}

// deafult
list<CSharpParser::VarNode*> CSharpParser::Node::get_visible_vars(int visibility) const
{
	if (parent != nullptr)
		return parent->get_visible_vars(visibility);

	return list<VarNode*>();
}

// default
list<CSharpParser::Node*> CSharpParser::Node::get_members(const string name, int visibility) const
{
	return list<CSharpParser::Node*>();
}

string CSharpParser::MethodNode::get_type() const
{
	if (CSP::is_base_type(return_type))
		return return_type;
	else {
		list<CSP::TypeNode*> tn = csc->get_types_by_name(return_type);
		if (tn.empty()) {
			return return_type; // TODO error? not found
		}
		else {
			return tn.front()->fullname();
		}
	}
}

list<CSP::Node*> CSharpParser::VarNode::get_members(const string name, int visibility) const
{
	string return_type = get_type();
	if (return_type.empty())
		return list<CSP::Node*>(); // TODO: is_base_type -> get members for base types

	if (is_base_type(return_type))
		return csc->get_children_of_base_type(return_type,name);

	// else -> cast to TypeNode and get members
	auto nodes = csc->get_nodes_by_expression(return_type);

	if (nodes.size() == 0)
		return list<CSP::Node*>();

	CSP::TypeNode* type_of_var = (CSP::TypeNode*)nodes.front();

	visibility &= ~VIS_STATIC; // no longer static!
	visibility |= VIS_NONSTATIC;
	visibility = csc->get_visibility_by_invoker_type(type_of_var, visibility);

	return type_of_var->get_members(name, visibility);

}

string CSharpParser::VarNode::prettyname() const 
{
	return name + " : " + type;
}

list<CSharpParser::VarNode*> CSharpParser::MethodNode::get_visible_vars(int visibility) const
{
	list<VarNode*> res;

	// arguments are visible
	MERGE_LISTS(res, arguments);

	// vars from class
	if (parent != nullptr) {
		auto visible_vars = parent->get_visible_vars(visibility);
		MERGE_LISTS_COND(res, visible_vars, IS_UNIQUE(res));
	}

	return res;
}

bool CSharpParser::MethodNode::is_constructor()
{
	return this->modifiers & (int)Modifier::MOD_CONSTRUCTOR;
}

CSharpParser::InterfaceNode::~InterfaceNode()
{
	FOREACH_DELETE(methods);
	FOREACH_DELETE(properties);
}

void CSharpParser::InterfaceNode::print(int indent) const
{
	indentation(indent);
	cout << "INTERFACE " << name << endl;

	// HEADERS:
	if (base_types.size() > 0)     print_header(indent + TAB, base_types, "> base types:");
	if (methods.size() > 0)		   print_header(indent + TAB, methods, "> methods:");
	if (properties.size() > 0)	   print_header(indent + TAB, properties, "> properties:");

}

list<CSharpParser::Node*> CSharpParser::InterfaceNode::get_members(const string name, int visibility) const
{
	list<Node*> res;

	// everything is 'public'
	MERGE_LISTS(res, properties);
	MERGE_LISTS(res, methods);

	return res;
}

void CSharpParser::EnumNode::print(int indent) const
{
	indentation(indent);     cout << "ENUM " << name << ":" << endl;
	indentation(indent + 2); cout << "type: " << type << endl;

	if (members.size() > 0)
		print_header(indent + TAB, members, "> members:");

}

CSharpParser::PropertyNode::~PropertyNode()
{
	delete get_statement;
	delete set_statement;
}

void CSharpParser::PropertyNode::print(int indent) const
{
	// TODO
}

CSharpParser::FileNode* CSharpParser::Node::get_parent_file()
{
	Node* temp = this;
	while (temp->parent != nullptr)
		temp = temp->parent;

	if (temp->node_type == Type::FILE)
		return (FileNode*)temp;

	return nullptr;

}

string CSharpParser::Node::fullname() const
{
	if (parent == nullptr || parent->node_type == Type::FILE)
		return name;

	if (IS_TYPE_NODE(parent) || parent->node_type == Type::NAMESPACE)
		return parent->fullname() + "." + name;

	return name;
}

string CSharpParser::Node::prettyname() const
{
	return name;
}


// default
list<CSharpParser::NamespaceNode*> CSharpParser::Node::get_visible_namespaces() const
{
	if (parent != nullptr)
		return parent->get_visible_namespaces();

	return list<NamespaceNode*>();
}

// default
list<CSharpParser::TypeNode*> CSharpParser::Node::get_visible_types(int visibility) const
{
	if (parent != nullptr)
		return parent->get_visible_types(visibility);

	return list<TypeNode*>();
}

list<CSharpParser::NamespaceNode*> CSharpParser::NamespaceNode::get_visible_namespaces() const
{
	list<NamespaceNode*> res;
	if (parent != nullptr)
		res = parent->get_visible_namespaces();

	MERGE_LISTS(res, namespaces);

	return res;
}

void CSharpParser::ClassNode::print(int indent) const
{
	indentation(indent);
	cout << "CLASS " << name << ":" << endl;

	// HEADERS:
	if (base_types.size() > 0)  print_header(indent + TAB, base_types, "> base types:");
	if (interfaces.size() > 0)	print_header(indent + TAB, interfaces, "> interfaces:");
	if (classes.size() > 0)		print_header(indent + TAB, classes, "> classes:");
	if (structures.size() > 0)	print_header(indent + TAB, structures, "> structures:");
	if (enums.size() > 0)		print_header(indent + TAB, enums, "> enums:");
	if (methods.size() > 0)		print_header(indent + TAB, methods, "> methods:");
	if (properties.size() > 0)	print_header(indent + TAB, properties, "> properties:");
	if (variables.size() > 0)	print_header(indent + TAB, variables, "> variables:");

	// NODES:
	if (interfaces.size() > 0)	for (InterfaceNode* x : interfaces)	x->print(indent + TAB);
	if (classes.size() > 0)		for (ClassNode* x : classes)		x->print(indent + TAB);
	if (structures.size() > 0)	for (StructNode* x : structures)	x->print(indent + TAB);
}

CSharpParser::StructNode::~StructNode()
{
	FOREACH_DELETE(variables);
	FOREACH_DELETE(methods);
	FOREACH_DELETE(classes);
	FOREACH_DELETE(interfaces);
	FOREACH_DELETE(structures);
	FOREACH_DELETE(enums);
	FOREACH_DELETE(properties);
	FOREACH_DELETE(delegates);
}

void CSharpParser::StructNode::print(int indent) const
{
	indentation(indent);
	cout << "STRUCT " << name << ":" << endl;

	// HEADERS:
	if (base_types.size() > 0)     print_header(indent + TAB, base_types, "> base types:");
	if (interfaces.size() > 0)	   print_header(indent + TAB, interfaces, "> interfaces:");
	if (classes.size() > 0)		   print_header(indent + TAB, classes, "> classes:");
	if (structures.size() > 0)	   print_header(indent + TAB, structures, "> structures:");
	if (enums.size() > 0)		   print_header(indent + TAB, enums, "> enums:");
	if (methods.size() > 0)		   print_header(indent + TAB, methods, "> methods:");
	if (properties.size() > 0)	   print_header(indent + TAB, properties, "> properties:");
	if (variables.size() > 0)	   print_header(indent + TAB, variables, "> variables:");

	// NODES:
	if (interfaces.size() > 0)	for (InterfaceNode* x : interfaces)	x->print(indent + TAB);
	if (classes.size() > 0)		for (ClassNode* x : classes)		x->print(indent + TAB);
	if (structures.size() > 0)	for (StructNode* x : structures)	x->print(indent + TAB);
}

list<CSharpParser::TypeNode*> CSharpParser::StructNode::get_visible_types(int visibility) const
{
	list<TypeNode*> res;

	// types from current node
	MERGE_LISTS_COND(res, structures, VISIBILITY_COND);
	MERGE_LISTS_COND(res, classes, VISIBILITY_COND);
	MERGE_LISTS_COND(res, interfaces, VISIBILITY_COND);
	
	if (!(visibility & VIS_STATIC)) { // if static -> don't take anything from base classes!

		// derived types
		for (auto x : base_types) {
			CSP::TypeNode* type_node = csc->get_type_by_expression(x);
			if (type_node != nullptr) { // from base class -> no longer private
				auto visible_from_base = type_node->get_visible_types(visibility & ~VIS_PRIVATE);
				MERGE_LISTS(res, visible_from_base);
			}
		}

		// nested types (eg. class defined inside class)
		if (parent != nullptr) {
			if (IS_TYPE_NODE(parent)) {
				auto outside_types = parent->get_visible_types(visibility);
				MERGE_LISTS(res, outside_types);
			}
		}
	}


	return res;
}

list<CSharpParser::MethodNode*> CSharpParser::StructNode::get_visible_methods(int visibility) const
{
	list<MethodNode*> res;

	// methods from current node
	MERGE_LISTS_COND(res, methods, 
		((!!(visibility & VIS_CONSTRUCT) == x->is_constructor()) && VISIBILITY_COND));

	if (!(visibility & VIS_STATIC)) { // if static -> don't take anything from base classes!

		// derived methods
		for (auto x : base_types) {
			CSP::TypeNode* type_node = csc->get_type_by_expression(x);
			if (type_node != nullptr) { // from base class -> no longer private
				auto visible_from_base = type_node->get_visible_methods(visibility & ~VIS_PRIVATE);
				MERGE_LISTS(res, visible_from_base);
			}
		}
	}

	return res;
}

list<CSharpParser::VarNode*> CSharpParser::StructNode::get_visible_vars(int visibility) const
{
	list<VarNode*> res;

	// vars from current node
	MERGE_LISTS_COND(res, variables, VISIBILITY_COND);
	MERGE_LISTS_COND(res, properties, VISIBILITY_COND);

	if (!(visibility & VIS_STATIC)) { // if static -> don't take anything from base classes!

		// derived types
		for (auto x : base_types) {
			CSP::TypeNode* type_node = csc->get_type_by_expression(x);
			if (type_node != nullptr) { // from base class -> no longer private
				auto visible_from_base = type_node->get_visible_vars(visibility & ~VIS_PRIVATE);
				MERGE_LISTS_COND(res, visible_from_base, IS_UNIQUE(res));
			}
		}
	}

	return res;
}

list<CSharpParser::Node*> CSharpParser::StructNode::get_members(const string name, int visibility) const
{
	list<Node*> res;
	
	auto visible_vars = get_visible_vars(visibility);
	auto visible_methods = get_visible_methods(visibility);
	auto visible_types = get_visible_types(visibility);

	MERGE_LISTS_COND(res, visible_types, CHILD_COND);
	MERGE_LISTS_COND(res, visible_vars, CHILD_COND);
	MERGE_LISTS_COND(res, visible_methods, CHILD_COND);

	return res;
}

list<CSP::MethodNode*> CSharpParser::StructNode::get_constructors(int visibility) const
{
	list<CSP::MethodNode*> res;

	MERGE_LISTS_COND(res, methods, 
		((x->modifiers & (int)Modifier::MOD_CONSTRUCTOR) && VISIBILITY_COND));

	return res;
}

CSP::MethodNode* CSharpParser::StructNode::create_auto_generated_constructor()
{
	CSharpLexer::TokenData td;
	MethodNode* node = new MethodNode(td);

	node->name = this->name;
	node->return_type = this->name;
	node->node_type = CSP::Node::Type::METHOD;
	node->parent = this;
	node->modifiers = ((int)Modifier::MOD_PUBLIC | (int)Modifier::MOD_CONSTRUCTOR);

	return node;
}

void CSharpParser::StatementNode::print(int indent) const
{
	indentation(indent);
	cout << "STATEMENT" << endl;
}

void CSharpParser::VarNode::print(int indent) const
{
	indentation(indent);
	cout << "VAR: ";
	if (modifiers & (int)CSM::MOD_CONST) cout << "const ";
	cout << type << " " << name << endl;
}

string CSharpParser::VarNode::get_type() const
{
	// deduce var type and remember it!
	if (type == "var") {

		if (value.empty())
			return wldc;

		auto ctx = csc->cinfo.ctx_cursor;
		if (ctx == nullptr)
			return wldc;

		string val = value;
		bool from_collection = false;
		if (parent != nullptr) { // expression from collection when 'var' comes from 'foreach'
			if (parent->node_type == Node::Type::LOOP) {
				LoopNode* lp = (LoopNode*)parent;
				if (lp != nullptr) {
					if (lp->loop_type == LoopNode::Type::FOREACH)
						from_collection = true;
				}
			}
		}

		auto nodes = csc->get_nodes_by_expression(val);
		if (nodes.empty())
			return wldc;

		Node* resolved_value = nodes.front();

		// ----- TODO for future: -----
		// collection types...? get type from collection
		// for now -> ONLY arrays are collections

		if (IS_TYPE_NODE(resolved_value)) {
			string new_type = ((TypeNode*)resolved_value)->name;
			
			if (from_collection) // for now -> if it comes from the collection, it must be an array!
				remove_array_type(new_type);

			type = new_type; // remember resolved type
			return type;
		}
		else if (resolved_value->node_type == Type::VAR || resolved_value->node_type == Type::PROPERTY) {
			string new_type = ((VarNode*)resolved_value)->get_type();

			if (from_collection) // for now -> if it comes from the collection, it must be an array!
				remove_array_type(new_type);

			type = new_type; // remember resolved type
			return type;
		}
		
		// else failed
		return CSP::wldc;
	}
	
	// base type
	if (CSP::is_base_type(type))
		return type;

	// type node
	list<CSP::TypeNode*> type_nodes = csc->get_types_by_name(type);
	if (type_nodes.empty()) {
		return type; // TODO error? not found
	}
	else
		return type_nodes.front()->fullname();

}

CSharpParser::MethodNode::~MethodNode()
{
	FOREACH_DELETE(arguments);
}

void CSharpParser::MethodNode::print(int indent) const
{
	indentation(indent);
	cout << "METHOD " << return_type << " " << name << endl;
	cout << "(";
	for (VarNode* v : arguments)
		cout << v->type << " " << v->name << " , ";
	cout << ")" << endl;

}

string CSharpParser::MethodNode::fullname() const
{
	string res;
	if (parent != nullptr)
		res = parent->fullname() + ".";

	res += name + "(";
	int n = arguments.size();
	if (n > 0) {
		for (int i = 0; i < n - 1; i++)
			res += arguments[i]->get_type() + ","; // TODO trzeba oszacowac typ np C1 -> N1.N2.C1
		res += arguments[n - 1]->get_type();
	}
	res += ")";

	return res;
}

string CSharpParser::MethodNode::prettyname() const
{
	string res = name;
	int cur_arg = -1;

	if (csc->cinfo.ctx_cursor != nullptr) {
		cur_arg = csc->cinfo.cur_arg;
	}

	res += '(';
	int n = arguments.size();
	for (int i = 0; i < n; i++) {
		if (i == cur_arg) res += "|>";
		res += arguments[i]->type;
		if (!arguments[i]->name.empty()) {
			res += " ";
			res += arguments[i]->name;
		}
		if (i == cur_arg) res += "<|";

		if (i < n - 1)
			res += ", ";
	}

	res += ")";
	res += " : ";
	res += return_type;

	return res;
}

CSharpParser::NamespaceNode::~NamespaceNode()
{
	FOREACH_DELETE(namespaces);
	FOREACH_DELETE(classes);
	FOREACH_DELETE(structures);
	FOREACH_DELETE(interfaces);
	FOREACH_DELETE(enums);
	FOREACH_DELETE(delegates);
}

void CSharpParser::NamespaceNode::print(int indent) const
{
	indentation(indent);
	cout << "NAMESPACE " << name << ":" << endl;

	// HEADERS:
	if (namespaces.size() > 0) print_header(indent + TAB, namespaces, "> namespaces:");
	if (interfaces.size() > 0) print_header(indent + TAB, interfaces, "> interfaces:");
	if (classes.size() > 0)    print_header(indent + TAB, classes, "> classes:");
	if (structures.size() > 0) print_header(indent + TAB, structures, "> structures:");
	if (enums.size() > 0)      print_header(indent + TAB, enums, "> enums:");

	// NODES:
	if (namespaces.size() > 0) for (NamespaceNode* x : namespaces) x->print(indent + TAB);
	if (interfaces.size() > 0) for (InterfaceNode* x : interfaces) x->print(indent + TAB);
	if (classes.size() > 0)    for (ClassNode* x : classes)        x->print(indent + TAB);
	if (structures.size() > 0) for (StructNode* x : structures)    x->print(indent + TAB);
}


// zwraca zewnetrzne symbole widoczne w pliku
set<string> CSharpParser::FileNode::get_external_identifiers()
{
	set<string> res;

	for (auto n : namespaces) res.insert(n->name);
	for (auto c : classes)    res.insert(c->name);
	for (auto s : structures) res.insert(s->name);
	for (auto i : interfaces) res.insert(i->name);
	for (auto e : enums)      res.insert(e->name);
	for (auto d : delegates)  res.insert(d->name);

	return res;
}

CSharpParser::FileNode::~FileNode()
{
}

void CSharpParser::FileNode::print(int indent) const
{
	indentation(indent);
	cout << "FILE " << name << ":" << endl;

	// HEADERS:
	if (using_directives.size() > 0) print_header(indent + TAB, using_directives, "> using:");
	if (namespaces.size() > 0) print_header(indent + TAB, namespaces, "> namespaces:");
	if (interfaces.size() > 0) print_header(indent + TAB, interfaces, "> interfaces:");
	if (classes.size() > 0)    print_header(indent + TAB, classes, "> classes:");
	if (structures.size() > 0) print_header(indent + TAB, structures, "> structures:");
	if (enums.size() > 0)      print_header(indent + TAB, enums, "> enums:");

	// NODES:
	if (namespaces.size() > 0) for (NamespaceNode* x : namespaces) x->print(indent + TAB);
	if (interfaces.size() > 0) for (InterfaceNode* x : interfaces) x->print(indent + TAB);
	if (classes.size() > 0)    for (ClassNode* x : classes)        x->print(indent + TAB);
	if (structures.size() > 0) for (StructNode* x : structures)    x->print(indent + TAB);

}

string CSharpParser::FileNode::fullname() const
{
	return ""; // empty
}

int CSharpParser::TypeNode::rank() const
{
	return compute_rank(this->name);
}

CSP::TypeNode * CSharpParser::TypeNode::create_array_type(int rank) const
{
	TypeNode* tn = new ClassNode();
	tn->name = this->name;
	CSP::add_array_type(tn->name, rank);
	tn->base_types.push_back("System.Array");
	tn->node_type = Node::Type::CLASS;

	return tn;
}

list<CSP::VarNode*> CSharpParser::LoopNode::get_visible_vars(int visibility) const
{
	list<VarNode*> res;

	if (local_variable != nullptr) {
		res.push_back(local_variable);
	}

	// all visible in parent
	if (parent != nullptr) {
		auto visible_vars = parent->get_visible_vars(visibility);
		MERGE_LISTS_COND(res, visible_vars, IS_UNIQUE(res));
	}

	return res;
}
