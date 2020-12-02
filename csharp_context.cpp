#include "csharp_context.h"

#include <fstream>
#include <string>
#include <regex>
#include <algorithm>

CSharpContext* CSharpContext::_instance = nullptr;

// update state oznacza, ze poproszono o sparsowanie kodu
// -> jesli plik juz byl w bazie, to info o nim jest zastepowane
// -> jesli nie byl, to jest dodawane
void CSharpContext::update_state(string &code, string &filename) {

	CSharpParser parser(code);
	CSP::FileNode* node = parser.parse();
	node->name = filename;
	node->identifiers = parser.identifiers;
	
	files.insert({ filename,node });
	ctx = parser.cursor;

	// copy ptrs
	completion_type = parser.completion_type;
	ctx_namespace   = parser.completion_namespace;
	ctx_class       = parser.completion_class;
	ctx_method      = parser.completion_method;
	ctx_block       = parser.completion_block;
	ctx_expression = parser.completion_expression;

	ctx_column = parser.cursor_column;
	ctx_line   = parser.cursor_line;

	completion_info_str = parser.completion_info_str;
	completion_info_int = parser.completion_info_int;

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
	for (auto f : files)
		for (CSP::NamespaceNode *n : f.second->namespaces)
			res.push_back(n);

	return res;
}

vector<CSP::ClassNode*> CSharpContext::get_visible_classes() {

	vector<CSP::ClassNode*> res;

	// all classes in current namespace
	for (auto f : files)
		for (CSP::NamespaceNode *n : f.second->namespaces)
			if (n->name == ctx_namespace->name)
				for (CSP::ClassNode *c : n->classes)
					res.push_back(c);

	// all classes from namespaces used in current file
	// TODO

	return res;
}

vector<CSP::StructNode*> CSharpContext::get_visible_structs()
{
	// TODO
	return vector<CSP::StructNode*>();
}

vector<CSP::MethodNode*> CSharpContext::get_visible_methods()
{
	vector<CSP::MethodNode*> res;

	// all methods declared in current class
	for (CSP::MethodNode* m : ctx_class->methods)
		res.push_back(m);

	// all methods from using static
	// TODO

	return res;
}

vector<CSP::VarNode*> CSharpContext::get_visible_vars()
{
	return vector<CSP::VarNode*>();
}

vector<CSP::PropertyNode*> CSharpContext::get_visible_properties()
{
	return vector<CSP::PropertyNode*>();
}

vector<CSP::InterfaceNode*> CSharpContext::get_visible_interfaces()
{
	return vector<CSP::InterfaceNode*>();
}

void CSharpContext::print_shortcuts()
{
	cout << " ----- --------- ----- " << endl;
	cout << " ----- Shortcuts -----" << endl;
	for (auto f : files) {
		cout << "File: " << f.second->name << endl;
		for (auto shortcut : f.second->node_shortcuts) {
			cout << shortcut.first << endl;
		}
	}


}

void CSharpContext::print() {

	for (auto f : files)
		f.second->print();

	if (ctx != nullptr) {
		cout << "Found cursor at: " << ctx_line << " " << ctx_column << endl;
		cout << "Completion type is: " << CSharpParser::completion_type_name(completion_type) << endl;
		cout << "Current expression is: " << ctx_expression << endl;
	}
}

CSharpContext* CSharpContext::instance() {
	if (_instance == nullptr)
		_instance = new CSharpContext();
	return _instance;
}

CSharpContext::CSharpContext() {

	//_load_xml_assembly(R"(C:\Users\ketra\Desktop\Godot322\godot\bin\GodotSharp\Api\Release\GodotSharp.xml)");

}

bool CSharpContext::_is_assembly_member_line(const string &s, string &res) {

	int pos = 0;
	while (s[pos] == ' ') pos++;

	string begining = "<member name=\"";
	for (int i=0; i<14; i++) {
		if (s[pos] != begining[i]) 
			return false;
		pos++;
	}

	res = "";
	for (; s[pos] != '\"'; pos++)
		res += s[pos];

	return true;
}

void CSharpContext::_load_xml_assembly(string path)
{
	ifstream file(path);
	if (!file.is_open()) {
		exit(1); // TODO
	}

	string line;
	string res;

	while (!file.eof()) {

		line = "";
		getline(file, line);

		if (_is_assembly_member_line(line,res)) {
			if      (res[0] == 'T') assembly_info_t.push_back(res);
			else if (res[0] == 'P') assembly_info_p.push_back(res);
			else if (res[0] == 'M') assembly_info_m.push_back(res);
			else if (res[0] == 'F') assembly_info_f.push_back(res);
		}

		

	}
}

CSharpContext::~CSharpContext() {}

vector<pair<CSharpContext::Option, string>> CSharpContext::get_options() {

	vector<pair<Option, string>> options;

	switch (completion_type) {

	case (CSharpParser::CompletionType::COMPLETION_NONE): {
		// empty list
		break;
	}
	case (CSharpParser::CompletionType::COMPLETION_TYPE): {
		// TODO
		break;
	}
	case (CSharpParser::CompletionType::COMPLETION_MEMBER): {

		break;
	}
	case (CSharpParser::CompletionType::COMPLETION_CALL_ARGUMENTS): {
		
		// creates list of signatures of functions which fit the name
		string func_name = completion_info_str;
		int cur_arg = completion_info_int;

		// TODO get functions by name
		// TODO add signature of function
		break;
	}
	case (CSharpParser::CompletionType::COMPLETION_LABEL): {

		break;
	}
	case (CSharpParser::CompletionType::COMPLETION_VIRTUAL_FUNC): {

		break;
	}
	case (CSharpParser::CompletionType::COMPLETION_ASSIGN): {

		break;
	}
	case (CSharpParser::CompletionType::COMPLETION_IDENTIFIER): {

		set<string> identifiers; // destination of merging

		set<string> keywords = CSharpLexer::get_keywords();
		set<string> ctx_file_identifiers = ctx_file->identifiers;
		
		my_merge(identifiers, keywords);
		my_merge(identifiers, ctx_file_identifiers);

		for (auto f : files) {
			set<string> s = f.second->get_external_identifiers();
			my_merge(identifiers, s);
		}
		
		for (auto v : identifiers) {
			options.push_back({ Option::KIND_PLAIN_TEXT, v });
		}

		break;
	}

	}


	return options;
}

// N1.C1.M1(...) --> zwróci typ, jaki zwraca M1
// N1.C1.P1 --> zwróci typ, jakiego jest ta w³aœciwoœæ
// N1.C1.C2 --> zwróci N1
string CSharpContext::map_to_type(string expr) {

	// find in user's files
	for (auto f : files) {
		auto &m = f.second->node_shortcuts;
		auto it = m.find(expr);
		if (it != m.end()) {
			CSP::Node* n = it->second;
			if (n->node_type == CSP::Node::Type::METHOD) {
				CSP::MethodNode* mn = dynamic_cast<CSP::MethodNode*>(n);
				return mn->return_type;
			}
			else if (n->node_type == CSP::Node::Type::PROPERTY) {
				CSP::PropertyNode* mn = dynamic_cast<CSP::PropertyNode*>(n);
				return mn->type;
			}
			else if (n->node_type == CSP::Node::Type::VAR) {
				CSP::VarNode* mn = dynamic_cast<CSP::VarNode*>(n);
				return mn->type;
			}
			// else return the same
		}

	}

	return expr;

}

// zwraca np:
// Namespace1.Namespace2.Class1.Class2 - typ
// Namespace1.Class1.ConcreteMethod - metoda
// 'Class1.Class2.Prop1' -> int - typ w³aœciwoœci Prop1 czyli np int
//
// C1.M1(int,string) --> T', gdzie T' to typ zwracany przez to przeci¹¿enie M1
//
// C() zwraca typ C', E to jakaœ funkcja w C'.D, x to zmienna typu int
// A.B.C().D.E(x, --> C'.D.E(int  --> bêdzie to trzeba próbowaæ dopasowaæ
//
// TODO dodac obslugiwanie blokow expr { }
string CSharpContext::deduce(const string &expr, int &pos)
{
	// rekurencyjna funkcja iteruje sie po stringu expr i dedukuje aktualny typ (fullname)

	// "N1.C1.GetSth()" -> typ, ktory zwraca GetSth
	// "N1.C1.GetSth(...) - wchodzi rekurencyjnie do œrodka

	string deduced = "";

	while (expr[pos] != '\0') {

		// inner context - argument of a function
		if (expr[pos+1] == ',' || expr[pos+1] == ')') {
			return map_to_type(deduced);
		}

		// outer ctx
		else if (expr[pos] == '(') {
			deduced += '(';
			pos++;
			deduced += deduce(expr, pos);
		}

		else if (expr[pos] == ')') {
			deduced += ')';
			deduced = 
			pos++;
		}

		else {
			deduced += expr[pos];
			pos++;
		}

	}

	return deduced;
}

vector<string> CSharpContext::get_visible_types() {
	return vector<string>();
}
vector<string> CSharpContext::get_function_signatures(string function_name) {
	return vector<string>();

}
vector<string> CSharpContext::get_visible_labels() {
	return vector<string>();

}


// put s2 values into s1
void CSharpContext::my_merge(set<string> &s1, const set<string> &s2) {

	for (auto v : s2)
		s1.insert(v);

}
