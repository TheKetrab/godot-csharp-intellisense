#ifndef CSHARP_CONTEXT_H
#define CSHARP_CONTEXT_H


#include <unordered_map>
#include "csharp_parser.h"

using CSP = CSharpParser;
using namespace std;

class type_deduction_error : exception {

};

class completion_error : exception {

};

// https://docs.microsoft.com/en-us/archive/msdn-magazine/2019/october/csharp-accessing-xml-documentation-via-reflection

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

	bool include_constructors; // for get_visible_methods - tylko jesli uzywane przy wyrazeniu takim, ¿e by³o 'new'

public:
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
	string map_to_type(string type_expr, bool ret_wldc = false);
	string map_function_to_type(string func_def, bool ret_wldc = false);
	string simplify_expression(const string expr);
	string simplify_expr_tokens(const vector<CSharpLexer::TokenData> &tokens, int &pos);
	void skip_redundant_prefix(const vector<CSharpLexer::TokenData> &tokens, int &pos);
	list<CSP::Node*> get_nodes_by_simplified_expression(string expr); // zwraca wszystkie pasuj¹ce wêz³y do tego wyra¿enia
	list<CSP::Node*> get_nodes_by_simplified_expression_rec(CSP::Node* invoker, const vector<CSharpLexer::TokenData> &tokens, int pos);
	list<CSP::Node*> get_nodes_by_expression(string expr);
	list<CSP::Node*> get_visible_in_ctx_by_name(string name);

	bool function_match(CSP::MethodNode* method, string function_call);

};

#endif // CSHARP_CONTEXT_H