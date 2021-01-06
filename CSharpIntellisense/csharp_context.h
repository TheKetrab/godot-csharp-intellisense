#ifndef CSHARP_CONTEXT_H
#define CSHARP_CONTEXT_H


#include <unordered_map>
#include "csharp_parser.h"
#include "csharp_utils.h"

using CSP = CSharpParser;
using namespace std;

class type_deduction_error : exception {

};

class completion_error : exception {

};

// https://docs.microsoft.com/en-us/archive/msdn-magazine/2019/october/csharp-accessing-xml-documentation-via-reflection

class ICSharpProvider {

public:
	vector<string> using_directives;

	virtual list<CSP::TypeNode*> find_class_by_name(string name) const = 0;
	virtual CSP::TypeNode* resolve_base_type(string base_type) const = 0;
	virtual list<CSP::Node*> get_child_dynamic(void* invoker, string name) const = 0; // invoker to wskaznik na node po stronie providera
};

class CSharpContext {
  public:
	enum class Option {
		KIND_CLASS,
		KIND_FUNCTION,
		KIND_VARIABLE,
		KIND_MEMBER,
		KIND_ENUM,
		KIND_CONSTANT,
		KIND_PLAIN_TEXT,
		KIND_LABEL
	};

private:
	static CSharpContext* _instance;
	CSharpContext();

	ICSharpProvider* _provider = nullptr;

	bool include_constructors; // for get_visible_methods - tylko jesli uzywane przy wyrazeniu takim, �e by�o 'new'
	int visibility = VIS_NONE;

public:
	void register_provider(ICSharpProvider* provider);
	static CSharpContext* instance();
	~CSharpContext();
	unordered_map<string, CSP::FileNode*> files; // dane o plikach przez nazwe
	CSP::CompletionInfo cinfo;

	void update_state(string &code, string &filename);
	CSP::CompletionType get_completion_type();

	// get in context
	list<CSP::NamespaceNode*> get_visible_namespaces();
	list<CSP::TypeNode*> get_visible_types();
	list<CSP::MethodNode*> get_visible_methods();
	list<CSP::VarNode*> get_visible_vars();
	list<string> get_visible_labels();
	list<CSP::TypeNode*> get_types_by_name(string name);
	list<CSP::MethodNode*> get_methods_by_name(string name);
	list<CSP::VarNode*> get_vars_by_name(string name);
	CSP::Node* get_by_fullname(string fullname);

	list<CSP::Node*> find_by_shortcuts(string shortname);

	// debug info
	void print_shortcuts();
	void print();
	void print_visible();

	// options
	vector<pair<Option, string>> get_options();
	void print_options();
	string option_to_string(Option opt);
	Option node_type_to_option(CSP::Node::Type node_type);

	// deduction
	// 
	// wyra�enie uproszczone, to takie, kt�re nie ma w sobie ZAMKNI�TYCH wywo�a� funkcji
	// zamkni�te wywo�anie funkcji, to takie, kt�re ma ')'
	// je�li takie wyst�puje, to musimy wydedukowa� zwracany typ
	// np: N1.C1.DoSth("abc",42).x -> B.x, przy za�o�eniu, �e DoSth zwraca B

	// uproszczanie
	string map_to_type(string type_expr, bool ret_wldc = false);
	string map_function_to_type(string func_def, bool ret_wldc = false);
	string simplify_expression(const string expr);
	string simplify_expr_tokens(const vector<CSharpLexer::TokenData> &tokens, int &pos);

	// mapowanie uproszczonych wyra�e� na w�z�y
	void skip_redundant_prefix(const vector<CSharpLexer::TokenData> &tokens, int &pos);
	list<CSP::Node*> get_nodes_by_simplified_expression(string expr); // zwraca wszystkie pasuj�ce w�z�y do tego wyra�enia
	list<CSP::Node*> get_nodes_by_simplified_expression(const vector<CSharpLexer::TokenData> &tokens); // zwraca wszystkie pasuj�ce w�z�y do tego wyra�enia
	list<CSP::Node*> get_nodes_by_simplified_expression_rec(CSP::Node* invoker, const vector<CSharpLexer::TokenData> &tokens, int pos);
	list<CSP::Node*> get_nodes_by_expression(string expr);
	list<CSP::Node*> get_visible_in_ctx_by_name(string name);

	bool function_match(CSP::MethodNode* method, string function_call) const;
	bool on_class_chain(const CSP::TypeNode* derive, const CSP::TypeNode* base); // czy jest na lancuchu dziedziczenia

	CSP::TypeNode* get_type_by_expression(string expr);
	int get_visibility_by_invoker_type(const CSP::TypeNode* type_of_invoker_object, int visibility);
	int get_visibility_by_var(const CSP::VarNode* var_invoker_object, int visibility);
};



/*
// contract for node provider
class ICSharpNode {

public:
	virtual int get_node_type() const = 0; // 0 - NONE, 1 - TYPE, 2 - METHOD, 3 - PROP, 4 - VAR
	virtual string fullname() const = 0;
	virtual string prettyname() const = 0;
	virtual list<ICSharpNode*> get_members(const string name, int visibility) const = 0;

	virtual string get_type() const = 0; // return type
	virtual bool is_public() const = 0;
	virtual bool is_protected() const = 0;
	virtual bool is_private() const = 0;
	virtual bool is_static() const = 0;
};
*/

#endif // CSHARP_CONTEXT_H