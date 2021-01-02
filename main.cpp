#include <iostream>
#include <fstream>
#include <sstream>
#include <iostream>

#include "csharp_lexer.h"
#include "csharp_parser.h"
#include "csharp_context.h"
#include "csharp_utils.h"

using namespace std;
int main() {

	string filename = "x.cs";

	ifstream file;
	file.open(filename);

	stringstream str_stream;
	str_stream << file.rdbuf();
	string str = str_stream.str();
	
	cout << "----- ----- -----" << endl;
	cout << "  Code: " << endl;
	cout << "----- ----- -----" << endl;
	cout << str << endl;

	/*
	// ASCII PRINT
	cout << "----- ----- -----" << endl;
	cout << "  ASCII print: " << endl;
	cout << "----- ----- -----" << endl;
	for (int i = 0; i < str.size(); i++) {
		std::cout << (int)str[i] << " ";
	}
	*/

	// LEXING
	CSharpLexer lexer(str);
	lexer.tokenize();

	cout << "----- ----- -----" << endl;
	cout << "  Tokens: " << endl;
	cout << "----- ----- -----" << endl;
	lexer.print_tokens();


	// PARSING
	csc->update_state(str, filename);



	// PRINT
	csc->print();

	csc->print_visible();

	cout << endl << endl << endl;
	cout << "OPTIONS:" << endl;
	csc->print_options();
	//cout << "deduced type = " << res << endl;

	//CSharpContext::instance()->print_shortcuts();

	return 0;
}