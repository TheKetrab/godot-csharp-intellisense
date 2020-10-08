#include "csharp_parser.h"


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

void CSharpParser::apply_attributes(Node *node){

	node->attributes = this->attributes; // copy
}

// read file = read namespace (even 'no-namespace')
CSharpParser::NamespaceNode* CSharpParser::parse_namespace() {

	NamespaceNode *node = new NamespaceNode();

	while (current->type != CSharpLexer::TK_EOF) {

		parse_attributes();
		apply_attributes(node);

		switch (GETTOKEN(0)) {

			case CSharpLexer::TK_KW_USING: {
				INCPOS(1);
				parse_using_directives();
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
				NamespaceNode *namespace_node = parse_namespace();
				node->napespaces.push_back(namespace_node);
				break;
			}
			case CSharpLexer::TK_KW_CLASS:		{
				ClassNode *class_node = parse_class();
				node->classes.push_back(class_node);
				break;
			}
			case CSharpLexer::TK_KW_STRUCT:		{
				StructNode* struct_node = parse_struct();
				node->structures.push_back(struct_node);
				break;
			}
			case CSharpLexer::TK_KW_INTERFACE:  { parse_interface();	INCPOS(1);	break;	}
			case CSharpLexer::TK_KW_ENUM:		{ parse_enum();			INCPOS(1);	break;	}
			case CSharpLexer::TK_KW_DELEGATE: {
				// TODO parse delegate
				break;
			}
			case CSharpLexer::TK_SEMICOLON: {
				// TODO ??>
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

			case CSharpLexer::TK_EOF: {
				// TODO unexpected eof??????
				break;
			}

									default: {
										// TODO unexpected token error
										return;
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

	ClassNode *node = new ClassNode(); // todo current
	node->modifiers = this->modifiers;
	node->is_partial = node->modifiers & MOD_PARTIAL;
	node->is_static = node->modifiers & MOD_STATIC;

	CSharpLexer::TokenData td = tokens[pos];
	if (td.type != CSharpLexer::TK_IDENTIFIER) {
		// TODO error unexpected token
	} else {
		node->name = td.data;
	}

	// parse generic params, base class and interfaces <-- todo zrobic z tego funkcje
	parse_possible_type_parameter_node(node); // wstrzykuje do klasy info z tego co jest za nazwa klasy

	while (GETTOKEN(0) != CSharpLexer::TK_CURLY_BRACKET_CLOSE) {
		parse_class_member(node);
	}

	return node;
}

CSharpParser::EnumNode *CSharpParser::parse_enum() {

	EnumNode *node = new EnumNode();
	apply_attributes(node);
	apply_modifiers(node);

	// enum name
	if (GETTOKEN(0) != CSharpLexer::TK_IDENTIFIER) {
		// todo error???
	} else {
		node->name = std::string(tokens[pos].data.ascii().get_data());
		INCPOS(1);
	}

	// enum type
	if (GETTOKEN(0) == CSharpLexer::TK_COLON) {
		node->type = parse_type();
		// TODO read type (typ moze sie skladac z kilku tokenow (np generycznie cos albo z [])
		node->type = "int"; // TODO !!!
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

void CSharpParser::parse_class_member(ClassNode* node) {

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
			if (member != nullptr)
				; // TODO !!!
		}

		// TODO parsowanie metod, pol, properties
										
	}

}

void CSharpParser::parse_enum_member(EnumNode* node) {
	// todo
}



CSharpParser::NamespaceNode::NamespaceNode() {
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

	string type = parse_type();


	node->

}
