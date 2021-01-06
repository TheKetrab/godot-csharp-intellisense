#ifndef CSHARP_MONO_H
#define CSHARP_MONO_H

#include "csharp_context.h"
#include "mono_gd/gd_mono.h"
#include "mono_gd/gd_mono_assembly.h"
#include "mono_gd/gd_mono_class.h"
#include "mono_gd/gd_mono_field.h"
#include "mono_gd/gd_mono_property.h"
#include "mono_gd/gd_mono_method.h"

using namespace std;
#include <map>


// node'y istnieja tylko na chwile!!!
// dlatego provider dynamicznie pobiera dzieci jesli potrzeba
// lub rodzicow itp

class CSharpProviderImpl : ICSharpProvider {

public:

	// impl
	virtual list<CSP::TypeNode*> find_class_by_name(string name) const = 0;
	virtual CSP::TypeNode* resolve_base_type(string base_type) const override;
	virtual list<CSP::Node*> get_child_dynamic(void* invoker, string name) const override;


// TODO: get children dynamic (dopiero gdy potrzebne)

	CSharpProviderImpl();

	
	static map<const char*,const char*> base_types_map;

	CSP::TypeNode* to_typenode_adapter(const GDMonoClass* mono_class) const;
	CSP::MethodNode* to_methodnode_adapter(const GDMonoMethod* mono_method) const;
	CSP::VarNode* to_varnode_adapter(const GDMonoField* mono_field) const;
	CSP::VarNode* to_varnode_adapter(const GDMonoProperty* mono_property) const;


	
};



// MONO_TYPE_ATTR_VISIBILITY_MASK       = 0x00000007,
// MONO_TYPE_ATTR_NOT_PUBLIC            = 0x00000000,
// MONO_TYPE_ATTR_PUBLIC                = 0x00000001,
// MONO_TYPE_ATTR_NESTED_PUBLIC         = 0x00000002,
// MONO_TYPE_ATTR_NESTED_PRIVATE        = 0x00000003,
// MONO_TYPE_ATTR_NESTED_FAMILY         = 0x00000004,
// MONO_TYPE_ATTR_NESTED_ASSEMBLY       = 0x00000005,
// MONO_TYPE_ATTR_NESTED_FAM_AND_ASSEM  = 0x00000006,
// MONO_TYPE_ATTR_NESTED_FAM_OR_ASSEM   = 0x00000007,

// MONO_TYPE_ATTR_LAYOUT_MASK           = 0x00000018,
// MONO_TYPE_ATTR_AUTO_LAYOUT           = 0x00000000,
// MONO_TYPE_ATTR_SEQUENTIAL_LAYOUT     = 0x00000008,
// MONO_TYPE_ATTR_EXPLICIT_LAYOUT       = 0x00000010,

// MONO_TYPE_ATTR_CLASS_SEMANTIC_MASK   = 0x00000020,
// MONO_TYPE_ATTR_CLASS                 = 0x00000000,
// MONO_TYPE_ATTR_INTERFACE             = 0x00000020,

// MONO_TYPE_ATTR_ABSTRACT              = 0x00000080,
// MONO_TYPE_ATTR_SEALED                = 0x00000100,
// MONO_TYPE_ATTR_SPECIAL_NAME          = 0x00000400,

// MONO_TYPE_ATTR_IMPORT                = 0x00001000,
// MONO_TYPE_ATTR_SERIALIZABLE          = 0x00002000,

// MONO_TYPE_ATTR_STRING_FORMAT_MASK    = 0x00030000,
// MONO_TYPE_ATTR_ANSI_CLASS            = 0x00000000,
// MONO_TYPE_ATTR_UNICODE_CLASS         = 0x00010000,
// MONO_TYPE_ATTR_AUTO_CLASS            = 0x00020000,
// MONO_TYPE_ATTR_CUSTOM_CLASS          = 0x00030000,
// MONO_TYPE_ATTR_CUSTOM_MASK           = 0x00c00000,

// MONO_TYPE_ATTR_BEFORE_FIELD_INIT     = 0x00100000,
// MONO_TYPE_ATTR_FORWARDER             = 0x00200000,

// MONO_TYPE_ATTR_RESERVED_MASK         = 0x00040800,
// MONO_TYPE_ATTR_RT_SPECIAL_NAME       = 0x00000800,
// MONO_TYPE_ATTR_HAS_SECURITY          = 0x00040000


#endif // CSHARP_MONO_H
