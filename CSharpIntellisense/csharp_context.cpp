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

		//delete it->second;
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

	// update cursor info only if found in current file!
	if (parser.cinfo.ctx_cursor != nullptr) {
		cinfo = parser.cinfo;
		cinfo.ctx_file = node;
	}

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

	for (auto f : files) {
		CSP::FileNode* root = f.second;
		//for (auto s : cinfo.ctx_file->node_shortcuts)
		for (auto s : root->node_shortcuts)
			if (substr(s.first, '(') == shortname)
				res.push_back(s.second);
	}

	return res;
}

void CSharpContext::print_shortcuts()
{
	cout << " ----- Shortcuts -----" << endl;
	for (auto f : files) {
		cout << "File: " << f.second->name << endl;
		for (auto shortcut : f.second->node_shortcuts) {
			cout << shortcut.first << endl;
		}
	}

	cout << endl;
}

void CSharpContext::print_visible() {

	cout << " ----- Visible in context ----- " << endl;

	int prev_visibility = this->visibility;
	this->visibility = VIS_IGNORE;

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


	this->visibility = prev_visibility;
}

void CSharpContext::print() {

	cout << " ----- Print ----- " << endl;
	for (auto f : files)
		f.second->print();

	if (cinfo.ctx_cursor != nullptr) {
		cout << "Found cursor at: " << cinfo.cursor_line << " " << cinfo.cursor_column << endl;
		cout << "Completion type is: " << CSharpParser::completion_type_name(cinfo.completion_type) << endl;
		cout << "Current expression is: " << cinfo.completion_expression << endl;
	}

	cout << endl;
}

void CSharpContext::register_provider(ICSharpProvider* provider)
{
	provider_registered = true;
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

	// lexicographic sort
	sort(options.begin(), options.end(),
		[](const pair<Option, string> &o1, const pair<Option, string> &o2)
			{ return strcmp(o1.second.c_str(), o2.second.c_str()) < 0; });

	// remove doubles
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

// N1.C1.M1(...) --> zwróci typ, jaki zwraca M1
// N1.C1.P1 --> zwróci typ, jakiego jest ta w³aœciwoœæ
// N1.C1.C2 --> zwróci N1
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

	// is array?
	if (contains(expr, '[')) {
		return map_array_to_type(expr, ret_wldc);
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

// na wejœciu dostaje N1.C1.X[,,,,] -> zwraca to jakiego typu jest X (to X, które pasuje to tego wymiaru)
string CSharpContext::map_array_to_type(string array_expr, bool ret_wldc)
{
	string expr = array_expr;

	// convention:
	// last '[]' is application
	//
	// x[] -> x is variable of some type of rank 1 and is applied to []
	// x[,,] -> x is variable of some type of rank 3 and is applied to [,,]
	// x[][] -> x[] is type and is a applied to []
	// x[,,][,,] -> x[,,] is type and is applied to [,,]

	int rank = CSP::remove_array_type(expr);

	// 1. array_expr is a variable
	if (!contains(expr, '[')) {
		auto nodes = get_nodes_by_expression(expr);
		for (auto x : nodes)
		{
			if (x->node_type == CSP::Node::Type::VAR
				|| x->node_type == CSP::Node::Type::PROPERTY)
			{
				string x_type = ((CSP::VarNode*)x)->get_type();
				int x_rank = CSP::remove_array_type(x_type);
				if (x_rank == rank) return x_type;
			}
			else if (IS_TYPE_NODE(x)) { // constructor
				return array_expr; // this is a type
			}
		}
	}

	// 2. array_expr is array type (eg. int[,])
	else {
		int x_rank = CSP::remove_array_type(expr);
		if (x_rank == rank) return expr;
	}

	if (ret_wldc) return CSP::wldc;
	return array_expr;
}

string CSharpContext::map_function_to_type(string func_def, bool ret_wldc)
{
	vector<string> splitted = split_func(func_def);

	string func_name = splitted[0];
	int n = splitted.size();

	int prev_visibility = this->visibility;
	this->visibility |= VIS_PPP;
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
	//  (dla funkcji zamkniêtych)
	// 1. funkcje, które maj¹ dok³adnie wszystkie argumenty i pasuj¹ idealnie
	// 2. funkcje, które maj¹ dok³adnie wszystkie argumenty i pasuj¹ przez koercjê
	//  (dla funkcji otwartych)
	// 3. funkcje, które maj¹ wiêcej argumentów i pasuj¹ idealnie
	// 4. funkcje, które maj¹ wiêcej argumentów i pasuj¹ przez koercjê


	// 1
	// find method that exactly match
	for (auto x : nodes) {

		int m = x->arguments.size();

		// przyrównywana metoda musi mieæ tyle argumentów ile func_def (n-1)
		if (m == n - 1)
		{
			bool success = true;
			for (int i = 1; i < n; i++) {

				string this_func_arg = splitted[i];
				string node_func_arg = x->arguments[i - 1]->type;

				if (!types_are_identical(this_func_arg,node_func_arg)) {
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

		// przyrównywana metoda musi mieæ przynajmniej tyle argumentów ile func_def (n-1)
		if (m > n - 1)
		{
			bool success = true;
			for (int i = 1; i < n; i++) {

				string this_func_arg = splitted[i];
				string node_func_arg = x->arguments[i - 1]->type;

				if (!types_are_identical(this_func_arg,node_func_arg)) {
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
	this->visibility |= VIS_PPP;

	int pos = 0;
	string res = simplify_expr_tokens(tokens, pos);

	this->visibility = prev_visibility;

	return res;
}

// dedukuje typ jakiegoœ wyra¿enia, korzystaj¹c z informacji, które ma (np o namespaceach i klasie w której rozwa¿amy to wyra¿enie)
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
				
				// EOF detected, not closed function, don't map to type, this is a simplified expression
				if (type == "") { 					
					// restore
					this->include_constructors = prev_include_constructors;
					return res;
				}

				// uda³o siê sparsowaæ jakiœ argument
				if (type != ")" && type != ",")
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

			// now it is object, so non-static
			visibility |= VIS_NONSTATIC;
			visibility &= ~VIS_STATIC;
			
		}

		// accessing from the array - don't care what is inside, just skip it but count ','
		else if (tokens[pos].type == CST::TK_BRACKET_OPEN) {
			res += "[";
			pos++;
			int depth = 1;
			int par_depth = 0;
			for (; tokens[pos].type != CST::TK_EOF; pos++) {
				if (tokens[pos].type == CST::TK_BRACKET_OPEN)
					depth++;
				else if (tokens[pos].type == CST::TK_BRACKET_CLOSE) {
					depth--;
					if (depth == 0) break;
				}
				else if (tokens[pos].type == CST::TK_PARENTHESIS_OPEN) {
					par_depth++;
				}
				else if (tokens[pos].type == CST::TK_PARENTHESIS_CLOSE) {
					par_depth--;
				}
				else if (tokens[pos].type == CST::TK_COMMA) {
					if (par_depth == 0) // inside [] can be a function invokation with commas
						res += ','; // comma must be outside function invokation!
				}

				// else ignore

			}

			res += "]";
			pos++;

			res = map_to_type(res);

			// now it is object, so non-static
			visibility |= VIS_NONSTATIC;
			visibility &= ~VIS_STATIC;
		}

		// outer parenthesis close
		else if (tokens[pos].type == CST::TK_PARENTHESIS_CLOSE) {
			end = true;
			if (res.empty()) {
				res = ")"; // konwencja - zwracamy taki znak, jesli koniec argumentu przez nawias zamykajacy
			}
		}

		// outer comma
		else if (tokens[pos].type == CST::TK_COMMA) {
			end = true;
			if (res.empty()) {
				res = ","; // konwencja - zwracamy taki znak, jesli koniec argumentu przez przecinek
			}
		}

		// keyword new
		else if (tokens[pos].type == CST::TK_KW_NEW) {
			this->include_constructors = true;
			pos++;
		}

		// is operator?
		else if (CSharpLexer::is_operator(tokens[pos].type)) {

			// totally ignore, first argument is decider:
			// int    (+) int    = int
			// int    (+) string = string
			// string (+) string = string
			// double (+) int    = double
			// int    (+) double = int -> TODO in the future: resolve separately args and consider some special cases
			
			int par_depth = 0;
			int bra_depth = 0;
			for (bool ignore = true; pos < n && ignore; pos++) {
				switch (tokens[pos].type) {

				case CST::TK_PARENTHESIS_OPEN: par_depth++; break;
				case CST::TK_BRACKET_OPEN: bra_depth++; break;

				case CST::TK_COMMA:
				case CST::TK_PARENTHESIS_CLOSE:
				case CST::TK_BRACKET_CLOSE:
					if (par_depth == 0 && bra_depth == 0) { ignore = false; pos--; } // end loop
					else if (tokens[pos].type == CST::TK_PARENTHESIS_CLOSE) par_depth--;
					else if (tokens[pos].type == CST::TK_BRACKET_CLOSE) bra_depth--;
					break;
				}
			}
		}

		// sth else
		else {

			res += tokens[pos].to_string(true);
			pos++;
		}
	}

	if (res != ")" && res != ",")
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

// symulacja DFS przy przechodzeniu po drzewie wêz³ów - szukamy wszystkiego co pasuje
// daje node'y takie, z jakim visibility przysz³o, ustawia nowe visibility dla dalszych wywolan rekurencyjnych
list<CSP::Node*> CSharpContext::get_nodes_by_simplified_expression_rec(CSP::Node* invoker, const vector<CSharpLexer::TokenData> &tokens, int pos) 
{
	// 1. . X   - konkretne dziecko
	// 2. . EOF - wszystkie dzieci
	// 3. ( - wywo³ane na funkcji

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

			if (!(IS_TYPE_NODE(x))) { // no longer static
				visibility &= ~VIS_STATIC;
				visibility |= VIS_NONSTATIC;
			}
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
		else {

		}

		// "" means any child -> see SCAN_AND_ADD macro
		auto children = invoker->get_members("", visibility);
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


void CSharpContext::scan_tokens_array_type(const vector<CSharpLexer::TokenData>& tokens, string& type, int& pos) {

	// scans: [,,,,,]
	// returns new pos behind the ']'

	if (tokens[pos].type == CST::TK_BRACKET_OPEN) {
		type += "[";
		for (pos = 2; tokens[pos].type != CST::TK_EOF; pos++) {
			if (tokens[pos].type == CST::TK_COMMA) type += ",";
			else if (tokens[pos].type == CST::TK_BRACKET_CLOSE) { type += "]"; pos++; break; }
			else { type = ""; break; } // failed
		}
	}
	
}

bool CSharpContext::types_are_identical(const string & type1, const string & type2)
{
	if (type1 == type2)
		return true;

	int c1 = CSharpParser::get_base_type_category(type1);
	int c2 = CSharpParser::get_base_type_category(type2);

	if (c1 != -1 && c2 != -1 && c1 == c2)
		return true;

	return false;
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
	// N1.C2.  -> zwraca listê memberów C2
	// N1.C2.Foo( -> zwraca wszystkie funkcje 'Foo' z klasy C2

	// Hence this is simplified expression, there is no ')' token inside.
	// Posible cases of first token:
	//   1. this
	//   2. base
	//   3. literal
	//   4. class-name or namespace-name
	//   5. variable
	//   6. (known by provider)
	//  
	//   Note: if '[' in expression then:
	//     a) array type - eg: int[] or X[,,] or System.Int32[,] - then resolve typenode
	//     b) expression - eg: N1.C1.P1[22+^| - then IGNORE it, do not help inside '[]'

	int prev_visibility = this->visibility;

	list<CSP::Node*> res;
	switch (tokens[0].type) {

	// --- 1: this ---
	case CST::TK_KW_THIS: {
		CSP::TypeNode* thisClass = csc->cinfo.ctx_class;
		if (thisClass != nullptr) {
			visibility &= ~VIS_STATIC;
			visibility |= VIS_NONSTATIC;
			auto nodes = get_nodes_by_simplified_expression_rec(thisClass, tokens, 1);
			MERGE_LISTS(res, nodes);
		}
		break;
	}

	// --- 2: base ---
	case CST::TK_KW_BASE: {
		
		CSP::TypeNode* thisClass = csc->cinfo.ctx_class;
		if (thisClass != nullptr) {

			for (string type_name : thisClass->base_types) {

				auto nodes = get_nodes_by_expression(type_name);
				for (auto x : nodes) {

					if (x->node_type == CSP::Node::Type::CLASS
						|| x->node_type == CSP::Node::Type::STRUCT
						|| x->node_type == CSP::Node::Type::INTERFACE)
					{
						visibility &= ~VIS_STATIC;
						visibility |= VIS_NONSTATIC;
						auto nodes = get_nodes_by_simplified_expression_rec(x, tokens, 1);
						MERGE_LISTS(res, nodes);
					}
				}
			}
		}

		break;
	}


	// --- 3: literal or base type ---
	case CST::TK_LT_CHAR:
	case CST::TK_LT_INTEGER:
	case CST::TK_LT_REAL:
	case CST::TK_LT_STRING:
	case CST::TK_LT_INTERPOLATED: {		
		break; // literals are not part of long expressions
	}

	// ----- ----- ----- ----- -----
	CASEBASETYPE
	case CST::TK_IDENTIFIER: {

		// --- Base type ? int, Int32, ...
		if (CSharpParser::is_base_type(tokens[0].to_string())) {

			if (_provider != nullptr) {
				string type = tokens[0].to_string(true);

				int pos = 1;
				scan_tokens_array_type(tokens, type, pos);

				CSP::TypeNode* tn = _provider->resolve_base_type(type);
				if (tn == nullptr) return res;
				auto nodes = get_nodes_by_simplified_expression_rec(tn, tokens, pos);
				MERGE_LISTS(res, nodes);
			}

		}

		list<CSP::Node*> start;
		int pos = 1;

		// --- A: is type? ---
		bool array_type_mode = false;
		string type = tokens[0].to_string(true);
		scan_tokens_array_type(tokens, type, pos);
		int rank = 0;
		if (contains(type, '[')) {
			array_type_mode = true;
			auto array_types = find_by_shortcuts(type);
			if (!array_types.empty()) {
				res.push_back(array_types.front());
				break; // goto finish
			}
			// else -> find by identifier and construct array type
			rank = CSP::compute_rank(type);
		}

		// --- 4: visible in shortcuts ---
		auto found_by_shortcuts = find_by_shortcuts(tokens[0].data);
		MERGE_LISTS(start, found_by_shortcuts);

		// --- 5: variable or resolvable identifier
		auto found_by_visible = get_visible_in_ctx_by_name(tokens[0].data);
		MERGE_LISTS(start, found_by_visible);

		// --- 6: find in provider ---
		if (start.empty()) {
			if (_provider != nullptr) {
				
				string class_name = tokens[0].data;
				auto found_in_provider = _provider->find_class_by_name(class_name);

				if (found_in_provider.empty()) {

					for (pos = 1; pos + 1 < (int)tokens.size(); pos += 2) {

						int X = 2; // TODO: to dzia³a aktualnie tylko dla tablic jednowymiarowych!
						if (tokens[pos].type == CSharpLexer::Token::TK_PERIOD
							&& tokens[pos + 1].type == CSharpLexer::Token::TK_IDENTIFIER) {
							class_name += "." + tokens[pos + 1].data;
							if (pos + 3 < tokens.size()) {
								if (tokens[pos + 2].type == CST::TK_BRACKET_OPEN
									&& tokens[pos + 3].type == CST::TK_BRACKET_CLOSE) {
									class_name += "[]";
									X = 4;
								}
							}
							auto fip = _provider->find_class_by_name(class_name);
							MERGE_LISTS(found_in_provider, fip);
							if (!fip.empty()) {
								pos += X;
								break;
							}
						}
						else {
							break;
						}

					}
				}

				MERGE_LISTS(start, found_in_provider);
			}
		}


		// IS IT ARRAY TYPE?
		if (array_type_mode) {
			for (auto x : start) {
				if (IS_TYPE_NODE(x)) {
					CSP::TypeNode* new_array_type = ((CSP::TypeNode*)x)->create_array_type(rank);
					csc->cinfo.ctx_file->node_shortcuts.insert({ new_array_type->name,new_array_type }); // add to shortcuts
					res.push_back(new_array_type);
				}
			} break; // goto restore
		}

		// FIND FURTHER
		for (auto x : start) {

			int pv = visibility;

			if ((visibility & VIS_STATIC) > 0 && (visibility & VIS_STATIC) > 0) {
				// decide - static or non static? - it means it wasn't simplified yet
				if (IS_TYPE_NODE(x)) {
					// static
					visibility &= ~VIS_NONSTATIC;
					visibility |= VIS_STATIC;
				}
				else {
					// non static
					visibility &= ~VIS_STATIC;
					visibility |= VIS_NONSTATIC;
				}
			}

			auto nodes = get_nodes_by_simplified_expression_rec(x, tokens, pos);
			MERGE_LISTS(res, nodes);

			visibility = pv;
			
		}
	}
	}
	
	// restore
	this->visibility = prev_visibility;

	return res;
}

list<CSP::Node*> CSharpContext::get_nodes_by_expression(string expr)
{
	ASSURE_CTX(list<CSP::Node*>());

	int prev_visibility = this->visibility;
	this->visibility = 0;
	this->visibility |= VIS_PPP;
	this->visibility |= VIS_STATIC;
	this->visibility |= VIS_NONSTATIC;
	this->visibility |= VIS_CONSTRUCT;

	if (!contains(expr, "new ")) // new + space
		this->visibility &= ~VIS_CONSTRUCT;
	else
		expr = expr.substr(4, expr.length() - 4);
	
	// jeœli wyra¿enie nie jest proste, to trzeba je uproœciæ
	// nie proste = jakieœ wywo³anie funkcji lub jakieœ tablice
	if (contains(expr, ')') || contains(expr,']')) {
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
	// odpowiednie argumenty albo s¹ dobrego typu, albo mo¿na zrobiæ koercjê

	// n-1 - ilosc dotychczas napisanych argumentow przez uzytkownika
	// m   - ilosc argumentow rozwazanej funkcji

	vector<string> splitted = split_func(function_call);

	string func_name = splitted[0];
	int n = splitted.size();

	if (func_name != method->name)
		return false;

	int m = method->arguments.size();

	// metoda musi mieæ przynajmniej tyle argumentow ile to, do czego dopasowujemy
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

	if (CSharpParser::is_base_type(expr)) {
		if (_provider != nullptr) {

			return _provider->resolve_base_type(expr);

		}
	}

	if (_provider != nullptr) {
		auto nodes2 = _provider->find_class_by_name(expr);
		if (nodes2.size() > 0)
			return nodes2.front();
	}

	return nullptr;
}

// zmienia widzialnoœæ public private protected
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

	// assure to disable static (because on object)
	visibility &= ~VIS_STATIC;
	visibility |= VIS_NONSTATIC;
	visibility = csc->get_visibility_by_invoker_type(type_of_var, visibility);

	return visibility;
}

list<CSP::Node*> CSharpContext::get_children_of_base_type(string base_type, string child_name) const
{
	if (_provider != nullptr) {

		auto node = _provider->resolve_base_type(base_type);
		if (node != nullptr) {
			auto res = node->get_members(child_name, visibility);
			return res;
		}


	}

	return list<CSP::Node*>();
}
