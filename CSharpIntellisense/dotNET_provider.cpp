#include "dotNET_provider.h"
#include "csharp_context.h"
#include "csharp_utils.h"

#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <map>

using namespace std;


map<const char*, const char*> DotNETProvider::base_types_map = {

	// base types
	{ "bool",    "System.Boolean" },
	{ "byte",    "System.Byte"    },
	{ "sbyte",   "System.SByte"   },
	{ "char",    "System.Char"    },
	{ "string",  "System.String"  },
	{ "decimal", "System.Decimal" },
	{ "double",  "System.Double"  },
	{ "float",   "System.Single"  },
	{ "int",     "System.Int32"   },
	{ "long",    "System.Int64"   },
	{ "short",   "System.Int16"   },
	{ "uint",    "System.UInt32"  },
	{ "ulong",   "System.UInt64"  },
	{ "ushort",  "System.UInt16"  },
	{ "object",  "System.Object"  },

	// base types - object
	{ "Boolean", "System.Boolean" },
	{ "Byte",    "System.Byte"    },
	{ "SByte",   "System.SByte"   },
	{ "Char",    "System.Char"    },
	{ "String",  "System.String"  },
	{ "Decimal", "System.Decimal" },
	{ "Double",  "System.Double"  },
	{ "Single",  "System.Single"  },
	{ "Int32",   "System.Int32"   },
	{ "Int64",   "System.Int64"   },
	{ "Int16",   "System.Int16"   },
	{ "UInt32",  "System.UInt32"  },
	{ "UInt64",  "System.UInt64"  },
	{ "UInt16",  "System.UInt16"  },
	{ "Object",  "System.Object"  }
};


string DotNETProvider::get_cmd_output(const char* cmd) const {

	array<char, 128> buffer;
	string result;
	unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd, "r"), _pclose);
	if (!pipe) {
		throw runtime_error("popen() failed!");
	}
	while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
		result += buffer.data();
	}
	return result;

}


DotNETProvider::DotNETProvider(string provider_path)
	: provider_path(provider_path)
{ }

list<CSP::TypeNode*> DotNETProvider::find_class_by_name(string name)
{
	list<CSP::TypeNode*> res;

	// try to match
	{
		string the_name = name;

		// find in cache
		for (auto it = cache.begin(); it != cache.end(); it++)
			if (it->first == the_name) {
				res.push_back(it->second);
				return res;
			}

		string query = provider_path;
		query += " -findtype " + the_name;
		string output = get_cmd_output(query.c_str());
		auto splitted = split(output, '\n');
		int n = splitted.size();
		if (n > 0 && splitted[0] == "TRUE")
		{
			CSP::TypeNode* node = create_class_from_info(splitted, the_name);
			if (node != nullptr) {
				cache.insert({ the_name,node });
				res.push_back(node);
			}

			// it was fulltype!
			return res;
		}

	}

	// try to match with using directives
	for (auto nmspc : using_directives) {

		string the_name = (nmspc + "." + name);

		// find in cache
		for (auto it = cache.begin(); it != cache.end(); it++)
			if (it->first == the_name) {
				res.push_back(it->second);
				return res;
			}

		string query = provider_path;
		query += " -findtype " + the_name;
		string output = get_cmd_output(query.c_str());
		auto splitted = split(output, '\n');
		int n = splitted.size();
		if (n > 0 && splitted[0] == "TRUE")
		{
			CSP::TypeNode* node = create_class_from_info(splitted, the_name);
			if (node != nullptr) {
				cache.insert({ the_name,node });
				res.push_back(node);
			}
		}

	}

	return res;
}

CSP::TypeNode* DotNETProvider::resolve_base_type(string base_type)
{
	int rank = CSP::remove_array_type(base_type);

	string name = "";
	for (auto x : base_types_map) {
		if (x.first == base_type)
			name = x.second;
	}

	if (name.empty())
		return nullptr;

	if (rank > 0) {
		CSP::add_array_type(name, rank);
	}

	auto nodes = find_class_by_name(name);
	return nodes.front();

}

bool DotNETProvider::to_base_type(string &t)
{
	map<const char*, const char*> types_map = {
		{ "Boolean"        , "bool" },
		{ "Byte"           , "byte" },
		{ "SByte"          , "sbyte" },
		{ "Char"           , "char" },
		{ "System.String"  , "string" },
		{ "System.Decimal" , "decimal" },
		{ "Double"  	   , "double" },
		{ "Single"  	   , "float" },
		{ "Int32"   	   , "int" },
		{ "Int64"   	   , "long" },
		{ "Int16"          , "short" },
		{ "UInt32"         , "uint" },
		{ "UInt64"         , "ulong" },
		{ "UInt16"         , "ushort" },
		{ "System.Object"  , "object" }
	};

	for (auto x : types_map) {
		if (strcmp(x.first, t.c_str()) == 0) {
			t = x.second;
			return true;
		}
	}
	auto it = types_map.find(t.c_str());
	if (it != types_map.end()) {
		t = it->second;
		return true;
	}

	return false;
}

CSP::TypeNode* DotNETProvider::to_typenode_adapter(const string class_str) const
{
	CSP::ClassNode* node = new CSP::ClassNode();
	
	auto splitted = split(class_str, ' ');

	// TODO

	//// name
	//node->name = to_string(mono_class->get_name());

	//// node_type
	//node->node_type = CSP::Node::Type::CLASS;

	//// base_types
	//GDMonoClass* parent = mono_class->get_parent_class();
	//if (parent != nullptr)
	//	node->base_types.push_back(to_string(parent->get_name()));

	//// node->modifiers
	//// TODO

	return node;
}

CSP::MethodNode* DotNETProvider::to_methodnode_adapter(const string method_str) const
{
	CSP::MethodNode* node = new CSP::MethodNode();

	auto splitted = split(method_str, ' ');
	int n = splitted.size();
	if (n < 3) return nullptr;

	// node_type
	node->node_type = CSP::Node::Type::METHOD;

	// return type 
	node->return_type = splitted[1];

	// name and arguments
	string signature; int pos = -1;
	for (int i = 2; i < n; i++) {
		signature += splitted[i];
		if (contains(splitted[i], ')')) {
			pos = i+1;
			break;
		}
	}

	auto splitted_function = split_func(signature);
	node->name = splitted_function[0];
	for (int i = 1; i < (int)splitted_function.size(); i++) {
		CSP::VarNode* v = new CSP::VarNode();
		v->type = splitted_function[i];
		node->arguments.push_back(v);
	}

	// modifiers
	for (int i = pos; i < n; i++) {

		if (splitted[i] == PUBLIC)
			node->modifiers |= (int)CSP::Modifier::MOD_PUBLIC;

		else if (splitted[i] == PROTECTED)
			node->modifiers |= (int)CSP::Modifier::MOD_PROTECTED;

		else if (splitted[i] == PRIVATE)
			node->modifiers |= (int)CSP::Modifier::MOD_PRIVATE;

		else if (splitted[i] == STATIC)
			node->modifiers |= (int)CSP::Modifier::MOD_STATIC;
	}

	return node;
}

CSP::VarNode* DotNETProvider::to_varnode_adapter(const string var_str) const
{
	CSP::VarNode* node = new CSP::VarNode();

	auto splitted = split(var_str, ' ');
	int n = splitted.size();
	if (n < 3) return nullptr;

	// node_type
	node->node_type = CSP::Node::Type::VAR;

	// type 
	node->type = splitted[1];

	// name
	node->name = splitted[2];

	// modifiers
	for (int i = 3; i < n; i++) {

		if (splitted[i] == PUBLIC)
			node->modifiers |= (int)CSP::Modifier::MOD_PUBLIC;

		else if (splitted[i] == PROTECTED)
			node->modifiers |= (int)CSP::Modifier::MOD_PROTECTED;

		else if (splitted[i] == PRIVATE)
			node->modifiers |= (int)CSP::Modifier::MOD_PRIVATE;

		else if (splitted[i] == STATIC)
			node->modifiers |= (int)CSP::Modifier::MOD_STATIC;
	}

	return node;
}


void DotNETProvider::inject_base_types(CSP::TypeNode* node, string base_types_line) const
{
	string str = base_types_line.substr(BASETYPES_PREFIX.length());
	auto splitted = split(str, ' ');
	for (auto x : splitted) {
		node->base_types.push_back(x);
	}
}


CSP::TypeNode* DotNETProvider::create_class_from_info(const vector<string>& info, string name) const
{
	CSP::ClassNode* node = new CSP::ClassNode();
	node->node_type = CSP::Node::Type::CLASS;
	node->name = name; // remember name, that match!

	for (auto line : info) {

		if (strncmp(line.c_str(), BASETYPES_PREFIX.c_str(), BASETYPES_PREFIX.size()) == 0) {
			inject_base_types(node, line);
		}

		else if (strncmp(line.c_str(), CLASS_PREFIX.c_str(), CLASS_PREFIX.size()) == 0) {
			CSP::ClassNode* tn = (CSP::ClassNode*)to_typenode_adapter(line);
			node->classes.push_back(tn);
		}
		else if (strncmp(line.c_str(), FUNCTION_PREFIX.c_str(), FUNCTION_PREFIX.size()) == 0) {
			CSP::MethodNode* mn = to_methodnode_adapter(line);
			node->methods.push_back(mn);
		}
		else if (strncmp(line.c_str(), VAR_PREFIX.c_str(), VAR_PREFIX.size()) == 0) {
			CSP::VarNode* vn = to_varnode_adapter(line);
			node->variables.push_back(vn);
		}

	}

	return node;
}
