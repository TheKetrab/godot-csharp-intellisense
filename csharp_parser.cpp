#include "csharp_parser.h"
#include <iostream>

void CSharpParser::indentation(int n) {
	cout << string(' ', n);
}


std::map<CSharpLexer::Token, CSharpParser::Modifier> CSharpParser::to_modifier = {
	{ CSharpLexer::TK_KW_PUBLIC,		MOD_PUBLIC },
	{ CSharpLexer::TK_KW_PROTECTED,		MOD_PROTECTED },
	{ CSharpLexer::TK_KW_PRIVATE,		MOD_PRIVATE },
	{ CSharpLexer::TK_KW_INTERNAL,		MOD_INTERNAL },
	{ CSharpLexer::TK_KW_EXTERN,		MOD_EXTERN },
	{ CSharpLexer::TK_KW_ABSTRACT,		MOD_ABSTRACT },
	{ CSharpLexer::TK_KW_CONST,			MOD_CONST },
	{ CSharpLexer::TK_KW_OVERRIDE,		MOD_OVERRIDE },
	{ CSharpLexer::TK_KW_PARTIAL,		MOD_PARTIAL },
	{ CSharpLexer::TK_KW_READONLY,		MOD_READONLY },
	{ CSharpLexer::TK_KW_SEALED,		MOD_SEALED },
	{ CSharpLexer::TK_KW_STATIC,		MOD_STATIC },
	{ CSharpLexer::TK_KW_UNSAFE,		MOD_UNSAFE },
	{ CSharpLexer::TK_KW_VIRTUAL,		MOD_VIRTUAL },
	{ CSharpLexer::TK_KW_VOLATILE,		MOD_VOLATILE }
};

CSharpParser::CSharpParser(List<CSharpLexer::TokenData> &tokens) {
	this->tokens = tokens; // copy
}

CSharpParser::CSharpParser(String code) {

	CSharpLexer lexer;
	lexer.set_code(code);
	lexer.tokenize();

	tokens = lexer.tokens; // copy
}

CSharpParser::CSharpParser() {
	// todo
}

void CSharpParser::set_tokens(List<CSharpLexer::TokenData> &tokens) {

	this->tokens = tokens;
	this->len = tokens.size();

}

// parse all tokens (whole file)
void CSharpParser::parse() {

	cout << "PARSER: parse" << endl;
	clear();
	root = parse_namespace(true);

	cout << "----------------------DONE" << endl;



}

void CSharpParser::clear() {

	cout << "PARSER: clear" << endl;

	cursor = nullptr;
	current = nullptr;
	root = nullptr;
	pos = 0;
	modifiers = 0;

}

#define GETTOKEN(ofs) ((ofs + pos) >= len ? CSharpLexer::TK_ERROR : tokens[ofs + pos].type)
#define INCPOS(ammount) { pos += ammount; }


void CSharpParser::parse_modifiers() {

	this->modifiers = 0;
	while (true) {

		switch (tokens[pos].type) {

			case CSharpLexer::TK_KW_NEW:
			case CSharpLexer::TK_KW_PUBLIC:
			case CSharpLexer::TK_KW_PROTECTED:
			case CSharpLexer::TK_KW_INTERNAL:
			case CSharpLexer::TK_KW_PRIVATE:
			case CSharpLexer::TK_KW_ABSTRACT:
			case CSharpLexer::TK_KW_SEALED:
			case CSharpLexer::TK_KW_STATIC:
			case CSharpLexer::TK_KW_UNSAFE: {
				pos++;
				modifiers |= to_modifier[tokens[pos].type];
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
			case CSharpLexer::TK_EOF: {
				// todo end
			}
			case CSharpLexer::TK_ERROR: {
				// todo error
			}
			case CSharpLexer::TK_CURSOR: {
				// todo ??
			}
			case CSharpLexer::TK_BRACKET_OPEN: {

				if (bracket_depth == 0) {
					attribute = "[ ";
				}

				bracket_depth++;
				INCPOS(1);
			}
			case CSharpLexer::TK_BRACKET_CLOSE: {
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
				else attribute += std::string(tokens[pos].data.ascii().get_data()) + " ";
				INCPOS(1);
			}
		}
	}

}

void CSharpParser::apply_attributes(CSharpParser::Node *node) {
	node->attributes = this->attributes; // copy
}

void CSharpParser::apply_modifiers(CSharpParser::Node* node) {
	node->modifiers = this->modifiers;
}

// read file = read namespace (even 'no-namespace')
// global -> for file parsing (no name parsing and keyword namespace)
CSharpParser::NamespaceNode* CSharpParser::parse_namespace(bool global=false) {

	cout << "PARSER: parse_namespace" << endl;

	string name = "global";
	if (!global) {
		// is namespace?
		if (GETTOKEN(0) != CSharpLexer::TK_KW_NAMESPACE) return nullptr;
		else INCPOS(1);

		// read name
		if (GETTOKEN(0) != CSharpLexer::TK_IDENTIFIER) return nullptr;
		else {
			name = tokens[pos].data.ascii().get_data();
			INCPOS(1);
		}
	}

	NamespaceNode *node = new NamespaceNode();
	node->name = name;

	// curly bracket open
	if (GETTOKEN(0) != CSharpLexer::TK_CURLY_BRACKET_OPEN) ; // todo error end destruct node
	else INCPOS(1);

	while (true) {

		parse_attributes();
		apply_attributes(node);

		cout << "actual token is: " << GETTOKEN(0) << tokens[pos].data.ascii().get_data() << endl;
		switch (GETTOKEN(0)) {

			// check exceptions
			case CSharpLexer::TK_EMPTY:
			case CSharpLexer::TK_EOF: {
				return node;
			}
			case CSharpLexer::TK_ERROR: {
				INCPOS(1); // skip
			}
			case CSharpLexer::TK_CURSOR: {
				cursor = node;
				INCPOS(1);
			}
			// -------------------------

			// using directives
			case CSharpLexer::TK_KW_USING: {
				parse_using_directives(node);
				break;
			}

			// all possible modifiers outside anything
			case CSharpLexer::TK_KW_NEW:
			case CSharpLexer::TK_KW_PUBLIC:
			case CSharpLexer::TK_KW_PROTECTED:
			case CSharpLexer::TK_KW_INTERNAL:
			case CSharpLexer::TK_KW_PRIVATE:
			case CSharpLexer::TK_KW_ABSTRACT:
			case CSharpLexer::TK_KW_SEALED:
			case CSharpLexer::TK_KW_STATIC:
			case CSharpLexer::TK_KW_UNSAFE: {
				INCPOS(1);
				modifiers |= to_modifier[GETTOKEN(0)];
				break;
			}
			case CSharpLexer::TK_KW_NAMESPACE: {
				NamespaceNode *member = parse_namespace();
				if (member != nullptr) {
					node->napespaces.push_back(member);
					member->parent = node;
				} break;
			}
			case CSharpLexer::TK_KW_CLASS: {
				ClassNode *member = parse_class();
				if (member != nullptr) {
					node->classes.push_back(member);
					member->parent = node;
				} break;
			}
			case CSharpLexer::TK_KW_STRUCT: {
				StructNode *member = parse_struct();
				if (member != nullptr) {
					node->structures.push_back(member);
					member->parent = node;
				} break;
			}
			case CSharpLexer::TK_KW_INTERFACE: {
				InterfaceNode *member = parse_interface();
				if (member != nullptr) {
					node->interfaces.push_back(member);
					member->parent = node;
				} break;
			}
			case CSharpLexer::TK_KW_ENUM: {
				EnumNode *member = parse_enum();
				if (member != nullptr) {
					node->enums.push_back(member);
					member->parent = node;
				} break;
			}
			case CSharpLexer::TK_KW_DELEGATE: {
				// TODO parse delegate
				break;
			}
			case CSharpLexer::TK_CURLY_BRACKET_OPEN: {
				this->depth.curly_bracket_depth++;
				break;
			}
			case CSharpLexer::TK_KW_EXTERN: {
				// TODO ??
				break;
			}
			case CSharpLexer::TK_KW_PARTIAL: {
				// TODO ??
				break;
			}

			
		}





	}


}

// after this function pos will be ON the token at the same level (depth)
void CSharpParser::skip_until_token(CSharpLexer::Token tk) {

	CSharpParser::Depth base_depth = this->depth;
	while (true) {

		switch (GETTOKEN(0)) {

			// TODO standard (error, empty, cursor, eof)

			case CSharpLexer::TK_BRACKET_OPEN: { // [
				this->depth.bracket_depth++;
				//if (depth == base_depth) 
				//	;
				break;
			}
			case CSharpLexer::TK_BRACKET_CLOSE: { // ]
			}
			case CSharpLexer::TK_CURLY_BRACKET_OPEN: { // {
			}
			case CSharpLexer::TK_CURLY_BRACKET_CLOSE: { // }
			}
			case CSharpLexer::TK_PARENTHESIS_OPEN: { // (
			}
			case CSharpLexer::TK_PARENTHESIS_CLOSE: { // )
			}
			default: {
				if (GETTOKEN(0) == tk) {

				}
			}
			


		}


	}

}


void CSharpParser::parse_possible_type_parameter_node(Node *node) {

	while (true) {

		switch (GETTOKEN(0)) {
			case CSharpLexer::TK_ERROR: {
				// todo
			}
			case CSharpLexer::TK_EOF: {
				// todo
			}
			case CSharpLexer::TK_BRACKET_CLOSE: {
				// todo -> end parsing
			}
			case CSharpLexer::TK_KW_WHERE: {
				// todo skip untill '{'
			}
			case CSharpLexer::TK_OP_LESS: {
			}
			case CSharpLexer::TK_IDENTIFIER: {
			}
			case CSharpLexer::TK_CURLY_BRACKET_OPEN: {
				// todo finish
			}
		}
	}

}

CSharpParser::ClassNode* CSharpParser::parse_class() {

	cout << "PARSER: parse_class" << endl;

	// is class?
	if (GETTOKEN(0) != CSharpLexer::TK_KW_CLASS) return nullptr;
	INCPOS(1);

	// read name
	if (GETTOKEN(0) != CSharpLexer::TK_IDENTIFIER) return nullptr;

	ClassNode *node = new ClassNode();
	node->name = tokens[pos].data.ascii().get_data();
	INCPOS(1);

	apply_attributes(node);
	apply_modifiers(node);

	// parse generic params, base class and interfaces <-- todo zrobic z tego funkcje
	parse_possible_type_parameter_node(node); // wstrzykuje do klasy info z tego co jest za nazwa klasy

	while (GETTOKEN(0) != CSharpLexer::TK_CURLY_BRACKET_CLOSE) {
		parse_class_member(node);
	}

	return node;
}

CSharpParser::EnumNode *CSharpParser::parse_enum() {

	// is enum?
	if (GETTOKEN(0) != CSharpLexer::TK_KW_ENUM) return nullptr;
	INCPOS(1);

	// read name
	if (GETTOKEN(0) != CSharpLexer::TK_IDENTIFIER) return nullptr;


	EnumNode *node = new EnumNode();
	node->name = tokens[pos].data.ascii().get_data();
	INCPOS(1);

	apply_attributes(node);
	apply_modifiers(node);

	node->type = "int"; // domyslnie

	// enum type
	if (GETTOKEN(0) == CSharpLexer::TK_COLON) {
		INCPOS(1);
		node->type = parse_type();
	}

	// parse members
	while (GETTOKEN(0) != CSharpLexer::TK_CURLY_BRACKET_CLOSE) {
		parse_enum_member(node);
	}

	return node;
}

CSharpParser::LoopNode *CSharpParser::parse_loop() {

	LoopNode *node = new LoopNode();

	// type of loop
	switch (GETTOKEN(0)) {
		case CSharpLexer::TK_KW_DO: node->type = CSharpParser::LoopNode::DO;
		case CSharpLexer::TK_KW_FOR: node->type = CSharpParser::LoopNode::FOR;
		case CSharpLexer::TK_KW_FOREACH: node->type = CSharpParser::LoopNode::FOREACH;
		case CSharpLexer::TK_KW_WHILE: node->type = CSharpParser::LoopNode::WHILE;
		default: { delete node; return nullptr; }
	}

	INCPOS(1);

	// ----- ----- -----
	// PARSE BEFORE BLOCK

	if (node->type == CSharpParser::LoopNode::FOR) {

		// init
		DeclarationNode* declaration = parse_declaration();
		if (declaration == nullptr) {
			skip_until_token(CSharpLexer::TK_SEMICOLON); // assignment or sth else
		} else {
			node->local_variable = declaration->variable;
			delete declaration;
		}

		// cond
		skip_until_token(CSharpLexer::TK_SEMICOLON);

		// iter
		skip_until_token(CSharpLexer::TK_PARENTHESIS_CLOSE);

	}

	else if (node->type == CSharpParser::LoopNode::FOREACH) {

		// declaration
		DeclarationNode *declaration = parse_declaration();
		if (declaration == nullptr) {
			skip_until_token(CSharpLexer::TK_KW_IN);
		} else {
			node->local_variable = declaration->variable;
			delete declaration;
		}

		// in
		skip_until_token(CSharpLexer::TK_PARENTHESIS_CLOSE);

	}

	else if (node->type == CSharpParser::LoopNode::WHILE) {

		// cond
		skip_until_token(CSharpLexer::TK_PARENTHESIS_CLOSE);

	}

	// ----- ----- -----
	// PARSE BLOCK
	node->body = parse_statement();

	// ----- ----- -----
	// PARSE AFTER BLOCK
	if (node->type == CSharpParser::LoopNode::DO) {
		//todo skip while
	}


	return node;
}

CSharpParser::DelegateNode *CSharpParser::parse_delegate() {
	return nullptr;
}

std::string CSharpParser::parse_type() {

	std::string res = "";

	// base type: int/bool/... (nullable ?) [,,...,]
	// complex type: NAMESPACE::NAME1. ... .NAMEn<type1,type2,...>[,,...,]

	// TODO dodac obslugiwanie '::'

	bool base_type = false;
	bool complex_type = false;
	bool generic_types_mode = false;
	bool array_mode = false;

	while (true) {

		switch (GETTOKEN(0)) {

			case CSharpLexer::TK_ERROR: {
				//todo
			}
			case CSharpLexer::TK_EOF: {
				// todo
			}
			case CSharpLexer::TK_CURSOR: {
				// todo
			}

			case CSharpLexer::TK_OP_QUESTION_MARK: {

				// nullable type
				if (base_type) {
					res += "?";
					INCPOS(1);
					break;
				} else {
					// TODO error
				}
			}

			case CSharpLexer::TK_OP_LESS: { // <
				generic_types_mode = true;
				res += "<";
				INCPOS(1);
				res += parse_type(); // recursive
			}

			case CSharpLexer::TK_OP_GREATER: { // >
				generic_types_mode = false;
				res += ">";
				INCPOS(1);

				// this is end unless it is '['
				if (GETTOKEN(0) != CSharpLexer::TK_BRACKET_OPEN) {
					return res;
				}
			}

			case CSharpLexer::TK_BRACKET_OPEN: { // [
				array_mode = true;
				res += "[";
				INCPOS(1);
			}
			case CSharpLexer::TK_BRACKET_CLOSE: { // ]
				array_mode = false;
				res += "]";
				INCPOS(1);

				// for sure this is end of type
				return res;
			}

			case CSharpLexer::TK_COMMA: { // ,
				if (generic_types_mode) {
					res += ",";
					INCPOS(1);
					res += parse_type();
				} else if (array_mode) {
					res += ",";
					INCPOS(1);
				} else {
					// TODO error
				}
			}

			// base type?
			case CSharpLexer::TK_KW_BOOL:
			case CSharpLexer::TK_KW_BYTE:
			case CSharpLexer::TK_KW_CHAR:
			case CSharpLexer::TK_KW_DECIMAL:
			case CSharpLexer::TK_KW_DOUBLE:
			case CSharpLexer::TK_KW_FLOAT:
			case CSharpLexer::TK_KW_LONG:
			case CSharpLexer::TK_KW_OBJECT:
			case CSharpLexer::TK_KW_SBYTE:
			case CSharpLexer::TK_KW_SHORT:
			case CSharpLexer::TK_KW_STRING:
			case CSharpLexer::TK_KW_UINT:
			case CSharpLexer::TK_KW_ULONG:
			case CSharpLexer::TK_KW_USHORT:
			case CSharpLexer::TK_KW_VOID: {

				if (complex_type) {
					// TODO error -> System.Reflexion.int ??????
					// vector<int> still allowed, cause int will be parser recursively
				}

				base_type = true;
				res += CSharpLexer::token_names[GETTOKEN(0)];
				INCPOS(1);

				// another has to be '?' either '[' - else it is finish
				if (GETTOKEN(0) != CSharpLexer::TK_OP_QUESTION_MARK
					|| GETTOKEN(0) != CSharpLexer::TK_BRACKET_OPEN)
				{
					return res; // end
				}
			}

			// complex type
			case CSharpLexer::TK_IDENTIFIER: {

				if (base_type) {
					// TODO error
				}

				complex_type = true;
				res += tokens[pos].data.ascii().get_data();
				INCPOS(1);

				// finish if
				if (GETTOKEN(0) != CSharpLexer::TK_PERIOD				// .
					|| GETTOKEN(0) != CSharpLexer::TK_OP_LESS			// <
					|| GETTOKEN(0) != CSharpLexer::TK_BRACKET_OPEN) {	// [
					return res;
				}
			}
		}
	}

	return res;
}

void CSharpParser::parse_using_directives(NamespaceNode *node) {

	cout << "parse using directives" << endl;


	// TODO nieskonczone

	// using X;
	// using static X;
	// using X = Y;

	// is using?
	if (GETTOKEN(0) != CSharpLexer::TK_KW_USING) return;

	INCPOS(1);

	// read name
	if (GETTOKEN(0) != CSharpLexer::TK_IDENTIFIER) return;

	string name = tokens[pos].data.ascii().get_data();
	node->using_directives.push_back(name);
	INCPOS(1);

	// ;
	INCPOS(1);


}

void CSharpParser::parse_class_member(ClassNode *node) {

	parse_attributes();
	parse_modifiers();

	switch (GETTOKEN(0)) {
		case CSharpLexer::TK_KW_CLASS: {
			ClassNode* member = parse_class();
			if (member != nullptr) node->classes.push_back(member);
		}
		case CSharpLexer::TK_KW_INTERFACE: {
			InterfaceNode* member = parse_interface();
			if (member != nullptr) node->interfaces.push_back(member);
		}
		case CSharpLexer::TK_KW_STRUCT: {
			StructNode* member = parse_struct();
			if (member != nullptr) node->structures.push_back(member);
		}
		case CSharpLexer::TK_KW_ENUM: {
			EnumNode* member =  parse_enum();
			if (member != nullptr) node->enums.push_back(member);
		}
		case CSharpLexer::TK_KW_DELEGATE: {
			DelegateNode* member = parse_delegate();
			if (member != nullptr) {
				// TODO !!!
			}
				
		}

		// TODO parsowanie metod, pol, properties
										
	}

}

void CSharpParser::parse_interface_member(InterfaceNode *node) {
	// todo
}

void CSharpParser::parse_enum_member(EnumNode* node) {
	// todo
}



CSharpParser::NamespaceNode::NamespaceNode() {
}

void CSharpParser::NamespaceNode::print(int indent) {
}


CSharpParser::DeclarationNode* CSharpParser::parse_declaration() {

	DeclarationNode *node = new DeclarationNode();

	// type name;
	// type name = x;
	// const type name = x;

	VarInfo variable;
	if (GETTOKEN(0) == CSharpLexer::TK_KW_CONST) {
		variable.is_const = true;
		INCPOS(1);
	}

	variable.type = parse_type();

	if (GETTOKEN(0) == CSharpLexer::TK_IDENTIFIER) {
		variable.name = tokens[pos].data.ascii().get_data();
		INCPOS(1);
	} else {
		delete node;
		return nullptr;
	}

	skip_until_token(CSharpLexer::TK_SEMICOLON);	

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

CSharpParser::TryNode *CSharpParser::parse_try() {
	return nullptr; // todo
}

CSharpParser::UsingNode* CSharpParser::parse_using() {
	return nullptr; // todo
}

CSharpParser::StatementNode *CSharpParser::parse_statement() {
	return nullptr;
}


void CSharpParser::ClassNode::print(int indent) {

	indentation(indent);
	cout << "CLASS " << name << ":" << endl;

	// HEADERS:
	if (base_class != nullptr) {
		indentation(indent + TAB);
		cout << "base class: " << base_class->name << endl;
	}

	if (impl_interfaces.size() > 0)		print_header(indent + TAB, impl_interfaces,	"implemented interfaces:"	);
	if (interfaces.size() > 0)			print_header(indent + TAB, interfaces,		"interfaces:"				);
	if (classes.size() > 0)				print_header(indent + TAB, classes,			"classes:"					);
	if (structures.size() > 0)			print_header(indent + TAB, structures,		"structures:"				);
	if (enums.size() > 0)				print_header(indent + TAB, enums,			"enums:"					);
	if (methods.size() > 0)				print_header(indent + TAB, methods,			"methods:"					);
	if (properties.size() > 0)			print_header(indent + TAB, properties,		"properties:"				);
	if (variables.size() > 0)		 print_variables(indent + TAB, variables,		"variables:"				);

	// NODES:
	if (interfaces.size() > 0)	for (InterfaceNode *x : interfaces)	x->print(indent + TAB);
	if (classes.size() > 0)		for (ClassNode *x : classes)		x->print(indent + TAB);
	if (structures.size() > 0)	for (StructNode *x : structures)	x->print(indent + TAB);
}

void CSharpParser::StructNode::print(int indent) {

	indentation(indent);
	cout << "STRUCT " << name << ":" << endl;

	// HEADERS:
	if (base_struct != nullptr) {
		indentation(indent + TAB);
		cout << "base struct: " << base_struct->name << endl;
	}

	if (impl_interfaces.size() > 0)		print_header(indent + TAB, impl_interfaces,	"implemented interfaces:"	);
	if (interfaces.size() > 0)			print_header(indent + TAB, interfaces,		"interfaces:"				);
	if (classes.size() > 0)				print_header(indent + TAB, classes,			"classes:"					);
	if (structures.size() > 0)			print_header(indent + TAB, structures,		"structures:"				);
	if (enums.size() > 0)				print_header(indent + TAB, enums,			"enums:"					);
	if (methods.size() > 0)				print_header(indent + TAB, methods,			"methods:"					);
	if (properties.size() > 0)			print_header(indent + TAB, properties,		"properties:"				);
	if (variables.size() > 0)		 print_variables(indent + TAB, variables,		"variables:"				);

	// NODES:
	if (interfaces.size() > 0)	for (InterfaceNode *x : interfaces)	x->print(indent + TAB);
	if (classes.size() > 0)		for (ClassNode *x : classes)		x->print(indent + TAB);
	if (structures.size() > 0)	for (StructNode *x : structures)	x->print(indent + TAB);
}

void CSharpParser::StatementNode::print(int indent) {

	indentation(indent);
	cout << "STATEMENT" << endl;

}

void CSharpParser::MethodNode::print(int indent) {

	indentation(indent);
	cout << "METHOD " << name << endl;

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
	for (string &member : members)
		cout << member << " ";
	cout << endl;

}

void CSharpParser::PropertyNode::print(int indent) {
	// TODO
}
