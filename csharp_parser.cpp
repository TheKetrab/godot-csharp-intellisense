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
		case CST::TK_KW_VOLATILE:

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
	{ CST::TK_KW_VOLATILE,		CSM::MOD_VOLATILE }
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

void CSharpParser::parse_attributes() {

	int bracket_depth = 0;
	string attribute = "";
	while (true) {

		switch (GETTOKEN(0)) {
		case CST::TK_EOF: {
			return; // todo end
		}
		case CST::TK_ERROR: {
			return; // todo error
		}
		case CST::TK_CURSOR: {
			return; // todo ??
		}
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
		if (GETTOKEN(0) != CST::TK_KW_NAMESPACE)
			return nullptr;
		else INCPOS(1);

		// read name
		if (GETTOKEN(0) != CST::TK_IDENTIFIER)
			return nullptr;
		else {
			name = TOKENDATA(0);
			INCPOS(1);
		}
	}

	NamespaceNode* node = new NamespaceNode();
	node->name = name;

	// curly bracket open
	if (GETTOKEN(0) != CST::TK_CURLY_BRACKET_OPEN); // todo error end destruct node
	else INCPOS(1);

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

	if (GETTOKEN(0) != CST::TK_OP_LESS) {
		// todo error
	}

	INCPOS(1); // skip '<'

	vector<string> generic_declarations;

	while (true) {
		switch (GETTOKEN(0)) {

			// TODO case error empty eof, cursor

		case CST::TK_IDENTIFIER: { 
			generic_declarations.push_back(TOKENDATA(0));
			INCPOS(1); break; 
		}
		case CST::TK_COMMA: { INCPOS(1); break; }
		case CST::TK_OP_GREATER: { INCPOS(1); return generic_declarations; }
		default: {
			// todo error unexpected token
		}
		}
	}

	return generic_declarations;
}

vector<string> CSharpParser::parse_derived_and_implements(bool generic_context) {

	if (GETTOKEN(0) != CST::TK_COLON) {
		// todo error
	}

	INCPOS(1); // skip ':'
	
	vector<string> types;

	while (true) {
		switch (GETTOKEN(0)) {

			// TODO case error empty eof, cursor

		case CST::TK_IDENTIFIER: {
			string type = parse_type();
			types.push_back(type);
			break;
		}
		case CST::TK_COMMA: { INCPOS(1); break; }
		case CST::TK_CURLY_BRACKET_OPEN: { return types; }
		default: {
			if (generic_context && GETTOKEN(0) == CST::TK_KW_WHERE) {
				return types;
			}
			// todo error unexpected token
		}
		}
	}

	return types;

}


// generic -> where T : C1, new()
string CSharpParser::parse_constraints() {

	if (GETTOKEN(0) != CST::TK_KW_WHERE) {
		// todo error
	}

	string constraints;
	// TODO in the future
	skip_until_token(CST::TK_CURLY_BRACKET_OPEN);

	return "";
}

CSharpParser::ClassNode* CSharpParser::parse_class() {

	// is class?
	if (GETTOKEN(0) != CST::TK_KW_CLASS) return nullptr;
	INCPOS(1);

	// read name
	if (GETTOKEN(0) != CST::TK_IDENTIFIER) return nullptr;

	ClassNode* node = new ClassNode();
	node->name = TOKENDATA(0);
	INCPOS(1);

	apply_attributes(node);
	apply_modifiers(node);

	// generic
	if (GETTOKEN(0) == CST::TK_OP_LESS) {
		node->is_generic = true;
		node->generic_declarations = parse_generic_declaration();
	}

	// derived and implements
	if (GETTOKEN(0) == CST::TK_COLON) {
		node->base_types = parse_derived_and_implements(node->is_generic);
	}

	// constraints
	if (GETTOKEN(0) == CST::TK_KW_WHERE) {
		node->constraints = parse_constraints();
	}
	
	if (GETTOKEN(0) != CST::TK_CURLY_BRACKET_OPEN) {
		// TODO ERROR
	}

	INCPOS(1);

	while (parse_class_member(node));

	return node;
}

string CSharpParser::parse_initialization_block() {
	
	if (GETTOKEN(0) != CST::TK_CURLY_BRACKET_OPEN) {
		// todo error
	}

	INCPOS(1); // skip '{'

	string initialization_block = "{ ";
	while (true) {

		switch (GETTOKEN(0)) {

			// TODO CASE empty error eof cursor

		case CST::TK_IDENTIFIER: {
			initialization_block += TOKENDATA(0);
			INCPOS(1);
			break;
		}

		case CST::TK_OP_ASSIGN: {
			initialization_block += " = ";
			INCPOS(1);
			string val = parse_expression();
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
			// todo error - unknown token
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

	if (GETTOKEN(0) != CST::TK_KW_NEW) {
		// todo error
	}

	string res = "new ";
	INCPOS(1); // skip 'new'

	switch (GETTOKEN(0)) {

		// TODO standard check error, eof, empty, cursor

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

		case CST::TK_EOF: {
			return res;
		}
		case CST::TK_ERROR: {
			return res;
		}
		case CST::TK_EMPTY: {
			return res;
		}
		case CST::TK_CURSOR:
		{
			INCPOS(1); break;
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
				// TODO ERROR !! Unknown token -> sth went wrong, stop parsing expression
			}
		}


		}

	}
	
}

CSharpParser::EnumNode* CSharpParser::parse_enum() {

	// is enum?
	if (GETTOKEN(0) != CST::TK_KW_ENUM) return nullptr;
	INCPOS(1);

	// read name
	if (GETTOKEN(0) != CST::TK_IDENTIFIER) return nullptr;


	EnumNode* node = new EnumNode();
	node->name = TOKENDATA(0);
	INCPOS(1);

	apply_attributes(node);
	apply_modifiers(node);

	node->type = "int"; // domyslnie

	// enum type
	if (GETTOKEN(0) == CST::TK_COLON) {
		INCPOS(1);
		node->type = parse_type();
	}

	// parse members
	bool end = false;
	while (!end) {

		switch (GETTOKEN(0)) {

			// todo typical

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
			// TODO error
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
	default: { delete node; return nullptr; }
	}

	node->raw = CSharpLexer::token_names[(int)GETTOKEN(0)];
	INCPOS(1); // skip keyword

	bool end = false;
	while (!end) {
		switch (GETTOKEN(0)) {
			// todo case typical
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
		DeclarationNode* declaration = parse_declaration();
		if (declaration == nullptr) {
			skip_until_token(CST::TK_SEMICOLON); // assignment or sth else
		}
		else {
			node->local_variable = declaration->variable;
			delete declaration;
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

		INCPOS(1); // sip '('

		// declaration
		DeclarationNode* declaration = parse_declaration();
		if (declaration == nullptr) {
			skip_until_token(CST::TK_KW_IN);
		}
		else {
			node->local_variable = declaration->variable;
			delete declaration;
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

		case CST::TK_ERROR: {
			//todo
		}
		case CST::TK_EOF: {
			// todo
		}
		case CST::TK_CURSOR: {
			// todo
		}

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
			if (GETTOKEN(0) == CST::TK_PERIOD) {
				res += ".";
				INCPOS(1);
				break;
			}
			else if (GETTOKEN(0) == CST::TK_OP_LESS) {
				break;
			}
			else if (GETTOKEN(0) == CST::TK_BRACKET_OPEN) {
				break;
			}
			else { // none of '.', '<', '['
				return res;
			}
			break;
		}
		default: {
			debug_info();
		}
		}
	}

	return res;
}

void CSharpParser::parse_using_directives(NamespaceNode* node) {

	// TODO nieskonczone

	// using X;
	// using static X;
	// using X = Y;

	// is using?
	if (GETTOKEN(0) != CST::TK_KW_USING) return;

	INCPOS(1);

	// read name
	if (GETTOKEN(0) != CST::TK_IDENTIFIER) return;

	string name = TOKENDATA(0);
	node->using_directives.push_back(name);
	INCPOS(1);

	// ;
	INCPOS(1);


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
		parse_using_directives(node);
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
		
		// unknown token - skip until '}' TODO
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

		// expected name
		if (GETTOKEN(0) != CST::TK_IDENTIFIER)
			return false; // TODO error

		string name = TOKENDATA(0);
		INCPOS(1);

		// FIELD
		if (GETTOKEN(0) == CST::TK_SEMICOLON) {

			VarNode* member = new VarNode(name, type);
			apply_modifiers(member); // is const?
			node->variables.push_back(member);
			INCPOS(1); // skip ';'
		}

		// FIELD WITH ASSIGNMENT
		else if (GETTOKEN(0) == CST::TK_OP_ASSIGN) {
			
			VarNode* member = new VarNode(name, type);
			INCPOS(1); // skip '='
			string expr = parse_expression();
			if (GETTOKEN(0) != CST::TK_SEMICOLON) {
				// todo error
			}
			INCPOS(1); // skip ';'
			member->value = expr;
			return member;
		}

		// PROPERTY
		else if (GETTOKEN(0) == CST::TK_CURLY_BRACKET_OPEN) {
			
			PropertyNode* member = parse_property(name, type);
			if (member != nullptr) node->properties.push_back(member);
			break;
		}

		// METHOD
		else if (GETTOKEN(0) == CST::TK_PARENTHESIS_OPEN) {

			MethodNode* member = parse_method(name, type);
			if (member != nullptr) node->methods.push_back(member);
			break;

		}

		else {
			// TODO ERROR
		}


	}


	}

	return true;

}

void CSharpParser::parse_interface_member(InterfaceNode* node) {
	// todo
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


CSharpParser::DeclarationNode* CSharpParser::parse_declaration() {

	DeclarationNode* node = new DeclarationNode();

	// type name;
	// type name = x;
	// const type name = x;

	string raw = "";
	VarNode* variable = new VarNode();
	if (GETTOKEN(0) == CST::TK_KW_CONST) {
		raw += "const ";
		variable->modifiers &= (int)CSM::MOD_CONST;
		INCPOS(1);
	}

	string type = parse_type();
	raw += type + " ";
	variable->type = type;

	if (GETTOKEN(0) == CST::TK_IDENTIFIER) {
		raw += TOKENDATA(0);
		variable->name = TOKENDATA(0);
		INCPOS(1);
	}
	else {
		delete variable;
		delete node;
		return nullptr;
	}

	// int x;
	if (GETTOKEN(0) == CST::TK_SEMICOLON) {
		raw += ";";
		INCPOS(1);
	}

	// int x = expr;
	else if (GETTOKEN(0) == CST::TK_OP_ASSIGN) {
		raw += " = ";
		INCPOS(1);
		string expression = parse_expression();
		if (GETTOKEN(0) != CST::TK_SEMICOLON) {
			// TODO ERROR
		}
		raw += expression + ";";
		variable->value = expression;
		INCPOS(1);
	}
	else {
		// TODO ERROR
	}

	node->raw = raw;
	node->variable = variable;

	return node;

}

CSharpParser::StructNode* CSharpParser::parse_struct() {
	// todo
	return nullptr;
}

CSharpParser::InterfaceNode* CSharpParser::parse_interface() {
	return nullptr; // todo
}

CSharpParser::TryNode* CSharpParser::parse_try() {
	return nullptr; // todo
}

CSharpParser::UsingNode* CSharpParser::parse_using() {
	return nullptr; // todo
}


CSharpParser::ConditionNode* CSharpParser::parse_if_statement() {
	
	// if ( expr ) statement else statement
	ConditionNode* node = new ConditionNode();

	if (GETTOKEN(0) != CST::TK_KW_IF) {
		// todo error
	}

	INCPOS(1);
	node->raw = "if ";

	if (GETTOKEN(0) != CST::TK_PARENTHESIS_OPEN) {
		// todo error
	}

	INCPOS(1);
	node->raw += "(";

	string expression = parse_expression();
	node->raw += expression;

	if (GETTOKEN(0) != CST::TK_PARENTHESIS_CLOSE) {
		// todo error
	}

	INCPOS(1);
	node->raw += ") ";

	StatementNode* then = parse_statement();
	if (then == nullptr) {
		// TODO error
	}

	node->raw += then->raw;

	// optional else
	if (GETTOKEN(0) == CST::TK_KW_ELSE) {

		INCPOS(1);
		node->raw += " else ";
		StatementNode* else_st = parse_statement();
		if (else_st == nullptr) {
			// todo error
		}
		node->raw += else_st->raw;

	}

	return node;

}
CSharpParser::ConditionNode* CSharpParser::parse_switch_statement() {
	return nullptr;
}

CSharpParser::StatementNode* CSharpParser::parse_property_definition() {
	debug_info();
	switch (GETTOKEN(0)) {

		// TODO case error cursor eof empty

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
		// TODO error
	}

	}

}

CSharpParser::PropertyNode* CSharpParser::parse_property(string name, string type)
{
	if (GETTOKEN(0) != CST::TK_CURLY_BRACKET_OPEN) {
		// todo error
	}

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
		debug_info();
		switch (GETTOKEN(0)) {
			// TODO case eof  empy cursor

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
			return node;
		}
		default: {
			// todo error unknown
		}
		}
	}

	return node;
}


CSharpParser::StatementNode* CSharpParser::parse_statement() {

	switch (GETTOKEN(0)) {

	case CST::TK_CURLY_BRACKET_OPEN: {
		BlockNode* node = parse_block();
		return node;
	}

	CASEBASETYPE {
		DeclarationNode* node = parse_declaration();
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
		StatementNode* node = parse_declaration();
		if (node != nullptr) return node;

		pos = cur_pos; // reset
		node = new StatementNode();

		// label?
		if (GETTOKEN(1) == CST::TK_COLON) {
			node->raw += TOKENDATA(0) + ":";
			// todo add TOKENDATA(0) to labels set
			INCPOS(2);
			return node;
		}

		// assignment or expression?
		string expr = parse_expression();
		if (GETTOKEN(0) != CST::TK_SEMICOLON) {
			// todo error
		}
		else {
			node->raw = expr + ";";
			INCPOS(1); // skip ';'
		}
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
		// todo
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
	case CST::TK_SEMICOLON: {
		StatementNode* node = new StatementNode();
		node->raw = ";";
		INCPOS(1);
		return node; // empty statement
	}




	}

	return nullptr;
}

CSharpParser::MethodNode* CSharpParser::parse_method(string name, string return_type) {

	cout << "parse method (" << name << "," << return_type << ")" << endl;

	MethodNode* node = new MethodNode();
	node->name = name;
	node->return_type = return_type;

	if (GETTOKEN(0) != CST::TK_PARENTHESIS_OPEN) {
		// todo error
	}
	else INCPOS(1);

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
		default: {

			string type = parse_type();
			if (GETTOKEN(0) != CST::TK_IDENTIFIER) {
				// todo error
			}
			string name = TOKENDATA(0);
			INCPOS(1);
			argument->type = type;
			argument->name = name;
		}
		}
	}

	// TODO GENERIC METHODS ( : where T is ... )

	// BODY
	node->body = parse_block();

	return node;
}

// parse just invocation without name 
string CSharpParser::parse_method_invocation() {

	if (GETTOKEN(0) != CST::TK_PARENTHESIS_OPEN) {
		// todo error
	}
	
	INCPOS(1);
	string res = "(";

	// ARGUMETNS
	while (GETTOKEN(0) != CST::TK_PARENTHESIS_CLOSE) {

		if (GETTOKEN(0) == CST::TK_COMMA) {
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

	if (GETTOKEN(0) != CST::TK_CURLY_BRACKET_OPEN) {
		// todo error
	}

	INCPOS(1); // skip '{'

	BlockNode* node = new BlockNode();
	while (GETTOKEN(0) != CST::TK_CURLY_BRACKET_CLOSE) {
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
