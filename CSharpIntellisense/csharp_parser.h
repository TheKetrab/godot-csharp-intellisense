#ifndef CSHARP_PARSER_H
#define CSHARP_PARSER_H


#include "csharp_lexer.h"
#include <string>
#include <vector>
#include <unordered_map>



// TODO: jesli znajdziesz kursor to nie parsuj dalej tego bloku - jesli jestes w bloku
// --> wtedy aktualny blok zawiera wszystkie statementy zanim jest nasze wyrazenie
// --> problem: czasem znajdujemy kursor, ale siê wycofujemy (cofiemy pozycjê pos), bo nie uda³o siê czegoœ sparsowaæ wiêc wracamy

using TD = CSharpLexer::TokenData;
using namespace std;
#include <map>
#include <iostream>
#include <unordered_map>
#include <exception>
const int TAB = 2;

class CSharpParser;


struct CSharpParserException : public exception {
	string msg;
	CSharpParserException(string msg) : msg(msg) {}
	const char* what() const throw () {
		return msg.c_str();
	}
};


class CSharpParser {

  public:
	
	// ***************************** ***************************** //
	// ----- ----- ----- ----- ----- ----- ----- ----- ----- ----- //
	//              ===== ===== STRUCTURES ===== =====             //
	// ----- ----- ----- ----- ----- ----- ----- ----- ----- ----- //
	// ***************************** ***************************** //

	// ----- ----- ----- ----- ----- //
	struct Node;
		struct NamespaceNode;
			struct FileNode;
		struct EnumNode;
		struct GenericNode; // abstract
			struct TypeNode; // abstract
				struct InterfaceNode;
				struct StructNode;
					struct ClassNode;
			struct MethodNode;
			struct DelegateNode;
		struct VarNode;
			struct PropertyNode;
		struct StatementNode;
			struct ExpressionNode; // (assignment, method invocation, object creation);
			struct ConditionNode; // (if, else, switch, case)
			struct LoopNode;  // (while, do while, for, foreach)
			struct DeclarationNode;
			struct JumpNode; // (break, continue, switch, goto, return, yeild)
			struct TryNode; //  (try-catch-finally)
			struct UsingNode;
	// ----- ----- ----- ----- ----- //



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

		// fields
		enum class Type {
			UNKNOWN, FILE, NAMESPACE, ENUM, INTERFACE,
			STRUCT, CLASS, METHOD, PROPERTY, VAR, STATEMENT, LAMBDA,
			DECLARATION, BLOCK, LOOP
		} node_type;

		CSharpLexer::TokenData creator;
		Node* parent = nullptr;

		int modifiers = 0;
		vector<string> attributes;
		string name;

		// methods
		Node() = default;
		Node(Type t, TD td);
		virtual ~Node();

		FileNode* get_parent_file();

		virtual void print(int indent = 0) const = 0;
		virtual string fullname() const; // eg. Namespace1.Namespace2.ClassX.MethodY(int,Namespace1.ClassY)
		virtual string prettyname() const; // for printing in IntelliSense result
		virtual list<NamespaceNode*> get_visible_namespaces() const;
		virtual list<TypeNode*> get_visible_types(int visibility) const;
		virtual list<MethodNode*> get_visible_methods(int visibility) const;
		virtual list<VarNode*> get_visible_vars(int visibility) const;
		virtual list<Node*> get_members(const string name, int visibility) const; // name = "" -> all members

		void print_header(int indent, const vector<string> &v, string title) const;

		template <class T>
		void print_header(int indent, const vector<T*>& v, string title) const {
			indentation(indent); cout << title;
			for (T* t : v) cout << " " << t->name;
			cout << endl;
		}

		virtual bool is_public();
		virtual bool is_protected();
		virtual bool is_private();
		virtual bool is_static();

	};

	struct NamespaceNode : public Node {

		vector<NamespaceNode*> namespaces;
		vector<ClassNode*> classes;
		vector<StructNode*> structures;
		vector<InterfaceNode*> interfaces;
		vector<EnumNode*> enums;
		vector<DelegateNode*> delegates;

		NamespaceNode(TD td) : Node(Type::NAMESPACE,td) {}
		virtual ~NamespaceNode();
		void print(int indent = 0) const override;

		virtual list<NamespaceNode*> get_visible_namespaces() const override;
		virtual list<TypeNode*> get_visible_types(int visibility) const override;
		virtual list<Node*> get_members(const string name, int visibility) const override;
	};

	// filenode is like namespace global, but has additional functionality -> sth what is visible only within the file
	struct FileNode : public NamespaceNode {

		// what is inside file?
		map<string, Node*> node_shortcuts;
		set<string> identifiers;

		vector<string> using_directives;
		vector<string> using_static_direcvites;
		vector<string> directives;
		vector<string> labels;

		set<string> get_external_identifiers();

		FileNode() : NamespaceNode(CSharpLexer::TokenData()) { node_type = Type::FILE; }
		virtual ~FileNode();
		void print(int indent = 0) const override;
		virtual string fullname() const override;
	};

	struct EnumNode : public Node {

		string type = "int"; // default
		vector<string> members;

		EnumNode(TD td) : Node(Type::ENUM, td) {}
		virtual ~EnumNode() {}
		void print(int indent = 0) const override;
	};

	// abstract
	struct GenericNode : public Node {

		bool is_generic = false;
		vector<string> generic_declarations;
		string constraints; // for generic constraints: where ... 
		GenericNode() {}
		GenericNode(Type t, TD td) : Node(t,td) {}
		virtual ~GenericNode() {}

	};

	// abstract
	struct TypeNode : public GenericNode {
		int rank() const; // array types have rank(dimension) >= 1
		vector<string> base_types; // base class and interfaces

		TypeNode* create_array_type(int rank) const;

		TypeNode() {}
		TypeNode(Type t, TD td) : GenericNode(t, td) {}
		virtual ~TypeNode() {}
	};

	struct InterfaceNode : public TypeNode {

		vector<MethodNode*> methods;
		vector<PropertyNode*> properties;

		InterfaceNode(TD td) : TypeNode(Type::INTERFACE,td) {}
		virtual ~InterfaceNode();
		void print(int indent = 0) const override;
		
		virtual list<Node*> get_members(const string name, int visibility) const override;
	};

	struct StructNode : public TypeNode {

		vector<VarNode*> variables;
		vector<MethodNode*> methods;
		vector<ClassNode*> classes;
		vector<InterfaceNode*> interfaces;
		vector<StructNode*> structures;
		vector<EnumNode*> enums;
		vector<PropertyNode*> properties;
		vector<DelegateNode*> delegates;

		StructNode() {}
		StructNode(TD td) : TypeNode(Type::STRUCT,td) {}
		virtual ~StructNode();
		void print(int indent = 0) const override;

		virtual list<TypeNode*> get_visible_types(int visibility) const override;
		virtual list<MethodNode*> get_visible_methods(int visibility) const override;
		virtual list<VarNode*> get_visible_vars(int visibility) const override;
		virtual list<Node*> get_members(const string name, int visibility) const override;
		list<MethodNode*> get_constructors(int visibility) const;

		bool any_constructor_declared = false;
		MethodNode* create_auto_generated_constructor();


	};

	struct ClassNode : public StructNode {

		ClassNode() {}
		ClassNode(TD td) : StructNode(td) { node_type = Type::CLASS; }
		virtual ~ClassNode() {}
		void print(int indent = 0) const override;
	};

	struct MethodNode : public GenericNode {

		string return_type;
		vector<VarNode*> arguments;
		StatementNode* body = nullptr;

		MethodNode() {}
		MethodNode(TD td) : GenericNode(Type::METHOD,td) {}
		virtual ~MethodNode();

		void print(int indent = 0) const override;
		virtual string fullname() const;
		virtual string prettyname() const override;
		virtual string get_type() const; // return type with context (resolve)

		// NOTE: this method will be visible because will be visible in current class (parrent)
		virtual list<VarNode*> get_visible_vars(int visibility) const override;
		bool is_constructor();

	};

	struct DelegateNode : public GenericNode {
		// todo
	};

	struct VarNode : public Node {

		mutable string type; // NOTE: type can be changed while resolving 'var' type from the value
		string value; // or bound expression

		void print(int indent = 0) const override;
		virtual string get_type() const; // type with context (resolve var)
		virtual list<Node*> get_members(const string name, int visibility) const override;

		virtual string prettyname() const override;

		VarNode() {}
		VarNode(TD td) : Node(Type::VAR,td) { }
		VarNode(string name, string type, TD td)
			: Node(Type::VAR,td)
		{
			this->name = name;
			this->type = type;
		}
		~VarNode() {}
	};

	struct PropertyNode : public VarNode {

		int get_modifiers = 0;
		int set_modifiers = 0;
		StatementNode* get_statement = nullptr;
		StatementNode* set_statement = nullptr;

		PropertyNode(TD td) : VarNode(td) { node_type = Type::PROPERTY; }
		virtual ~PropertyNode();
		void print(int indent = 0) const override;
	};

	// ----- ----- ----- ----- -----
	//		STATEMENT NODE
	// ----- ----- ----- ----- -----
	struct StatementNode : public Node {

		// TODO parse label statement: identifier ":" statement (goto, labels)
		string raw; // expression that is represented by this statement

		StatementNode(TD td) : Node(Type::STATEMENT,td) {}
		virtual ~StatementNode() {}
		void print(int indent = 0) const override;
	};

	struct ExpressionNode : public StatementNode {

		ExpressionNode(TD td) : StatementNode(td) {}
		virtual ~ExpressionNode() {}
		string expression;
	};

	struct BlockNode : public StatementNode {
		BlockNode(TD td) : StatementNode(td) { node_type = Type::BLOCK; }
		vector<StatementNode*> statements;

		virtual ~BlockNode();
		virtual list<VarNode*> get_visible_vars(int visibility) const override;
	};

	struct ConditionNode : public StatementNode {
		virtual ~ConditionNode() {}
		ConditionNode(TD td) : StatementNode(td) {}
	};

	struct LoopNode : public StatementNode {

		enum class Type {
			UNKNOWN, DO, FOR, FOREACH, WHILE
		};

		LoopNode(TD td) : StatementNode(td) {}
		virtual ~LoopNode() {}
		LoopNode::Type loop_type = Type::UNKNOWN;
		
		virtual list<VarNode*> get_visible_vars(int visibility) const override;
		VarNode* local_variable = nullptr;
		StatementNode* body = nullptr;
	};

	struct DeclarationNode : public StatementNode {

		DeclarationNode(TD td) : StatementNode(td) { node_type = Type::DECLARATION; }
		virtual ~DeclarationNode() {}
		VarNode* variable;
	};

	struct JumpNode : public StatementNode {

		enum class Type {
			UNKNOWN, BREAK, CONTINUE, GOTO, RETURN, YIELD, THROW
		};

		JumpNode(TD td) : StatementNode(td) {}
		virtual ~JumpNode() {}
		JumpNode::Type jump_type = Type::UNKNOWN;
	};

	struct TryNode : public StatementNode {

		TryNode(TD td) : StatementNode(td) {}
		virtual ~TryNode() {}
		vector<StatementNode*> blocks; // try, catch blocks and finally
	};

	struct UsingNode : public StatementNode {

		UsingNode(TD td) : StatementNode(td) {}
		virtual ~UsingNode() {}
		VarNode* local_variable = nullptr;
		StatementNode* body = nullptr;
	};
	// ----- ----- ----- ----- -----
public:
	enum class Modifier {
		MOD_PUBLIC      = 1,
		MOD_PROTECTED   = 1 << 1,
		MOD_PRIVATE     = 1 << 2,
		MOD_INTERNAL    = 1 << 3,
		MOD_EXTERN      = 1 << 4,
		MOD_ABSTRACT    = 1 << 5,
		MOD_CONST       = 1 << 6,
		MOD_OVERRIDE    = 1 << 7,
		MOD_PARTIAL     = 1 << 8,
		MOD_READONLY    = 1 << 9,
		MOD_SEALED      = 1 << 10,
		MOD_STATIC      = 1 << 11,
		MOD_UNSAFE      = 1 << 12,
		MOD_VIRTUAL     = 1 << 13,
		MOD_VOLATILE    = 1 << 14,
		MOD_ASYNC       = 1 << 15,
		MOD_REF         = 1 << 16,    // \ 
		MOD_IN          = 1 << 17,    //  |for variables
		MOD_OUT         = 1 << 18,    //  /
		MOD_PARAMS      = 1 << 19,    // /
		MOD_CONSTRUCTOR = 1 << 20,

		MOD_PPP = MOD_PUBLIC | MOD_PROTECTED | MOD_PRIVATE
	};

	// ***************************** ***************************** //
	// ----- ----- ----- ----- ----- ----- ----- ----- ----- ----- //
	//            ===== ===== CLASS MEMBERS ===== =====            //
	// ----- ----- ----- ----- ----- ----- ----- ----- ----- ----- //
	// ***************************** ***************************** //
private:

	FileNode* root;

	// where is the parser now
	NamespaceNode* cur_namespace;
	ClassNode* cur_class;
	MethodNode* cur_method;
	BlockNode* cur_block;
	string prev_expression;
	string cur_expression;
	string cur_type;
	Node* current;

	int pos;                    // position of current token
	int len;                    // total amount of tokens
	int modifiers;              // state of flags of modifiers

	vector<CSharpLexer::TokenData> tokens; // tokens of being parsed file
	vector<string> attributes;
	set<string> identifiers;

	bool kw_value_allowed = false; // enable only when parse property->set

	// completion info
	struct CompletionInfo {

		// error flag
		int error = 0;

		// msg
		int cur_arg = -1;
		string completion_expression;   // expression where the cursor was found

		// cursor
		Node* ctx_cursor;               // node where the cursor was found
		int cursor_column = -1;         // column of TK_CURSOR
		int cursor_line = -1;           // line of TK_CURSOR

		// type
		CompletionType completion_type; // expected kind of autocompletion

		// nodes
		FileNode* ctx_file;             // file in which is coursor
		NamespaceNode *ctx_namespace;   // namespace in which is coursor
		ClassNode *ctx_class;           // class in which is coursor
		MethodNode *ctx_method;         // method in which is coursor
		BlockNode *ctx_block;           // block in which is coursor
	} cinfo;

	static const string wldc; // Wild Cart Type

	// ***************************** ***************************** //
	// ----- ----- ----- ----- ----- ----- ----- ----- ----- ----- //
	//            ===== ===== CLASS METHODS ===== =====            //
	// ----- ----- ----- ----- ----- ----- ----- ----- ----- ----- //
	// ***************************** ***************************** //
public:
	CSharpParser(string code);
	~CSharpParser();
	FileNode* parse();
	void clear_state(); // set state to zero

	static void indentation(int n);
	static map<CSharpLexer::Token, Modifier> to_modifier;
	static bool is_base_type(string type);
	static int remove_array_type(string& array_type);
	static void add_array_type(string& type, int rank);
	static int compute_rank(const string& type);
	static string completion_type_name(CompletionType type);

	static map<string, int> base_type_category;
	static int get_base_type_category(const string& type);

	static CSharpParser* active_parser; // TODO: probably better would be to use shared_ptr & weak_ptr


	friend class CSharpContext;

	template <typename T, typename U>
	static bool is_unique(const list<T*>& lst, const U* node)
	{
		for (auto x : lst)
			if (x->name == node->name)
				return false;

		return true;
	}

	// ----- ----- ERRORS & CURSOR ----- ----- //
  public:
	void error(string msg) const;
	void debug_info() const;

  private:
	void _unexpeced_token_error() const;
	bool _is_actual_token(CSharpLexer::Token tk, bool assert = false);
	void _assert(CSharpLexer::Token tk); // make sure what is curtok
	void _skip_until_token(CSharpLexer::Token tk); // TODO
	void _found_cursor();
	void _escape();
	void _skip_until_end_of_block();
	void _skip_until_end_of_class();
	void _skip_until_end_of_namespace();
	void _skip_until_next_line();

	// ----- ----- PARSE ----- ----- //

	// member
	NamespaceNode* _parse_namespace(bool global);
	FileNode* _parse_file();
	ClassNode* _parse_class();
	StructNode* _parse_struct();
	InterfaceNode* _parse_interface();
	EnumNode* _parse_enum();
	MethodNode* _parse_method_declaration(string name, string return_type, bool interface_context = false);
	DelegateNode* _parse_delegate();
	bool _parse_using_directive(FileNode* node);
	bool _parse_class_member(ClassNode* node);
	bool _parse_namespace_member(NamespaceNode* node);
	bool _parse_interface_member(InterfaceNode* node);
	void _parse_modifiers();
	void _parse_attributes();
	void _apply_modifiers(Node* node);
	void _apply_attributes(Node* node);

	// statement
	StatementNode* _parse_statement();
	LoopNode* _parse_loop();
	VarNode* _parse_declaration();
	JumpNode* _parse_jump();
	UsingNode* _parse_using_statement();
	string _parse_expression(bool inside = false, CSharpLexer::Token opener = CSharpLexer::Token::TK_EMPTY);
	BlockNode* _parse_block();
	ConditionNode* _parse_if_statement();
	ConditionNode* _parse_switch_statement();
	TryNode* _parse_try_statement();
	PropertyNode* _parse_property(string name, string type);
	StatementNode* _parse_property_definition();
	string _parse_type(bool array_constructor = false);
	string _parse_new();
	string _parse_initialization_block();
	string _parse_method_invocation();
	string _parse_constraints();
	vector<string> _parse_generic_declaration(); // parse <T,U,...>
	vector<string> _parse_derived_and_implements(bool generic_context = false); // parse : C1, I1, I2, ...

	// ----- ----- COMPLETION ----- ----- //
	CompletionType _deduce_completion_type();

};


#endif // CSHARP_PARSER_H
