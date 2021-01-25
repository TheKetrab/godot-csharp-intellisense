#include "csharp_provider_mono.h"
#include "mono_gd/gd_mono.h"
#include "mono_gd/gd_mono_assembly.h"
#include "mono_gd/gd_mono_class.h"
#include "mono_gd/gd_mono_field.h"
#include "mono_gd/gd_mono_property.h"
#include "mono_gd/gd_mono_method.h"

using namespace std;

#include <iostream>
#include <map>
#include "csharp_utils.h"


// EXAMPLE!!!
//GDMonoClass* x = GDMono::get_singleton()->get_class("System","Console");
//GDMonoClass* x = GDMono::get_singleton()->get_class("System.IO","DirectoryInfo");
//String fullname = x->get_full_name();
//print("FULLNAME: ",fullname);
//
//auto mm = x->get_all_methods();
//int n = mm.size();
//for (int i=0; i<n; i++) {
//
//	auto m = mm[i];
//	print("get_full_name            : ",m->get_full_name());
//	print("get_full_name(true)      : ",m->get_full_name(true));
//	print("get_full_name_no_class   : ",m->get_full_name_no_class());
//	print("get_name                 : ",m->get_name());
//	print("get_ret_type_fullname    : ",m->get_ret_type_full_name());
//	print("get_signature_desc       : ",m->get_signature_desc());
//	print("get_signature_desc(true) : ",m->get_signature_desc(true));
//
//}

string to_string(String s) {
    return string(s.ascii().get_data());
}

String to_String(string s) {
    return String(s.c_str());
}

void print(string label, String s) {
    cout << label << s.ascii().get_data() << endl;
}

CSharpProviderImpl::CSharpProviderImpl() {

}

CSP::TypeNode* CSharpProviderImpl::do_type_query(const string& type_fullname)
{
	// find in cache; TODO: O(1), not O(n) !!!
	for (auto it = cache.begin(); it != cache.end(); it++)
		if (it->first == type_fullname)
			return it->second;

	// not resolved before - resolve now and add to cache
	auto p = split_to_namespace_and_classname(type_fullname);
	GDMonoClass* mono_class = GDMono::get_singleton()->
		get_class(to_String(p.first), to_String(p.second));

	if (mono_class != nullptr) {
		CSP::TypeNode* node = to_typenode_adapter(mono_class);
		cache.insert({ type_fullname,node }); // possible null
		return node;
	}

	else {
		cache.insert({ type_fullname, nullptr });
		return nullptr;
	}

}

// creates type from 'raw' name - if failed tries to find with opened namespaces
list<CSP::TypeNode*> CSharpProviderImpl::find_class_by_name(string name)
{
	list<CSP::TypeNode*> res;

	// try to match
	CSP::TypeNode* node = do_type_query(name);
	if (node != nullptr)
		res.push_back(node);

	// try to match with using directives
	for (auto nmspc : using_directives) {
		string the_name = (nmspc + "." + name);
		node = do_type_query(the_name);
		if (node != nullptr)
			res.push_back(node);
	}

	return res;
}

// TODO: mozna to przerzucic do CSharpContext
CSP::TypeNode* CSharpProviderImpl::resolve_base_type(string base_type) {

	int rank = CSP::remove_array_type(base_type);

	string name = "";
	for (auto x : CSharpContext::base_types_map) {
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

CSP::TypeNode* CSharpProviderImpl::to_typenode_adapter(GDMonoClass* mono_class) const
{
    CSP::ClassNode* node = new CSP::ClassNode();

    // name
    node->name = to_string(mono_class->get_name());
    
    // node_type
    node->node_type = CSP::Node::Type::CLASS;

    // base_types
    GDMonoClass* parent = mono_class->get_parent_class();
    if (parent != nullptr)
        node->base_types.push_back(to_string(parent->get_name()));

    // node->modifiers TODO!!!
	node->modifiers |= (int)CSP::Modifier::MOD_PUBLIC;

	
	// TODO: mono_class->get_all_delegates

	auto fields = mono_class->get_all_fields();
	for (int i = 0; i < fields.size(); i++) {

		GDMonoField* mono_field = fields[i];
		CSP::VarNode* var_node = to_varnode_adapter(mono_field);
		if (var_node != nullptr)
			node->variables.push_back(var_node);

	}

	auto properties = mono_class->get_all_properties();
	for (int i = 0; i < properties.size(); i++) {

		GDMonoProperty* mono_property = properties[i];
		CSP::VarNode* prop_node = to_varnode_adapter(mono_property);
		if (prop_node != nullptr)
			node->properties.push_back((CSP::PropertyNode*)prop_node);

	}

	auto methods = mono_class->get_all_methods();
	for (int i = 0; i < methods.size(); i++) {

		GDMonoMethod* mono_method = methods[i];
		CSP::MethodNode* method_node = to_methodnode_adapter(mono_method);
		if (method_node != nullptr)
			node->methods.push_back(method_node);

	}

	return node;
}

CSP::MethodNode* CSharpProviderImpl::to_methodnode_adapter(GDMonoMethod* mono_method) const 
{
    CSP::MethodNode* node = new CSP::MethodNode();

    // arguments
    string signature = to_string(mono_method->get_signature_desc(true));
    auto args = split_func(signature);
    for (auto x : args) {
        CSP::VarNode* v = new CSP::VarNode();
        v->type = x;
        node->arguments.push_back(v);
    }

	// node->modifiers TODO!!!
	node->modifiers |= (int)CSP::Modifier::MOD_PUBLIC;

    // name
    node->name = to_string(mono_method->get_name());

    // node_type
    node->node_type = CSP::Node::Type::METHOD;

    // return type 
    node->return_type = to_string(mono_method->get_ret_type_full_name());

    return node;
}

CSP::VarNode* CSharpProviderImpl::to_varnode_adapter(GDMonoField* mono_field) const
{
    CSP::VarNode* node = new CSP::VarNode();

	// node->modifiers TODO!!!
	node->modifiers |= (int)CSP::Modifier::MOD_PUBLIC;

    // name
    node->name = to_string(mono_field->get_name());

    //node->node_type
    node->node_type = CSP::Node::Type::VAR;

    //node->type
    node->type = to_string(mono_field->get_type().type_class->get_full_name());


    return node;
}

CSP::VarNode* CSharpProviderImpl::to_varnode_adapter(GDMonoProperty* mono_property) const 
{
    CSP::VarNode* node = new CSP::VarNode();

	// node->modifiers TODO!!!
	node->modifiers |= (int)CSP::Modifier::MOD_PUBLIC;

    // name
    node->name = to_string(mono_property->get_name());

    //node->node_type
    node->node_type = CSP::Node::Type::PROPERTY;

    //node->type
    node->type = to_string(mono_property->get_type().type_class->get_full_name());

    return node;
}

// System.X.Y.Z -> { System.X.Y , Z }
pair<string, string> CSharpProviderImpl::split_to_namespace_and_classname(string fullname) const
{
	int n = fullname.length();
	int i = n - 1;

	for (; i >= 0; i--)
		if (fullname[i] == '.')
			break;

	if (i <= 0)
		return { "", fullname };

	string nmspc = fullname.substr(0, i);
	string classname = fullname.substr(i + 1, n - i);

	return { nmspc,classname };
}
