#include "csharp_context.h"
CSharpContext* CSharpContext::_instance = nullptr;

// update state oznacza, ze poproszono o sparsowanie kodu
// -> jesli plik juz byl w bazie, to info o nim jest zastepowane
// -> jesli nie byl, to jest dodawane
void CSharpContext::update_state(string &code, string &filename) {

	CSharpParser parser(code);
	CSP::FileNode* node = parser.parse();
	node->name = filename;
	
	files.insert({ filename,node });
	ctx = parser.cursor;

	// copy ptrs
	completion_type = parser.completion_type;
	ctx_namespace = parser.completion_namespace;
	ctx_class = parser.completion_class;
	ctx_method = parser.completion_method;
	ctx_block = parser.completion_block;

	ctx_file = node;
}

CSP::CompletionType CSharpContext::get_completion_type()
{
	return completion_type;
}

vector<CSP::NamespaceNode*> CSharpContext::get_namespaces()
{
	// all declared namespaces
	vector<CSP::NamespaceNode*> res;
	for (auto& file : files)

	return res;
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

void CSharpContext::print() {

	for (auto f : files)
		f.second->print();

}

CSharpContext* CSharpContext::instance() {
	if (_instance == nullptr)
		_instance = new CSharpContext();
	return _instance;
}

CSharpContext::CSharpContext() {}

CSharpContext::~CSharpContext() {}