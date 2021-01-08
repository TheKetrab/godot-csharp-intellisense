#include <iostream>
#include <fstream>
#include <sstream>
#include <iostream>

#include "csharp_lexer.h"
#include "csharp_parser.h"
#include "csharp_context.h"
#include "csharp_utils.h"

using namespace std;

string read_file(string path) {

	ifstream file;
	file.open(path);

	stringstream str_stream;
	str_stream << file.rdbuf();
	string code = str_stream.str();

	file.close();

	return code;
}

int main(int argc, const char* argv[]) {

	// input: filename x code
	vector<pair<string, string>> files;
	
	if (argc == 1) {

		string filename = "x.cs";
		string code = read_file(filename);

		files.push_back({ filename,code });

	}
	else {

		string paths = argv[1];
		auto splitted = split(paths, ' ');

		for (auto x : splitted) {

			string filename = x;
			string code = read_file(filename);

			files.push_back({ filename,code });
		}

	}

	/*
	cout << "----- ----- -----" << endl;
	cout << "  Code: " << endl;
	cout << "----- ----- -----" << endl;
	cout << code << endl;
	*/
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
	/*
	CSharpLexer lexer(code);
	lexer.tokenize();

	cout << "----- ----- -----" << endl;
	cout << "  Tokens: " << endl;
	cout << "----- ----- -----" << endl;
	lexer.print_tokens();
	*/

	// PARSING
	for (auto x : files) {

		string filename = x.first;
		string code = x.second;
		csc->update_state(code, filename);

	}



	// PRINT
	//csc->print(); // print structure of files

	//csc->print_visible();

	//cout << endl << endl << endl;
	//cout << "OPTIONS:" << endl;
	csc->print_options();
	//cout << "deduced type = " << res << endl;

	//CSharpContext::instance()->print_shortcuts();

	return 0;
}