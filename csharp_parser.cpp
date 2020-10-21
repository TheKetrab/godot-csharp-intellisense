#include "csharp_parser.h"
#include <iostream>

using CST = CSharpLexer::Token;
using CSM = CSharpParser::Modifier;



#define CASEBASETYPE case CST::TK_KW_BOOL: \
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

#define CASEMODIFIER case CST::TK_KW_PUBLIC: \
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
	{ CST::TK_KW_ASYNC,			CSM::MOD_ASYNC }
};

CSharpParser::CSharpParser(vector<CSharpLexer::TokenData>& tokens) {
	this->tokens = tokens; // copy
}

CSharpParser::CSharpParser(string code) {

	CSharpLexer lexer;
	lexer.set_code(code);
	lexer.tokenize();

	tokens = lexer.tokens; // copy
}

CSharpParser::CSharpParser() {

	root = nullptr;
	current = nullptr;
	cursor = nullptr;

	pos = 0;
	len = 0;

	modifiers = 0;

}

void CSharpParser::set_tokens(vector<CSharpLexer::TokenData>& tokens) {

	this->tokens = tokens;
	this->len = tokens.size();

}

// parse all tokens (whole file)
void CSharpParser::parse() {

	cout << "PARSER: parse" << endl;
	clear();
	root = parse_namespace(true);

	cout << "----------------------DONE" << endl;

	root->print(0);



}

void CSharpParser::clear() {

	cursor = nullptr;
	current = nullptr;
	root = nullptr;
	pos = 0;
	modifiers = 0;

}

// todo funckja? get_next_valid_token - czasem chcemy, zeby nastepny token byl czyms, a moze byc np kursorem, wiec trzeba skipnac cursor i wtedy nastepnym valid tokenem jest cos tam dalej
#define GETTOKEN(ofs) ((ofs + pos) >= len ? CST::TK_ERROR : tokens[ofs + pos].type)
#define TOKENDATA(ofs) ((ofs + pos >= len ? "" : tokens[ofs + pos].data))
#define INCPOS(ammount) { pos += ammount; }
#define CASEATYPICAL(def_return) \
	case CST::TK_EOF: { return def_return; } \
	case CST::TK_ERROR: { error("Found error token."); return def_return; } \
	case CST::TK_EMPTY: { error("Empty token -> skipped."); INCPOS(1); break; } \
	case CST::TK_CURSOR: { cursor = current; INCPOS(1); }

void CSharpParser::parse_modifiers() {

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

void CSharpParser::error(string msg) {
	cout << "--> Error: " << msg << endl;
}

void CSharpParser::unexpeced_token_error() {
	string msg = "Unexpected token: ";
	msg += CSharpLexer::token_names[(int)(GETTOKEN(0))];
	error(msg);
}

// upewnia sie czy aktualny token jest taki jak trzeba
// jesli current to CURSOR to ustawia cursor i sprawdza
// czy nastepny token jest wymaganym przez assercje tokenem
bool CSharpParser::assert(CSharpLexer::Token tk) {
	return is_actual_token(tk, true);
}

bool CSharpParser::is_actual_token(CSharpLexer::Token tk, bool assert) {

	CSharpLexer::Token token = tokens[pos].type;

	switch (token) {

	case CST::TK_EOF: return false;
	case CST::TK_ERROR: error("Found error token."); return false;
	case CST::TK_EMPTY: error("Empty token -> skipped."); INCPOS(1); return is_actual_token(tk);
	case CST::TK_CURSOR: cursor = current; INCPOS(1); return is_actual_token(tk);
	default: {

		if (token != tk) {
			if (assert) {
				string msg = "Assertion fail. Expected tk: ";
				msg += CSharpLexer::token_names[(int)tk];
				msg += " but current token is: ";
				msg += CSharpLexer::token_names[(int)token];
				error(msg);
			}
			return false;
		}
	}
	}

	return true;
}

void CSharpParser::parse_attributes() {

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

void CSharpParser::apply_attributes(CSharpParser::Node* node) {
	node->attributes = this->attributes; // copy
}

void CSharpParser::apply_modifiers(CSharpParser::Node* node) {
	node->modifiers = this->modifiers;
}

// read file = read namespace (even 'no-namespace')
// global -> for file parsing (no name parsing and keyword namespace)
CSharpParser::NamespaceNode* CSharpParser::parse_namespace(bool global = false) {

	string name = "global";
	if (!global) {
		// is namespace?
		if (is_actual_token(CST::TK_KW_NAMESPACE))
			return nullptr;
		else INCPOS(1);

		// read name
		if (is_actual_token(CST::TK_IDENTIFIER))
			return nullptr;
		else {
			name = TOKENDATA(0);
			INCPOS(1);
		}
	}

	NamespaceNode* node = new NamespaceNode();
	node->name = name;

	assert(CST::TK_CURLY_BRACKET_OPEN);
	INCPOS(1); // skip '{'

	Depth d = this->depth;
	while (parse_namespace_member(node));

	return node;

}

// after this function pos will be ON the token at the same level (depth)
void CSharpParser::skip_until_token(CSharpLexer::Token tk) {

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



vector<string> CSharpParser::parse_generic_declaration() {

	assert(CST::TK_OP_LESS);
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
			unexpeced_token_error();
			INCPOS(1);
		}
		}
	}

	return generic_declarations;
}

vector<string> CSharpParser::parse_derived_and_implements(bool generic_context) {

	assert(CST::TK_COLON);
	INCPOS(1); // skip ':'
	
	vector<string> types;

	while (true) {
		switch (GETTOKEN(0)) {

		CASEATYPICAL(types)

		case CST::TK_IDENTIFIER: {
			string type = parse_type();
			types.push_back(type);
			break;
		}
		case CST::TK_COMMA: { INCPOS(1); break; }
		case CST::TK_CURLY_BRACKET_OPEN: { return types; }
		default: {
			if (generic_context && is_actual_token(CST::TK_KW_WHERE)) {
				return types;
			}

			unexpeced_token_error();
			INCPOS(1);
		}
		}
	}

	return types;

}

// generic -> where T : C1, new()
string CSharpParser::parse_constraints() {

	assert(CST::TK_KW_WHERE);

	string constraints;
	// TODO in the future
	skip_until_token(CST::TK_CURLY_BRACKET_OPEN);

	return "";
}

CSharpParser::ClassNode* CSharpParser::parse_class() {

	assert(CST::TK_KW_CLASS);
	INCPOS(1); // skip 'class'
	assert(CST::TK_IDENTIFIER);

	ClassNode* node = new ClassNode();
	node->name = TOKENDATA(0);
	INCPOS(1);

	apply_attributes(node);
	apply_modifiers(node);

	// generic
	if (is_actual_token(CST::TK_OP_LESS)) {
		node->is_generic = true;
		node->generic_declarations = parse_generic_declaration();
	}

	// derived and implements
	if (is_actual_token(CST::TK_COLON)) {
		node->base_types = parse_derived_and_implements(node->is_generic);
	}

	// constraints
	if (is_actual_token(CST::TK_KW_WHERE)) {
		node->constraints = parse_constraints();
	}
	
	assert(CST::TK_CURLY_BRACKET_OPEN);
	INCPOS(1); // skip '{'

	while (parse_class_member(node));

	return node;
}

string CSharpParser::parse_initialization_block() {
	
	assert(CST::TK_CURLY_BRACKET_OPEN);
	INCPOS(1); // skip '{'

	string initialization_block = "{ ";
	while (true) {

		switch (GETTOKEN(0)) {

		CASEATYPICAL(initialization_block)

		case CST::TK_IDENTIFIER: {
			initialization_block += TOKENDATA(0);
			INCPOS(1);
			break;
		}

		case CST::TK_OP_ASSIGN: {
			initialization_block += " = ";
			INCPOS(1);
			string val = parse_expression();
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
			unexpeced_token_error();
			INCPOS(1);
		}

		}


	}
	return initialization_block;
}

string CSharpParser::parse_new() {

	// new constructor()       --> new X(a1,a2);
	// new type block          --> new Dict<int,string> { 1 = "one" }
	// new type[n] {?}         --> new int[5] { optional_block }
	// new { ... } (anonymous) --> new { x = 1, y = 2 }

	assert(CST::TK_KW_NEW);
	INCPOS(1); // skip 'new'
	string res = "new ";

	switch (GETTOKEN(0)) {

	CASEATYPICAL("")

	case CST::TK_CURLY_BRACKET_OPEN: {
		string initialization_block = parse_initialization_block();
		res += initialization_block;
		return res;
	}
	case CST::TK_CURLY_BRACKET_CLOSE: {
		// todo
	}

	CASEBASETYPE {
		string type = parse_type(true);
		res += type;
		if (GETTOKEN(0) == CST::TK_CURLY_BRACKET_OPEN) {
			string initialization_block = parse_initialization_block();
			res += " " + initialization_block;
		}
		break;
	}

	case CST::TK_IDENTIFIER: {
		string type = parse_type(true);
		res += type;
		if (GETTOKEN(0) == CST::TK_CURLY_BRACKET_OPEN) {
			string initialization_block = parse_initialization_block();
			res += " " + initialization_block;
		} else if (GETTOKEN(0) == CST::TK_PARENTHESIS_OPEN) {
			string invocation = parse_method_invocation();
			res += invocation;
		}
		return res;
	}

	default: {
		unexpeced_token_error();
		INCPOS(1);
	}
	}
}

// parse expression -> (or assignment: x &= 10 is good as well)
string CSharpParser::parse_expression() {

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
			res += parse_new();
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
			res += parse_expression(); // inside
			break;
		}

		CASELITERAL { res += TOKENDATA(0); INCPOS(1); break; }
		case CST::TK_IDENTIFIER: { res += TOKENDATA(0); INCPOS(1); break; }
		case CST::TK_PERIOD: { res += "."; INCPOS(1); break; }
		case CST::TK_BRACKET_OPEN: { bracket_depth++; INCPOS(1); res += "[" + parse_expression(); break; }

		case CST::TK_BRACKET_CLOSE: {
			if (parenthesis_depth == 0 && bracket_depth == 0) return res;
			else { bracket_depth--; res += "]"; INCPOS(1); break; }
		}
		case CST::TK_CURLY_BRACKET_CLOSE: { // for initialization block: var x = new { a = EXPR };
			if (parenthesis_depth == 0 && bracket_depth == 0) return res;
			else {
				// todo error
			}
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

			else {
				unexpeced_token_error();
				INCPOS(1);
			}
		}


		}

	}
	
}

CSharpParser::EnumNode* CSharpParser::parse_enum() {

	assert(CST::TK_KW_ENUM);
	INCPOS(1); // skip 'enum'

	// read name
	assert(CST::TK_IDENTIFIER);

	EnumNode* node = new EnumNode();
	node->name = TOKENDATA(0);
	INCPOS(1);

	apply_attributes(node);
	apply_modifiers(node);

	node->type = "int"; // default

	// enum type
	if (is_actual_token(CST::TK_COLON)) {
		INCPOS(1); // skip ';'
		node->type = parse_type();
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
			parse_expression(); // don't care about values
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
			unexpeced_token_error();
			INCPOS(1);
		}
		}
	}

	return node;
}

CSharpParser::JumpNode* CSharpParser::parse_jump() {

	JumpNode* node = new JumpNode();
	
	// type of jump
	switch (GETTOKEN(0)) {
	case CST::TK_KW_BREAK: node->jump_type = JumpNode::Type::BREAK; break;
	case CST::TK_KW_CONTINUE: node->jump_type = JumpNode::Type::CONTINUE; break;
	case CST::TK_KW_GOTO: node->jump_type = JumpNode::Type::GOTO; break;
	case CST::TK_KW_RETURN: node->jump_type = JumpNode::Type::RETURN; break;
	case CST::TK_KW_YIELD: node->jump_type = JumpNode::Type::YIELD; break;
	case CST::TK_KW_THROW: node->jump_type = JumpNode::Type::THROW; break;
	default: { 
		unexpeced_token_error();
		INCPOS(1);
		delete node;
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

	return node;
	
}

CSharpParser::LoopNode* CSharpParser::parse_loop() {

	LoopNode* node = new LoopNode();

	// type of loop
	switch (GETTOKEN(0)) {
	case CST::TK_KW_DO: node->loop_type = LoopNode::Type::DO; break;
	case CST::TK_KW_FOR: node->loop_type = LoopNode::Type::FOR; break;
	case CST::TK_KW_FOREACH: node->loop_type = LoopNode::Type::FOREACH; break;
	case CST::TK_KW_WHILE: node->loop_type = LoopNode::Type::WHILE; break;
	default: { delete node; return nullptr; }
	}

	INCPOS(1); // skip keyword

	// ----- ----- -----
	// PARSE BEFORE BLOCK

	// ----- FOR -----
	if (node->loop_type == LoopNode::Type::FOR) {

		INCPOS(1); // skip '('

		// init
		VarNode* variable = parse_declaration();
		if (variable == nullptr) {
			skip_until_token(CST::TK_SEMICOLON); // assignment or sth else
		}
		else {
			node->local_variable = variable;
		}

		INCPOS(1); // skip ';'

		// cond
		skip_until_token(CST::TK_SEMICOLON);
		INCPOS(1); // skip ';'

		// iter
		skip_until_token(CST::TK_PARENTHESIS_CLOSE);
		INCPOS(1); // skip ')'

	}

	// ----- FOREACH -----
	else if (node->loop_type == LoopNode::Type::FOREACH) {

		INCPOS(1); // skip '('

		// declaration
		VarNode* variable = parse_declaration();
		if (variable == nullptr) {
			skip_until_token(CST::TK_KW_IN);
		}
		else {
			node->local_variable = variable;
		}

		// in
		skip_until_token(CST::TK_PARENTHESIS_CLOSE);
		INCPOS(1); // skip ')'
	}

	// ----- WHILE -----
	else if (node->loop_type == LoopNode::Type::WHILE) {

		// cond
		skip_until_token(CST::TK_PARENTHESIS_CLOSE);
		INCPOS(1); // skip ')'

	}

	// ----- ----- -----
	// PARSE BLOCK
	node->body = parse_statement();

	// ----- ----- -----
	// PARSE AFTER BLOCK
	if (node->loop_type == LoopNode::Type::DO) {
		//todo skip while
	}


	return node;
}

CSharpParser::DelegateNode* CSharpParser::parse_delegate() {
	return nullptr;
}

std::string CSharpParser::parse_type(bool array_constructor) {

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
				// TODO error
			}
		}

		case CST::TK_OP_LESS: { // <
			generic_types_mode = true;
			res += "<";
			INCPOS(1);
			res += parse_type(); // recursive
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
				res += parse_type();
			}
			else if (array_mode) {
				res += ",";
				INCPOS(1);
			}
			else {
				// TODO error
			}
			break;
		}

		// base type?
		CASEBASETYPE {

			if (complex_type) {
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
				// TODO error
			}

			complex_type = true;
			res += TOKENDATA(0);
			INCPOS(1);

			// finish if
			if (is_actual_token(CST::TK_PERIOD)) {
				res += ".";
				INCPOS(1);
				break;
			}
			else if (is_actual_token(CST::TK_OP_LESS)) {
				break;
			}
			else if (is_actual_token(CST::TK_BRACKET_OPEN)) {
				break;
			}
			else { // none of '.', '<', '['
				return res;
			}
			break;
		}
		default: {
			unexpeced_token_error();
			INCPOS(1);
		}
		}
	}

	return res;
}

void CSharpParser::parse_using_directive(NamespaceNode* node) {

	// TODO struct using directives node

	// using X;
	// using static X;
	// using X = Y;

	// is using?
	assert(CST::TK_KW_USING);
	INCPOS(1); // skip 'using'

	bool is_static = false;
	bool is_alias = false;

	if (is_actual_token(CST::TK_KW_STATIC)) {
		INCPOS(1);
		is_static = true;
	}

	// read name or type
	string name_or_type = parse_type();
	string refers_to;

	if (is_actual_token(CST::TK_OP_ASSIGN)) {
		INCPOS(1);
		is_alias = true;
		refers_to = parse_type();
	}

	assert(CST::TK_SEMICOLON);
	INCPOS(1); // skip ';'
	node->using_directives.push_back(name_or_type);

}

// returns information if should continue parsing member or if it is finish
bool CSharpParser::parse_namespace_member(NamespaceNode* node) {

	parse_attributes();
	apply_attributes(node);
	parse_modifiers();

	debug_info();

	switch (GETTOKEN(0)) {

		// check exceptions
	case CST::TK_EMPTY:
	case CST::TK_EOF: {
		return false;
	}
	case CST::TK_ERROR: {
		//INCPOS(1); // skip
		return false;
	}
	case CST::TK_CURSOR: {
		cursor = node;
		INCPOS(1);
	}
							   // -------------------------

							   // using directives
	case CST::TK_KW_USING: {
		parse_using_directive(node);
		break;
	}

	case CST::TK_KW_NAMESPACE: {
		NamespaceNode* member = parse_namespace();
		if (member != nullptr) {
			node->namespaces.push_back(member);
			member->parent = node;
		}
		break;
	}
	case CST::TK_KW_CLASS: {
		ClassNode* member = parse_class();
		if (member != nullptr) {
			node->classes.push_back(member);
			member->parent = node;
		}
		break;
	}
	case CST::TK_KW_STRUCT: {
		StructNode* member = parse_struct();
		if (member != nullptr) {
			node->structures.push_back(member);
			member->parent = node;
		}
		break;
	}
	case CST::TK_KW_INTERFACE: {
		InterfaceNode* member = parse_interface();
		if (member != nullptr) {
			node->interfaces.push_back(member);
			member->parent = node;
		}
		break;
	}
	case CST::TK_KW_ENUM: {
		EnumNode* member = parse_enum();
		if (member != nullptr) {
			node->enums.push_back(member);
			member->parent = node;
		}
		break;
	}
	case CST::TK_KW_DELEGATE: {
		// TODO parse delegate
		break;
	}
	case CST::TK_CURLY_BRACKET_OPEN: {
		this->depth.curly_bracket_depth++;
		break;
	}
	case CST::TK_CURLY_BRACKET_CLOSE: {

		// TODO if depth ok - ok else error
		return false;
	}

	default: {
		
		unexpeced_token_error();
		INCPOS(1);
	}
	}

	return true;

}

void CSharpParser::debug_info() {

	cout << "Token: " << (GETTOKEN(0) == CST::TK_IDENTIFIER ? TOKENDATA(0) : CSharpLexer::token_names[(int)GETTOKEN(0)]) << " Pos: " << pos << endl;
}


bool CSharpParser::parse_class_member(ClassNode* node) {

	debug_info();
	parse_attributes();
	parse_modifiers();

	switch (GETTOKEN(0)) {
	case CST::TK_KW_CLASS: {
		ClassNode* member = parse_class();
		if (member != nullptr) node->classes.push_back(member);
		break;
	}
	case CST::TK_KW_INTERFACE: {
		InterfaceNode* member = parse_interface();
		if (member != nullptr) node->interfaces.push_back(member);
		break;
	}
	case CST::TK_KW_STRUCT: {
		StructNode* member = parse_struct();
		if (member != nullptr) node->structures.push_back(member);
		break;
	}
	case CST::TK_KW_ENUM: {
		EnumNode* member = parse_enum();
		if (member != nullptr) node->enums.push_back(member);
		break;
	}
	case CST::TK_KW_DELEGATE: {
		DelegateNode* member = parse_delegate();
		if (member != nullptr) {
			// TODO !!!
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
		string type = parse_type();

		assert(CST::TK_IDENTIFIER);
		string name = TOKENDATA(0);
		INCPOS(1);

		// FIELD
		if (is_actual_token(CST::TK_SEMICOLON)) {

			VarNode* member = new VarNode(name, type);
			apply_modifiers(member); // is const?
			node->variables.push_back(member);
			INCPOS(1); // skip ';'
		}

		// FIELD WITH ASSIGNMENT
		else if (is_actual_token(CST::TK_OP_ASSIGN)) {
			
			VarNode* member = new VarNode(name, type);
			INCPOS(1); // skip '='
			string expr = parse_expression();
			assert(CST::TK_SEMICOLON);
			INCPOS(1); // skip ';'
			member->value = expr;
			return member;
		}

		// PROPERTY
		else if (is_actual_token(CST::TK_CURLY_BRACKET_OPEN)) {
			
			PropertyNode* member = parse_property(name, type);
			if (member != nullptr) node->properties.push_back(member);
			break;
		}

		// METHOD
		else if (is_actual_token(CST::TK_PARENTHESIS_OPEN)) {

			MethodNode* member = parse_method_declaration(name, type);
			if (member != nullptr) node->methods.push_back(member);
			break;

		}

		else {
			unexpeced_token_error();
			INCPOS(1);
		}

	}

	}

	return true;

}

bool CSharpParser::parse_interface_member(InterfaceNode* node) {

	parse_attributes();
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
		string type = parse_type();

		// expected name
		assert(CST::TK_IDENTIFIER);

		string name = TOKENDATA(0);
		INCPOS(1); // skip name

		// PROPERTY
		if (is_actual_token(CST::TK_CURLY_BRACKET_OPEN)) {

			PropertyNode* member = parse_property(name, type);
			if (member != nullptr) node->properties.push_back(member);
			break;
		}

		// METHOD
		else if (is_actual_token(CST::TK_PARENTHESIS_OPEN)) {

			MethodNode* member = parse_method_declaration(name, type, true);
			if (member != nullptr) node->methods.push_back(member);
			break;

		}

		else {
			unexpeced_token_error();
			INCPOS(1);
		}
	}

	}

	return true;
}

CSharpParser::NamespaceNode::NamespaceNode() {
}

void CSharpParser::NamespaceNode::print(int indent) {

	indentation(indent);
	cout << "NAMESPACE " << name << ":" << endl;

	// HEADERS:
	indentation(indent + TAB);
	cout << "using: ";
	for (auto x : using_directives)
		cout << x << " ";
	cout << endl;

	if (namespaces.size() > 0) print_header(indent + TAB, namespaces, "namespaces:");
	if (interfaces.size() > 0) print_header(indent + TAB, interfaces, "interfaces:");
	if (classes.size() > 0) print_header(indent + TAB, classes, "classes:");
	if (structures.size() > 0) print_header(indent + TAB, structures, "structures:");
	if (enums.size() > 0) print_header(indent + TAB, enums, "enums:");

	// NODES:

	if (namespaces.size() > 0)
		for (NamespaceNode* x : namespaces)
			x->print(indent + TAB);
	if (interfaces.size() > 0)
		for (InterfaceNode* x : interfaces)
			x->print(indent + TAB);
	if (classes.size() > 0)
		for (ClassNode* x : classes)
			x->print(indent + TAB);
	if (structures.size() > 0)
		for (StructNode* x : structures)
			x->print(indent + TAB);


}

CSharpParser::VarNode* CSharpParser::parse_declaration() {

	// type name;
	// type name = x;
	// const type name = x;

	VarNode* variable = new VarNode();
	if (is_actual_token(CST::TK_KW_CONST)) {
		variable->modifiers |= (int)CSM::MOD_CONST;
		INCPOS(1);
	}

	string type = parse_type();
	variable->type = type;

	if (is_actual_token(CST::TK_IDENTIFIER)) {
		variable->name = TOKENDATA(0);
		INCPOS(1);
	}
	else {
		delete variable;
		return nullptr;
	}

	// int x;
	if (!is_actual_token(CST::TK_OP_ASSIGN))
		return variable;

	// int x = expr;
	INCPOS(1); // skip '='
	string expression = parse_expression();
	variable->value = expression;

	return variable;
}

CSharpParser::StructNode* CSharpParser::parse_struct() {
	// todo
	return nullptr;
}

CSharpParser::InterfaceNode* CSharpParser::parse_interface() {

	assert(CST::TK_KW_INTERFACE);
	INCPOS(1); // skip 'interface'

	// read name
	assert(CST::TK_IDENTIFIER);
	InterfaceNode* node = new InterfaceNode();
	node->name = TOKENDATA(0);
	INCPOS(1);

	apply_attributes(node);
	apply_modifiers(node);

	// generic
	if (is_actual_token(CST::TK_OP_LESS)) {
		node->is_generic = true;
		node->generic_declarations = parse_generic_declaration();
	}

	// derived and implements
	if (is_actual_token(CST::TK_COLON)) {
		node->base_types = parse_derived_and_implements(node->is_generic);
	}

	// constraints
	if (is_actual_token(CST::TK_KW_WHERE)) {
		node->constraints = parse_constraints();
	}

	assert(CST::TK_CURLY_BRACKET_OPEN);
	INCPOS(1); // skip '}'

	while (parse_interface_member(node));

	return node;

}

CSharpParser::TryNode* CSharpParser::parse_try() {
	return nullptr; // todo
}

CSharpParser::UsingNode* CSharpParser::parse_using_statement() {
	
	assert(CST::TK_KW_USING);

	// using ( declaration or expression ) statement
	UsingNode* node = new UsingNode();

	INCPOS(1);
	node->raw = "using ";


	assert(CST::TK_PARENTHESIS_OPEN);
	INCPOS(1); // skip '('
	node->raw += "(";

	// parse declaration or expression
	int cur_pos = pos;
	VarNode* variable = parse_declaration();
	if (variable != nullptr) { 
		// it was declaration
		node->local_variable = variable;
	}
	else { 
		// it is expression
		pos = cur_pos; // reset and retry
		string expr = parse_expression();
		node->raw += expr;
	}

	assert(CST::TK_PARENTHESIS_CLOSE);
	INCPOS(1); // skip ')'
	node->raw += ")";

	StatementNode* statement = parse_statement();
	node->body = statement;
			
	return node;
}


CSharpParser::ConditionNode* CSharpParser::parse_if_statement() {
	
	// if ( expr ) statement else statement
	ConditionNode* node = new ConditionNode();

	assert(CST::TK_KW_IF);
	INCPOS(1); // skip 'if'
	node->raw = "if ";

	assert(CST::TK_PARENTHESIS_OPEN);
	INCPOS(1); // skip '('
	node->raw += "(";

	string expression = parse_expression();
	node->raw += expression;

	assert(CST::TK_PARENTHESIS_CLOSE);
	INCPOS(1); // skip ')'
	node->raw += ") ";

	StatementNode* then = parse_statement();
	if (then == nullptr) {
		// TODO error
	}

	node->raw += then->raw;

	// optional else
	if (is_actual_token(CST::TK_KW_ELSE)) {

		INCPOS(1); // skip 'else'
		node->raw += " else ";
		StatementNode* else_st = parse_statement();
		if (else_st == nullptr) {
			// todo error
		}
		node->raw += else_st->raw;

	}

	return node;

}

CSharpParser::TryNode* CSharpParser::parse_try_statement()
{
	// try { ... } (catch { ... } (when ...)? )* (finally { ... })?
	TryNode* node = new TryNode();

	assert(CST::TK_KW_TRY);
	INCPOS(1); // skip 'try'

	assert(CST::TK_CURLY_BRACKET_OPEN);

	BlockNode* try_block = parse_block();

	while (is_actual_token(CST::TK_KW_CATCH)) {

		INCPOS(1); // skip 'catch'
		assert(CST::TK_PARENTHESIS_OPEN); INCPOS(1); // skip '('		
		parse_expression(); // ignore
		assert(CST::TK_PARENTHESIS_CLOSE); INCPOS(1); // skip ')'

		// possible when
		if (is_actual_token(CST::TK_KW_WHEN)) {

			INCPOS(1); // skip 'when'
			assert(CST::TK_PARENTHESIS_OPEN); INCPOS(1); // skip '('
			parse_expression(); // ignore
			assert(CST::TK_PARENTHESIS_CLOSE); INCPOS(1); // skip ')'
		}

		// block
		assert(CST::TK_CURLY_BRACKET_OPEN);

		BlockNode* block = parse_block();
		node->blocks.push_back(block);

	}

	// possible finally
	if (is_actual_token(CST::TK_KW_FINALLY)) {

		INCPOS(1); // skip 'finally'
		assert(CST::TK_CURLY_BRACKET_OPEN);
		BlockNode* finally_block = parse_block();
		node->blocks.push_back(finally_block);
	}

	return node;
}

CSharpParser::ConditionNode* CSharpParser::parse_switch_statement() {
	
	// switch (...) { (case ... (when ...)?)* default: ?}
	ConditionNode* node = new ConditionNode();

	assert(CST::TK_KW_SWITCH);
	INCPOS(1); // skip 'switch'

	assert(CST::TK_PARENTHESIS_OPEN);
	INCPOS(1); // skip '('

	skip_until_token(CST::TK_PARENTHESIS_CLOSE);
	INCPOS(1); // skip ')'

	assert(CST::TK_CURLY_BRACKET_OPEN);

	skip_until_token(CST::TK_CURLY_BRACKET_CLOSE);
	INCPOS(1); // skip '}'

	return node;
}

CSharpParser::StatementNode* CSharpParser::parse_property_definition() {

	switch (GETTOKEN(0)) {

	CASEATYPICAL(nullptr)

	case CST::TK_OP_LAMBDA: {
		INCPOS(1); // skip '=>'
		return parse_statement();
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
		return parse_block();
	}
	default: {
		unexpeced_token_error();
		INCPOS(1);
	}

	}

}

CSharpParser::PropertyNode* CSharpParser::parse_property(string name, string type)
{
	assert(CST::TK_CURLY_BRACKET_OPEN);
	INCPOS(1); // skip '{'

	PropertyNode* node = new PropertyNode();
	node->name = name;
	node->type = type;

	// we don't know what is declared first: get or set
	bool get_parsed = false;
	bool set_parsed = false;

	parse_attributes();
	parse_modifiers();

	while (true) {

		switch (GETTOKEN(0)) {
		
		CASEATYPICAL(nullptr)

		case CST::TK_KW_GET: {
			if (get_parsed) {
				// todo error
			} else {
				INCPOS(1); // skip 'get'
				node->get_modifiers = this->modifiers;
				node->get_statement = parse_property_definition();
				get_parsed = true;
				if (!set_parsed) {
					// probably following definition of set
					parse_attributes();
					parse_modifiers();
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
				node->set_statement = parse_property_definition();
				this->kw_value_allowed = false;
				set_parsed = true;
				if (!get_parsed) {
					// probably following definition of get
					parse_attributes();
					parse_modifiers();
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
				parse_expression(); // ignore
				INCPOS(1); // skip ';'
			}

			return node;
		}
		default: {
			unexpeced_token_error();
			INCPOS(1);
		}
		}
	}

	return node;
}


CSharpParser::StatementNode* CSharpParser::parse_statement() {

	switch (GETTOKEN(0)) {

	CASEATYPICAL(nullptr)

	case CST::TK_CURLY_BRACKET_OPEN: {
		BlockNode* node = parse_block();
		return node;
	}

	CASEBASETYPE {
		VarNode *variable = parse_declaration();
		DeclarationNode* node = new DeclarationNode();
		node->variable = variable;
		INCPOS(1); // skip ';'

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
		VarNode* variable = parse_declaration();
		if (variable != nullptr) {
			DeclarationNode* node = new DeclarationNode();
			node->variable = variable;
			INCPOS(1); // skip ';'
			return node;
		}

		pos = cur_pos; // reset
		StatementNode* node = new StatementNode();

		// label?
		if (is_actual_token(CST::TK_COLON)) {
			node->raw += TOKENDATA(0) + ":";
			// todo add TOKENDATA(0) to labels set
			INCPOS(2);
			return node;
		}

		// assignment or expression?
		string expr = parse_expression();
		assert(CST::TK_SEMICOLON);
		INCPOS(1); // skip ';'
		node->raw = expr + ";";
		return node;

	}
	case CST::TK_KW_FOR:
	case CST::TK_KW_WHILE:
	case CST::TK_KW_FOREACH:
	case CST::TK_KW_DO: {
		LoopNode* node = parse_loop();
		return node;
	}

	// CONDITION STATEMENT
	case CST::TK_KW_IF: {
		ConditionNode* node = parse_if_statement();
		return node;
	}
	case CST::TK_KW_SWITCH: {
		ConditionNode* node = parse_switch_statement();
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
		JumpNode* node = parse_jump();
		return node;
	}
	
	// TRY-CATCH-FINALLY
	case CST::TK_KW_TRY: {
		TryNode* node = parse_try_statement();
		return node;
	}

	 // USING STATEMENT
	case CST::TK_KW_USING: {
		UsingNode* node = parse_using_statement();
		return node;
	}

	// OTHER
	case CST::TK_KW_AWAIT: {
		// todo
	}
	case CST::TK_KW_FIXED: {
		// todo
	}
	case CST::TK_KW_LOCK: {
		// todo
	}
	case CST::TK_KW_THIS:
	case CST::TK_KW_BASE: {
		INCPOS(1);
	}
	case CST::TK_SEMICOLON: {
		StatementNode* node = new StatementNode();
		node->raw = ";";
		INCPOS(1);
		return node; // empty statement
	}
	default: {
		unexpeced_token_error();
		INCPOS(1);
	}

	}

	return nullptr;
}

CSharpParser::MethodNode* CSharpParser::parse_method_declaration(string name, string return_type, bool interface_context) {

	MethodNode* node = new MethodNode();
	node->name = name;
	node->return_type = return_type;

	assert(CST::TK_PARENTHESIS_OPEN);
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
			string val = parse_expression();
			argument->value = val;
			break;
		}
		case CST::TK_KW_IN: {
			INCPOS(1); // ignore
			break;
		}
		case CST::TK_KW_OUT: {
			INCPOS(1); // ignore
			break;
		}
		case CST::TK_KW_REF: {
			INCPOS(1); // ignore
			break;
		}
		case CST::TK_KW_PARAMS: {
			INCPOS(1); // ignore
			break;
		}
		
		default: {

			string type = parse_type();
			assert(CST::TK_IDENTIFIER);
			string name = TOKENDATA(0);
			INCPOS(1);
			argument->type = type;
			argument->name = name;
		}
		}
	}

	// TODO GENERIC METHODS ( : where T is ... )
	debug_info();
	if (is_actual_token(CST::TK_COLON)) {
		INCPOS(1); // skip ':'
		if (is_actual_token(CST::TK_KW_BASE)) {
			INCPOS(1); // skip 'base'
			parse_method_invocation();
		}
	}

	// BODY
	if (interface_context) {
		assert(CST::TK_SEMICOLON);
		INCPOS(1); // skip ';'
	}
	else { // class context
		node->body = parse_block();
	}

	return node;
}

// parse just invocation without name 
string CSharpParser::parse_method_invocation() {

	assert(CST::TK_PARENTHESIS_OPEN);
	INCPOS(1);
	string res = "(";

	// ARGUMETNS
	while (!is_actual_token(CST::TK_PARENTHESIS_CLOSE)) {

		if (is_actual_token(CST::TK_COMMA)) {
			res += " , ";
			INCPOS(1);
		}
		else {
			res += parse_expression();
		}

	}

	res += ")";
	INCPOS(1);

	return res;

}

CSharpParser::BlockNode* CSharpParser::parse_block() {

	assert(CST::TK_CURLY_BRACKET_OPEN);
	INCPOS(1); // skip '{'

	BlockNode* node = new BlockNode();
	while (!is_actual_token(CST::TK_CURLY_BRACKET_CLOSE)) {
		debug_info();
		StatementNode* statement = parse_statement();
		// TODO dodaj na koniec listy statementow
	}

	INCPOS(1); // skip '}'

	return node;

}


void CSharpParser::ClassNode::print(int indent) {

	indentation(indent);
	cout << "CLASS " << name << ":" << endl;

	// HEADERS:
	if (base_types.size() > 0) {
		indentation(indent + TAB);
		cout << "base types:";
		for (string& x : base_types)
			cout << " " << x;
		cout << endl;
	}

	if (interfaces.size() > 0)			print_header(indent + TAB, interfaces, "interfaces:");
	if (classes.size() > 0)				print_header(indent + TAB, classes, "classes:");
	if (structures.size() > 0)			print_header(indent + TAB, structures, "structures:");
	if (enums.size() > 0)				print_header(indent + TAB, enums, "enums:");
	if (methods.size() > 0)				print_header(indent + TAB, methods, "methods:");
	if (properties.size() > 0)			print_header(indent + TAB, properties, "properties:");
	if (variables.size() > 0)			print_header(indent + TAB, variables, "variables:");

	// NODES:
	if (interfaces.size() > 0)	for (InterfaceNode* x : interfaces)	x->print(indent + TAB);
	if (classes.size() > 0)		for (ClassNode* x : classes)		x->print(indent + TAB);
	if (structures.size() > 0)	for (StructNode* x : structures)	x->print(indent + TAB);
}

void CSharpParser::StructNode::print(int indent) {

	indentation(indent);
	cout << "STRUCT " << name << ":" << endl;

	// HEADERS:
	if (base_types.size() > 0) {
		indentation(indent + TAB);
		cout << "base types:";
		for (string& x : base_types)
			cout << " " << x;
		cout << endl;
	}

		
	if (interfaces.size() > 0)			print_header(indent + TAB, interfaces, "interfaces:");
	if (classes.size() > 0)				print_header(indent + TAB, classes, "classes:");
	if (structures.size() > 0)			print_header(indent + TAB, structures, "structures:");
	if (enums.size() > 0)				print_header(indent + TAB, enums, "enums:");
	if (methods.size() > 0)				print_header(indent + TAB, methods, "methods:");
	if (properties.size() > 0)			print_header(indent + TAB, properties, "properties:");
	if (variables.size() > 0)			print_header(indent + TAB, variables, "variables:");

	// NODES:
	if (interfaces.size() > 0)	for (InterfaceNode* x : interfaces)	x->print(indent + TAB);
	if (classes.size() > 0)		for (ClassNode* x : classes)		x->print(indent + TAB);
	if (structures.size() > 0)	for (StructNode* x : structures)	x->print(indent + TAB);
}

void CSharpParser::StatementNode::print(int indent) {

	indentation(indent);
	cout << "STATEMENT" << endl;

}

void CSharpParser::VarNode::print(int indent) {

	indentation(indent);
	cout << "VAR: ";
	if (modifiers & (int)CSM::MOD_CONST) cout << "const ";
	cout << type << " " << name << endl;

}


void CSharpParser::MethodNode::print(int indent) {

	indentation(indent);
	cout << "METHOD " << return_type << " " << name << endl;
	cout << "(";
	for (VarNode* v : arguments)
		cout << v->type << " " << v->name << " , ";
	cout << ")" << endl;

}

void CSharpParser::InterfaceNode::print(int indent) {

	indentation(indent);
	cout << "INTERFACE " << name << endl;
	// TODO members

}

void CSharpParser::EnumNode::print(int indent) {

	indentation(indent); cout << "ENUM " << name << ":" << endl;
	indentation(indent + 2); cout << "type: " << type << endl;

	indentation(indent + 2); cout << "members: ";
	for (string& member : members)
		cout << member << " ";
	cout << endl;

}

void CSharpParser::PropertyNode::print(int indent) {
	// TODO
}
