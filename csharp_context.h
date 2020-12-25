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

  //private:
	public:
	unordered_map<string, CSP::FileNode*> files; // dane o plikach przez nazwe
	static CSharpContext* _instance;

	CSP::CompletionInfo cinfo;

	vector<string> assembly_info_t;
	vector<string> assembly_info_p;
	vector<string> assembly_info_m;
	vector<string> assembly_info_f;

public:
	static CSharpContext* instance();
	~CSharpContext();

	void update_state(string &code, string &filename);
	CSP::CompletionType get_completion_type();

	list<CSP::NamespaceNode*> get_visible_namespaces();
	list<CSP::TypeNode*> get_visible_types();
	list<CSP::MethodNode*> get_visible_methods();
	list<CSP::VarNode*> get_visible_vars();
	list<string> get_visible_labels();
	CSP::TypeNode* get_type_by_name(string name);
	CSP::MethodNode* get_method_by_name(string name);
	CSP::VarNode* get_var_by_name(string name);
	CSP::Node* get_by_fullname(string fullname);


	list<CSP::Node*> find_by_shortcuts(string shortname);
	void print_shortcuts();
	void print();
	void print_visible();

	vector<pair<Option, string>> get_options();
	void print_options();
	string option_to_string(Option opt);


	// dedukuje typ wyrazenia
	string deduce(const string &expr, int &pos); // TODO wywalic
	string map_to_type(string type_expr);

	string simplify_expression(const string expr);
	string deduce_type(const vector<CSharpLexer::TokenData> &tokens, int &pos);
	void skip_redundant_prefix(const vector<CSharpLexer::TokenData> &tokens, int &pos);
	list<CSP::Node*> get_nodes_by_simplified_expression(string expr); // zwraca wszystkie pasuj¹ce wêz³y do tego wyra¿enia
	list<CSP::Node*> get_nodes_by_simplified_expression_rec(CSP::Node* invoker, const vector<CSharpLexer::TokenData> &tokens, int pos);
	list<CSP::Node*> get_nodes_by_expression(string expr);
	Option node_type_to_option(CSP::Node::Type node_type);



private:
	CSharpContext();

	// https://docs.microsoft.com/en-us/archive/msdn-magazine/2019/october/csharp-accessing-xml-documentation-via-reflection
	// Methods M; Types T; Fields F; Properties P; Constructors M; Events E.
	//
	bool _is_assembly_member_line(const string &s, string &res);
	void _load_xml_assembly(string path);


	void my_merge(set<string> &s1, const set<string> &s2);
};

#endif // CSHARP_CONTEXT_H