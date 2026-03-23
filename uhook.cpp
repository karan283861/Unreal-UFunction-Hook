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
	void __fastcall ProcessEventHook(UObject *calling_uobject,
									 void *unused, UFunction *calling_ufunction,
									 void *parameters, void *result)
	{
#if defined(_DEBUG)
		indent_level++;
		log_function(std::format("{0}{1} : {2}", GetIndentedTabString(), calling_ufunction->GetFullName(), calling_uobject->GetFullName()));
#endif
		if (processevent_hooks.ExecuteHook(calling_ufunction, calling_uobject, unused, calling_ufunction, parameters, result) == ExecuteHookResult::kFailedNoOriginalFunctionFound)
		{
			original_processevent(calling_uobject, unused, calling_ufunction, parameters, result);
		}
#if defined(_DEBUG)
		indent_level--;
#endif
	}

	void __fastcall ProcessInternalHook(UObject *calling_uobject,
										void *unused, FFrame &stack,
										void *result)
	{
		auto calling_ufunction = reinterpret_cast<UFunction *>(stack.node_);
#if defined(_DEBUG)
		indent_level++;
		log_function(std::format("{0}{1} : {2}", GetIndentedTabString(), calling_ufunction->GetFullName(), calling_uobject->GetFullName()));
#endif
		if (processinternal_hooks.ExecuteHook(calling_ufunction, calling_uobject, unused, stack, result) == ExecuteHookResult::kFailedNoOriginalFunctionFound)
		{
			original_processinternal(calling_uobject, unused, stack, result);
		}
#if defined(_DEBUG)
		indent_level--;
#endif
	}

	void __fastcall CallFunctionHook(UObject *calling_uobject,
									 void *unused, FFrame &stack,
									 void *result, UFunction *calling_ufunction)
	{
#if defined(_DEBUG)
		static constexpr unsigned int kFUNC_Native{0x00000400};
		auto is_native{calling_ufunction->iNative};
		auto is_funcnative{calling_ufunction->FunctionFlags & kFUNC_Native};
		indent_level++;
		log_function(std::format("{0}{1} {2} : {3}", GetIndentedTabString(), is_native ? "[NATIVE]" : (is_funcnative ? "[FUNC_Native]" : ""), calling_ufunction->GetFullName(), calling_uobject->GetFullName()));
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

namespace UE4
{
	void __fastcall ProcessEventHook(UObject *calling_uobject,
									 UFunction *calling_ufunction,
									 void *parameters)
	{
#if defined(_DEBUG)
		indent_level++;
		log_function(std::format("{0}{1} : {2}", GetIndentedTabString(), calling_ufunction->GetFullName(), calling_uobject->GetFullName()));
#endif
		if (processevent_hooks.ExecuteHook(calling_ufunction, calling_uobject, calling_ufunction, parameters) == ExecuteHookResult::kFailedNoOriginalFunctionFound)
		{
			original_processevent(calling_uobject, calling_ufunction, parameters);
		}
#if defined(_DEBUG)
		indent_level--;
#endif
	}

	void __fastcall ProcessInternalHook(UObject *calling_uobject,
										FFrame &stack,
										void *result)
	{
		auto calling_ufunction = reinterpret_cast<UFunction *>(stack.node_);
#if defined(_DEBUG)
		indent_level++;
		log_function(std::format("{0}{1} : {2}", GetIndentedTabString(), calling_ufunction->GetFullName(), calling_uobject->GetFullName()));
#endif
		if (processinternal_hooks.ExecuteHook(calling_ufunction, calling_uobject, stack, result) == ExecuteHookResult::kFailedNoOriginalFunctionFound)
		{
			original_processinternal(calling_uobject, stack, result);
		}
#if defined(_DEBUG)
		indent_level--;
#endif
	}

	void __fastcall CallFunctionHook(UObject *calling_uobject,
									 FFrame &stack,
									 void *result, UFunction *calling_ufunction)
	{
#if defined(_DEBUG)
		static constexpr unsigned int kFUNC_Native{0x00000400};
		auto is_native{calling_ufunction->iNative};
		auto is_funcnative{calling_ufunction->FunctionFlags & kFUNC_Native};
		indent_level++;
		log_function(std::format("{0}{1} {2} : {3}", GetIndentedTabString(), is_native ? "[NATIVE]" : (is_funcnative ? "[FUNC_Native]" : ""), calling_ufunction->GetFullName(), calling_uobject->GetFullName()));
#endif
		if (callfunction_hooks.ExecuteHook(calling_ufunction, calling_uobject, stack, result, calling_ufunction) == ExecuteHookResult::kFailedNoOriginalFunctionFound)
		{
			original_callfunction(calling_uobject, stack, result, calling_ufunction);
		}
#if defined(_DEBUG)
		indent_level--;
#endif
	}
}

namespace UE5
{
	void __fastcall ProcessEventHook(UObject *calling_uobject,
									 UFunction *calling_ufunction,
									 void *parameters)
	{
#if defined(_DEBUG)
		indent_level++;
		log_function(std::format("{0}{1} : {2}", GetIndentedTabString(), calling_ufunction->GetFullName(), calling_uobject->GetFullName()));
#endif
		if (processevent_hooks.ExecuteHook(calling_ufunction, calling_uobject, calling_ufunction, parameters) == ExecuteHookResult::kFailedNoOriginalFunctionFound)
		{
			original_processevent(calling_uobject, calling_ufunction, parameters);
		}
#if defined(_DEBUG)
		indent_level--;
#endif
	}

	void __fastcall ProcessInternalHook(UObject *calling_uobject,
										FFrame &stack,
										void *result)
	{
		auto calling_ufunction = reinterpret_cast<UFunction *>(stack.node_);
#if defined(_DEBUG)
		indent_level++;
		log_function(std::format("{0}{1} : {2}", GetIndentedTabString(), calling_ufunction->GetFullName(), calling_uobject->GetFullName()));
#endif
		if (processinternal_hooks.ExecuteHook(calling_ufunction, calling_uobject, stack, result) == ExecuteHookResult::kFailedNoOriginalFunctionFound)
		{
			original_processinternal(calling_uobject, stack, result);
		}
#if defined(_DEBUG)
		indent_level--;
#endif
	}

	void __fastcall CallFunctionHook(UObject *calling_uobject,
									 FFrame &stack,
									 void *result, UFunction *calling_ufunction)
	{
#if defined(_DEBUG)
		static constexpr unsigned int kFUNC_Native{0x00000400};
		auto is_native{calling_ufunction->iNative};
		auto is_funcnative{calling_ufunction->FunctionFlags & kFUNC_Native};
		indent_level++;
		log_function(std::format("{0}{1} {2} : {3}", GetIndentedTabString(), is_native ? "[NATIVE]" : (is_funcnative ? "[FUNC_Native]" : ""), calling_ufunction->GetFullName(), calling_uobject->GetFullName()));
#endif
		if (callfunction_hooks.ExecuteHook(calling_ufunction, calling_uobject, stack, result, calling_ufunction) == ExecuteHookResult::kFailedNoOriginalFunctionFound)
		{
			original_callfunction(calling_uobject, stack, result, calling_ufunction);
		}
#if defined(_DEBUG)
		indent_level--;
#endif
	}
}