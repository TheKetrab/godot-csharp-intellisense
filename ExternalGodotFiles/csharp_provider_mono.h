#ifndef CSHARP_MONO_H
#define CSHARP_MONO_H

#include "csharp_utils.h"
#include "csharp_context.h"
#include "mono_gd/gd_mono.h"
#include "mono_gd/gd_mono_assembly.h"
#include "mono_gd/gd_mono_class.h"
#include "mono_gd/gd_mono_field.h"
#include "mono_gd/gd_mono_property.h"
#include "mono_gd/gd_mono_method.h"

using namespace std;

class CSharpProviderImpl : ICSharpProvider
{
	map<string, CSP::TypeNode*> cache;

public:

	CSharpProviderImpl();

	// implementation
	list<CSP::TypeNode*> find_class_by_name(string name) override;
	CSP::TypeNode* resolve_base_type(string base_type) override;

	// adapters
	CSP::TypeNode* to_typenode_adapter(GDMonoClass* mono_class) const;
	CSP::MethodNode* to_methodnode_adapter(GDMonoMethod* mono_method) const;
	CSP::VarNode* to_varnode_adapter(GDMonoField* mono_field) const;
	CSP::VarNode* to_varnode_adapter(GDMonoProperty* mono_property) const;

	// helpers
	pair<string, string> split_to_namespace_and_classname(string fullname) const;
	CSP::TypeNode* do_type_query(const string& type_fullname);
	void inject_modifiers(CSP::Node* node, IMonoClassMember* member) const;
};

#endif // CSHARP_MONO_H
