#include <iostream>
#include <fstream>
#include <sstream>
#include <iostream>

#include "csharp_lexer.h"
#include "csharp_parser.h"

using namespace std;
int main() {

	ifstream file;
	file.open("script2.cs");

	stringstream str_stream;
	str_stream << file.rdbuf();
	string str = str_stream.str();
	
	cout << "----- ----- -----" << endl;
	cout << "  Code: " << endl;
	cout << "----- ----- -----" << endl;
	cout << str << endl;

	// ASCII PRINT
	cout << "----- ----- -----" << endl;
	cout << "  ASCII print: " << endl;
	cout << "----- ----- -----" << endl;
	for (int i = 0; i < str.size(); i++) {
		std::cout << (int)str[i] << " ";
	}


	CSharpLexer lexer;
	lexer.set_code(str);
	lexer.tokenize();

	cout << "----- ----- -----" << endl;
	cout << "  Tokens: " << endl;
	cout << "----- ----- -----" << endl;

	lexer.print_tokens();
	
	CSharpParser parser;
	parser.set_tokens(lexer.tokens);
	parser.parse();


	cout << endl << endl << endl;
	return 0;
}