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

string to_string(String s) {
    return string(s.ascii().get_data());
}

String to_String(string s) {
    return String(s.c_str());
}

map<const char*,const char*> CSharpProviderImpl::base_types_map = { 
    { "bool",    "Boolean" },
    { "byte",    "Byte"    },
    { "sbyte",   "SByte"   },
    { "char",    "Char"    },
    { "string",  "String"  },
    { "decimal", "Decimal" },
    { "double",  "Double"  },
    { "float",   "Single"  },
    { "int",     "Int32"   },
    { "long",    "Int64"   },
    { "short",   "Int16"   },
    {" uint",    "UInt32"  },
    {" ulong",   "UInt64"  },
    {" ushort",  "UInt16"  },
    {" object",  "Object"  }
}; 




void print(string label, String s) {
    cout << label << s.ascii().get_data() << endl;
}

CSharpProviderImpl::CSharpProviderImpl() {

    //GDMonoClass* x = GDMono::get_singleton()->get_class("System","Console");
    GDMonoClass* x = GDMono::get_singleton()->get_class("System.IO","DirectoryInfo");
    String fullname = x->get_full_name();
    print("FULLNAME: ",fullname);

    auto mm = x->get_all_methods();
    int n = mm.size();
    for (int i=0; i<n; i++) {

        auto m = mm[i];
        print("get_full_name            : ",m->get_full_name());
        print("get_full_name(true)      : ",m->get_full_name(true));
        print("get_full_name_no_class   : ",m->get_full_name_no_class());
        print("get_name                 : ",m->get_name());
        print("get_ret_type_fullname    : ",m->get_ret_type_full_name());
        print("get_signature_desc       : ",m->get_signature_desc());
        print("get_signature_desc(true) : ",m->get_signature_desc(true));

    }

}

list<CSP::TypeNode*> CSharpProviderImpl::find_class_by_name(string name) const
{    
    list<CSP::TypeNode*> res;

    for (auto nmspc : using_directives) {

        GDMonoClass* mono_class = GDMono::get_singleton()->get_class(to_String(nmspc),to_String(name));
        if (mono_class != nullptr) {
            CSP::TypeNode* node = to_typenode_adapter(mono_class);
            res.push_back(node);
        }

    }

    return res;    
}

CSP::TypeNode* CSharpProviderImpl::resolve_base_type(string base_type) const {

    auto it = base_types_map.find(base_type.c_str());
    if (it == base_types_map.end())
        return nullptr;

    String class_name(it->second);
    GDMonoClass* mono_class = GDMono::get_singleton()->get_class("System",class_name);
    CSP::TypeNode* node = to_typenode_adapter(mono_class);

    return node;
}

CSP::TypeNode* CSharpProviderImpl::to_typenode_adapter(const GDMonoClass* mono_class) const 
{
    CSP::ClassNode* node = new CSP::ClassNode();
    node->created_by_provider = true;
    node->provider_data = (void*)mono_class;

    // name
    node->name = to_string(mono_class->get_name());
    
    // node_type
    node->node_type = CSP::Node::Type::CLASS;

    // base_types
    GDMonoClass* parent = mono_class->get_parent_class();
    if (parent != nullptr)
        node->base_types.push_back(to_string(parent->get_name()));

    // node->modifiers
    // TODO

    return node;
}

CSP::MethodNode* CSharpProviderImpl::to_methodnode_adapter(const GDMonoMethod* mono_method) const 
{
    CSP::MethodNode* node = new CSP::MethodNode();
    node->created_by_provider = true;
    node->provider_data = (void*)mono_method;

    // arguments
    string signature = to_string(mono_method->get_signature_desc(true));
    auto args = split_func(signature);
    for (auto x : args) {
        CSP::VarNode* v = new CSP::VarNode();
        v->type = x;
        node->arguments.push_back(v);
    }

    //node->modifiers
    // TODO

    // name
    node->name = to_string(mono_method->get_name());

    // node_type
    node->node_type = CSP::Node::Type::METHOD;

    // return type 
    node->return_type = to_string(mono_method->get_ret_type_full_name());

    return node;
}

CSP::VarNode* CSharpProviderImpl::to_varnode_adapter(const GDMonoField* mono_field) const
{
    CSP::VarNode* node = new CSP::VarNode();

    node->created_by_provider = true;
    node->provider_data = (void*)mono_field;

    // TODO node->modifiers

    // name
    node->name = to_string(mono_field->get_name());

    //node->node_type
    node->node_type = CSP::Node::Type::VAR;

    //node->type
    node->type = to_string(mono_field->get_type());


    return node;
}

CSP::VarNode* CSharpProviderImpl::to_varnode_adapter(const GDMonoProperty* mono_property) const 
{
    CSP::VarNode* node = new CSP::VarNode();

    node->created_by_provider = true;
    node->provider_data = (void*)mono_field;

    // TODO node->modifiers

    // name
    node->name = to_string(mono_property->get_name());

    //node->node_type
    node->node_type = CSP::Node::Type::PROPERTY;

    //node->type
    node->type = to_string(mono_property->get_type());

    return node;
}

list<CSP::Node*> CSharpProviderImpl::get_child_dynamic(void* invoker, string name) const
{
    GDMonoClass* mono_class = static_cast<GDMonoClass*>(invoker);
    if (mono_class == nullptr)
        return list<CSP::Node*>();

    list<CSP::Node*> res;

    // TODO: mono_class->get_all_delegates

    auto fields = mono_class->get_all_fields();
    for (int i=0; i<fields.size(); i++) {

        GDMonoField* mono_field = fields[i];
        CSP::VarNode* node = to_varnode_adapter(mono_field);
        res.push_back(node);

    }

    auto properties = mono_class->get_all_properties();
    for (int i=0; i<properties.size(); i++) {

        GDMonoProperty* mono_property = properties[i];
        CSP::VarNode* node = to_varnode_adapter(mono_property);
        res.push_back(node);
        
    }

    auto methods = mono_class->get_all_methods();
    for (int i=0; i<methods.size(); i++) {

        GDMonoMethod* mono_method = methods[i];
        CSP::MethodNode* node = to_methodnode_adapter(mono_method);
        res.push_back(node);
        
    }

    return res;
}