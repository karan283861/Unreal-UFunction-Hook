#pragma once

#include <format>
#include <vector>
#include <string>
#include <functional>

class UObject;
class UFunction;
class UStruct;

constexpr size_t kSizeOfUFunctionInternalIndex{2000000};
constexpr size_t kMaxHookCountPerUFunction{1};

inline std::function<void(const std::string &log_string)> log_function{};
inline std::function<UFunction *(const std::string &ufunction_name)> get_ufunction_from_name{};
inline std::function<int(const UFunction *ufunction_object)> get_ufunction_id{};
inline std::function<std::string(UObject *uobject_object)> get_uobject_name{};
inline std::function<bool(const UFunction *ufunction_object)> is_ufunction_native{};

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
		// int ufunction_id_{};
		// UFunction *ufunction_{};
		UFunctionHookPrototype hook_function_{};
		FunctionHookType hook_type_{FunctionHookType::kPre};
		FunctionHookAbsorb hook_absorb_{FunctionHookAbsorb::kDoNotAbsorb};
	};

	UFunctionHookPrototype original_function_{};

	UFunctionHooks(void) = default;

	HookResult AddHook(const UFunctionHookInformation &hook_information)
	{
		if (hook_information.hook_type_ != FunctionHookType::kPre && hook_information.hook_absorb_ == FunctionHookAbsorb::kAbsorb)
		{
			return HookResult::kFailedIncorrectHookTypeAndHookAbsorb;
		}

		if (auto ufunction_object{get_ufunction_from_name(hook_information.name_)})
		{
			const auto index{get_ufunction_id(ufunction_object)};

			if (index >= kSizeOfUFunctionInternalIndex || index < 0)
			{
				return HookResult::kFailedUFunctionOutOfBounds;
			}

			// UFunctionHookInformation ufunction_hook_information{ufunction_as_string, ufunction_object,
			// 													hook_function, hook_type, hook_absorb};
			switch (hook_information.hook_type_)
			{
			case FunctionHookType::kPre:
			{
				if (const auto hook_count{ufunction_internal_index_to_hook_information_[index].pre_hooks_.size()}; hook_count < kMaxHookCountPerUFunction)
				{
					ufunction_internal_index_to_hook_information_[index].pre_hooks_.push_back(hook_information);
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
					ufunction_internal_index_to_hook_information_[index].post_hooks_.push_back(hook_information);
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

	const UFunctionHooksInformation *GetHooks(const UFunction *ufunction_object) const
	{
		const auto index{get_ufunction_id(ufunction_object)};
		if (ufunction_object && index < kSizeOfUFunctionInternalIndex && index >= 0)
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

template <typename UFunctionHookInformation>
std::string ValidateUFunctionHookResult(const HookResult &hook_result, const UFunctionHookInformation &hook_information)
{
	const auto &name{hook_information.name_};
	const auto &is_absorbing{hook_information.hook_absorb_ == FunctionHookAbsorb::kAbsorb};
	switch (hook_result)
	{
	case HookResult::kSuccess:
	{
		return std::format("Successfully hooked {0}{1}", name, is_absorbing ? " [ABSORBING]" : "");
		break;
	}
	case HookResult::kFailedIncorrectHookTypeAndHookAbsorb:
	{
		return std::format("Failed to hook {0} due to incorrect hook type and hook absorb", name);
		break;
	}
	case HookResult::kFailedToFindUFunction:
	{
		return std::format("Failed to hook {0} as the UFunction was not found", name);
		break;
	}
	case HookResult::kFailedUFunctionOutOfBounds:
	{
		return std::format("Failed to hook {0} as the UFunction index was out of bounds", name);
		break;
	}
	case HookResult::kFailedOverMaxHookCount:
	{
		return std::format("Failed to hook {0} as the UFunction already has maximum number of hooks", name);
		break;
	}
	case HookResult::kFailedUnknownHookType:
	{
		return std::format("Failed to hook {0} due to unknown hook type", name);
		break;
	}
	default:
	{
		return std::format("Hooking {0} resulted in unhandled behaviour", name);
		break;
	}
	}
	return "Unhandled error";
}

namespace UE3
{
	using ProcessEventPrototype = void(__fastcall *)(UObject *calling_uobject,
													 void *unused, UFunction *calling_ufunction,
													 void *parameters, void *result);

	void __fastcall ProcessEventHook(UObject *calling_uobject,
									 void *unused, UFunction *calling_ufunction,
									 void *parameters, void *result);

	inline ProcessEventPrototype original_processevent{};

	class FFrame
	{
	public:
		void *vmt_{};
		char unknown_data_00_[12]{};
		UStruct *node_{};
		UObject *object_{};
		unsigned char *code_{};
		char *locals_{};
		int line_nums_{};
		FFrame *previous_frame_{};
	};

	using ProcessInternalPrototype = void(__fastcall *)(UObject *calling_uobject,
														void *unused, FFrame &stack,
														void *result);

	void __fastcall ProcessInternalHook(UObject *calling_uobject,
										void *unused, FFrame &stack,
										void *result);

	inline ProcessInternalPrototype original_processinternal{};

	using CallFunctionPrototype = void(__fastcall *)(UObject *calling_uobject,
													 void *unused, FFrame &stack,
													 void *result, UFunction *calling_ufunction);

	void __fastcall CallFunctionHook(UObject *calling_uobject,
									 void *unused, FFrame &stack,
									 void *result, UFunction *calling_ufunction);

	inline CallFunctionPrototype original_callfunction{};

	inline UFunctionHooks<ProcessEventPrototype> processevent_hooks{};
	inline UFunctionHooks<ProcessInternalPrototype> processinternal_hooks{};
	inline UFunctionHooks<CallFunctionPrototype> callfunction_hooks{};
}

namespace UE4
{
	using ProcessEventPrototype = void(__fastcall *)(UObject *calling_uobject,
													 UFunction *calling_ufunction,
													 void *parameters);

	void __fastcall ProcessEventHook(UObject *calling_uobject,
									 UFunction *calling_ufunction,
									 void *parameters);

	inline ProcessEventPrototype original_processevent{};

	class FFrame
	{
	public:
		void *vmt_{};
		char unknown_data_00_[8]{};
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

	inline ProcessInternalPrototype original_processinternal{};

	using CallFunctionPrototype = void(__fastcall *)(UObject *calling_uobject,
													 FFrame &stack,
													 void *result, UFunction *calling_ufunction);

	void __fastcall CallFunctionHook(UObject *calling_uobject,
									 FFrame &stack,
									 void *result, UFunction *calling_ufunction);

	inline CallFunctionPrototype original_callfunction{};

	inline UFunctionHooks<ProcessEventPrototype> processevent_hooks{};
	inline UFunctionHooks<ProcessInternalPrototype> processinternal_hooks{};
	inline UFunctionHooks<CallFunctionPrototype> callfunction_hooks{};
}

namespace UE5
{
	using ProcessEventPrototype = void(__fastcall *)(UObject *calling_uobject,
													 UFunction *calling_ufunction,
													 void *parameters);

	void __fastcall ProcessEventHook(UObject *calling_uobject,
									 UFunction *calling_ufunction,
									 void *parameters);

	inline ProcessEventPrototype original_processevent{};

	class FFrame
	{
	public:
		void *vmt_{};
		char unknown_data_00_[8]{};
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

	inline ProcessInternalPrototype original_processinternal{};

	using CallFunctionPrototype = void(__fastcall *)(UObject *calling_uobject,
													 FFrame &stack,
													 void *result, UFunction *calling_ufunction);

	void __fastcall CallFunctionHook(UObject *calling_uobject,
									 FFrame &stack,
									 void *result, UFunction *calling_ufunction);

	inline CallFunctionPrototype original_callfunction{};

	inline UFunctionHooks<ProcessEventPrototype> processevent_hooks{};
	inline UFunctionHooks<ProcessInternalPrototype> processinternal_hooks{};
	inline UFunctionHooks<CallFunctionPrototype> callfunction_hooks{};
}

#define UE3_PROCESSEVENT_HOOK(function_hook_name) void __fastcall function_hook_name(UObject *calling_uobject, void *unused, UFunction *calling_ufunction, void *parameters, void *result)
#define UE3_PROCESSINTERNAL_HOOK(function_hook_name) void __fastcall function_hook_name(UObject *calling_uobject, void *unused, UE3::FFrame &stack, void *result)

#define UE4_PROCESSEVENT_HOOK(function_hook_name) void __fastcall function_hook_name(UObject *calling_uobject, UFunction *calling_ufunction, void *parameters)

#define UE5_PROCESSEVENT_HOOK(function_hook_name) void __fastcall function_hook_name(UObject *calling_uobject, UFunction *calling_ufunction, void *parameters)

// TODO: Implement UE4 & UE5 macros for ProcessInternal

void SetupUFunctionHooks(size_t base_address);
void PerformUFunctionHooks(void);