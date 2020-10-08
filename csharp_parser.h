#ifndef CSHARP_PARSER_H
#define CSHARP_PARSER_H


#include "csharp_lexer.h"
#include <string>
#include <vector>
// klasa nie ma parsowac i wykonywac, tylko okreslic kontekst
// czyli wystarczy, ze zbudujemy taka strukture listowo-drzewiasta i okreslimy co widac

// dokument to jesen wielki NodeBlock


// jesli bedzie jakis nieobslugiwalny blad to funkcje parse_XXX zwracaja nullptr


// wszystkie zmienne jakie sa widoczne, wszystkie metody - to sobie okresle jak juz sparsuje


// Node
//   NamespaceNode
//	 ClassNode
//	 InterfaceNode
//   EnumNode
//	 StructNode
//	 MethodNode
//   PropertyNode // czy jest setter, getter? settable, gettable?
//	 StatementNode
//     ExpressionNode (assignment, method invocation, object creation);
//     ConditionNode (if, else, switch, case)
//	   LoopNode (while, do while, for, foreach)
//	   DeclarationNode
//	   JumpNode (break, continue, switch, goto, return, yeild)
//	   TryNode (try-catch-finally)
//	   UsingNode

using namespace std;
#include <map>
#include <iostream>
const int TAB = 2;

class CSharpParser {


	
	static void indentation(int n);

	// declarations
	struct Node;
	struct ClassNode;
	struct InterfaceNode;
	struct EnumNode;
	struct StructNode;
	struct MethodNode;
	struct PropertyNode;
	struct StatementNode;
	struct LoopNode;
	struct DeclarationNode;
	struct JumpNode;
	struct TryNode;
	struct UsingNode;

	struct VarInfo; // name and type
	

	// definitions
	struct Node {

		int line;
		int column;

		int modifiers;
		vector<string> attributes;
		
		Node *parent;
		string name;

		Node() {}
		Node(const CSharpLexer::TokenData td) {
			this->line = td.line;
			this->column = td.column;
		}

		virtual void print(int indent) = 0;

		template <class T>
		void print_header(int indent, vector<T *> &v, string title) {

			indentation(indent);
			cout << title << " ";
			for (T *t : v)
				cout << t->name << " ";
			cout << endl;
		}

		void print_variables(int indent, vector<VarInfo> &v, string title) {

			indentation(indent);
			cout << title << " ";
			for (VarInfo vi : v)
				cout << vi.name << " ";
			cout << endl;
		}

	};

	struct VarInfo {
		string name;
		string type;
		bool is_const;
	};


	struct ClassNode : public Node {

		
		ClassNode *base_class;
		vector<InterfaceNode*> impl_interfaces;

		// members:
		vector<VarInfo> variables;
		vector<MethodNode*> methods;
		vector<ClassNode*> classes;
		vector<InterfaceNode*> interfaces;
		vector<StructNode*> structures;
		vector<EnumNode*> enums;
		vector<PropertyNode *> properties;

		void print(int indent) override;

		ClassNode() {}
	};

	struct StructNode : public Node {

		StructNode *base_struct;
		vector<InterfaceNode*> impl_interfaces;

		// members:
		vector<VarInfo> variables;
		vector<VarInfo> static_variables;

		vector<MethodNode*> methods;
		vector<MethodNode*> static_methods;

		vector<ClassNode *> classes;
		vector<InterfaceNode *> interfaces;
		vector<StructNode *> structures;
		vector<EnumNode *> enums;
		vector<PropertyNode *> properties;

		void print(int indent) override;
	};


	// ----- ----- ----- ----- -----
	//		STATEMENT NODE
	// ----- ----- ----- ----- -----
	struct StatementNode : public Node {

		StatementNode *next;
		void print(int indent) override;
	};

	struct BlockNode : public StatementNode {

	};

	struct ConditionNode : public StatementNode {

	};

	struct LoopNode : public StatementNode {

		enum Type {
			DO, FOR, FOREACH, WHILE
		};

		Type type;
		VarInfo local_variable;
		StatementNode *body;


	};

	struct DeclarationNode : public StatementNode {
		VarInfo variable;
	};

	struct JumpNode : public StatementNode {

	};

	struct TryNode : public StatementNode {

	};

	struct UsingNode : public StatementNode {

	};
	// ----- ----- ----- ----- -----


	struct MethodNode : public Node {
		string type;
		vector<VarInfo> arguments;

		void print(int indent) override;
	};

	struct PropertyNode : public Node {
		// todo info about get set

		void print(int indent) override;
	};

	// w namespace'ie sa inne namespace'y i klasy
	// using directives refers ONLY to current namespace or global(aka filenode)
	// N1 { using System; } ... N1 { SYSTEM NOT VISIBLE !!! }
	struct NamespaceNode : public Node {
		vector<NamespaceNode*> napespaces;
		vector<ClassNode*> classes;
		vector<StructNode*> structures;
		vector<InterfaceNode*> interfaces;
		vector<EnumNode*> enums;

		vector<string> using_directives;
		vector<string> using_static_directives;

		NamespaceNode();

		void print(int indent) override;
	};

	struct InterfaceNode : public Node {

		string name;
		vector<InterfaceNode> implemented_interfaces;

		// members:
		vector<MethodNode*> methods;
		vector<PropertyNode*> properties;


		void print(int indent) override;

	};

	struct EnumNode : public Node {

		string type = "int"; // domyslnie int
		vector<string> members;
		EnumNode() {}
		// todo

		void print(int indent) override;
	};

	struct DelegateNode : public Node {
		// todo
	};


	// ----- ----- -----
	// CLASS

	Node *root;		// glowny wezel pliku
	Node *current;  // dla parsera (to gdzie jest podczas parsowania)
	Node *cursor;   // wezel w ktorym obecnie jestesmy, trzeba okreslic kontekst

	int pos;
	int len; // ilosc tokenow

	int modifiers;
	List<CSharpLexer::TokenData> tokens;
	vector<string> attributes;

public:
	// constructors
	CSharpParser(List<CSharpLexer::TokenData> &tokens);
	CSharpParser(String code);
	CSharpParser();

	void set_tokens(List<CSharpLexer::TokenData> &tokens);

	// methods
	void parse(); // parse tokens to tree-structure
	void clear(); // set state to zero

private:

	NamespaceNode *parse_namespace(bool global);
	ClassNode *parse_class();
	StructNode *parse_struct();
	InterfaceNode *parse_interface();
	EnumNode *parse_enum();
	LoopNode *parse_loop();
	DeclarationNode *parse_declaration();
	JumpNode *parse_jump();
	TryNode *parse_try();
	UsingNode *parse_using();
	StatementNode *parse_statement();

	DelegateNode *parse_delegate();
	string parse_type();

	// skipuje az do danego tokena, ktory jest na takim poziomie zaglebienia parsera w blokach (depth)
	void skip_until_token(CSharpLexer::Token tk);
	

	//void parse

	
	

	// for mask
	enum Modifier {
		MOD_PUBLIC			= 1,
		MOD_PROTECTED		= 1 << 1,
		MOD_PRIVATE			= 1 << 2,
		MOD_INTERNAL		= 1 << 3,
		MOD_EXTERN			= 1 << 4,
		MOD_ABSTRACT		= 1 << 5,
		MOD_CONST			= 1 << 6,
		MOD_OVERRIDE		= 1 << 7,
		MOD_PARTIAL			= 1 << 8,
		MOD_READONLY		= 1 << 9,
		MOD_SEALED			= 1 << 10,
		MOD_STATIC			= 1 << 11,
		MOD_UNSAFE			= 1 << 12,
		MOD_VIRTUAL			= 1 << 13,
		MOD_VOLATILE		= 1 << 14
	};

public:
	static map<CSharpLexer::Token, Modifier> to_modifier;

	
	void parse_modifiers();
	void parse_attributes();

	void apply_attributes(Node *node); // to co nazbieral info aplikuje
	void apply_modifiers(Node *node);

	void parse_using_directives(NamespaceNode* node);
	void parse_class_member(ClassNode *node);
	void parse_interface_member(InterfaceNode *node);
	void parse_enum_member(EnumNode *node);


	void parse_possible_type_parameter_node(Node* node);

	// global -> jak parser.depth == node.depth, to wezel przeczytany
	struct Depth {
		int curly_bracket_depth = 0;
		int bracket_depth = 0;
		int parenthesis_depth = 0;

		bool operator==(const struct Depth &d) {
			return curly_bracket_depth == d.curly_bracket_depth
				&& bracket_depth == d.bracket_depth
				&& parenthesis_depth == d.parenthesis_depth;
		}
	} depth;

	
};
	
#endif // CSHARP_PARSER_H
