
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

	std::cout << " ======================== " << ILOSC++ << " ================================= " << std::endl;
	std::cout<<"P_PATH:"<<std::endl;
	std::cout<<p_path.ascii().get_data()<<std::endl;

	std::string code = p_code.ascii().get_data();

	CSharpLexer lexer(code);
	std::cout << "CODE:"<<std::endl;
	std::cout << code << std::endl;
	lexer.tokenize();
	lexer.print_tokens();

	std::cout << "============== 0" << std::endl;
	auto csc = CSharpContext::instance();

	std::cout << "============== 1" << std::endl;
	std::string filename = p_path.ascii().get_data();
	csc->update_state(code,filename);
	
	std::cout << "============== 2" << std::endl;
	auto opts = csc->get_options();

	std::cout << "============== 3" << std::endl;
	std::string s = csc->cinfo.completion_expression;
	std::cout << "Expression: " << csc->cinfo.completion_expression << std::endl;
	std::cout << "OPTIONS:" << std::endl;
	std::cout << "============== 4" << std::endl;
	for (auto opt : opts) {
		std::cout << csc->option_to_string(opt.first) << std::endl;
		std::cout << " : " << opt.second << std::endl;


	std::cout << "============== 4a" << std::endl;
		ScriptCodeCompletionOption::Kind optKind;
		if (opt.first == CSharpContext::Option::KIND_CLASS) optKind = ScriptCodeCompletionOption::Kind::KIND_CLASS;
		else if (opt.first == CSharpContext::Option::KIND_FUNCTION) optKind = ScriptCodeCompletionOption::Kind::KIND_FUNCTION;
		else if (opt.first == CSharpContext::Option::KIND_VARIABLE) optKind = ScriptCodeCompletionOption::Kind::KIND_VARIABLE;
		else if (opt.first == CSharpContext::Option::KIND_MEMBER) optKind = ScriptCodeCompletionOption::Kind::KIND_MEMBER;
		else if (opt.first == CSharpContext::Option::KIND_ENUM) optKind = ScriptCodeCompletionOption::Kind::KIND_ENUM;
		else if (opt.first == CSharpContext::Option::KIND_CONSTANT) optKind = ScriptCodeCompletionOption::Kind::KIND_CONSTANT;
		else if (opt.first == CSharpContext::Option::KIND_PLAIN_TEXT) optKind = ScriptCodeCompletionOption::Kind::KIND_PLAIN_TEXT;
		else if (opt.first == CSharpContext::Option::KIND_LABEL) optKind = ScriptCodeCompletionOption::Kind::KIND_PLAIN_TEXT;

		std::cout << "============== 4b" << std::endl;
		const char* cstr = opt.second.c_str();
		String str(cstr);
		ScriptCodeCompletionOption option(str, optKind);
		std::cout << "============== 4c" << std::endl;
		r_options->push_back(option);
	}

	std::cout << "============== 5" << std::endl;

	return OK;
/*
	CSharpLexer lexer;
	lexer.set_code(p_code);
	std::cout << "CODE:"<<std::endl;
	std::cout << p_code.ascii().get_data() << std::endl;
	lexer.tokenize();
	lexer.print_tokens();

	CSharpParser parser;
	parser.set_tokens(lexer.tokens);
	parser.parse();
*/

	/*
	std::cout<<"----- ----- -----"<<std::endl;
	std::cout<<"P_CODE:"<<std::endl;
	std::cout<<p_code.ascii().get_data()<<std::endl;

	std::cout<<"----- ----- -----"<<std::endl;
	std::cout<<"P_PATH:"<<std::endl;
	std::cout<<p_path.ascii().get_data()<<std::endl;
	*/

	return OK;
}

//#else

//Error CSharpLanguage::complete_code(const String &p_code, const String &p_path, Object *p_owner, List<ScriptCodeCompletionOption> *r_options, bool &r_forced, String &r_call_hint) {
//	return OK;
//}

//#endif