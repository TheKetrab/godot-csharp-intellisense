#include <iostream>
#include <fstream>
#include <sstream>
#include <iostream>

#include "csharp_lexer.h"
#include "csharp_parser.h"
#include "csharp_context.h"
#include "csharp_utils.h"

using namespace std;
int main(int argc, const char* argv[]) {

	string code;
	string filename;

	if (argc == 1) {

		filename = "x.cs";

		ifstream file;
		file.open(filename);

		stringstream str_stream;
		str_stream << file.rdbuf();
		code = str_stream.str();

	}
	else {

		code = argv[1];
		filename = "input.cs";

	}

	
	cout << "----- ----- -----" << endl;
	cout << "  Code: " << endl;
	cout << "----- ----- -----" << endl;
	cout << code << endl;

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
	CSharpLexer lexer(code);
	lexer.tokenize();

	cout << "----- ----- -----" << endl;
	cout << "  Tokens: " << endl;
	cout << "----- ----- -----" << endl;
	lexer.print_tokens();


	// PARSING
	csc->update_state(code, filename);



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