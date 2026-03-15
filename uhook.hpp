#pragma once

// #include <format>
// #include <unordered_map>
#include <vector>
#include <array>
#include <memory>
#include <mutex>
#include <atomic>
#include <plog/Log.h>
#include <CppSDK/SDK/CoreUObject_classes.hpp>

constexpr size_t kSizeOfUFunctionInternalIndex{2000000};
// constexpr size_t kSizeOfUFunctionInternalIndex{500000};
constexpr size_t kMaxHookCountPerUFunction{1};

using SDK::UFunction;
using SDK::UObject;
using SDK::UStruct;

using ProcessEventPrototype = void(__fastcall *)(UObject *calling_uobject,
												 UFunction *calling_ufunction,
												 void *parameters);

void __fastcall ProcessEventHook(UObject *calling_uobject,
								 UFunction *calling_ufunction,
								 void *parameters);

extern ProcessEventPrototype original_processevent;

class FFrame
{
public:
	void *vmt_{};
	char unknown_data_0[8]{};
	UStruct *node_{};
	UObject *object_{};
	unsigned char *code_{};
	char *locals_{};
	int line_nums_{};
	FFrame *previous_frame_{};
};

using ProcessInternalPrototype = void(__fastcall *)(UObject *calling_uobject,
													FFrame &stack,
													void *result);

void __fastcall ProcessInternalHook(UObject *calling_uobject,
									FFrame &stack,
									void *result);

extern ProcessInternalPrototype original_processinternal;

using InvokePrototype = void(__fastcall *)(UFunction *calling_ufunction,
										   UObject *calling_uobject,
										   FFrame &stack,
										   void *result);

void __fastcall InvokeHook(UFunction *calling_ufunction,
						   UObject *calling_uobject,
						   FFrame &stack,
						   void *result);

extern InvokePrototype original_invoke;

using CallFunctionPrototype = void(__fastcall *)(UObject *calling_uobject,
												 FFrame &stack,
												 void *result, UFunction *calling_ufunction);

void __fastcall CallFunctionHook(UObject *calling_uobject,
								 FFrame &stack,
								 void *result, UFunction *calling_ufunction);

extern CallFunctionPrototype original_callfunction;

enum class FunctionHookType
{
	kPre,
	kPost
};

enum class FunctionHookAbsorb
{
	kDoNotAbsorb,
	kAbsorb
};

enum class HookResult
{
	kSuccess,
	kFailedIncorrectHookTypeAndHookAbsorb,
	kFailedToFindUFunction,
	kFailedUFunctionOutOfBounds,
	kFailedOverMaxHookCount,
	kFailedUnknownHookType
};

enum class ExecuteHookResult
{
	kSuccess,
	kFailedNoOriginalFunctionFound
};

template <typename UFunctionHookPrototype>
class UFunctionHooks
{
public:
	struct UFunctionHookInformation
	{
		std::string name_{};
		UFunction *ufunction_{};
		UFunctionHookPrototype hook_function_{};
		FunctionHookType hook_type_{FunctionHookType::kPre};
		FunctionHookAbsorb hook_absorb_{FunctionHookAbsorb::kDoNotAbsorb};
	};

	UFunctionHookPrototype original_function_{};

	UFunctionHooks(void) = default;

	HookResult AddHook(const UFunctionHookInformation &ufunction_hook_information)
	{
		return AddHook(ufunction_hook_information.name_, ufunction_hook_information.hook_function_,
					   ufunction_hook_information.hook_type_, ufunction_hook_information.hook_absorb_);
	}

	template <typename... Args>
	ExecuteHookResult ExecuteHook(UFunction *ufunction, Args &&...args) const
	{
		if (!ready_.load(std::memory_order_acquire) || !original_function_)
		{
			return ExecuteHookResult::kFailedNoOriginalFunctionFound;
		}

		const auto hooks{GetHooks(ufunction)};

		if (!hooks)
		{
			original_function_(std::forward<Args>(args)...);
		}
		else
		{
			auto absorbs{false};
			for (auto &ufunction_hook_information : hooks->pre_hooks_)
			{
				ufunction_hook_information.hook_function_(args...);
				absorbs |= ufunction_hook_information.hook_absorb_ == FunctionHookAbsorb::kAbsorb;
			}

			if (!absorbs)
			{
				original_function_(std::forward<Args>(args)...);
			}

			for (auto &ufunction_hook_information : hooks->post_hooks_)
			{
				ufunction_hook_information.hook_function_(args...);
			}
		}
		return ExecuteHookResult::kSuccess;
	}

	void SetOriginalFunction(UFunctionHookPrototype original_function)
	{
		original_function_ = original_function;
		ready_.store(true, std::memory_order_release);
	}

private:
	struct UFunctionHooksInformation
	{
		using Hooks = std::vector<UFunctionHookInformation>;
		Hooks pre_hooks_{};	 // = Hooks(kMaxHookCountPerUFunction);
		Hooks post_hooks_{}; // = Hooks(kMaxHookCountPerUFunction);
	};

	using Hooks = std::vector<UFunctionHooksInformation>;
	Hooks ufunction_internal_index_to_hook_information_ = Hooks(kSizeOfUFunctionInternalIndex);
	inline static std::atomic<bool> ready_{};

	HookResult
	AddHook(const std::string &ufunction_as_string, UFunctionHookPrototype hook_function,
			FunctionHookType hook_type = FunctionHookType::kPre,
			FunctionHookAbsorb hook_absorb = FunctionHookAbsorb::kDoNotAbsorb)
	{
		if (hook_type != FunctionHookType::kPre && hook_absorb == FunctionHookAbsorb::kAbsorb)
		{
			return HookResult::kFailedIncorrectHookTypeAndHookAbsorb;
		}

		if (auto ufunction_object{reinterpret_cast<UFunction *>(UObject::FindObject<UFunction>(ufunction_as_string.c_str()))})
		{
			const auto &index{ufunction_object->Index};

			if (index >= kSizeOfUFunctionInternalIndex)
			{
				return HookResult::kFailedUFunctionOutOfBounds;
			}

			UFunctionHookInformation ufunction_hook_information{ufunction_as_string, ufunction_object,
																hook_function, hook_type, hook_absorb};
			switch (hook_type)
			{
			case FunctionHookType::kPre:
			{
				if (const auto hook_count{ufunction_internal_index_to_hook_information_[index].pre_hooks_.size()}; hook_count < kMaxHookCountPerUFunction)
				{
					ufunction_internal_index_to_hook_information_[index].pre_hooks_.push_back(ufunction_hook_information);
				}
				else
				{
					return HookResult::kFailedOverMaxHookCount;
				}
				break;
			}
			case FunctionHookType::kPost:
			{
				if (const auto hook_count{ufunction_internal_index_to_hook_information_[index].post_hooks_.size()}; hook_count < kMaxHookCountPerUFunction)
				{
					ufunction_internal_index_to_hook_information_[index].post_hooks_.push_back(ufunction_hook_information);
				}
				else
				{
					return HookResult::kFailedOverMaxHookCount;
				}
				break;
			}
			default:
			{
				return HookResult::kFailedUnknownHookType;
				break;
			}
			}
			return HookResult::kSuccess;
		}
		return HookResult::kFailedToFindUFunction;
	}

	const UFunctionHooksInformation *GetHooks(const UFunction *ufunction_object) const
	{
		const auto &index{ufunction_object->Index};
		if (ufunction_object && index < kSizeOfUFunctionInternalIndex)
		{
			const auto &hook_information{ufunction_internal_index_to_hook_information_[index]};
			if (hook_information.pre_hooks_.size() == 0 && hook_information.post_hooks_.size() == 0)
			{
				return nullptr;
			}
			else
			{
				return &hook_information;
			}
		}
		else
		{
			return nullptr;
		}
	}
};

extern UFunctionHooks<ProcessEventPrototype> processevent_hooks;
// extern UFunctionHooks<ProcessInternalPrototype> processinternal_hooks;
// extern UFunctionHooks<CallFunctionPrototype> callfunction_hooks;

#define PROCESSEVENT_HOOK(function_hook_name) void __fastcall function_hook_name(UObject *calling_uobject, UFunction *calling_ufunction, void *parameters)
