#ifndef CSHARP_CONTEXT_H
#define CSHARP_CONTEXT_H

#include <unordered_map>
#include "csharp_parser.h"

using CSP = CSharpParser;
using namespace std;


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
	unordered_map<string, CSP::FileNode*> files; // dane o plikach przez nazwe
	static CSharpContext* _instance;

	CSP::CompletionType completion_type;
	CSP::FileNode *ctx_file;
	CSP::NamespaceNode *ctx_namespace;
	CSP::ClassNode *ctx_class;
	CSP::MethodNode *ctx_method;
	CSP::BlockNode *ctx_block;
	CSP::Node *ctx; // ptr to current context
	string ctx_expression;

	string completion_info_str;
	int completion_info_int = 0;

	vector<string> assembly_info_t;
	vector<string> assembly_info_p;
	vector<string> assembly_info_m;
	vector<string> assembly_info_f;

	int ctx_line;
	int ctx_column;

public:
	static CSharpContext* instance();
	~CSharpContext();

	void update_state(string &code, string &filename);
	CSP::CompletionType get_completion_type();

	vector<CSP::NamespaceNode*> get_namespaces();
	vector<CSP::ClassNode*> get_visible_classes();
	vector<CSP::StructNode*> get_visible_structs();
	vector<CSP::MethodNode*> get_visible_methods();
	vector<CSP::VarNode*> get_visible_vars();
	vector<CSP::PropertyNode*> get_visible_properties();
	vector<CSP::InterfaceNode*> get_visible_interfaces();

	vector<string> get_visible_types();
	vector<string> get_function_signatures(string function_name);
	vector<string> get_visible_labels();

	void print_shortcuts();
	void print();

	vector<pair<Option, string>> get_options();

	string deduce(const string &expr, int &pos);
	string map_to_type(string type_expr);


private:
	CSharpContext();

	// https://docs.microsoft.com/en-us/archive/msdn-magazine/2019/october/csharp-accessing-xml-documentation-via-reflection
	// Methods “M:”; Types “T:”; Fields “F:”; Properties “P:”; Constructors “M:”; Events “E:”.
	//
	bool _is_assembly_member_line(const string &s, string &res);
	void _load_xml_assembly(string path);


	void my_merge(set<string> &s1, const set<string> &s2);

};

#endif // CSHARP_CONTEXT_H