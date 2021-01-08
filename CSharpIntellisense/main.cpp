#include <iostream>
#include <fstream>
#include <sstream>
#include <iostream>

#include "csharp_lexer.h"
#include "csharp_parser.h"
#include "csharp_context.h"
#include "csharp_utils.h"

#define FILENAME "x.cs"

#define HEADER(title) \
	cout << "----- ----- -----" << endl; \
	cout << "  " << title << endl; \
	cout << "----- ----- -----" << endl;

using namespace std;

// input convention:
//
// -debug     -> only FILENAME is parsed
// -f [files] -> files separated by space: -f "file1 file2 file3 ..."
// -details   -> show details info about context
// -ascii     -> prints ascii codes for every char in all files
// -code      -> prints read code-string of every file
// -tokens    -> prints generated tokens for every file
int main(int argc, const char* argv[]) {

	string paths;
	bool debug = false;
	bool details = false;
	bool ascii = false;
	bool codemode = false;
	bool tokens = false;

	// ----- ----- SCAN INPUT ----- -----

	for (int i = 1; i < argc; i++) {

		string arg = argv[i];
		if (arg == "-details")
			details = true;
		else if (arg == "-debug")
			debug = true;
		else if (arg == "-f") {
			if (i + 1 < argc) {
				paths = argv[i + 1];
				i++;
			}
		}
		else if (arg == "-ascii")
			ascii = true;
		else if (arg == "-code")
			codemode = true;
		else if (arg == "-tokens")
			tokens = true;
	}

	// ----- ----- READ FILES ----- -----

	// files to parse: filename x code
	vector<pair<string, string>> files;

	if (debug) // force paths to debug mode
		paths = FILENAME;

	auto splitted = split(paths, ' ');
	for (auto x : splitted) {
		string filename = x;
		string code = read_file(filename);
		files.push_back({ filename,code });
	}

	// ----- ----- MODE PRINTS ----- -----

	if (codemode) {
		HEADER("Code:");
		for (auto x : files) {
			if (files.size() > 1)
				cout << " ----- " << x.first << " -----" << endl;
			cout << x.second << endl << endl;
		} cout << endl;
	}

	if (ascii) {
		HEADER("ASCII:");
		for (auto x : files) {
			if (files.size() > 1)
				cout << " ----- " << x.first << " -----" << endl;
			string code = x.second;
			for (int i = 0; i < code.size(); i++)
				cout << (int)code[i] << " ";
		} cout << endl;
	}

	if (tokens) {
		HEADER("Tokens:");
		for (auto x : files) {
			if (files.size() > 1)
				cout << " ----- " << x.first << " -----" << endl;
			CSharpLexer lexer(x.second);
			lexer.tokenize();
			lexer.print_tokens();
		} cout << endl;
	}

	// ----- ----- ADD FILES ----- -----

	for (auto x : files) {
		string filename = x.first;
		string code = x.second;
		csc->update_state(code, filename);
	}

	// ----- ----- PRINT DETAILS ----- -----

	if (details) {
		csc->print(); // structure of filenodes
		csc->print_visible();
		csc->print_shortcuts();

		cout << " ----- OPTIONS ----- " << endl;
	}

	// ----- ----- PRINT OPTIONS ----- -----
	csc->print_options();

	return 0;
}