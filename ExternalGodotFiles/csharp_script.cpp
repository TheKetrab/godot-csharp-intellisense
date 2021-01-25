
// ----- ----- ----- ----- -----
//
// PASTE THIS PIECE OF CODE SOMEWHERE IN:
// ...\godot\modules\mono\csharp_script.cpp
//
// @Bartłomiej Grochowski 2020-2021
// ----- ----- ----- ----- -----

//#if defined(DEBUG_METHODS_ENABLED) && defined(TOOLS_ENABLED)
#include <iostream>
#include "csharp_lexer.h"
#include "csharp_parser.h"
#include "csharp_context.h"

static int ILOSC = 0;
Error CSharpLanguage::complete_code(const String &p_code, const String &p_path, Object *p_owner, List<ScriptCodeCompletionOption> *r_options, bool &r_forced, String &r_call_hint) {

	// std::cout<<"P_PATH:"<<std::endl;
	// std::cout<<p_path.ascii().get_data()<<std::endl;

	std::string code = p_code.ascii().get_data();

	// CSharpLexer lexer(code);
	// std::cout << "CODE:"<<std::endl;
	// std::cout << code << std::endl;
	// lexer.tokenize();
	// lexer.print_tokens();

	auto _csc = CSharpContext::instance();
	if (!_csc->provider_registered) {
		CSharpProviderImpl* provider = new CSharpProviderImpl();
		_csc->register_provider((ICSharpProvider*)provider);
	}

	std::string filename = p_path.ascii().get_data();

	try {
		_csc->update_state(code, filename);

		auto opts = _csc->get_options();

		std::string s = _csc->cinfo.completion_expression;
		std::cout << "Expression: " << _csc->cinfo.completion_expression << std::endl;
		std::cout << "OPTIONS:" << std::endl;

		for (auto opt : opts) {

			std::cout << _csc->option_to_string(opt.first) << " : " << opt.second << std::endl;

			ScriptCodeCompletionOption::Kind optKind;
			if (opt.first == CSharpContext::Option::KIND_CLASS) optKind = ScriptCodeCompletionOption::Kind::KIND_CLASS;
			else if (opt.first == CSharpContext::Option::KIND_FUNCTION) optKind = ScriptCodeCompletionOption::Kind::KIND_FUNCTION;
			else if (opt.first == CSharpContext::Option::KIND_VARIABLE) optKind = ScriptCodeCompletionOption::Kind::KIND_VARIABLE;
			else if (opt.first == CSharpContext::Option::KIND_MEMBER) optKind = ScriptCodeCompletionOption::Kind::KIND_MEMBER;
			else if (opt.first == CSharpContext::Option::KIND_ENUM) optKind = ScriptCodeCompletionOption::Kind::KIND_ENUM;
			else if (opt.first == CSharpContext::Option::KIND_CONSTANT) optKind = ScriptCodeCompletionOption::Kind::KIND_CONSTANT;
			else if (opt.first == CSharpContext::Option::KIND_PLAIN_TEXT) optKind = ScriptCodeCompletionOption::Kind::KIND_PLAIN_TEXT;
			else if (opt.first == CSharpContext::Option::KIND_LABEL) optKind = ScriptCodeCompletionOption::Kind::KIND_PLAIN_TEXT;

			const char* cstr = opt.second.c_str();
			String str(cstr);
			ScriptCodeCompletionOption option(str, optKind);
			r_options->push_back(option);
		}
		std::cout << "End of printing options." << std::endl << std::endl;

	}
	catch (const std::exception e) {
		std::cout << "Intellisense exception: " << e.what() << std::endl;
	}


	return OK;
}
