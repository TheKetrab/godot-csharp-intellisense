#ifndef CSHARP_CONTEXT_H
#define CSHARP_CONTEXT_H

#include <unordered_map>
#include "csharp_parser.h"

using CSP = CSharpParser;
using namespace std;

class CSharpContext {

private:
	CSP::Node* ctx; // ptr to current context
	unordered_map<string, CSP::FileNode*> files; // dane o plikach przez nazwe
	static CSharpContext* _instance;

public:
	static CSharpContext* instance();
	~CSharpContext();


	void update_state(string &code, string &filename);

	vector<CSP::NamespaceNode*> get_namespaces();
	vector<CSP::ClassNode*> get_classes();
	vector<CSP::StructNode*> get_structs();
	vector<CSP::MethodNode*> get_methods();
	vector<CSP::VarNode*> get_vars();
	vector<CSP::PropertyNode*> get_properties();
	vector<CSP::InterfaceNode*> get_interfaces();

private:
	CSharpContext();
};

#endif // CSHARP_CONTEXT_H