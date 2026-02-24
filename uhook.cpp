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

ProcessEventPrototype original_processevent{reinterpret_cast<ProcessEventPrototype>(0)};
UFunctionHooks<ProcessEventPrototype> processevent_hooks{};
void __fastcall ProcessEventHook(UObject *calling_uobject,
								 UFunction *calling_ufunction,
								 void *parameters)
{
#if defined(_DEBUG)
	indent_level++;
	PLOG_VERBOSE << GetIndentedTabString() << std::format("{0} : {1}", calling_ufunction->GetFullName(), calling_uobject->GetFullName());
#endif
	if (processevent_hooks.ExecuteHook(calling_ufunction, calling_uobject, calling_ufunction, parameters) == ExecuteHookResult::kFailedNoOriginalFunctionFound)
	{
		return original_processevent(calling_uobject, calling_ufunction, parameters);
	}
#if defined(_DEBUG)
	indent_level--;
#endif
}

ProcessInternalPrototype original_processinternal{reinterpret_cast<ProcessInternalPrototype>(0)};
UFunctionHooks<ProcessInternalPrototype> processinternal_hooks{};
void __fastcall ProcessInternalHook(UObject *calling_uobject,
									FFrame &stack,
									void *result)
{
	auto calling_ufunction = reinterpret_cast<UFunction *>(stack.node_);
#if defined(_DEBUG)
	indent_level++;
	PLOG_VERBOSE << GetIndentedTabString() << std::format("{0} : {1}", calling_ufunction->GetFullName(), calling_uobject->GetFullName());
#endif
	if (processinternal_hooks.ExecuteHook(calling_ufunction, calling_uobject, stack, result) == ExecuteHookResult::kFailedNoOriginalFunctionFound)
	{
		original_processinternal(calling_uobject, stack, result);
	}
#if defined(_DEBUG)
	indent_level--;
#endif
}

InvokePrototype original_invoke{reinterpret_cast<InvokePrototype>(0)};
UFunctionHooks<InvokePrototype> invoke_hooks{};
void __fastcall InvokeHook(UFunction *calling_ufunction,
						   UObject *calling_uobject,
						   FFrame &stack,
						   void *result)
{
#if defined(_DEBUG)
	indent_level++;
	PLOG_VERBOSE << GetIndentedTabString() << std::format("{0}. {1} : {2}", indent_level, calling_ufunction->GetFullName(), calling_uobject->GetFullName());
#endif
	if (invoke_hooks.ExecuteHook(calling_ufunction, calling_ufunction, calling_uobject, stack, result) == ExecuteHookResult::kFailedNoOriginalFunctionFound)
	{
		original_invoke(calling_ufunction, calling_uobject, stack, result);
	}
#if defined(_DEBUG)
	indent_level--;
#endif
}

CallFunctionPrototype original_callfunction{reinterpret_cast<CallFunctionPrototype>(0)};
UFunctionHooks<CallFunctionPrototype> callfunction_hooks{};
void __fastcall CallFunctionHook(UObject *calling_uobject,
								 FFrame &stack,
								 void *result, UFunction *calling_ufunction)
{
#if defined(_DEBUG)
	static constexpr unsigned int kFUNC_Native{0x00000400};
	auto is_native{false};
	auto is_funcnative{calling_ufunction->FunctionFlags & kFUNC_Native};
	indent_level++;
	PLOG_VERBOSE << GetIndentedTabString() << std::format("{0}. {1} {2} : {3}", indent_level, is_native ? "[NATIVE]" : (is_funcnative ? "[FUNC_Native]" : ""), calling_ufunction->GetFullName(), calling_uobject->GetFullName());
#endif
	if (callfunction_hooks.ExecuteHook(calling_ufunction, calling_uobject, stack, result, calling_ufunction) == ExecuteHookResult::kFailedNoOriginalFunctionFound)
	{
		original_callfunction(calling_uobject, stack, result, calling_ufunction);
	}
#if defined(_DEBUG)
	indent_level--;
#endif
}