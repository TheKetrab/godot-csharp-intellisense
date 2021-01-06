#include "csharp_context.h"
#include "csharp_utils.h"

#include <fstream>
#include <string>
#include <regex>
#include <algorithm>


using CST = CSharpLexer::Token;

CSharpContext* CSharpContext::_instance = nullptr;

// update state oznacza, ze poproszono o sparsowanie kodu
// -> jesli plik juz byl w bazie, to info o nim jest zastepowane
// -> jesli nie byl, to jest dodawane
void CSharpContext::update_state(string &code, string &filename) {

	// delete old
	auto it = files.find(filename);
	if (it != files.end()) {

		if (cinfo.ctx_file == it->second) {
			// clear state
			this->cinfo = CSP::CompletionInfo();
		}

		delete it->second;
		files.erase(it);
	}

	// add new
	CSharpParser parser(code);
	CSP::FileNode* node = parser.parse();
	node->name = filename;
	node->identifiers = parser.identifiers;
	
	// register using directives for provider
	if (_provider != nullptr)
		_provider->using_directives = node->using_directives;

	files.insert({ filename,node });

	cinfo = parser.cinfo;
	cinfo.ctx_file = node;

}

CSP::CompletionType CSharpContext::get_completion_type()
{
	return cinfo.completion_type;
}

list<CSP::Node*> CSharpContext::find_by_shortcuts(string shortname)
{
	// for function -> compare until '(' -> only name

	list<CSP::Node*> res;
	shortname = substr(shortname, '(');

	for (auto s : cinfo.ctx_file->node_shortcuts) {
		if (substr(s.first, '(') == shortname)
			res.push_back(s.second);
	}

	return res;
}

void CSharpContext::print_shortcuts()
{
	cout << " ----- --------- ----- " << endl;
	cout << " ----- Shortcuts -----" << endl;
	cout << " ----- --------- ----- " << endl;
	for (auto f : files) {
		cout << "File: " << f.second->name << endl;
		for (auto shortcut : f.second->node_shortcuts) {
			cout << shortcut.first << endl;
		}
	}
}

void CSharpContext::print_visible() {

	cout << endl;
	cout << " ----- --------- ----- " << endl;
	cout << "  VISIBLE IN CONTEXT (VIS_ALL):" << endl;
	cout << " ----- --------- ----- " << endl;
	cout << endl;

	cout << "LABELS:" << endl;
	for (auto x : get_visible_labels())
		cout << "  " << x << endl;

	cout << "NAMESPACES:" << endl;
	for (auto x : get_visible_namespaces())
		cout << "  " << x->fullname() << endl;

	cout << "TYPES:" << endl;
	for (auto x : get_visible_types())
		cout << "  " << x->fullname() << endl;

	cout << "METHODS:" << endl;
	for (auto x : get_visible_methods()) {
		cout << "  " << x->fullname() << " : ";

		if (CSP::is_base_type(x->return_type))
			cout << x->return_type << endl;
		else {
			list<CSP::TypeNode*> tn = get_types_by_name(x->return_type);
			if (tn.empty()) {
				cout << x->return_type << " [not found]" << endl;
			} else {
				cout << tn.front()->fullname() << endl;
			}
		}

	}

	cout << "VARIABLES:" << endl;
	for (auto x : get_visible_vars()) {

		cout << "  " << x->fullname() << " : ";

		string type = x->get_type();

		if (CSP::is_base_type(type)) {
			if (x->type == "var") {
				cout << "var(" << type << ")" << endl;
			} else {
				cout << type << endl;
			}
		}
		else {
			
			list<CSP::TypeNode*> tn = get_types_by_name(type);
			if (tn.empty()) {
				cout << x->type << " [not found]" << endl;
			}
			else {
				if (x->type == "var") {
					cout << "var(" << tn.front()->fullname() << ")" << endl;
				}
				else {
					cout << tn.front()->fullname() << endl;
				}
			}
		}


	}

	cout << "----- ----- ----- ----- ----- -----" << endl;
}

void CSharpContext::print() {

	for (auto f : files)
		f.second->print();

	if (cinfo.ctx_cursor != nullptr) {
		cout << "Found cursor at: " << cinfo.cursor_line << " " << cinfo.cursor_column << endl;
		cout << "Completion type is: " << CSharpParser::completion_type_name(cinfo.completion_type) << endl;
		cout << "Current expression is: " << cinfo.completion_expression << endl;
	}
}

void CSharpContext::register_provider(ICSharpProvider* provider)
{
	_provider = provider;
}

CSharpContext* CSharpContext::instance() {
	if (_instance == nullptr)
		_instance = new CSharpContext();
	return _instance;
}

CSharpContext::CSharpContext() {

	//_load_xml_assembly(R"(C:\Users\ketra\Desktop\Godot322\godot\bin\GodotSharp\Api\Release\GodotSharp.xml)");

}
//
//bool CSharpContext::_is_assembly_member_line(const string &s, string &res) {
//
//	int pos = 0;
//	while (s[pos] == ' ') pos++;
//
//	string begining = "<member name=\"";
//	for (int i=0; i<14; i++) {
//		if (s[pos] != begining[i]) 
//			return false;
//		pos++;
//	}
//
//	res = "";
//	for (; s[pos] != '\"'; pos++)
//		res += s[pos];
//
//	return true;
//}
//
//void CSharpContext::_load_xml_assembly(string path)
//{
//	ifstream file(path);
//	if (!file.is_open()) {
//		exit(1); // TODO
//	}
//
//	string line;
//	string res;
//
//	while (!file.eof()) {
//
//		line = "";
//		getline(file, line);
//
//		if (_is_assembly_member_line(line,res)) {
//			if      (res[0] == 'T') assembly_info_t.push_back(res);
//			else if (res[0] == 'P') assembly_info_p.push_back(res);
//			else if (res[0] == 'M') assembly_info_m.push_back(res);
//			else if (res[0] == 'F') assembly_info_f.push_back(res);
//		}
//
//		
//
//	}
//}

CSharpContext::~CSharpContext() {}

vector<pair<CSharpContext::Option, string>> CSharpContext::get_options() {

	vector<pair<Option, string>> options;

	switch (cinfo.completion_type) {

	case (CSharpParser::CompletionType::COMPLETION_NONE): {
		// empty list
		break;
	}
	case (CSharpParser::CompletionType::COMPLETION_TYPE): {
		
		auto types = get_visible_types();
		for (auto t : types)
			options.push_back({ Option::KIND_CLASS, t->fullname() });

		break;
	}
	case (CSharpParser::CompletionType::COMPLETION_MEMBER): {

		list<CSharpParser::Node*> nodes = get_nodes_by_expression(cinfo.completion_expression);
		for (auto x : nodes) {

			options.push_back({ node_type_to_option(x->node_type), x->prettyname() });

		}

		break;
	}
	case (CSharpParser::CompletionType::COMPLETION_CALL_ARGUMENTS): {
		
		cout << "call arguments" << endl;
		// creates list of signatures of functions which fit the name

		list<CSharpParser::Node*> nodes = get_nodes_by_expression(cinfo.completion_expression);
		for (auto x : nodes) {

			options.push_back({ node_type_to_option(x->node_type), x->prettyname() });

		}

		break;
	}
	case (CSharpParser::CompletionType::COMPLETION_LABEL): {

		auto labels = get_visible_labels();
		for (auto x : labels)
			options.push_back({ Option::KIND_LABEL, x });

		break;
	}
	case (CSharpParser::CompletionType::COMPLETION_VIRTUAL_FUNC): {
		// TODO
		break;
	}
	case (CSharpParser::CompletionType::COMPLETION_ASSIGN): {
		// TODO
		break;
	}
	case (CSharpParser::CompletionType::COMPLETION_IDENTIFIER): {

		set<string> identifiers; // destination of merging

		set<string> keywords = CSharpLexer::get_keywords();
		set<string> ctx_file_identifiers = cinfo.ctx_file->identifiers;
		
		MERGE_SETS(identifiers, keywords);
		MERGE_SETS(identifiers, ctx_file_identifiers);

		for (auto f : files) {
			set<string> s = f.second->get_external_identifiers();
			MERGE_SETS(identifiers, s);
		}
		
		for (auto v : identifiers) {
			options.push_back({ Option::KIND_PLAIN_TEXT, v });
		}

		break;
	}

	}


	// remove doubles
	sort(options.begin(), options.end());
	options.erase(unique(options.begin(), options.end()), options.end());

	return options;
}

void CSharpContext::print_options()
{
	auto opts = get_options();
	for (auto opt : opts) {
		cout << option_to_string(opt.first);
		cout << " : " << opt.second << endl;
	}
}

string CSharpContext::option_to_string(Option opt)
{
	switch (opt) {
	case Option::KIND_CLASS:      return "CLASS   ";
	case Option::KIND_FUNCTION:   return "FUNCTION";
	case Option::KIND_VARIABLE:   return "VARIABLE";
	case Option::KIND_MEMBER:     return "MEMBER  ";
	case Option::KIND_ENUM:       return "ENUM    ";
	case Option::KIND_CONSTANT:   return "CONSTANT";
	case Option::KIND_PLAIN_TEXT: return "TEXT    ";
	case Option::KIND_LABEL:      return "LABEL   ";
	};

	return string();
}

// N1.C1.M1(...) --> zwr�ci typ, jaki zwraca M1
// N1.C1.P1 --> zwr�ci typ, jakiego jest ta w�a�ciwo��
// N1.C1.C2 --> zwr�ci N1
// ret_wldc = true -> return "?" if not found
// else -> return the same expr
string CSharpContext::map_to_type(string expr, bool ret_wldc) {

	// is base type?
	if (CSP::is_base_type(expr))
		return expr;

	// is function?
	if (contains(expr, '(')) {

		return map_function_to_type(expr,ret_wldc);

	}

	// found in shortcuts?
	for (auto f : files) {
		auto &m = f.second->node_shortcuts;
		auto it = m.find(expr);

		if (it != m.end()) {
			CSP::Node* n = it->second;
			if (n->node_type == CSP::Node::Type::METHOD) {
				CSP::MethodNode* mn = dynamic_cast<CSP::MethodNode*>(n);
				return mn->return_type;
			}
			else if (n->node_type == CSP::Node::Type::PROPERTY
				|| n->node_type == CSP::Node::Type::VAR) {
				CSP::VarNode* vn = (CSP::VarNode*)n;
				return vn->get_type();
			}
			// else return the same
		}
	}

	// try to find by fullname	
	CSP::Node* n = get_by_fullname(expr);
	if (n != nullptr) {
		switch (n->node_type) {
		case CSP::Node::Type::ENUM:
		case CSP::Node::Type::NAMESPACE:
		case CSP::Node::Type::CLASS:
		case CSP::Node::Type::INTERFACE:
			return n->fullname();
		case CSP::Node::Type::METHOD:
			return ((CSP::MethodNode*)n)->get_type();
		case CSP::Node::Type::PROPERTY:
		case CSP::Node::Type::VAR:
			return ((CSP::VarNode*)n)->get_type();
		}
	}

	// find by expression
	auto nodes = get_nodes_by_expression(expr);
	if (nodes.size() > 0) {
		CSP::Node* n = nodes.front();
		switch (n->node_type) {
		case CSP::Node::Type::ENUM:
		case CSP::Node::Type::NAMESPACE:
		case CSP::Node::Type::CLASS:
		case CSP::Node::Type::INTERFACE:
			return n->fullname();
		case CSP::Node::Type::METHOD:
			return ((CSP::MethodNode*)n)->get_type();
		case CSP::Node::Type::PROPERTY:
		case CSP::Node::Type::VAR:
			return ((CSP::VarNode*)n)->get_type();
		}
	}

	// not found -> return wildcart "?"
	if (ret_wldc) return CSharpParser::wldc;
	else return expr;
}

string CSharpContext::map_function_to_type(string func_def, bool ret_wldc)
{
	vector<string> splitted = split_func(func_def);

	string func_name = splitted[0];
	int n = splitted.size();

	int prev_visibility = this->visibility;
	this->visibility = VIS_ALL;
	this->visibility &= ~VIS_CONSTRUCT;
	auto nodesAll = get_nodes_by_simplified_expression(func_name);
	this->visibility = prev_visibility;

	list<CSP::MethodNode*> nodes;
	for (auto x : nodesAll)
		if (x->node_type == CSP::Node::Type::METHOD) {
			nodes.push_back((CSP::MethodNode*)x);
		}
//	auto nodes = get_methods_by_name(func_name);

	// KOLEJNOSC DOPASOWYWANIA:
	//  (dla funkcji zamkni�tych)
	// 1. funkcje, kt�re maj� dok�adnie wszystkie argumenty i pasuj� idealnie
	// 2. funkcje, kt�re maj� dok�adnie wszystkie argumenty i pasuj� przez koercj�
	//  (dla funkcji otwartych)
	// 3. funkcje, kt�re maj� wi�cej argument�w i pasuj� idealnie
	// 4. funkcje, kt�re maj� wi�cej argument�w i pasuj� przez koercj�


	// 1
	// find method that exactly match
	for (auto x : nodes) {

		int m = x->arguments.size();

		// przyr�wnywana metoda musi mie� tyle argument�w ile func_def (n-1)
		if (m == n - 1)
		{
			bool success = true;
			for (int i = 1; i < n; i++) {

				string this_func_arg = splitted[i];
				string node_func_arg = x->arguments[i - 1]->type;

				if (this_func_arg != node_func_arg) {
					success = false;
					break;
				}
			}
			if (success) {
				return x->return_type;
			}
		}
	}

	// 2
	// find method that match with implicit casting (coercion)
	for (auto x : nodes) {

		int m = x->arguments.size();

		if (m == n - 1)
		{

			bool success = true;
			for (int i = 1; i < n; i++) {

				string this_func_arg = splitted[i];
				string node_func_arg = x->arguments[i - 1]->type;

				if (!CSharpParser::coercion_possible(this_func_arg, node_func_arg)) {
					success = false;
					break;
				}
			}
			if (success) {
				return x->return_type;
			}
		}

	}

	// 3
	// find method that exactly match
	for (auto x : nodes) {

		int m = x->arguments.size();

		// przyr�wnywana metoda musi mie� przynajmniej tyle argument�w ile func_def (n-1)
		if (m > n - 1)
		{
			bool success = true;
			for (int i = 1; i < n; i++) {

				string this_func_arg = splitted[i];
				string node_func_arg = x->arguments[i - 1]->type;

				if (this_func_arg != node_func_arg) {
					success = false;
					break;
				}
			}
			if (success) {
				return x->return_type;
			}
		}
	}

	// 4
	// find method that match with implicit casting (coercion)
	for (auto x : nodes) {

		int m = x->arguments.size();

		if (m > n - 1)
		{

			bool success = true;
			for (int i = 1; i < n; i++) {

				string this_func_arg = splitted[i];
				string node_func_arg = x->arguments[i - 1]->type;

				if (!CSharpParser::coercion_possible(this_func_arg, node_func_arg)) {
					success = false;
					break;
				}
			}
			if (success) {
				return x->return_type;
			}
		}

	}

	// none fit
	if (ret_wldc) return CSharpParser::wldc;
	else return func_def;
}

string CSharpContext::simplify_expression(const string expr)
{
	CSharpLexer lexer(expr);
	lexer.tokenize();
	auto tokens = lexer.get_tokens();

	int prev_visibility = this->visibility;
	this->visibility = VIS_ALL;

	int pos = 0;
	string res = simplify_expr_tokens(tokens, pos);

	this->visibility = prev_visibility;

	return res;
}

// dedukuje typ jakiego� wyra�enia, korzystaj�c z informacji, kt�re ma (np o namespaceach i klasie w kt�rej rozwa�amy to wyra�enie)
// additionally: remove redundant - dodatkowo usuwa niepotrzebne: np je�li N1.N2.C1 a jest to u�ywane w klasie C1 albo s� otwarte te namespace'y (klauzul� using) to nie dodawaj do typu
string CSharpContext::simplify_expr_tokens(const vector<CSharpLexer::TokenData> &tokens, int &pos)
{
	bool prev_include_constructors = this->include_constructors;

	string res;
	int n = tokens.size();

	bool end = false;
	while (pos < n && !end) {

		if (tokens[pos].type == CST::TK_EOF)
			return res;

		// function invokation
		if (tokens[pos].type == CST::TK_PARENTHESIS_OPEN) {

			pos++;
			res += "(";

			// parse arguments' types
			while (pos < n) {
				string type = simplify_expr_tokens(tokens, pos);
				if (type == "") { // end
					res += ")";
					pos++;
					break;
				}
				res += type;

				// inner parenthesis close
				if (tokens[pos].type == CST::TK_PARENTHESIS_CLOSE) {
					pos++;
					res += ")";
					break;		
				}

				// inner comma
				else if (tokens[pos].type == CST::TK_COMMA) {
					pos++;
					res += ",";
				}

				else {
					throw type_deduction_error();
				}
			}

			res = map_to_type(res);
			
		}

		// outer parenthesis close
		else if (tokens[pos].type == CST::TK_PARENTHESIS_CLOSE) {
			end = true;
		}

		// outer comma
		else if (tokens[pos].type == CST::TK_COMMA) {
			end = true;
		}

		// keyword new
		else if (tokens[pos].type == CST::TK_KW_NEW) {
			this->include_constructors = true;
			pos++;
		}

		// sth else
		else {

			res += tokens[pos].to_string(true);
			pos++;
		}
	}

	res = map_to_type(res);

	//restore
	this->include_constructors = prev_include_constructors;
	return res;
}

list<CSP::NamespaceNode*> CSharpContext::get_visible_namespaces()
{
	ASSURE_CTX(list<CSP::NamespaceNode*>());

	//list<CSP::NamespaceNode*> res;
	auto visible_namespaces = cinfo.ctx_cursor->get_visible_namespaces();
	//MERGE_LISTS(res, visible_namespaces);

	return visible_namespaces;
}

list<CSP::TypeNode*> CSharpContext::get_visible_types()
{
	ASSURE_CTX(list<CSP::TypeNode*>());

	//list<CSP::TypeNode*> res;
	auto visible_types = cinfo.ctx_cursor->get_visible_types(visibility);
	//MERGE_LISTS(res, visible_types);

	return visible_types;
}

list<CSP::MethodNode*> CSharpContext::get_visible_methods()
{
	ASSURE_CTX(list<CSP::MethodNode*>());

	list<CSP::MethodNode*> res;

	// methods
	auto visible_methods = cinfo.ctx_cursor->get_visible_methods(visibility);
	MERGE_LISTS(res, visible_methods);

	// constructors of visible classess
	if (this->include_constructors) {
		auto visible_types = cinfo.ctx_cursor->get_visible_types(visibility);
		for (auto x : visible_types) {
			if (x->node_type == CSP::Node::Type::STRUCT
				|| x->node_type == CSP::Node::Type::CLASS)
			{
				auto constructors = ((CSP::StructNode*)x)->get_constructors(visibility);
				MERGE_LISTS(res, constructors);
			}
		}
	}

	return res;
}

list<CSP::VarNode*> CSharpContext::get_visible_vars()
{
	ASSURE_CTX(list<CSP::VarNode*>());

	//list<CSP::VarNode*> res;
	auto visible_vars = cinfo.ctx_cursor->get_visible_vars(visibility);
	//MERGE_LISTS(res, visible_vars);
	
	return visible_vars;
}

list<string> CSharpContext::get_visible_labels()
{
	ASSURE_CTX(list<string>());

	list<string> res;
	auto visible_labels = cinfo.ctx_file->labels;
	for (auto x : visible_labels)
		res.push_back(x);

	return res;
}

list<CSP::TypeNode*> CSharpContext::get_types_by_name(string name)
{
	ASSURE_CTX(list<CSP::TypeNode*>());

	list<CSP::TypeNode*> res;
	auto visible_types = get_visible_types();
	MERGE_LISTS_COND(res, visible_types, NAME_COND);

	return res;
}

list<CSP::MethodNode*> CSharpContext::get_methods_by_name(string name)
{
	ASSURE_CTX(list<CSP::MethodNode*>());

	list<CSP::MethodNode*> res;
	auto visible_methods = get_visible_methods();
	MERGE_LISTS_COND(res, visible_methods, NAME_COND);

	return res;
}

list<CSP::VarNode*> CSharpContext::get_vars_by_name(string name)
{
	ASSURE_CTX(list<CSP::VarNode*>());

	list<CSP::VarNode*> res;
	auto visible_vars = get_visible_vars();
	MERGE_LISTS_COND(res, visible_vars, NAME_COND);

	return res;
}

CSP::Node* CSharpContext::get_by_fullname(string fullname)
{
	ASSURE_CTX(nullptr);

	for (auto x : get_visible_namespaces()) {
		if (fullname == x->fullname())
			return x;
	}

	for (auto x : get_visible_types()) {
		if (fullname == x->fullname())
			return x;
	}

	for (auto x : get_visible_vars()) {
		if (fullname == x->fullname())
			return x;
	}

	for (auto x : get_visible_methods()) {
		if (fullname == x->fullname())
			return x;
	}


	return nullptr;

}

// symulacja DFS przy przechodzeniu po drzewie w�z��w - szukamy wszystkiego co pasuje
// daje node'y takie, z jakim visibility przysz�o, ustawia nowe visibility dla dalszych wywolan rekurencyjnych
list<CSP::Node*> CSharpContext::get_nodes_by_simplified_expression_rec(CSP::Node* invoker, const vector<CSharpLexer::TokenData> &tokens, int pos) 
{
	// 1. . X   - konkretne dziecko
	// 2. . EOF - wszystkie dzieci
	// 3. ( - wywo�ane na funkcji

	ASSURE_CTX(list<CSP::Node*>());

	int n = tokens.size();

	// EXIT IF -> NOTE: invoker is visible
	if (pos >= n || tokens[pos].type == CST::TK_EOF)
		return list<CSP::Node*>({ invoker });

	// ELSE -> tokens[pos] && tokens[pos+1] exist!

	list<CSP::Node*> res;

	// --- 1: .X .Y -> get concrete child, find inside ---
	if (tokens[pos].type == CST::TK_PERIOD
		&& tokens[pos + 1].type == CST::TK_IDENTIFIER)
	{
		string child_name = tokens[pos + 1].data;
		auto children = invoker->get_members(child_name, visibility);
		for (auto x : children) {

			if (!(IS_TYPE_NODE(x))) // no longer static
				visibility &= ~VIS_STATIC;
			else if (x->node_type == CSP::Node::Type::VAR) // set visibility (can become even public!)
				visibility = csc->get_visibility_by_var((CSP::VarNode*)x, visibility);

			auto res_results = get_nodes_by_simplified_expression_rec(x, tokens, pos + 2);
			MERGE_LISTS(res, res_results);
		}
	}

	// --- 2: only '.' -> all children ---
	else if (tokens[pos].type == CST::TK_PERIOD
		&& tokens[pos+1].type == CST::TK_EOF)
	{
		if (IS_TYPE_NODE(invoker)) {			
			visibility = get_visibility_by_invoker_type((CSP::TypeNode*)invoker, visibility);
		}

		// "" means any child -> see SCAN_AND_ADD macro
		auto children = invoker->get_members("",visibility);
		MERGE_LISTS(res, children);
	}

	// --- 3: '(' not closed function -> resolve args and match ---
	else if (tokens[pos].type == CST::TK_PARENTHESIS_OPEN)
	{
		// jesli to funkcja, to na pewno jest niedomknieta, sprawdzmy czy pasuje
		string function_call = tokens[pos-1].data + "(";

		string function_arg; // resolve args type
		for (int i = pos + 1; tokens[i].type != CST::TK_EOF; i++) {

			if (tokens[i].type == CST::TK_COMMA) {
				string type = map_to_type(function_arg);
				function_call += type + ',';
				function_arg = "";
			}
			else {
				function_arg += tokens[i].to_string(true);
			}

		}

		string type = map_to_type(function_arg);
		function_call += type;

		if (function_match((CSP::MethodNode*)invoker,function_call))
			res.push_back(invoker);

	}

	return res;

}

list<CSP::Node*> CSharpContext::get_nodes_by_simplified_expression(string expr) {

	ASSURE_CTX(list<CSP::Node*>());

	CSharpLexer lexer(expr);
	lexer.tokenize();

	auto tokens = lexer.get_tokens();
	return get_nodes_by_simplified_expression(tokens);
}

// cinfo.ctx_expression najpierw trzeba uproscic, a nastepnie wywolac get_nodes_by_simplified_expression
list<CSP::Node*> CSharpContext::get_nodes_by_simplified_expression(const vector<CSharpLexer::TokenData> &tokens)
{
	// N1.C2.  -> zwraca list� member�w C2
	// N1.C2.Foo( -> zwraca wszystkie funkcje 'Foo' z klasy C2

	// Hence this is simplified expression, there is no ')' token inside.
	// Posible cases of first token:
	//   1. this
	//   2. literal
	//   3. class-name or namespace-name
	//   4. variable

	list<CSP::Node*> res;
	switch (tokens[0].type) {

	// --- 1: this ---
	case CST::TK_KW_THIS: {
		CSP::TypeNode* thisClass = csc->cinfo.ctx_class;
		if (thisClass != nullptr) {
			visibility &= ~VIS_STATIC;
			auto nodes = get_nodes_by_simplified_expression_rec(thisClass, tokens, 1);
			MERGE_LISTS(res, nodes);
		}
		break;
	}

	// --- 2: literal ---
	case CST::TK_LT_CHAR:
	case CST::TK_LT_INTEGER:
	case CST::TK_LT_REAL:
	case CST::TK_LT_STRING:
	case CST::TK_LT_INTERPOLATED:
	{
		if (_provider != nullptr) {
			string type = tokens[0].to_string(true);
			CSP::TypeNode* tn = _provider->resolve_base_type(type);
			auto nodes = get_nodes_by_simplified_expression_rec(tn, tokens, 1);
			MERGE_LISTS(res, nodes);
		}

		break;
	}

	case CST::TK_IDENTIFIER: {

		list<CSP::Node*> start;

		// --- 3: visible in shortcuts ---
		auto found_by_shortcuts = find_by_shortcuts(tokens[0].data);
		MERGE_LISTS(start, found_by_shortcuts);

		// --- 4: variable or resolvable identifier
		auto found_by_visible = get_visible_in_ctx_by_name(tokens[0].data);
		MERGE_LISTS(start, found_by_visible);

		// --- 5: find in provider ---
		if (start.empty()) {
			if (_provider != nullptr) {
				auto found_in_provider = _provider->find_class_by_name(tokens[0].data);
				MERGE_LISTS(start, found_in_provider);
			}
		}

		// FIND FURTHER
		for (auto x : start) {
			auto nodes = get_nodes_by_simplified_expression_rec(x, tokens, 1);
			MERGE_LISTS(res, nodes);
		}
	}
	}
	
	return res;
}

list<CSP::Node*> CSharpContext::get_nodes_by_expression(string expr)
{
	ASSURE_CTX(list<CSP::Node*>());

	int prev_visibility = this->visibility;
	this->visibility = VIS_ALL;

	if (!contains(expr, "new ")) // new + space
		this->visibility &= ~VIS_CONSTRUCT;
	else
		expr = expr.substr(4, expr.length() - 4);

	// je�li wyra�enie nie jest proste, to trzeba je upro�ci�
	// nie proste = jakie� wywo�anie funkcji
	if (contains(expr, ')')) {
		expr = simplify_expression(expr);
		visibility &= ~VIS_STATIC; // no static, object context!
	}

	auto nodes = get_nodes_by_simplified_expression(expr);

	this->visibility = prev_visibility;
	return nodes;
}

list<CSP::Node*> CSharpContext::get_visible_in_ctx_by_name(string name)
{
	list<CSP::Node*> res;

	auto visible_namespaces = get_visible_namespaces();
	MERGE_LISTS_COND(res, visible_namespaces, CHILD_COND);

	auto visible_types = get_visible_types();
	MERGE_LISTS_COND(res, visible_types, CHILD_COND);

	auto visible_methods = get_visible_methods();
	MERGE_LISTS_COND(res, visible_methods, CHILD_COND);

	auto visible_vars = get_visible_vars();
	MERGE_LISTS_COND(res, visible_vars, CHILD_COND);

	return res;
}

bool CSharpContext::function_match(CSP::MethodNode* method, string function_call) const
{
	// odpowiednie argumenty albo s� dobrego typu, albo mo�na zrobi� koercj�

	// n-1 - ilosc dotychczas napisanych argumentow przez uzytkownika
	// m   - ilosc argumentow rozwazanej funkcji

	vector<string> splitted = split_func(function_call);

	string func_name = splitted[0];
	int n = splitted.size();

	if (func_name != method->name)
		return false;

	int m = method->arguments.size();

	// metoda musi mie� przynajmniej tyle argumentow ile to, do czego dopasowujemy
	if (m < n - 1)
		return false;

	bool success = true;
	for (int i = 1; i < n; i++) {

		string this_func_arg = splitted[i];
		string node_func_arg = method->arguments[i - 1]->type;

		if (this_func_arg != node_func_arg
			&& !CSharpParser::coercion_possible(this_func_arg, node_func_arg))
		{
			success = false;
			break;
		}
	}

	if (success)
		return true;

	return false;

}

bool CSharpContext::on_class_chain(const CSP::TypeNode* derive, const CSP::TypeNode* base)
{
	if (derive == nullptr || base == nullptr)
		return false;

	if (derive == base)
		return true;

	bool res = false;
	for (string type_name : derive->base_types) {

		auto nodes = get_nodes_by_expression(type_name);
		for (auto n : nodes) {

			if (n->node_type == CSP::Node::Type::CLASS
				|| n->node_type == CSP::Node::Type::STRUCT
				|| n->node_type == CSP::Node::Type::INTERFACE)
			{
				if (on_class_chain((CSP::TypeNode*)n, base))
					res = true;
			}

		}

	}

	return res;
}



CSharpContext::Option CSharpContext::node_type_to_option(CSP::Node::Type node_type)
{
	switch (node_type) {

	case CSP::Node::Type::CLASS:
	case CSP::Node::Type::INTERFACE:
	case CSP::Node::Type::STRUCT:
		return Option::KIND_CLASS;

	case CSP::Node::Type::ENUM:
		return Option::KIND_ENUM;

	case CSP::Node::Type::METHOD:
		return Option::KIND_FUNCTION;

	case CSP::Node::Type::PROPERTY:
	case CSP::Node::Type::VAR:
		return Option::KIND_VARIABLE;

	default:
		return Option::KIND_PLAIN_TEXT; // todo unknown?
	}

}

CSP::TypeNode* CSharpContext::get_type_by_expression(string expr)
{
	// auto nodes = get_nodes_by_expression(expr); TODO: to powoduje nieskonczona rekursje!
	auto nodes = find_by_shortcuts(expr); // TODO powinno byc by expression
	for (auto x : nodes)
		if (IS_TYPE_NODE(x))
			return (CSP::TypeNode*)x;

	return nullptr;
}

// zmienia widzialno�� public private protected
int CSharpContext::get_visibility_by_invoker_type(const CSP::TypeNode* type_of_invoker_object, int visibility) {

	// disable pri, pro & pub and set it again
	visibility &= ~VIS_PPP;

	if (csc->cinfo.ctx_class == type_of_invoker_object)
		visibility |= VIS_PPP;
	else if (csc->on_class_chain(csc->cinfo.ctx_class, type_of_invoker_object))
		visibility |= VIS_PP;
	else
		visibility |= VIS_PUBLIC;

	return visibility;
}

int CSharpContext::get_visibility_by_var(const CSP::VarNode* var_invoker_object, int visibility)
{
	string return_type = var_invoker_object->get_type();
	auto nodes = csc->get_nodes_by_expression(return_type);

	if (nodes.size() == 0)
		return VIS_NONE;

	CSP::TypeNode* type_of_var = (CSP::TypeNode*)nodes.front();

	visibility &= ~VIS_STATIC; // assure to disable static (because on object)
	visibility = csc->get_visibility_by_invoker_type(type_of_var, visibility);

	return visibility;
}