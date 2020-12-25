#include "csharp_context.h"

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

	CSharpParser parser(code);
	CSP::FileNode* node = parser.parse();
	node->name = filename;
	node->identifiers = parser.identifiers;
	
	files.insert({ filename,node });

	cinfo = parser.cinfo;
	cinfo.ctx_file = node;

}

CSP::CompletionType CSharpContext::get_completion_type()
{
	return cinfo.completion_type;
}

CSP::Node * CSharpContext::find_by_shortcuts(string shortname)
{
	for (auto s : cinfo.ctx_file->node_shortcuts)
		if (s.first == shortname)
			return s.second;

	return nullptr;
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

void CSharpContext::print_visible() {

	cout << endl;
	cout << " ----- --------- ----- " << endl;
	cout << "  VISIBLE IN CONTEXT:" << endl;
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
			CSP::TypeNode* tn = get_type_by_name(x->return_type);
			if (tn == nullptr) {
				cout << x->return_type << " [not found]" << endl;
			} else {
				cout << tn->fullname() << endl;
			}
		}

	}

	cout << "VARIABLES:" << endl;
	for (auto x : get_visible_vars()) {

		cout << "  " << x->fullname() << " : ";

		if (CSP::is_base_type(x->type))
			cout << x->type << endl;
		else {
			CSP::TypeNode* tn = get_type_by_name(x->type);
			if (tn == nullptr) {
				cout << x->type << " [not found]" << endl;
			}
			else {
				cout << tn->fullname() << endl;
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
		cout << " member" << endl;
		list<CSharpParser::Node*> nodes = get_nodes_by_expression(cinfo.completion_expression);
		for (auto x : nodes)
			options.push_back({ node_type_to_option(x->node_type), x->name });


		break;
	}
	case (CSharpParser::CompletionType::COMPLETION_CALL_ARGUMENTS): {
		
		// creates list of signatures of functions which fit the name

		string func_name = cinfo.completion_info_str;
		int cur_arg = cinfo.completion_info_int;

		// TODO get functions by name
		// TODO add signature of function
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
string CSharpContext::map_to_type(string expr) {

	// is base type?
	if (CSP::is_base_type(expr))
		return expr;

	// find in user's files
	for (auto f : files) {
		auto &m = f.second->node_shortcuts;
		auto it = m.find(expr);
		
		// found in shortcuts?
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

		// try to find by fullname
		else {
			CSP::Node* n = get_by_fullname(expr);
			if (n != nullptr) {
				switch (n->node_type) {
				case CSP::Node::Type::ENUM:
				case CSP::Node::Type::NAMESPACE:
				case CSP::Node::Type::CLASS:
				case CSP::Node::Type::INTERFACE:
					return n->fullname();
				case CSP::Node::Type::METHOD:
					return ((CSP::MethodNode*)n)->get_return_type();
				case CSP::Node::Type::PROPERTY:
				case CSP::Node::Type::VAR:
					return ((CSP::VarNode*)n)->get_return_type();
				}
			}
		}

	}

	// TODO: find in external assemblies

	// not found -> return wildcart "?"
	return expr; // TODO

}

// dedukuje typ jakiegoœ wyra¿enia, korzystaj¹c z informacji, które ma (np o namespaceach i klasie w której rozwa¿amy to wyra¿enie)
string CSharpContext::deduce_type(const string expr)
{
	CSharpLexer lexer(expr);
	lexer.tokenize();
	auto tokens = lexer.get_tokens();

	int pos = 0;
	string res = deduce_type(tokens, pos);

	return res;
}

// skipuje pozycje o te tokeny, które s¹ niepotrzebne
void CSharpContext::skip_redundant_prefix(const vector<CSharpLexer::TokenData> &tokens, int &pos)
{
	vector<string> redundant_prefix;

	bool end = false;
	while (!end) {

		string tk = tokens[pos].to_string(true);
		if (tokens[pos].type == CST::TK_IDENTIFIER
			&& tokens[pos + 1].type == CST::TK_PERIOD)
		{
			redundant_prefix.push_back(tk);
			//if (ctx->is_visible(redundant_prefix)) { TODO: ctx jest niedokonczonym obiektem... jakis blad przy parsowaniu
			//if (ctx_namespace->is_visible(redundant_prefix)) {
			if (get_type_by_name(tk) != nullptr) {
				pos += 2;
			} else if (cinfo.ctx_file->is_visible(redundant_prefix)) {
				pos += 2;
			}
			else {
				end = true;
			}

		} else end = true;
	}

}

// additionally: remove redundant - dodatkowo usuwa niepotrzebne: np jeœli N1.N2.C1 a jest to u¿ywane w klasie C1 albo s¹ otwarte te namespace'y (klauzul¹ using) to nie dodawaj do typu
string CSharpContext::deduce_type(const vector<CSharpLexer::TokenData> &tokens, int &pos)
{
	string res;
	int n = tokens.size();

	skip_redundant_prefix(tokens,pos);

	bool end = false;
	while (pos < n && !end) {

		//function invokation
		if (tokens[pos].type == CST::TK_PARENTHESIS_OPEN) {

			pos++;
			res += "(";

			// parse arguments' types
			while (pos < n) {
				string type = deduce_type(tokens, pos);
				if (type == "EOF") return res;
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
			break;

		}

		// outer parenthesis close
		else if (tokens[pos].type == CST::TK_PARENTHESIS_CLOSE) {
			end = true;
		}

		// outer comma
		else if (tokens[pos].type == CST::TK_COMMA) {
			end = true;
		}

		// sth else
		else {

			res += tokens[pos].to_string(true);
			pos++;
		}
	}

	res = map_to_type(res);
	return res;
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

list<CSP::NamespaceNode*> CSharpContext::get_visible_namespaces()
{
	list<CSP::NamespaceNode*> lst;
	
	for (auto x : cinfo.ctx_cursor->get_visible_namespaces())
		lst.push_back(x);
	
	// TODO: append visible namespaces from assembly_provider

	return lst;
}

list<CSP::TypeNode*> CSharpContext::get_visible_types()
{
	list<CSP::TypeNode*> lst;

	for (auto x : cinfo.ctx_cursor->get_visible_types())
		lst.push_back(x);

	// TODO: append visible types from assembly_provider

	return lst;
}

list<CSP::MethodNode*> CSharpContext::get_visible_methods()
{
	list<CSP::MethodNode*> lst;

	for (auto x : cinfo.ctx_cursor->get_visible_methods())
		lst.push_back(x);

	// TODO: append visible methods from assembly_provider

	return lst;
}

list<CSP::VarNode*> CSharpContext::get_visible_vars()
{
	list<CSP::VarNode*> lst;

	for (auto x : cinfo.ctx_cursor->get_visible_vars())
		lst.push_back(x);

	// TODO: append visible vars from assembly_provider
	
	return lst;
}

list<string> CSharpContext::get_visible_labels() {

	list<string> lst;

	for (auto x : cinfo.ctx_file->labels)
		lst.push_back(x);

	return lst;
}


CSP::TypeNode* CSharpContext::get_type_by_name(string name)
{
	if (this->cinfo.ctx_cursor == nullptr)
		return nullptr;

	// TODO to mo¿na przyspieszyæ szukaj¹c explicite a nie korzystaj¹c z get_visible_types
	// TODO dodaæ cache dla kontekstu (byæ mo¿e wiele razy odwo³ujemy siê do nazwy danego typu)
	auto types = get_visible_types();
	for (auto x : types)
		if (x->name == name)
			return x;

	// not found
	return nullptr;
}

CSP::MethodNode * CSharpContext::get_method_by_name(string name)
{
	auto methods = get_visible_methods();
	for (auto x : methods)
		if (x->name == name)
			return x;

	// not found
	return nullptr;
}

CSP::VarNode* CSharpContext::get_var_by_name(string name)
{
	auto vars = get_visible_vars();
	for (auto x : vars)
		if (x->name == name)
			return x;

	// not found
	return nullptr;
}

CSP::Node* CSharpContext::get_by_fullname(string fullname)
{
	if (cinfo.ctx_cursor == nullptr)
		return nullptr;

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


// cinfo.ctx_expression najpierw trzeba uproscic, a nastepnie wywolac get_nodes_by_expression
list<CSP::Node*> CSharpContext::get_nodes_by_expression(string expr)
{
	// N1.C2.  -> zwraca listê memberów C2
	// N1.C2.Foo( -> zwraca wszystkie funkcje 'Foo' z klasy C2

	CSharpLexer lexer(expr);
	lexer.tokenize();

	auto tokens = lexer.get_tokens();
	int n = tokens.size();

	string start_tok = tokens[0].data;
	CSP::Node* temp = find_by_shortcuts(start_tok);
	
	if (temp == nullptr)
		return list<CSP::Node*>();
	
	int i = 0;
	for (int i = 0; i < n; i++) {

		//temp->get_child()


	}



	return list<CSP::Node*>();
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



// put s2 values into s1
void CSharpContext::my_merge(set<string> &s1, const set<string> &s2) {

	for (auto v : s2)
		s1.insert(v);

}


