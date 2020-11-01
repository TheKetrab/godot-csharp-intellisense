#ifndef CSHARP_PARSER_H
#define CSHARP_PARSER_H


#include "csharp_lexer.h"
#include <string>
#include <vector>
#include <unordered_map>

// klasa nie ma parsowac i wykonywac, tylko okreslic kontekst
// czyli wystarczy, ze zbudujemy taka strukture listowo-drzewiasta i okreslimy co widac
// parser przechowuje sparsowane przez siebie pliki w formie FileNode

// TODO zapamietaj glebokosc!

// Node
//    NamespaceNode
//       FileNode
//    EnumNode
//    GenericNode (abstract)
//       InterfaceNode
//       StructNode
//          ClassNode
//       MethodNode
//       DelegateNode
//    VarNode
//       PropertyNode
//	  StatementNode
//       ExpressionNode (assignment, method invocation, object creation);
//       ConditionNode (if, else, switch, case)
//	     LoopNode (while, do while, for, foreach)
//	     DeclarationNode
//	     JumpNode (break, continue, switch, goto, return, yeild)
//	     TryNode (try-catch-finally)
//	     UsingNode

// todo: File powinien miec vector<string> labels - zeby w razie goto intellisense podpowiadalo widoczne etykiety
using TD = CSharpLexer::TokenData;
using namespace std;
#include <map>
#include <iostream>
#include <unordered_map>
#include <exception>
const int TAB = 2;

struct CSharpParserException : public exception {
	string msg;
	CSharpParserException(string msg) : msg(msg) {}
	const char* what() const throw () {
		return msg.c_str();
	}
};


class CSharpParser {

public:
	struct Node;
	struct FileNode;
	struct NamespaceNode;
	struct EnumNode;
	struct GenericNode;
	struct InterfaceNode;
	struct StructNode;
	struct ClassNode;
	struct MethodNode;
	struct DelegateNode;
	struct VarNode;
	struct PropertyNode;
	struct StatementNode;
	struct ExpressionNode;
	struct ConditionNode;
	struct LoopNode;
	struct DeclarationNode;
	struct JumpNode;
	struct TryNode;
	struct UsingNode;

	// co my chcemy uzyskac
	enum class CompletionType {
		COMPLETION_NONE,            // niewiadomo co
		COMPLETION_TYPE,            // przy deklarowaniu czegos np w klasie
		COMPLETION_MEMBER,          // po kropce
		COMPLETION_CALL_ARGUMENTS,  // w funkcji -> trzeba sprawdzic typ
		COMPLETION_LABEL,           // po dwukropku w jump (goto: )
		COMPLETION_VIRTUAL_FUNC,    // przy napisaniu override
		COMPLETION_ASSIGN,          // po '='
		COMPLETION_IDENTIFIER       // trzeba dokonczyc jakies slowo
	};

	// abstract
	struct Node {

		enum class Type {
			UNKNOWN, FILE, NAMESPACE, ENUM, INTERFACE, STRUCT, CLASS, METHOD, PROPERTY, VAR, STATEMENT, LAMBDA
		};

		int modifiers = 0;
		vector<string> attributes;
		string name;
		string fullname; // eg. Namespace1.Namespace2.ClassX.MethodY
		Type node_type;
		CSharpLexer::TokenData creator;

		Node* parent = nullptr;

		Node() {}
		Node(Type t, TD td) {
			this->creator = td;
			this->node_type = t;
		}

		virtual void print(int indent = 0) const = 0;
		virtual vector<NamespaceNode*> get_namespaces();
		virtual vector<ClassNode*> get_classes();
		virtual vector<MethodNode*> get_methods();
		virtual vector<VarNode*> get_vars(); // vars and properties

		template <class T>
		void print_header(int indent, const vector<T*>& v, string title) const {

			indentation(indent);
			cout << title;
			for (T* t : v)
				cout << " " << t->name;
			cout << endl;
		}

		void print_header(int indent, const vector<string> &v, string title) const {
			
			indentation(indent);
			cout << title;
			for (string x : v)
				cout << " " << x;
			cout << endl;
		}

	};

	// w namespace'ie sa inne namespace'y i klasy
	// using directives refers ONLY to current namespace or global(aka filenode)
	// N1 { using System; } ... N1 { SYSTEM NOT VISIBLE !!! }
	struct NamespaceNode : public Node {

		vector<NamespaceNode*> namespaces;
		vector<ClassNode*> classes;
		vector<StructNode*> structures;
		vector<InterfaceNode*> interfaces;
		vector<EnumNode*> enums;
		vector<DelegateNode*> delegates;

		NamespaceNode(TD td) : Node(Type::NAMESPACE,td) {}
		void print(int indent = 0) const override;
	};

	// filenode jest jak namespace global, tylko ze ma dodatkowe funkcjonalnosci (to co widac TYLKO w tym pliku)
	struct FileNode : public NamespaceNode {

		// dzieki temu mozna szybko dostac po nazwie (fullname) odpowiedni node
		map<string, Node*> node_shortcuts;
		set<string> identifiers;

		vector<string> using_directives;
		vector<string> using_static_direcvites;
		vector<string> directives;
		vector<string> labels;

		set<string> get_external_identifiers();

		FileNode() : NamespaceNode(CSharpLexer::TokenData()) { node_type = Type::FILE; }
		void print(int indent = 0) const override;
	};

	struct EnumNode : public Node {

		string type = "int"; // domyslnie int
		vector<string> members;

		EnumNode(TD td) : Node(Type::ENUM, td) {}
		void print(int indent = 0) const override;
	};

	// abstract
	struct GenericNode : public Node {

		bool is_generic = false;
		vector<string> generic_declarations;
		string constraints;
		GenericNode(Type t, TD td) : Node(t,td) {}
	};

	struct InterfaceNode : public GenericNode {

		vector<string> base_types; // base interfaces
		vector<MethodNode*> methods;
		vector<PropertyNode*> properties;

		InterfaceNode(TD td) : GenericNode(Type::INTERFACE,td) {}
		void print(int indent = 0) const override;
	};

	struct StructNode : public GenericNode {

		vector<string> base_types; // base class and interfaces
		vector<VarNode*> variables;
		vector<MethodNode*> methods;
		vector<ClassNode*> classes;
		vector<InterfaceNode*> interfaces;
		vector<StructNode*> structures;
		vector<EnumNode*> enums;
		vector<PropertyNode*> properties;
		vector<DelegateNode*> delegates;

		StructNode(TD td) : GenericNode(Type::STRUCT,td) {}
		void print(int indent = 0) const override;
	};

	struct ClassNode : public StructNode {

		ClassNode(TD td) : StructNode(td) { node_type = Type::CLASS; }
		void print(int indent = 0) const override;
	};

	struct MethodNode : public GenericNode {

		string return_type;
		vector<VarNode*> arguments;
		StatementNode* body = nullptr;

		MethodNode(TD td) : GenericNode(Type::METHOD,td) {}
		void print(int indent = 0) const override;
	};

	struct DelegateNode : public GenericNode {
		// todo
	};

	struct VarNode : public Node {

		string type;
		string value;

		void print(int indent = 0) const override;

		VarNode(TD td) : Node(Type::VAR,td) { }
		VarNode(string name, string type, TD td)
			: Node(Type::VAR,td)
		{
			this->name = name;
			this->type = type;
		}
	};

	struct PropertyNode : public VarNode {

		int get_modifiers = 0;
		int set_modifiers = 0;
		StatementNode* get_statement = nullptr;
		StatementNode* set_statement = nullptr;

		PropertyNode(TD td) : VarNode(td) { node_type = Type::PROPERTY; }
		void print(int indent = 0) const override;
	};

	// ----- ----- ----- ----- -----
	//		STATEMENT NODE
	// ----- ----- ----- ----- -----
	struct StatementNode : public Node {
		// TODO parse label statement: identifier ":" statement (dla goto, etykiety)
		string raw;
		StatementNode* prev = nullptr; // polaczone w liste, zeby mozna bylo przejsc do tylu i poszukac deklaracji

		StatementNode(TD td) : Node(Type::STATEMENT,td) {}
		void print(int indent = 0) const override;
	};

	struct ExpressionNode : public StatementNode {

		ExpressionNode(TD td) : StatementNode(td) {}
		string expression;
	};

	struct BlockNode : public StatementNode {
		BlockNode(TD td) : StatementNode(td) {}
		StatementNode* last_node = nullptr;
	};

	struct ConditionNode : public StatementNode {
		ConditionNode(TD td) : StatementNode(td) {}
	};

	struct LoopNode : public StatementNode {

		enum class Type {
			UNKNOWN, DO, FOR, FOREACH, WHILE
		};

		LoopNode(TD td) : StatementNode(td) {}
		LoopNode::Type loop_type = Type::UNKNOWN;
		VarNode* local_variable = nullptr;
		StatementNode* body = nullptr;
	};

	struct DeclarationNode : public StatementNode {

		DeclarationNode(TD td) : StatementNode(td) {}
		VarNode* variable;
	};

	struct JumpNode : public StatementNode {

		enum class Type {
			UNKNOWN, BREAK, CONTINUE, GOTO, RETURN, YIELD, THROW
		};

		JumpNode(TD td) : StatementNode(td) {}
		JumpNode::Type jump_type = Type::UNKNOWN;
	};

	struct TryNode : public StatementNode {

		TryNode(TD td) : StatementNode(td) {}
		vector<StatementNode*> blocks; // try, catch blocks and finally
	};

	struct UsingNode : public StatementNode {

		UsingNode(TD td) : StatementNode(td) {}
		VarNode* local_variable = nullptr;
		StatementNode* body = nullptr;
	};
	// ----- ----- ----- ----- -----
public:
	enum class Modifier {
		MOD_PUBLIC = 1,
		MOD_PROTECTED = 1 << 1,
		MOD_PRIVATE = 1 << 2,
		MOD_INTERNAL = 1 << 3,
		MOD_EXTERN = 1 << 4,
		MOD_ABSTRACT = 1 << 5,
		MOD_CONST = 1 << 6,
		MOD_OVERRIDE = 1 << 7,
		MOD_PARTIAL = 1 << 8,
		MOD_READONLY = 1 << 9,
		MOD_SEALED = 1 << 10,
		MOD_STATIC = 1 << 11,
		MOD_UNSAFE = 1 << 12,
		MOD_VIRTUAL = 1 << 13,
		MOD_VOLATILE = 1 << 14,
		MOD_ASYNC = 1 << 15,
		MOD_REF = 1 << 16,       //
		MOD_IN = 1 << 17,        // for variables
		MOD_OUT = 1 << 18,       //
		MOD_PARAMS = 1 << 19     //
	};


	// ----- ----- -----
	// CLASS MEMBERS
private:

	FileNode* root;

	// where is the parser now
	NamespaceNode* cur_namespace;
	ClassNode* cur_class;
	MethodNode* cur_method;
	BlockNode* cur_block;
	string cur_expression;

	Node* current;				// dla parsera (to gdzie jest podczas parsowania)
	Node* cursor;				// wezel w ktorym obecnie jestesmy, trzeba okreslic kontekst

	int cursor_column = 0;
	int cursor_line = 0;

	int pos;                    // position of current token
	int len;                    // total amount of tokens
	int modifiers;              // state of flags of modifiers

	vector<CSharpLexer::TokenData> tokens; // tokens of being parsed file
	vector<string> attributes;
	set<string> identifiers;

	string actual_fullname;

	bool kw_value_allowed = false; // enable only when parse property->set

	// completion info
	string completion_info_str;
	int completion_info_int = 0;
	CompletionType completion_type;
	NamespaceNode *completion_namespace;
	ClassNode *completion_class;
	MethodNode *completion_method;
	BlockNode *completion_block;
	string completion_expression;

	// ----- ----- -----
	// CLASS METHODS
public:
	CSharpParser(string code);
	~CSharpParser();
	FileNode* parse();
	void clear_state(); // set state to zero

	void error(string msg) const;
	void debug_info() const;

	
	static void indentation(int n);
	static map<CSharpLexer::Token, Modifier> to_modifier;

private:
	
	void _unexpeced_token_error() const;

	bool _is_actual_token(CSharpLexer::Token tk, bool assert = false);
	void _assert(CSharpLexer::Token tk); // make sure what is curtok

	NamespaceNode* _parse_namespace(bool global);
	FileNode* _parse_file();
	ClassNode* _parse_class();
	StructNode* _parse_struct();
	InterfaceNode* _parse_interface();
	EnumNode* _parse_enum();
	LoopNode* _parse_loop();
	VarNode* _parse_declaration();
	JumpNode* _parse_jump();
	UsingNode* _parse_using_statement();
	StatementNode* _parse_statement();
	string _parse_expression();
	MethodNode* _parse_method_declaration(string name, string return_type, bool interface_context = false);
	BlockNode* _parse_block();
	ConditionNode* _parse_if_statement();
	TryNode* _parse_try_statement();
	ConditionNode* _parse_switch_statement();
	PropertyNode* _parse_property(string name, string type);
	StatementNode* _parse_property_definition();
	DelegateNode* _parse_delegate();
	string _parse_type(bool array_constructor = false);
	string _parse_new();
	string _parse_initialization_block();
	string _parse_method_invocation();
	string _parse_constraints();
	vector<string> _parse_generic_declaration(); // parse <T,U,...>
	vector<string> _parse_derived_and_implements(bool generic_context = false); // parse : C1, I1, I2, ...

	bool _parse_using_directive(FileNode* node);
	bool _parse_class_member(ClassNode* node);
	bool _parse_namespace_member(NamespaceNode* node);
	bool _parse_interface_member(InterfaceNode* node);

	void _parse_modifiers();
	void _parse_attributes();
	void _apply_modifiers(Node* node);
	void _apply_attributes(Node* node);
	
	CompletionType _deduce_completion_type();
	string _deduce_owner_type(int from_pos);
	int get_position_of_begining(int cur_pos);


	// skipuje az do danego tokena, ktory jest na takim poziomie zaglebienia parsera w blokach (depth)
	void _skip_until_token(CSharpLexer::Token tk);

public:
	
	// TODO future
	// global -> jak parser.depth == node.depth, to wezel przeczytany
	struct Depth {
		int curly_bracket_depth = 0;
		int bracket_depth = 0;
		int parenthesis_depth = 0;

		bool operator==(const struct Depth& d) {
			return curly_bracket_depth == d.curly_bracket_depth
				&& bracket_depth == d.bracket_depth
				&& parenthesis_depth == d.parenthesis_depth;
		}
	} depth;


	static string completion_type_name(CompletionType type);
	friend class CSharpContext;

	void _found_cursor();

	void _escape();
	void _skip_until_end_of_block();
	void _skip_until_end_of_class();
	void _skip_until_end_of_namespace();
	void _skip_until_next_line();



};




#endif // CSHARP_PARSER_H
