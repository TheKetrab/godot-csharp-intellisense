#ifndef CSHARP_CONTEXT_H
#define CSHARP_CONTEXT_H

#include "csharp_parser.h"
#include "csharp_lexer.h"
#include "csharp_utils.h"

using CSP = CSharpParser;
using CSL = CSharpLexer;

class type_deduction_error : exception {};
class completion_error : exception {};


using namespace std;

class ICSharpProvider {

public:
	vector<string> using_directives;
	virtual list<CSP::TypeNode*> find_class_by_name(string name) = 0;
	virtual CSP::TypeNode* resolve_base_type(string base_type) = 0;
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

	// ===============================
	// ===== ===== MEMBERS ===== =====
	// ===============================

public:
	bool provider_registered = false;
	unordered_map<string, CSP::FileNode*> files;
	CSP::CompletionInfo cinfo;

private:
	static CSharpContext* _instance;
	ICSharpProvider* _provider = nullptr;
	int visibility = VIS_NONE;
	bool include_constructors; // TODO: czy tego nie za³atwia VIS_CONSTRUCT?


	// ===============================
	// ===== ===== METHODS ===== =====
	// ===============================

public:

	// singleton
	static CSharpContext* instance();
	~CSharpContext();

	// get in context
	list<CSP::NamespaceNode*> get_visible_namespaces() const;
	list<CSP::TypeNode*> get_visible_types() const;
	list<CSP::MethodNode*> get_visible_methods() const;
	list<CSP::VarNode*> get_visible_vars() const;
	list<string> get_visible_labels() const;
	list<CSP::TypeNode*> get_types_by_name(string name) const;
	list<CSP::MethodNode*> get_methods_by_name(string name) const;
	list<CSP::VarNode*> get_vars_by_name(string name) const;
	CSP::Node* get_by_fullname(string fullname) const;

	list<CSP::Node*> find_by_shortcuts(string shortname) const;

	// debug info
	void print();
	void print_shortcuts();
	void print_visible();

	// options
	void print_options();
	vector<pair<Option, string>> get_options();
	string option_to_string(Option opt);
	Option node_type_to_option(CSP::Node::Type node_type);

	// simplifying
	string map_to_type(string type_expr, bool ret_wldc = true);
	string map_function_to_type(string func_def, bool ret_wldc = true);
	string map_array_to_type(string array_expr, bool ret_wldc = true);
	string simplify_expression(const string expr);
	string simplify_expr_tokens(const vector<CSharpLexer::TokenData> &tokens, int &pos);

	// expression to node
	list<CSP::Node*> get_nodes_by_simplified_expression(string expr);
	list<CSP::Node*> get_nodes_by_simplified_expression(const vector<CSharpLexer::TokenData> &tokens);
	list<CSP::Node*> get_nodes_by_simplified_expression_rec(CSP::Node* invoker, const vector<CSharpLexer::TokenData> &tokens, int pos);
	list<CSP::Node*> get_nodes_by_expression(string expr);
	list<CSP::Node*> get_visible_in_ctx_by_name(string name);
	CSP::TypeNode* get_type_by_expression(string expr);

	// others
	static map<const char*, const char*> base_types_map;
	CSP::CompletionType get_completion_type() const;
	void register_provider(ICSharpProvider* provider);
	void update_state(string &code, string &filename);

	// helpers
	void scan_tokens_array_type(const vector<CSharpLexer::TokenData>& tokens, string& type, int& pos) const;
	bool types_are_identical(const string &type1, const string &type2) const;
	bool coercion_possible(string from, string to) const;
	bool to_base_type(string &t) const;
	bool function_match(CSP::MethodNode* method, string function_call) const;
	bool on_class_chain(const CSP::TypeNode* derive, const CSP::TypeNode* base) const;
	int get_visibility_by_invoker_type(const CSP::TypeNode* type_of_invoker_object, int visibility) const;
	int get_visibility_by_var(const CSP::VarNode* var_invoker_object, int visibility) const;
	list<CSP::Node*> get_children_of_base_type(string base_type, string child_name) const;

private:

	// singleton
	CSharpContext();


};


#endif // CSHARP_CONTEXT_H