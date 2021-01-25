#ifndef DOTNET_PROVIDER_H
#define DOTNET_PROVIDER_H

#include "csharp_utils.h"
#include "csharp_context.h"

using namespace std;

class DotNETProvider : ICSharpProvider
{
	const string provider_path;
	
	const string BASETYPES_PREFIX = "BASETYPES ";
	const string CLASS_PREFIX = "CLASS ";
	const string FUNCTION_PREFIX = "METHOD ";
	const string VAR_PREFIX = "VAR ";
	const string PUBLIC = "PUBLIC";
	const string PROTECTED = "PROTECTED";
	const string PRIVATE = "PRIVATE";
	const string STATIC = "STATIC";

	map<string, CSP::TypeNode*> cache;
	string get_cmd_output(const char* cmd) const;

public:

	DotNETProvider(string provider_path);
	
	// implementation
	list<CSP::TypeNode*> find_class_by_name(string name) override;
	CSP::TypeNode* resolve_base_type(string base_type) override;

	// adapters
	CSP::TypeNode* to_typenode_adapter(const string class_str) const;
	CSP::MethodNode* to_methodnode_adapter(const string method_str) const;
	CSP::VarNode* to_varnode_adapter(const string var_str) const;

	// helpers
	CSP::TypeNode* create_class_from_info(const vector<string>& info, string name) const;
	void inject_base_types(CSP::TypeNode* node, string base_types_line) const;
	CSP::TypeNode* do_type_query(const string& type_fullname);

};


#endif // DOTNET_PROVIDER_H