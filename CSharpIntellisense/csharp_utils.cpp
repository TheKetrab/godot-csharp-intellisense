#include "csharp_utils.h"
#include <string>

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

bool contains(string s, string substr)
{
	auto found = s.find(substr);
	if (found != string::npos)
		return true;

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
	for (int i = 0; s[i] != '\0'; i++)
	{
		if (s[i] == '(' || s[i] == ',' || s[i] == ')' || s[i] == ' ') {
			if (!word.empty()) {
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

