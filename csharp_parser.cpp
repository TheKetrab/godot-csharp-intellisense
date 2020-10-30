#include "csharp_parser.h"
#include <iostream>

using CST = CSharpLexer::Token;
using CSM = CSharpParser::Modifier;
using namespace std;

#define GETTOKEN(ofs) ((ofs + pos) >= len ? CST::TK_ERROR : tokens[ofs + pos].type)
#define TOKENDATA(ofs) ((ofs + pos >= len ? "" : tokens[ofs + pos].data))
#define INCPOS(ammount) { pos += ammount; }

#define CASEATYPICAL(def_return) \
	case CST::TK_EOF: { return def_return; } \
	case CST::TK_ERROR: { error("Found error token."); return def_return; } \
	case CST::TK_EMPTY: { error("Empty token -> skipped."); INCPOS(1); break; } \
	case CST::TK_CURSOR: { \
		cursor = current; \
		completion_namespace = cur_namespace; \
		completion_class = cur_class; \
		completion_method = cur_method; \
		completion_block = cur_block; \
		INCPOS(1); break; }

#define CASEBASETYPE \
		case CST::TK_KW_BOOL: \
		case CST::TK_KW_BYTE: \
		case CST::TK_KW_CHAR: \
		case CST::TK_KW_DECIMAL: \
		case CST::TK_KW_DOUBLE: \
		case CST::TK_KW_DYNAMIC: \
		case CST::TK_KW_FLOAT: \
		case CST::TK_KW_INT: \
		case CST::TK_KW_LONG: \
		case CST::TK_KW_OBJECT: \
		case CST::TK_KW_SBYTE: \
		case CST::TK_KW_SHORT: \
		case CST::TK_KW_STRING: \
		case CST::TK_KW_UINT: \
		case CST::TK_KW_ULONG: \
		case CST::TK_KW_USHORT: \
		case CST::TK_KW_VAR: \
		case CST::TK_KW_VOID:

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


CSharpParser::FileNode* CSharpParser::parse() {

	clear_state();
	this->tokens = tokens;
	FileNode* node = _parse_file();

	return node;
}

CSharpParser::CSharpParser(string code) {

	clear_state();

	// lexical analyze
	CSharpLexer lexer(code);
	lexer.tokenize();

	this->tokens = lexer.get_tokens();
	this->len = this->tokens.size();

}


void CSharpParser::clear_state() {

	cur_namespace    = nullptr;
	cur_class        = nullptr;
	cur_method       = nullptr;
	cur_block        = nullptr;
	current          = nullptr;
	cursor           = nullptr;
	pos              = 0;      
	
	modifiers        = 0;
	kw_value_allowed = false;

	completion_type      = CompletionType::COMPLETION_NONE;
	completion_namespace = nullptr;
	completion_class     = nullptr;
	completion_method    = nullptr;
	completion_block     = nullptr;
}


void CSharpParser::_parse_modifiers() {

	this->modifiers = 0;
	while (true) {

		switch (GETTOKEN(0)) {

		CASEMODIFIER {
			modifiers |= (int)to_modifier[GETTOKEN(0)];
			pos++;
			break;
		}
		default: {
			return; // end of modifiers, sth other
		}
		}
	}
}

void CSharpParser::error(string msg) const {
	cout << "--> Error: " << msg << endl;
}

void CSharpParser::_unexpeced_token_error() const {
	string msg = "Unexpected token: ";
	msg += CSharpLexer::token_names[(int)(GETTOKEN(0))];
	error(msg);
}

// upewnia sie czy aktualny token jest taki jak trzeba
// jesli current to CURSOR to ustawia cursor i sprawdza
// czy nastepny token jest wymaganym przez assercje tokenem
bool CSharpParser::_assert(CSharpLexer::Token tk) {
	return _is_actual_token(tk, true);
}

bool CSharpParser::_is_actual_token(CSharpLexer::Token tk, bool assert) {

	CSharpLexer::Token token = GETTOKEN(0);

	switch (token) {

	case CST::TK_EOF: return false;
	case CST::TK_ERROR: error("Found error token."); return false;
	case CST::TK_EMPTY: error("Empty token -> skipped."); INCPOS(1); return _is_actual_token(tk,assert);
	case CST::TK_CURSOR: cursor = current; INCPOS(1); return _is_actual_token(tk,assert);
	default: {

		if (token != tk) {
			if (assert) {
				string msg = "Assertion fail. Expected token: ";
				msg += CSharpLexer::token_names[(int)tk];
				msg += " but current token is: ";
				msg += (token == CST::TK_IDENTIFIER) ? TOKENDATA(0) : CSharpLexer::token_names[(int)token];
				error(msg);
			}
			return false;
		}
	}
	}

	return true;
}

void CSharpParser::_parse_attributes() {

	int bracket_depth = 0;
	string attribute = "";
	while (true) {

		switch (GETTOKEN(0)) {

		CASEATYPICAL()

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

void CSharpParser::_apply_attributes(CSharpParser::Node* node) {
	node->attributes = this->attributes; // copy
}

void CSharpParser::_apply_modifiers(CSharpParser::Node* node) {
	node->modifiers = this->modifiers;
}

// read file = read namespace (even 'no-namespace')
// global -> for file parsing (no name parsing and keyword namespace)
CSharpParser::NamespaceNode* CSharpParser::_parse_namespace(bool global = false) {

	string name = "global";
	if (!global) {
		// is namespace?
		if (!_is_actual_token(CST::TK_KW_NAMESPACE))
			return nullptr;
		else INCPOS(1);

		// read name
		if (!_is_actual_token(CST::TK_IDENTIFIER))
			return nullptr;
		else {
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
	}

	NamespaceNode* node = new NamespaceNode();
	Node* prev_current = current;
	NamespaceNode* prev_namespace = cur_namespace;
	this->current = node;
	this->cur_namespace = node;
	node->name = name;

	_assert(CST::TK_CURLY_BRACKET_OPEN);
	INCPOS(1); // skip '{'

	Depth d = this->depth;
	while (_parse_namespace_member(node));

	this->current = prev_current;
	this->cur_namespace = prev_namespace;
	return node;

}

void CSharpParser::FileNode::print(int indent) const {

	indentation(indent);
	cout << "FILE " << name << ":" << endl;

	// HEADERS:
	if (namespaces.size() > 0) print_header(indent + TAB, namespaces, "> namespaces:");
	if (interfaces.size() > 0) print_header(indent + TAB, interfaces, "> interfaces:");
	if (classes.size() > 0)    print_header(indent + TAB, classes,    "> classes:");
	if (structures.size() > 0) print_header(indent + TAB, structures, "> structures:");
	if (enums.size() > 0)      print_header(indent + TAB, enums,      "> enums:");

	// NODES:
	if (namespaces.size() > 0) for (NamespaceNode* x : namespaces) x->print(indent + TAB);
	if (interfaces.size() > 0) for (InterfaceNode* x : interfaces) x->print(indent + TAB);
	if (classes.size() > 0)    for (ClassNode* x : classes)        x->print(indent + TAB);
	if (structures.size() > 0) for (StructNode* x : structures)    x->print(indent + TAB);

}

CSharpParser::FileNode * CSharpParser::_parse_file()
{
	FileNode* node = new FileNode();
	this->current = node;

	while (_parse_using_directive(node));

	while (_parse_namespace_member(node));

	this->current = nullptr;
	return node;
}

// after this function pos will be ON the token at the same level (depth)
void CSharpParser::_skip_until_token(CSharpLexer::Token tk) {

	while (true) {
		if (GETTOKEN(0) == tk)
			return;
		else
			INCPOS(1);
	}

	// TODO in the future -> depth

	CSharpParser::Depth base_depth = this->depth;
	while (true) {

		switch (GETTOKEN(0)) {

			// TODO standard (error, empty, cursor, eof)

		case CST::TK_BRACKET_OPEN: { // [
			this->depth.bracket_depth++;
			//if (depth == base_depth) 
			//	;
			break;
		}
		case CST::TK_BRACKET_CLOSE: { // ]
		}
		case CST::TK_CURLY_BRACKET_OPEN: { // {
		}
		case CST::TK_CURLY_BRACKET_CLOSE: { // }
		}
		case CST::TK_PARENTHESIS_OPEN: { // (
		}
		case CST::TK_PARENTHESIS_CLOSE: { // )
		}
		default: {
			if (GETTOKEN(0) == tk) {

			}
		}



		}


	}

}



vector<string> CSharpParser::_parse_generic_declaration() {

	_assert(CST::TK_OP_LESS);
	INCPOS(1); // skip '<'

	vector<string> generic_declarations;

	while (true) {
		switch (GETTOKEN(0)) {

		CASEATYPICAL(generic_declarations)

		case CST::TK_IDENTIFIER: { 
			generic_declarations.push_back(TOKENDATA(0));
			INCPOS(1); break; 
		}
		case CST::TK_COMMA: { INCPOS(1); break; }
		case CST::TK_OP_GREATER: { INCPOS(1); return generic_declarations; }
		default: {
			_unexpeced_token_error();
			INCPOS(1);
		}
		}
	}

	return generic_declarations;
}

vector<string> CSharpParser::_parse_derived_and_implements(bool generic_context) {

	_assert(CST::TK_COLON);
	INCPOS(1); // skip ':'
	
	vector<string> types;

	while (true) {
		switch (GETTOKEN(0)) {

		CASEATYPICAL(types)

		case CST::TK_IDENTIFIER: {
			string type = _parse_type();
			types.push_back(type);
			break;
		}
		case CST::TK_COMMA: { INCPOS(1); break; }
		case CST::TK_CURLY_BRACKET_OPEN: { return types; }
		default: {
			if (generic_context && _is_actual_token(CST::TK_KW_WHERE)) {
				return types;
			}

			_unexpeced_token_error();
			INCPOS(1);
		}
		}
	}

	return types;

}

// generic -> where T : C1, new()
string CSharpParser::_parse_constraints() {

	_assert(CST::TK_KW_WHERE);

	string constraints;
	// TODO in the future
	_skip_until_token(CST::TK_CURLY_BRACKET_OPEN);

	return "";
}

CSharpParser::ClassNode* CSharpParser::_parse_class() {

	_assert(CST::TK_KW_CLASS);
	INCPOS(1); // skip 'class'
	_assert(CST::TK_IDENTIFIER);

	ClassNode* node = new ClassNode();
	Node* prev_current = current;
	ClassNode* prev_class = cur_class;
	this->current = node;
	this->cur_class = node;
	node->name = TOKENDATA(0);
	INCPOS(1);

	_apply_attributes(node);
	_apply_modifiers(node);

	// generic
	if (_is_actual_token(CST::TK_OP_LESS)) {
		node->is_generic = true;
		node->generic_declarations = _parse_generic_declaration();
	}

	// derived and implements
	if (_is_actual_token(CST::TK_COLON)) {
		node->base_types = _parse_derived_and_implements(node->is_generic);
	}

	// constraints
	if (_is_actual_token(CST::TK_KW_WHERE)) {
		node->constraints = _parse_constraints();
	}
	
	_assert(CST::TK_CURLY_BRACKET_OPEN);
	INCPOS(1); // skip '{'

	while (_parse_class_member(node));

	this->current = prev_current;
	this->cur_class = prev_class;
	return node;
}

string CSharpParser::_parse_initialization_block() {
	
	_assert(CST::TK_CURLY_BRACKET_OPEN);
	INCPOS(1); // skip '{'

	string initialization_block = "{ ";
	while (true) {

		switch (GETTOKEN(0)) {

		CASEATYPICAL(initialization_block)

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
			return initialization_block;
		}
		default: {
			_unexpeced_token_error();
			INCPOS(1);
		}

		}


	}
	return initialization_block;
}

string CSharpParser::_parse_new() {

	// new constructor()       --> new X(a1,a2);
	// new type block          --> new Dict<int,string> { 1 = "one" }
	// new type[n] {?}         --> new int[5] { optional_block }
	// new { ... } (anonymous) --> new { x = 1, y = 2 }

	_assert(CST::TK_KW_NEW);
	INCPOS(1); // skip 'new'
	string res = "new ";

	switch (GETTOKEN(0)) {

	CASEATYPICAL("")

	case CST::TK_CURLY_BRACKET_OPEN: {
		string initialization_block = _parse_initialization_block();
		res += initialization_block;
		return res;
	}
	case CST::TK_CURLY_BRACKET_CLOSE: {
		// todo
		break;
	}

	CASEBASETYPE {
		string type = _parse_type(true);
		res += type;
		if (GETTOKEN(0) == CST::TK_CURLY_BRACKET_OPEN) {
			string initialization_block = _parse_initialization_block();
			res += " " + initialization_block;
		}
		break;
	}

	case CST::TK_IDENTIFIER: {
		string type = _parse_type(true);
		res += type;
		if (GETTOKEN(0) == CST::TK_CURLY_BRACKET_OPEN) {
			string initialization_block = _parse_initialization_block();
			res += " " + initialization_block;
		} else if (GETTOKEN(0) == CST::TK_PARENTHESIS_OPEN) {
			string invocation = _parse_method_invocation();
			res += invocation;
		}
		break;
	}

	default: {
		_unexpeced_token_error();
		INCPOS(1);
	}
	}

	return res;
}

// parse expression -> (or assignment: x &= 10 is good as well)
string CSharpParser::_parse_expression() {

	// https://docs.microsoft.com/en-us/dotnet/csharp/language-reference/language-specification/expressions
	// po wyjsciu z funkcji karetka ma byc nad tokenem zaraz ZA wyrazeniem
	// try untill ')' at the same depth or ',' or ';'

	int parenthesis_depth = 0;
	int bracket_depth = 0;
	string res = "";
	while (true) {
		debug_info();
		switch (GETTOKEN(0)) {

		CASEATYPICAL("")

		CASEBASETYPE {
			string type = _parse_type();
			res += type;
			break;
		}

		case CST::TK_KW_AWAIT: {
			INCPOS(1); // ignore
			break;
		}
		case CST::TK_KW_BASE: {
			INCPOS(1); // reference to base type
			break;
		}
		case CST::TK_KW_AS: {
			INCPOS(1); // ignore (postfix cast)
			break;
		}
		case CST::TK_KW_THIS: {
			INCPOS(1); // reference to pointer
			break;
		}
		case CST::TK_KW_NAMEOF: {
			INCPOS(1);
			break;
		}
		case CST::TK_KW_NULL: {
			INCPOS(1);
			break;
		}
		case CST::TK_KW_NEW: {
			
			// new Int32(1) + 9 <-- this is expression
			res += _parse_new();
			break;
		}

		case CST::TK_COLON: {
			INCPOS(1); // ignore (TODO zmienic na ogarnianie operatora ? : )
			res += ":";
			break;
		}

		// end if
		case CST::TK_COMMA: {
			if (parenthesis_depth == 0 && bracket_depth == 0) return res; 
			else { res += ","; INCPOS(1); break; }
		}
		case CST::TK_SEMICOLON: {
			if (parenthesis_depth == 0 && bracket_depth == 0) return res;
			else { }// TODO ERROR
		}
		case CST::TK_PARENTHESIS_CLOSE: {
			
			if (parenthesis_depth == 0 && bracket_depth == 0) return res;
			else {
				parenthesis_depth--;
				res += ")"; 
				INCPOS(1); break; }
		}

		// function invokation
		case CST::TK_PARENTHESIS_OPEN: {
			parenthesis_depth++;
			res += "(";
			INCPOS(1);
			res += _parse_expression(); // inside
			break;
		}

		case CST::TK_OP_LAMBDA: {
			// TODO to jest bardzo slabe rozwiazanie
			res += " => ";
			INCPOS(1);
			string s = _parse_expression();
			res += s;
			break;
		}

		CASELITERAL { res += TOKENDATA(0); INCPOS(1); break; }
		case CST::TK_IDENTIFIER: { 
			
			// maybe it is a type (MyOwnType)
			// or array access (MyIdentifier[expression])
			int cur_pos = pos;
			string type = _parse_type(true);

			if (type == "") {
				// this is not a type, neither access to array
				// it can be eg method invocation -> add to res and decide in next iteration
				pos = cur_pos;
				res += TOKENDATA(0);
				INCPOS(1);
				break;
			}
			else {

				res += type;
				break;

			}
		}
		case CST::TK_PERIOD: { res += "."; INCPOS(1); break; }
		case CST::TK_BRACKET_OPEN: { bracket_depth++; INCPOS(1); res += "[" + _parse_expression(); break; }

		case CST::TK_BRACKET_CLOSE: {
			if (parenthesis_depth == 0 && bracket_depth == 0) return res;
			else { bracket_depth--; res += "]"; INCPOS(1); break; }
		}
		case CST::TK_CURLY_BRACKET_CLOSE: { // for initialization block: var x = new { a = EXPR };
			if (parenthesis_depth == 0 && bracket_depth == 0) return res;
			else {
				// todo error
			}
			break;
		}
		case CST::TK_CURLY_BRACKET_OPEN: {
			BlockNode* node = _parse_block();
			res += "{ ... }";
			break;
		}


		default: {

			CSharpLexer::Token t = GETTOKEN(0);
			if (CSharpLexer::is_operator(t)) {
				res += CSharpLexer::token_names[(int)GETTOKEN(0)];
				INCPOS(1);

				//if (CSharpLexer::is_assignment_operator(t)) {
				//	string expr = parse_expression();
				//}

			}

			else if (this->kw_value_allowed && t == CST::TK_KW_VALUE) {
				res += "value";
				INCPOS(1);
				break;
			}

			else if (CSharpLexer::is_context_keyword(t)) {

				res += CSharpLexer::token_names[(int)t];
				INCPOS(1);
				break;
			}

			else {
				_unexpeced_token_error();
				INCPOS(1);
			}
		}


		}

	}

	return res;
	
}

CSharpParser::EnumNode* CSharpParser::_parse_enum() {

	_assert(CST::TK_KW_ENUM);
	INCPOS(1); // skip 'enum'

	// read name
	_assert(CST::TK_IDENTIFIER);

	EnumNode* node = new EnumNode();
	Node* prev_current = current;
	this->current = node;
	node->name = TOKENDATA(0);
	INCPOS(1);

	_apply_attributes(node);
	_apply_modifiers(node);

	node->type = "int"; // default

	// enum type
	if (_is_actual_token(CST::TK_COLON)) {
		INCPOS(1); // skip ';'
		node->type = _parse_type();
	}

	// parse members
	bool end = false;
	while (!end) {

		switch (GETTOKEN(0)) {

		CASEATYPICAL(nullptr)

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
			INCPOS(1);
		}
		}
	}

	this->current = prev_current;
	return node;
}

CSharpParser::JumpNode* CSharpParser::_parse_jump() {

	JumpNode* node = new JumpNode();
	Node* prev_current = current;
	this->current = node;

	// type of jump
	switch (GETTOKEN(0)) {
	case CST::TK_KW_BREAK: node->jump_type = JumpNode::Type::BREAK; break;
	case CST::TK_KW_CONTINUE: node->jump_type = JumpNode::Type::CONTINUE; break;
	case CST::TK_KW_GOTO: node->jump_type = JumpNode::Type::GOTO; break;
	case CST::TK_KW_RETURN: node->jump_type = JumpNode::Type::RETURN; break;
	case CST::TK_KW_YIELD: node->jump_type = JumpNode::Type::YIELD; break;
	case CST::TK_KW_THROW: node->jump_type = JumpNode::Type::THROW; break;
	default: { 
		_unexpeced_token_error();
		INCPOS(1);
		delete node;
		this->current = prev_current;
		return nullptr;
	}
	}

	node->raw = CSharpLexer::token_names[(int)GETTOKEN(0)];
	INCPOS(1); // skip keyword

	bool end = false;
	while (!end) {
		switch (GETTOKEN(0)) {
			
		CASEATYPICAL(nullptr)

		case CST::TK_SEMICOLON: {
			node->raw += ";";
			end = true;
			INCPOS(1);
			break;
		}
		default: {
			node->raw += (GETTOKEN(0) == CST::TK_IDENTIFIER ? TOKENDATA(0) : CSharpLexer::token_names[(int)GETTOKEN(0)]);
			INCPOS(1);
		}
		}
	}

	this->current = prev_current;
	return node;
	
}

CSharpParser::LoopNode* CSharpParser::_parse_loop() {

	LoopNode* node = new LoopNode();
	Node* prev_current = current;
	this->current = node;

	// type of loop
	switch (GETTOKEN(0)) {
	case CST::TK_KW_DO: node->loop_type = LoopNode::Type::DO; break;
	case CST::TK_KW_FOR: node->loop_type = LoopNode::Type::FOR; break;
	case CST::TK_KW_FOREACH: node->loop_type = LoopNode::Type::FOREACH; break;
	case CST::TK_KW_WHILE: node->loop_type = LoopNode::Type::WHILE; break;
	default: { delete node; this->current = prev_current; return nullptr; }
	}

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
			node->local_variable = variable;
		}

		_assert(CST::TK_KW_IN);
		INCPOS(1); // skip 'in'


		// in
		_parse_expression();

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
	node->body->parent = node; // bind

	// ----- ----- -----
	// PARSE AFTER BLOCK
	if (node->loop_type == LoopNode::Type::DO) {
		//todo skip while
	}

	this->current = prev_current;
	return node;
}

CSharpParser::DelegateNode* CSharpParser::_parse_delegate() {
	return nullptr;
}

std::string CSharpParser::_parse_type(bool array_constructor) {

	std::string res = "";

	// base type: int/bool/... (nullable ?) [,,...,]
	// complex type: NAMESPACE::NAME1. ... .NAMEn<type1,type2,...>[,,...,]

	// note: 
	// array_constructor = false -> int[n] is NOT a type
	// array_constructor = true -> int[n] is a type

	// TODO dodac obslugiwanie '::'

	bool base_type = false;
	bool complex_type = false;
	bool generic_types_mode = false;
	bool array_mode = false;

	while (true) {
		debug_info();

		switch (GETTOKEN(0)) {

		CASEATYPICAL("")

		case CST::TK_OP_QUESTION_MARK: {

			// nullable type
			if (base_type) {
				res += "?";
				INCPOS(1);
				break;
			}
			else {
				return ""; // error
			}
		}

		case CST::TK_OP_LESS: { // <

			generic_types_mode = true;
			res += "<";
			INCPOS(1);
			res += _parse_type(); // recursive
			break;
		}

		case CST::TK_OP_GREATER: { // >

			generic_types_mode = false;
			res += ">";
			INCPOS(1);

			// this is end unless it is '['
			if (GETTOKEN(0) != CST::TK_BRACKET_OPEN) {
				return res;
			}

			break;
		}

		case CST::TK_BRACKET_OPEN: { // [
			array_mode = true;
			res += "[";
			INCPOS(1);
			if (array_constructor) {
				string expr = _parse_expression();
				res += expr;
			}
			break;
		}
		case CST::TK_BRACKET_CLOSE: { // ]
			array_mode = false;
			res += "]";
			INCPOS(1);

			// for sure this is end of type
			return res;
		}

		case CST::TK_COMMA: { // ,
			if (generic_types_mode) {
				res += ",";
				INCPOS(1);
				res += _parse_type();
			}
			else if (array_mode) {
				res += ",";
				INCPOS(1);
			}
			else {
				return ""; // error
			}
			break;
		}

		// base type?
		CASEBASETYPE {

			if (complex_type) {
				return "";
				// TODO error -> System.Reflexion.int ??????
				// vector<int> still allowed, cause int will be parsed recursively
			}

			base_type = true;
			res += CSharpLexer::token_names[(int)GETTOKEN(0)];
			INCPOS(1);

			// another has to be '?' either '[' - else it is finish
			if (GETTOKEN(0) != CST::TK_OP_QUESTION_MARK
				&& GETTOKEN(0) != CST::TK_BRACKET_OPEN)
			{
				return res; // end
			}
			break;
		}

		// complex type
		case CST::TK_IDENTIFIER: {

			if (base_type) {
				return ""; // error
			}

			complex_type = true;
			res += TOKENDATA(0);
			INCPOS(1);

			// finish if
			if (_is_actual_token(CST::TK_PERIOD)) {
				res += ".";
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
				return res;
			}
			break;
		}
		default: {

			_unexpeced_token_error();
			INCPOS(1);
		}
		}
	}

	return res;
}

bool CSharpParser::_parse_using_directive(FileNode* node) {

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

	return true;
}

// returns information if should continue parsing member or if it is finish
bool CSharpParser::_parse_namespace_member(NamespaceNode* node) {

	_parse_attributes();
	_apply_attributes(node);
	_parse_modifiers();

	debug_info();

	switch (GETTOKEN(0)) {

		CASEATYPICAL(false)

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
	case CST::TK_CURLY_BRACKET_OPEN: {
		this->depth.curly_bracket_depth++;
		break;
	}
	case CST::TK_CURLY_BRACKET_CLOSE: {

		// TODO if depth ok - ok else error
		INCPOS(1); // skip '}'
		return false;
	}

	default: {
		
		_unexpeced_token_error();
		INCPOS(1);
	}
	}

	return true;

}

void CSharpParser::debug_info() const {
	if (pos == 1549) {
		int x = 1;
	}
	cout << "Token: " << (GETTOKEN(0) == CST::TK_IDENTIFIER ? TOKENDATA(0) : CSharpLexer::token_names[(int)GETTOKEN(0)]) << " Pos: " << pos << endl;
}


bool CSharpParser::_parse_class_member(ClassNode* node) {

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

		// field (var), property or method
		string type = _parse_type();

		// constructor?
		if (_is_actual_token(CST::TK_PARENTHESIS_OPEN)) {

			if (type != node->name) {
				_unexpeced_token_error();
				INCPOS(1);
				break;
			}

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

			VarNode* member = new VarNode(name, type);
			_apply_modifiers(member); // is const?
			node->variables.push_back(member);
			member->parent = node;
			INCPOS(1); // skip ';'
		}

		// FIELD WITH ASSIGNMENT
		else if (_is_actual_token(CST::TK_OP_ASSIGN)) {
			
			VarNode* member = new VarNode(name, type);
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
			INCPOS(1);
		}

	}

	}

	return true;

}

bool CSharpParser::_parse_interface_member(InterfaceNode* node) {

	_parse_attributes();
	// note: there are no modifiers in interfaces

	switch (GETTOKEN(0)) {

	CASEATYPICAL(false)

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
			INCPOS(1);
		}
	}

	}

	return true;
}

void CSharpParser::NamespaceNode::print(int indent) const {

	indentation(indent);
	cout << "NAMESPACE " << name << ":" << endl;

	// HEADERS:
	if (namespaces.size() > 0) print_header(indent + TAB, namespaces, "> namespaces:");
	if (interfaces.size() > 0) print_header(indent + TAB, interfaces, "> interfaces:");
	if (classes.size() > 0)    print_header(indent + TAB, classes,    "> classes:");
	if (structures.size() > 0) print_header(indent + TAB, structures, "> structures:");
	if (enums.size() > 0)      print_header(indent + TAB, enums,      "> enums:");
	
	// NODES:
	if (namespaces.size() > 0) for (NamespaceNode* x : namespaces) x->print(indent + TAB);
	if (interfaces.size() > 0) for (InterfaceNode* x : interfaces) x->print(indent + TAB);
	if (classes.size() > 0)    for (ClassNode* x : classes)        x->print(indent + TAB);
	if (structures.size() > 0) for (StructNode* x : structures)    x->print(indent + TAB);
}

CSharpParser::VarNode* CSharpParser::_parse_declaration() {

	// type name;
	// type name = x;
	// const type name = x;

	VarNode* variable = new VarNode();
	Node* prev_current = current;
	this->current = variable;

	if (_is_actual_token(CST::TK_KW_CONST)) {
		variable->modifiers |= (int)CSM::MOD_CONST;
		INCPOS(1);
	}

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
	INCPOS(1); // skip '='
	string expression = _parse_expression();
	variable->value = expression;

	this->current = prev_current;
	return variable;
}

CSharpParser::StructNode* CSharpParser::_parse_struct() {
	// todo
	return nullptr;
}

CSharpParser::InterfaceNode* CSharpParser::_parse_interface() {

	_assert(CST::TK_KW_INTERFACE);
	INCPOS(1); // skip 'interface'

	// read name
	_assert(CST::TK_IDENTIFIER);
	InterfaceNode* node = new InterfaceNode();
	Node* prev_current = current;
	this->current = node;
	node->name = TOKENDATA(0);
	INCPOS(1);

	_apply_attributes(node);
	_apply_modifiers(node);

	// generic
	if (_is_actual_token(CST::TK_OP_LESS)) {
		node->is_generic = true;
		node->generic_declarations = _parse_generic_declaration();
	}

	// derived and implements
	if (_is_actual_token(CST::TK_COLON)) {
		node->base_types = _parse_derived_and_implements(node->is_generic);
	}

	// constraints
	if (_is_actual_token(CST::TK_KW_WHERE)) {
		node->constraints = _parse_constraints();
	}

	_assert(CST::TK_CURLY_BRACKET_OPEN);
	INCPOS(1); // skip '}'

	while (_parse_interface_member(node));

	this->current = prev_current;
	return node;

}

CSharpParser::UsingNode* CSharpParser::_parse_using_statement() {
	
	_assert(CST::TK_KW_USING);

	// using ( declaration or expression ) statement
	// using declaration // TODO to jeszcze nie dziala (C#8)

	UsingNode* node = new UsingNode();
	Node* prev_current = current;
	this->current = node;

	INCPOS(1);
	node->raw = "using ";


	// USING DECLARATION VERSION
	if (!_is_actual_token(CST::TK_PARENTHESIS_OPEN)) {

		// TODO to nie dziala

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
	
	this->current = prev_current;
	return node;
}


CSharpParser::ConditionNode* CSharpParser::_parse_if_statement() {
	
	// if ( expr ) statement else statement
	ConditionNode* node = new ConditionNode();
	Node* prev_current = current;
	this->current = node;

	_assert(CST::TK_KW_IF);
	INCPOS(1); // skip 'if'
	node->raw = "if ";

	_assert(CST::TK_PARENTHESIS_OPEN);
	INCPOS(1); // skip '('
	node->raw += "(";

	string expression = _parse_expression();
	node->raw += expression;

	_assert(CST::TK_PARENTHESIS_CLOSE);
	INCPOS(1); // skip ')'
	node->raw += ") ";

	StatementNode* then = _parse_statement();
	if (then == nullptr) {
		// TODO error
	}
	then->parent = node;
	node->raw += then->raw;

	// optional else
	if (_is_actual_token(CST::TK_KW_ELSE)) {

		INCPOS(1); // skip 'else'
		node->raw += " else ";
		StatementNode* else_st = _parse_statement();
		if (else_st == nullptr) {
			// todo error
		}
		else_st->parent = node;
		node->raw += else_st->raw;

	}

	this->current = prev_current;
	return node;

}

CSharpParser::TryNode* CSharpParser::_parse_try_statement()
{
	// try { ... } (catch { ... } (when ...)? )* (finally { ... })?
	TryNode* node = new TryNode();
	Node* prev_current = current;
	this->current = node;

	_assert(CST::TK_KW_TRY);
	INCPOS(1); // skip 'try'

	_assert(CST::TK_CURLY_BRACKET_OPEN);

	BlockNode* try_block = _parse_block();
	try_block->parent = node;

	while (_is_actual_token(CST::TK_KW_CATCH)) {

		INCPOS(1); // skip 'catch'
		_assert(CST::TK_PARENTHESIS_OPEN); INCPOS(1); // skip '('		
		_parse_expression(); // ignore
		_assert(CST::TK_PARENTHESIS_CLOSE); INCPOS(1); // skip ')'

		// possible when
		if (_is_actual_token(CST::TK_KW_WHEN)) {

			INCPOS(1); // skip 'when'
			_assert(CST::TK_PARENTHESIS_OPEN); INCPOS(1); // skip '('
			_parse_expression(); // ignore
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

	this->current = prev_current;
	return node;
}

CSharpParser::ConditionNode* CSharpParser::_parse_switch_statement() {
	
	// switch (...) { (case ... (when ...)?)* default: ?}
	ConditionNode* node = new ConditionNode();
	Node* prev_current = current;
	this->current = node;

	_assert(CST::TK_KW_SWITCH);
	INCPOS(1); // skip 'switch'

	_assert(CST::TK_PARENTHESIS_OPEN);
	INCPOS(1); // skip '('

	_skip_until_token(CST::TK_PARENTHESIS_CLOSE);
	INCPOS(1); // skip ')'

	_assert(CST::TK_CURLY_BRACKET_OPEN);

	_skip_until_token(CST::TK_CURLY_BRACKET_CLOSE);
	INCPOS(1); // skip '}'

	this->current = prev_current;
	return node;
}

CSharpParser::StatementNode* CSharpParser::_parse_property_definition() {

	switch (GETTOKEN(0)) {

	CASEATYPICAL(nullptr)

	case CST::TK_OP_LAMBDA: {
		INCPOS(1); // skip '=>'
		return _parse_statement();
	}
	case CST::TK_SEMICOLON: {
		StatementNode* node = new StatementNode();
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
		INCPOS(1);
	}

	}

	return nullptr;
}

CSharpParser::PropertyNode* CSharpParser::_parse_property(string name, string type)
{
	_assert(CST::TK_CURLY_BRACKET_OPEN);
	INCPOS(1); // skip '{'

	PropertyNode* node = new PropertyNode();
	Node* prev_current = current;
	this->current = node;
	
	node->name = name;
	node->type = type;

	// we don't know what is declared first: get or set
	bool get_parsed = false;
	bool set_parsed = false;

	_parse_attributes();
	_parse_modifiers();

	while (true) {

		switch (GETTOKEN(0)) {
		
		CASEATYPICAL(nullptr)

		case CST::TK_KW_GET: {
			if (get_parsed) {
				// todo error
			} else {
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
				// todo error
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
			return node;
		}
		default: {
			_unexpeced_token_error();
			INCPOS(1);
		}
		}
	}

	this->current = prev_current;
	return node;
}


CSharpParser::StatementNode* CSharpParser::_parse_statement() {

	switch (GETTOKEN(0)) {

	CASEATYPICAL(nullptr)

	case CST::TK_CURLY_BRACKET_OPEN: {
		BlockNode* node = _parse_block();
		return node;
	}

	CASEBASETYPE {
		VarNode *variable = _parse_declaration();
		DeclarationNode* node = new DeclarationNode();
		node->variable = variable;
		INCPOS(1); // skip ';'

		return node;
	}

	case CST::TK_KW_THIS:
	case CST::TK_KW_BASE: {

		StatementNode* node = new StatementNode();
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
			DeclarationNode* node = new DeclarationNode();
			node->variable = variable;
			INCPOS(1); // skip ';'
			return node;
		}

		pos = cur_pos; // reset
		StatementNode* node = new StatementNode();

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
		StatementNode* node = new StatementNode();
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
		StatementNode* node = new StatementNode();
		node->raw = ";";
		INCPOS(1);
		return node; // empty statement
	}
	default: {
		_unexpeced_token_error();
		INCPOS(1);
	}

	}

	return nullptr;
}

CSharpParser::MethodNode* CSharpParser::_parse_method_declaration(string name, string return_type, bool interface_context) {

	MethodNode* node = new MethodNode();
	Node* prev_current = current;
	MethodNode* prev_method = cur_method;
	this->current = node;
	this->cur_method = node;

	node->name = name;
	node->return_type = return_type;

	_assert(CST::TK_PARENTHESIS_OPEN);
	INCPOS(1);

	// ARGUMETNS
	VarNode* argument = new VarNode();
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
			node->arguments.push_back(argument);
			argument = new VarNode();
			INCPOS(1);
			break;
		}
		case CST::TK_OP_ASSIGN: { // argumenty donyslne
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

	// BODY
	if (interface_context) {
		_assert(CST::TK_SEMICOLON);
		INCPOS(1); // skip ';'
	}
	else { // class context
		node->body = _parse_block();
	}

	this->current = prev_current;
	this->cur_method = prev_method;
	return node;
}

// parse just invocation without name 
string CSharpParser::_parse_method_invocation() {

	_assert(CST::TK_PARENTHESIS_OPEN);
	INCPOS(1);
	string res = "(";

	// ARGUMETNS
	while (!_is_actual_token(CST::TK_PARENTHESIS_CLOSE)) {

		if (_is_actual_token(CST::TK_COMMA)) {
			res += " , ";
			INCPOS(1);
		}
		else {
			res += _parse_expression();
		}

	}

	res += ")";
	INCPOS(1);

	return res;

}

CSharpParser::BlockNode* CSharpParser::_parse_block() {

	_assert(CST::TK_CURLY_BRACKET_OPEN);
	INCPOS(1); // skip '{'

	BlockNode* node = new BlockNode();
	Node* prev_current = current;
	BlockNode* prev_block = cur_block;
	this->current = node;
	this->cur_block = node;

	while (!_is_actual_token(CST::TK_CURLY_BRACKET_CLOSE)) {

		StatementNode* statement = _parse_statement();
		if (statement == nullptr) {
			error("Failed to parse statement.");
			break;
		}
		statement->parent = node;

		// dodaj na koniec listy statementow
		statement->prev = node->last_node;
		node->last_node = statement;
	}

	INCPOS(1); // skip '}'

	this->current = prev_current;
	this->cur_block = prev_block;
	return node;

}



CSharpParser::~CSharpParser() {

}

void CSharpParser::ClassNode::print(int indent) const {

	indentation(indent);
	cout << "CLASS " << name << ":" << endl;

	// HEADERS:
	if (base_types.size() > 0)  print_header(indent + TAB, base_types, "> base types:");
	if (interfaces.size() > 0)	print_header(indent + TAB, interfaces, "> interfaces:");
	if (classes.size() > 0)		print_header(indent + TAB, classes,    "> classes:");
	if (structures.size() > 0)	print_header(indent + TAB, structures, "> structures:");
	if (enums.size() > 0)		print_header(indent + TAB, enums,      "> enums:");
	if (methods.size() > 0)		print_header(indent + TAB, methods,    "> methods:");
	if (properties.size() > 0)	print_header(indent + TAB, properties, "> properties:");
	if (variables.size() > 0)	print_header(indent + TAB, variables,  "> variables:");
	
	// NODES:
	if (interfaces.size() > 0)	for (InterfaceNode* x : interfaces)	x->print(indent + TAB);
	if (classes.size() > 0)		for (ClassNode* x : classes)		x->print(indent + TAB);
	if (structures.size() > 0)	for (StructNode* x : structures)	x->print(indent + TAB);
}

void CSharpParser::StructNode::print(int indent) const {

	indentation(indent);
	cout << "STRUCT " << name << ":" << endl;

	// HEADERS:
	if (base_types.size() > 0)     print_header(indent + TAB, base_types, "> base types:");
	if (interfaces.size() > 0)	   print_header(indent + TAB, interfaces, "> interfaces:");
	if (classes.size() > 0)		   print_header(indent + TAB, classes,    "> classes:");
	if (structures.size() > 0)	   print_header(indent + TAB, structures, "> structures:");
	if (enums.size() > 0)		   print_header(indent + TAB, enums,      "> enums:");
	if (methods.size() > 0)		   print_header(indent + TAB, methods,    "> methods:");
	if (properties.size() > 0)	   print_header(indent + TAB, properties, "> properties:");
	if (variables.size() > 0)	   print_header(indent + TAB, variables,  "> variables:");
	
	// NODES:
	if (interfaces.size() > 0)	for (InterfaceNode* x : interfaces)	x->print(indent + TAB);
	if (classes.size() > 0)		for (ClassNode* x : classes)		x->print(indent + TAB);
	if (structures.size() > 0)	for (StructNode* x : structures)	x->print(indent + TAB);
}

void CSharpParser::StatementNode::print(int indent) const {

	indentation(indent);
	cout << "STATEMENT" << endl;

}

void CSharpParser::VarNode::print(int indent) const {

	indentation(indent);
	cout << "VAR: ";
	if (modifiers & (int)CSM::MOD_CONST) cout << "const ";
	cout << type << " " << name << endl;

}


void CSharpParser::MethodNode::print(int indent) const {

	indentation(indent);
	cout << "METHOD " << return_type << " " << name << endl;
	cout << "(";
	for (VarNode* v : arguments)
		cout << v->type << " " << v->name << " , ";
	cout << ")" << endl;

}

void CSharpParser::InterfaceNode::print(int indent) const {

	indentation(indent);
	cout << "INTERFACE " << name << endl;

	// HEADERS:
	if (base_types.size() > 0)     print_header(indent + TAB, base_types, "> base types:");
	if (methods.size() > 0)		   print_header(indent + TAB, methods,    "> methods:");
	if (properties.size() > 0)	   print_header(indent + TAB, properties, "> properties:");
	
}

void CSharpParser::EnumNode::print(int indent) const {

	indentation(indent); cout << "ENUM " << name << ":" << endl;
	indentation(indent + 2); cout << "type: " << type << endl;

	if (members.size() > 0)     print_header(indent + TAB, members, "> members:");

}

void CSharpParser::PropertyNode::print(int indent) const {
	// TODO
}

vector<CSharpParser::NamespaceNode*> CSharpParser::Node::get_namespaces()
{
	return vector<NamespaceNode*>();
}

vector<CSharpParser::ClassNode*> CSharpParser::Node::get_classes()
{
	return vector<ClassNode*>();
}

vector<CSharpParser::MethodNode*> CSharpParser::Node::get_methods()
{
	return vector<MethodNode*>();
}

vector<CSharpParser::VarNode*> CSharpParser::Node::get_vars()
{
	return vector<VarNode*>();
}
