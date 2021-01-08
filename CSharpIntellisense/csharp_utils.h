#ifndef CSHARP_UTILS_H
#define CSHARP_UTILS_H

#include <string>
#include <vector>

using namespace std;

#define ASSURE_CTX(def_ret) \
	if (cinfo.ctx_cursor == nullptr || cinfo.error != 0) \
		return def_ret;

#define MERGE_LISTS(lst1,lst2) \
	for (auto x : lst2) \
		lst1.push_back(x);

#define MERGE_SETS(col1,col2) \
	for (auto x : col2) \
		col1.insert(x);

#define csc CSharpContext::instance()

#define MERGE_LISTS_COND(lst1,lst2,cond) \
	for (auto x : lst2) \
		if cond lst1.push_back(x);

#define IS_UNIQUE(lst1) (is_unique(lst1,x))

#define CHILD_COND (x->name == name || name == "")
#define NAME_COND (x->name == name)

#define VISIBILITY_COND ( \
	(visibility & VIS_IGNORE) \
	|| \
	( \
		(!!(visibility & VIS_STATIC) == x->is_static()) \
		&& \
		(((visibility & VIS_PRIVATE) && (x->is_private() || x->is_protected() || x->is_public())) \
		|| ((visibility & VIS_PROTECTED) && (x->is_protected() || x->is_public())) \
		|| ((visibility & VIS_PUBLIC) && x->is_public())) \
	) \
)

#define FOREACH_DELETE(collection) \
	for (auto x : collection) \
		delete x;

#define IS_TYPE_NODE(node) \
	(node->node_type == CSP::Node::Type::CLASS) \
	|| (node->node_type == CSP::Node::Type::STRUCT) \
	|| (node->node_type == CSP::Node::Type::INTERFACE)


const int VIS_NONE         = 0;
const int VIS_PUBLIC       = 1;       // public
const int VIS_PROTECTED    = 1 << 1;  // protected
const int VIS_PRIVATE      = 1 << 2;  // private
const int VIS_STATIC       = 1 << 3;  // static
const int VIS_PPP          = 7;       // public, protected & private
const int VIS_PP           = 3;       // public & protected
const int VIS_CONSTRUCT    = 1 << 4;  // constructors
const int VIS_IGNORE       = 1 << 31; // everything

string substr(string s, char c);
bool contains(string s, char c);
bool contains(string s, string substr);
vector<string> split_func(string s);
string join_vector(const vector<string> &v, const string &joiner);
vector<string> split(string s, char c);

string read_file(string path);



#endif // CSHARP_UTILS_H
