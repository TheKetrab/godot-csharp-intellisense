#ifndef CSHARP_CONTEXT_H
#define CSHARP_CONTEXT_H

#include <unordered_map>
#include "csharp_parser.h"

using CSP = CSharpParser;
using namespace std;

class CSharpContext {

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


public:
	static CSharpContext* instance();
	~CSharpContext();

	void update_state(string &code, string &filename);
	CSP::CompletionType get_completion_type();

	vector<CSP::NamespaceNode*> get_namespaces();
	vector<CSP::ClassNode*> get_classes();
	vector<CSP::StructNode*> get_structs();
	vector<CSP::MethodNode*> get_methods();
	vector<CSP::VarNode*> get_vars();
	vector<CSP::PropertyNode*> get_properties();
	vector<CSP::InterfaceNode*> get_interfaces();

	void print();

private:
	CSharpContext();
};

#endif // CSHARP_CONTEXT_H