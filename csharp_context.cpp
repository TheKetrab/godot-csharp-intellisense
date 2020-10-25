#include "csharp_context.h"
CSharpContext* CSharpContext::_instance = nullptr;


void CSharpContext::update_state(string &code, string &filename) {

	CSharpParser parser(code);
	CSP::FileNode* node = parser.parse();
	
	files.insert({ filename,node });
	ctx = parser.cursor;
}

vector<CSP::NamespaceNode*> CSharpContext::get_namespaces()
{
	return vector<CSP::NamespaceNode*>();
}

vector<CSP::ClassNode*> CSharpContext::get_classes()
{
	return vector<CSP::ClassNode*>();
}

vector<CSP::StructNode*> CSharpContext::get_structs()
{
	return vector<CSP::StructNode*>();
}

vector<CSP::MethodNode*> CSharpContext::get_methods()
{
	return vector<CSP::MethodNode*>();
}

vector<CSP::VarNode*> CSharpContext::get_vars()
{
	return vector<CSP::VarNode*>();
}

vector<CSP::PropertyNode*> CSharpContext::get_properties()
{
	return vector<CSP::PropertyNode*>();
}

vector<CSP::InterfaceNode*> CSharpContext::get_interfaces()
{
	return vector<CSP::InterfaceNode*>();
}


CSharpContext* CSharpContext::instance() {
	if (_instance == nullptr)
		_instance = new CSharpContext();
	return _instance;
}

CSharpContext::CSharpContext() {}

CSharpContext::~CSharpContext() {}