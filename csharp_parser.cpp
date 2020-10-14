#include "csharp_parser.h"
#include <iostream>

using CST = CSharpLexer::Token;
using CSM = CSharpParser::Modifier;

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

	cout << "PARSER: clear" << endl;

	cursor = nullptr;
	current = nullptr;
	root = nullptr;
	pos = 0;
	modifiers = 0;

}

#define GETTOKEN(ofs) ((ofs + pos) >= len ? CST::TK_ERROR : tokens[ofs + pos].type)
#define INCPOS(ammount) { pos += ammount; }


void CSharpParser::parse_modifiers() {

	this->modifiers = 0;
	while (true) {

		switch (tokens[pos].type) {

		case CST::TK_KW_PUBLIC:
		case CST::TK_KW_PROTECTED:
		case CST::TK_KW_PRIVATE:
		case CST::TK_KW_INTERNAL:
		case CST::TK_KW_EXTERN:
		case CST::TK_KW_ABSTRACT:
		case CST::TK_KW_CONST:
		case CST::TK_KW_OVERRIDE:
		case CST::TK_KW_PARTIAL:
		case CST::TK_KW_READONLY:
		case CST::TK_KW_SEALED:
		case CST::TK_KW_STATIC:
		case CST::TK_KW_UNSAFE:
		case CST::TK_KW_VIRTUAL:
		case CST::TK_KW_VOLATILE: {
			pos++;
			modifiers |= (int)to_modifier[tokens[pos].type];
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
	std::string attribute = "";
	while (true) {

		switch (GETTOKEN(0)) {
		case CST::TK_EOF: {
			// todo end
		}
		case CST::TK_ERROR: {
			// todo error
		}
		case CST::TK_CURSOR: {
			// todo ??
		}
		case CST::TK_BRACKET_OPEN: {

			if (bracket_depth == 0) {
				attribute = "[ ";
			}

			bracket_depth++;
			INCPOS(1);
		}
		case CST::TK_BRACKET_CLOSE: {
			bracket_depth--;
			INCPOS(1);

			if (bracket_depth == 0) {
				attribute += "]";
				this->attributes.push_back(attribute);
				attribute = "";
			}
		}
		default: {
			if (bracket_depth == 0) return;
			else attribute += tokens[pos].data + " ";
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

	cout << "PARSER: parse_namespace" << endl;

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
			name = tokens[pos].data;
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


void CSharpParser::parse_possible_type_parameter_node(Node* node) {

	skip_until_token(CST::TK_CURLY_BRACKET_OPEN);
	INCPOS(1);
	return;
	// TODO in the future

	while (true) {

		switch (GETTOKEN(0)) {
		case CST::TK_ERROR: {
			// todo
		}
		case CST::TK_EOF: {
			// todo
		}
		case CST::TK_BRACKET_CLOSE: {
			// todo -> end parsing
		}
		case CST::TK_KW_WHERE: {
			// todo skip untill '{'
		}
		case CST::TK_OP_LESS: {
		}
		case CST::TK_IDENTIFIER: {
		}
		case CST::TK_CURLY_BRACKET_OPEN: {
			// todo finish
		}
		}
	}

}

CSharpParser::ClassNode* CSharpParser::parse_class() {

	cout << "PARSER: parse_class" << endl;

	// is class?
	if (GETTOKEN(0) != CST::TK_KW_CLASS) return nullptr;
	INCPOS(1);

	// read name
	if (GETTOKEN(0) != CST::TK_IDENTIFIER) return nullptr;

	ClassNode* node = new ClassNode();
	node->name = tokens[pos].data;
	INCPOS(1);

	apply_attributes(node);
	apply_modifiers(node);

	cout << " -- 1 -- " << endl;
	// parse generic params, base class and interfaces <-- todo zrobic z tego funkcje
	parse_possible_type_parameter_node(node); // wstrzykuje do klasy info z tego co jest za nazwa klasy
	cout << " -- 2 -- " << endl;

	while (GETTOKEN(0) != CST::TK_CURLY_BRACKET_CLOSE) {
		parse_class_member(node);
	}

	INCPOS(1);

	return node;
}

CSharpParser::EnumNode* CSharpParser::parse_enum() {

	// is enum?
	if (GETTOKEN(0) != CST::TK_KW_ENUM) return nullptr;
	INCPOS(1);

	// read name
	if (GETTOKEN(0) != CST::TK_IDENTIFIER) return nullptr;


	EnumNode* node = new EnumNode();
	node->name = tokens[pos].data;
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
	while (GETTOKEN(0) != CST::TK_CURLY_BRACKET_CLOSE) {
		parse_enum_member(node);
	}

	return node;
}

CSharpParser::LoopNode* CSharpParser::parse_loop() {

	LoopNode* node = new LoopNode();

	// type of loop
	switch (GETTOKEN(0)) {
	case CST::TK_KW_DO: node->loop_type = LoopNode::Type::DO;
	case CST::TK_KW_FOR: node->loop_type = LoopNode::Type::FOR;
	case CST::TK_KW_FOREACH: node->loop_type = LoopNode::Type::FOREACH;
	case CST::TK_KW_WHILE: node->loop_type = LoopNode::Type::WHILE;
	default: { delete node; return nullptr; }
	}

	INCPOS(1);

	// ----- ----- -----
	// PARSE BEFORE BLOCK

	if (node->loop_type == LoopNode::Type::FOR) {

		// init
		DeclarationNode* declaration = parse_declaration();
		if (declaration == nullptr) {
			skip_until_token(CST::TK_SEMICOLON); // assignment or sth else
		}
		else {
			node->local_variable = declaration->variable;
			delete declaration;
		}

		// cond
		skip_until_token(CST::TK_SEMICOLON);

		// iter
		skip_until_token(CST::TK_PARENTHESIS_CLOSE);

	}

	else if (node->loop_type == LoopNode::Type::FOREACH) {

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

	}

	else if (node->loop_type == LoopNode::Type::WHILE) {

		// cond
		skip_until_token(CST::TK_PARENTHESIS_CLOSE);

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

std::string CSharpParser::parse_type() {

	cout << "parse type" << endl;

	std::string res = "";

	// base type: int/bool/... (nullable ?) [,,...,]
	// complex type: NAMESPACE::NAME1. ... .NAMEn<type1,type2,...>[,,...,]

	// TODO dodac obslugiwanie '::'

	bool base_type = false;
	bool complex_type = false;
	bool generic_types_mode = false;
	bool array_mode = false;

	while (true) {
		cout << "token: " << pos << " name: " << CSharpLexer::token_names[(int)GETTOKEN(0)] << endl;
		cout << "res is: " << res << endl;
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
		}

		case CST::TK_OP_GREATER: { // >
			generic_types_mode = false;
			res += ">";
			INCPOS(1);

			// this is end unless it is '['
			if (GETTOKEN(0) != CST::TK_BRACKET_OPEN) {
				return res;
			}
		}

		case CST::TK_BRACKET_OPEN: { // [
			array_mode = true;
			res += "[";
			INCPOS(1);
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
		}

								  // base type?
		case CST::TK_KW_BOOL:
		case CST::TK_KW_BYTE:
		case CST::TK_KW_CHAR:
		case CST::TK_KW_DECIMAL:
		case CST::TK_KW_DOUBLE:
		case CST::TK_KW_FLOAT:
		case CST::TK_KW_LONG:
		case CST::TK_KW_OBJECT:
		case CST::TK_KW_SBYTE:
		case CST::TK_KW_SHORT:
		case CST::TK_KW_STRING:
		case CST::TK_KW_UINT:
		case CST::TK_KW_ULONG:
		case CST::TK_KW_USHORT:
		case CST::TK_KW_VOID: {

			cout << "base type!" << endl;

			if (complex_type) {
				// TODO error -> System.Reflexion.int ??????
				// vector<int> still allowed, cause int will be parsed recursively
			}

			base_type = true;
			res += CSharpLexer::token_names[(int)GETTOKEN(0)];
			INCPOS(1);

			// another has to be '?' either '[' - else it is finish
			if (GETTOKEN(0) != CST::TK_OP_QUESTION_MARK
				|| GETTOKEN(0) != CST::TK_BRACKET_OPEN)
			{
				return res; // end
			}
		}

									// complex type
		case CST::TK_IDENTIFIER: {

			if (base_type) {
				// TODO error
			}

			cout << "identifier: " << tokens[pos].data << endl;

			complex_type = true;
			res += tokens[pos].data;
			INCPOS(1);

			// finish if
			if (GETTOKEN(0) != CST::TK_PERIOD				// .
				|| GETTOKEN(0) != CST::TK_OP_LESS			// <
				|| GETTOKEN(0) != CST::TK_BRACKET_OPEN) {	// [
				return res;
			}
		}
		}
	}

	return res;
}

void CSharpParser::parse_using_directives(NamespaceNode* node) {

	cout << "parse using directives" << endl;


	// TODO nieskonczone

	// using X;
	// using static X;
	// using X = Y;

	// is using?
	if (GETTOKEN(0) != CST::TK_KW_USING) return;

	INCPOS(1);

	// read name
	if (GETTOKEN(0) != CST::TK_IDENTIFIER) return;

	string name = tokens[pos].data;
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

	cout << "namespace:: actual token is: " << (int)GETTOKEN(0) << " at pos: " << pos << endl;

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

void CSharpParser::parse_class_member(ClassNode* node) {

	cout << "parse class member" << endl;
	cout << "actual token is: " << (int)GETTOKEN(0) << " and current pos is: " << pos << endl;

	parse_attributes();
	cout << " >> a" << endl;
	parse_modifiers();
	cout << " >> b" << endl;

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
	default: {

		// field (var), property or method
		string type = parse_type();
		cout << "found type is: " << type << endl;

		// expected name
		if (GETTOKEN(0) != CST::TK_IDENTIFIER)
			return; // TODO error

		string name = tokens[pos].data;
		INCPOS(1);

		// FIELD
		if (GETTOKEN(0) == CST::TK_SEMICOLON) {

			VarNode* member = new VarNode(name, type);
			apply_modifiers(member); // is const?
			node->variables.push_back(member);
			INCPOS(1);
		}

		// FIELD WITH ASSIGNMENT
		else if (GETTOKEN(0) == CST::TK_OP_EQUAL) {
			// TODO
		}

		// PROPERTY
		else if (GETTOKEN(0) == CST::TK_CURLY_BRACKET_OPEN) {
			// TODO parse property, PropertyNode ??? getable, settable?
		}

		// METHOD
		else if (GETTOKEN(0) == CST::TK_PARENTHESIS_OPEN) {

			MethodNode* member = parse_method(name, type);
			cout << "method parsed" << endl;
			if (member != nullptr) node->methods.push_back(member);
			break;

		}

		else {
			// TODO ERROR
		}


	}


	}

}

void CSharpParser::parse_interface_member(InterfaceNode* node) {
	// todo
}

void CSharpParser::parse_enum_member(EnumNode* node) {
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

	VarNode* variable = new VarNode();
	if (GETTOKEN(0) == CST::TK_KW_CONST) {
		variable->modifiers &= (int)CSM::MOD_CONST;
		INCPOS(1);
	}

	variable->type = parse_type();

	if (GETTOKEN(0) == CST::TK_IDENTIFIER) {
		variable->name = tokens[pos].data;
		INCPOS(1);
	}
	else {
		delete variable;
		delete node;
		return nullptr;
	}

	skip_until_token(CST::TK_SEMICOLON);

	return node;

}

CSharpParser::StructNode* CSharpParser::parse_struct() {
	// todo
	return nullptr;
}

CSharpParser::InterfaceNode* CSharpParser::parse_interface() {
	return nullptr; // todo
}


CSharpParser::JumpNode* CSharpParser::parse_jump() {
	return nullptr; // todo
}

CSharpParser::TryNode* CSharpParser::parse_try() {
	return nullptr; // todo
}

CSharpParser::UsingNode* CSharpParser::parse_using() {
	return nullptr; // todo
}

CSharpParser::StatementNode* CSharpParser::parse_statement() {
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
		}
		case CST::TK_OP_EQUAL: { // argumenty donyslne
			// TODO skip untill , or until ) or parse expression ????
		}
		default: {

			string type = parse_type();
			if (GETTOKEN(0) != CST::TK_IDENTIFIER) {
				// todo error
			}
			string name = tokens[pos].data;

			argument->type = type;
			argument->name = name;
		}
		}
	}

	// TODO GENERIC METHODS ( : where T is ... )

	// BODY
	if (GETTOKEN(0) != CST::TK_CURLY_BRACKET_OPEN) {
		// todo error
	}

	node->body = parse_block();

	return node;
}

CSharpParser::BlockNode* CSharpParser::parse_block() {

	if (GETTOKEN(0) != CST::TK_CURLY_BRACKET_OPEN) {
		// todo error
	}

	INCPOS(1);

	BlockNode* node = new BlockNode();
	while (GETTOKEN(0) != CST::TK_CURLY_BRACKET_CLOSE) {
		StatementNode* statement = parse_statement();
		// TODO dodaj na koniec listy statementow
	}

	return node;

}


void CSharpParser::ClassNode::print(int indent) {

	indentation(indent);
	cout << "CLASS " << name << ":" << endl;

	// HEADERS:
	if (base_class != nullptr) {
		indentation(indent + TAB);
		cout << "base class: " << base_class->name << endl;
	}

	if (impl_interfaces.size() > 0)		print_header(indent + TAB, impl_interfaces, "impl-interfaces:");
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
	if (base_struct != nullptr) {
		indentation(indent + TAB);
		cout << "base struct: " << base_struct->name << endl;
	}

	if (impl_interfaces.size() > 0)		print_header(indent + TAB, impl_interfaces, "impl-interfaces:");
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
