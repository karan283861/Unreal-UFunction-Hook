#include <format>
#include <string>
#include "uhook.hpp"

#if defined(_DEBUG)
size_t indent_level{0};

std::string GetIndentedTabString(void)
{
	std::string s;
	for (int i = 0; i < indent_level; i++)
	{
		s.append("\t");
	}
	return s;
}
#endif

namespace UE3
{
	void __fastcall ProcessEventHook(UObjectInternal *calling_uobject,
									 void *unused, UFunctionInternal *calling_ufunction,
									 void *parameters, void *result)
	{
#if defined(_DEBUG)
		indent_level++;
		log_function(std::format("{0}{1} : {2}", GetIndentedTabString(), get_uobject_name(reinterpret_cast<UObjectInternal *>(calling_ufunction)), get_uobject_name(calling_uobject)));
#endif
		if (processevent_hooks.ExecuteHook(calling_ufunction, calling_uobject, unused, calling_ufunction, parameters, result) == ExecuteHookResult::kFailedNoOriginalFunctionFound)
		{
			original_processevent(calling_uobject, unused, calling_ufunction, parameters, result);
		}
#if defined(_DEBUG)
		indent_level--;
#endif
	}

	void __fastcall ProcessInternalHook(UObjectInternal *calling_uobject,
										void *unused, FFrame &stack,
										void *result)
	{
		auto calling_ufunction = reinterpret_cast<UFunctionInternal *>(stack.node_);
#if defined(_DEBUG)
		indent_level++;
		log_function(std::format("{0}{1} : {2}", GetIndentedTabString(), get_uobject_name(reinterpret_cast<UObjectInternal *>(calling_ufunction)), get_uobject_name(calling_uobject)));
#endif
		if (processinternal_hooks.ExecuteHook(calling_ufunction, calling_uobject, unused, stack, result) == ExecuteHookResult::kFailedNoOriginalFunctionFound)
		{
			original_processinternal(calling_uobject, unused, stack, result);
		}
#if defined(_DEBUG)
		indent_level--;
#endif
	}

	void __fastcall CallFunctionHook(UObjectInternal *calling_uobject,
									 void *unused, FFrame &stack,
									 void *result, UFunctionInternal *calling_ufunction)
	{
#if defined(_DEBUG)
		auto is_native{is_ufunction_native(calling_ufunction)};
		indent_level++;
		log_function(std::format("{0}{1} {2} : {3}", GetIndentedTabString(), is_native ? "[NATIVE/FUNC_Native]" : "", get_uobject_name(reinterpret_cast<UObjectInternal *>(calling_ufunction)), get_uobject_name(calling_uobject)));
#endif
		if (callfunction_hooks.ExecuteHook(calling_ufunction, calling_uobject, unused, stack, result, calling_ufunction) == ExecuteHookResult::kFailedNoOriginalFunctionFound)
		{
			original_callfunction(calling_uobject, unused, stack, result, calling_ufunction);
		}
#if defined(_DEBUG)
		indent_level--;
#endif
	}
}