#include "csharp_utils.h"
#include <string>
#include <fstream>
#include <sstream>
string substr(string s, char c) {
	string res;
	for (int i = 0; i < s.size() && s[i] != c; i++)
		res += s[i];
	return res;
}

bool contains(string s, char c) {
	for (int i = 0; i < s.size(); i++)
		if (s[i] == c)
			return true;
	return false;
}

bool contains(string s, string substr, int offset)
{
	// offset != -1 means force found to be on special pos
	auto found = s.find(substr);
	if (found != string::npos) {

		if (offset == -1)
			return true;
		else if (found == offset)
			return true;
	}

	return false;
}

vector<string> split(string s, char c) {

	vector<string> res;
	string word;

	bool end = false;
	for (int i = 0; !end; i++)
	{
		if (s[i] == '\0' || s[i] == c) {
			if (!word.empty()) {
				res.push_back(word);
				word.clear();
			}
			if (s[i] == '\0')
				end = true;
		}
		else {
			word += s[i];
		}
	}

	return res;


}

// eg. f1(int,bool) or f2(int,double,  <-- not totally resolved
vector<string> split_func(string s) {
	vector<string> res;
	string word;

	bool array_type_mode = false;
	for (int i = 0; s[i] != '\0'; i++)
	{
		if (s[i] == '(' || s[i] == ')' || s[i] == ' ') {
			if (!word.empty()) {
				res.push_back(word);
				word.clear();
			}
		}
		else if (s[i] == '[') {
			array_type_mode = true;
			word += s[i];
		}
		else if (s[i] == ']') {
			array_type_mode = false;
			word += s[i];
		}
		else if (s[i] == ',') {
			if (array_type_mode) {
				word += s[i];
			}
			else if (!word.empty()) {
				res.push_back(word);
				word.clear();
			}
		}
		else {
			word += s[i];
		}
	}

	return res;
}

string join_vector(const vector<string> &v, const string &joiner)
{
	int n = v.size();
	if (n == 0) return "";

	string res = v[0];
	for (int i = 1; i < n; i++)
		res += joiner + v[i];

	return res;
}

string read_file(string path) {

	ifstream file;
	file.open(path);

	stringstream str_stream;
	str_stream << file.rdbuf();
	string code = str_stream.str();

	file.close();

	return code;
}
