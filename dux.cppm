module;

#include <stdint.h>

#include <type_traits>
#include <typeinfo>
#include <memory>

#include "duktape.h"

export module dux;

import wx;

export namespace Dux {

#pragma region duktape HAL helpers (reference named of duk_xxx)

bool duk_always_construct(duk_context *ctx) {
	if (duk_is_constructor_call(ctx))
		return false;
	auto nargs = duk_get_top(ctx);
	duk_push_current_function(ctx);
	duk_insert(ctx, 0);
	duk_new(ctx, nargs);
	return true;
}

constexpr duk_uint_t DUK_DEFPROP_PUBLIC_CONST =
	DUK_DEFPROP_HAVE_VALUE |		// value
	DUK_DEFPROP_CLEAR_WRITABLE |	// unwritable
	DUK_DEFPROP_SET_ENUMERABLE |	// enumerable
	DUK_DEFPROP_CLEAR_CONFIGURABLE;	// unconfigurable
constexpr duk_uint_t DUK_DEFPROP_PRIVATE_CONST =
	DUK_DEFPROP_HAVE_VALUE |		// value
	DUK_DEFPROP_CLEAR_WRITABLE |	// unwritable
	DUK_DEFPROP_CLEAR_ENUMERABLE |	// unenumerable
	DUK_DEFPROP_CLEAR_CONFIGURABLE;	// unconfigurable
						
void *duk_set_c_instance_nassert(duk_context *ctx, duk_size_t size, duk_idx_t idx = -1) {
	// ... object(idx) ...
	duk_push_string(ctx, "__c");
	// ... object(idx - 1) ... "__c"
	auto pC = duk_push_fixed_buffer(ctx, size);
	// ... object(idx - 2) ... "__c" buffer
	if (idx < 0) idx -= 2;
	duk_def_prop(ctx, idx, DUK_DEFPROP_PUBLIC_CONST); // object(idx)["__c"] = buffer
	// ... object(idx) ...
	return pC;
}
void *duk_get_c_instance_nassert(duk_context *ctx, duk_size_t *size, duk_idx_t idx = -1) {
	// ... object(idx) ...
	duk_get_prop_string(ctx, idx, "__c");
	// ... object(idx) ... object(idx)["__c"]
	auto pC = duk_get_buffer(ctx, -1, size); // pC = object(idx)["__c"]
	// ... object(idx) ... object(idx)["__c"]
	duk_pop(ctx);
	// ... object(idx) ...
	return pC;
}

void duk_set_c_pointer_nassert(duk_context *ctx, void *ptr, duk_idx_t idx = -1) {
	// ... object(idx) ...
	duk_push_string(ctx, "__c");
	// ... object(idx - 1) ... "__c"
	duk_push_pointer(ctx, ptr);
	// ... object(idx - 2) ... "__c" ptr
	if (idx < 0) idx -= 2;
	duk_def_prop(ctx, idx, DUK_DEFPROP_PUBLIC_CONST); // object(idx)["__c"] = ptr
	// ... object(idx) ...
}
void *duk_get_c_pointer_nassert(duk_context *ctx, duk_idx_t idx = -1) {
	// ... object(idx) ...
	duk_get_prop_string(ctx, idx, "__c");
	// ... object(idx) ... "__c"
	auto pC = duk_get_pointer(ctx, -1); // pC = object(idx)["__c"]
	// ... object(idx) ... "__c"
	duk_pop(ctx);
	// ... object(idx) ...
	return pC;
}

void *duk_get_c_auto_nassert(duk_context *ctx, duk_size_t *size, duk_idx_t idx = -1) {
	void *pC = (void *)-1;
	*size = (duk_size_t)-1;
	// ... object(idx) ...
	duk_get_prop_string(ctx, idx, "__c");
	// ... object(idx) ... object(idx)["__c"]
	if (duk_is_buffer(ctx, -1))
		pC = duk_get_buffer(ctx, -1, size); // pC = object(idx)["__c"]
	else if (duk_is_pointer(ctx, -1))
		pC = duk_get_pointer(ctx, -1); // pC = object(idx)["__c"]
	// ... object(idx) ... "__c"
	duk_pop(ctx);
	// ... object(idx) ...
	return pC;
}

void duk_set_self_name(duk_context *ctx, const char *name, duk_idx_t idx = -1) {
	// ... object(idx) ...
	duk_push_string(ctx, "name");
	// ... object(idx - 1) ... "name"
	duk_push_string(ctx, name);
	// ... object(idx - 2) ... "name" name
	if (idx < 0) idx -= 2;
	duk_def_prop(ctx, idx - 2, DUK_DEFPROP_PUBLIC_CONST); // object(idx).name = name
	// ... object(idx) ...
}

void duk_set_destructor(duk_context *ctx, duk_c_function destructor, duk_idx_t idx = -1) {
	// ... object(idx) ...
	duk_push_c_function(ctx, destructor, 1);
	// ... object(idx - 1) ... destructor
	if (idx < 0) idx -= 1;
	duk_set_finalizer(ctx, idx); // object(idx).$destructor = destructor
	// ... object(idx) ...
}

template<class AnyClass>
inline AnyClass *duk_set_c_instance(duk_context *ctx, duk_idx_t idx = -1)
{ return (AnyClass *)duk_set_c_instance_nassert(ctx, sizeof(AnyClass), idx); }
template<class AnyClass>
AnyClass *duk_get_c_instance(duk_context *ctx, duk_idx_t idx = -1) {
	duk_size_t size = 0;
	auto pInstance = (AnyClass *)duk_get_c_instance_nassert(ctx, &size, idx);
	if (!pInstance)
		duk_error(ctx, DUK_ERR_TYPE_ERROR,
				  "C instance of \"%s\" is invalid",
				  typeid(AnyClass).name());
	if (size != sizeof(AnyClass))
		duk_error(ctx, DUK_ERR_TYPE_ERROR,
				  "C instance size %d of \"%s\" invalid, should be %d",
				  (int)size, typeid(AnyClass).name(), (int)sizeof(AnyClass));
	return pInstance;
}

template<class AnyClass>
inline void duk_set_c_pointer(duk_context *ctx, AnyClass *pInstance, duk_idx_t idx = -1)
{ duk_set_c_pointer_nassert(ctx, (void *)pInstance, idx); }
template<class AnyClass>
AnyClass *duk_get_c_pointer(duk_context *ctx, duk_idx_t idx = -1) {
	auto pInstance = (AnyClass *)duk_get_c_pointer_nassert(ctx, idx);
	if (!pInstance)
		duk_error(ctx, DUK_ERR_TYPE_ERROR,
				  "C pointer of \"%s\" is invalid",
				  typeid(AnyClass).name());
	return pInstance;
}

template<class AnyClass>
inline AnyClass *duk_get_c_auto(duk_context *ctx, duk_idx_t idx = -1) {
	duk_size_t size = 0;
	auto pInstance = (AnyClass *)duk_get_c_auto_nassert(ctx, &size, idx);
	if (!pInstance)
		duk_error(ctx, DUK_ERR_TYPE_ERROR,
				  "C object of \"%s\" is invalid",
				  typeid(AnyClass).name());
	if (pInstance == (void *)-1)
		duk_error(ctx, DUK_ERR_TYPE_ERROR,
				  "C object of \"%s\" is invalid, '__c' is not buffer or pointer ",
				  typeid(AnyClass).name());
	if (size != sizeof(AnyClass) && size != (duk_size_t)-1)
		duk_error(ctx, DUK_ERR_TYPE_ERROR,
				  "C object size %d of \"%s\" invalid, should be %d",
				  (int)size, typeid(AnyClass).name(), (int)sizeof(AnyClass));
	return pInstance;
}

#pragma region C++/JavaScript object exchange helpers

#pragma region Push C++ object to JavaScript
template<class AnyType>
inline duk_ret_t duk_push_c(duk_context *ctx, AnyType val);

template<>
inline duk_ret_t duk_push_c<std::nullptr_t>(duk_context *ctx, std::nullptr_t)
{ return (duk_push_null(ctx), 1); }
template<>
inline duk_ret_t duk_push_c<bool>(duk_context *ctx, bool val) 
{ return (duk_push_boolean(ctx, val), 1); }
template<>
inline duk_ret_t duk_push_c<int32_t>(duk_context *ctx, int32_t val) 
{ return (duk_push_int(ctx, val), 1); }
template<>
inline duk_ret_t duk_push_c<int16_t>(duk_context *ctx, int16_t val) 
{ return (duk_push_int(ctx, val), 1); }
template<>
inline duk_ret_t duk_push_c<uint32_t>(duk_context *ctx, uint32_t val) 
{ return (duk_push_uint(ctx, val), 1); }
template<>
inline duk_ret_t duk_push_c<uint16_t>(duk_context *ctx, uint16_t val) 
{ return (duk_push_uint(ctx, val), 1); }
template<>
inline duk_ret_t duk_push_c<double>(duk_context *ctx, double val) 
{ return (duk_push_number(ctx, val), 1); }
template<>
inline duk_ret_t duk_push_c<float>(duk_context *ctx, float val) 
{ return (duk_push_number(ctx, val), 1); }
template<>
inline duk_ret_t duk_push_c<const char *>(duk_context *ctx, const char *val) 
{ return (duk_push_string(ctx, val), 1); }
#pragma endregion

template<class... Args>
inline duk_ret_t duk_push_c(duk_context *ctx, Args... args)
{ return (duk_push_c(ctx, args) + ...); }

#pragma region Converts JavaScript object to C++
template<class AnyType>
inline AnyType duk_get(duk_context *ctx, duk_idx_t idx);

template<>
inline std::nullptr_t duk_get(duk_context *ctx, duk_idx_t idx)
{ return (duk_to_null(ctx, idx), nullptr); }
template<>
inline bool duk_get(duk_context *ctx, duk_idx_t idx)
{ return duk_to_boolean(ctx, idx) != 0; }
template<>
inline int32_t duk_get(duk_context *ctx, duk_idx_t idx)
{ return duk_to_int(ctx, idx); }
template<>
inline int16_t duk_get(duk_context *ctx, duk_idx_t idx)
{ return (int16_t)duk_to_int(ctx, idx); }
template<>
inline uint32_t duk_get(duk_context *ctx, duk_idx_t idx)
{ return duk_to_uint32(ctx, idx); }
template<>
inline uint16_t duk_get(duk_context *ctx, duk_idx_t idx)
{ return duk_to_uint16(ctx, idx); }
template<>
inline double duk_get(duk_context *ctx, duk_idx_t idx)
{ return duk_to_number(ctx, idx); }
template<>
inline float duk_get(duk_context *ctx, duk_idx_t idx)
{ return (float)duk_to_number(ctx, idx); }
template<>
inline const char *duk_get(duk_context *ctx, duk_idx_t idx) {
	if (auto lpsz = duk_to_string(ctx, idx))
		return lpsz;
	duk_type_error(ctx, "Cannot convert from_js const char *");
	return nullptr;
}
#pragma endregion

#pragma region Requires JavaScript object to C++
template<class AnyType>
inline AnyType duk_require(duk_context *ctx, duk_idx_t idx);
template<>
inline std::nullptr_t duk_require(duk_context *ctx, duk_idx_t idx) {
	duk_require_null(ctx, idx);
	return nullptr;
}
template<>
inline bool duk_require(duk_context *ctx, duk_idx_t idx)
{ return duk_require_boolean(ctx, idx) != 0; }
template<>
inline int32_t duk_require(duk_context *ctx, duk_idx_t idx)
{ return duk_require_int(ctx, idx); }
template<>
inline int16_t duk_require(duk_context *ctx, duk_idx_t idx)
{ return (int16_t)duk_require_int(ctx, idx); }
template<>
inline uint32_t duk_require(duk_context *ctx, duk_idx_t idx)
{ return (uint32_t)duk_require_uint(ctx, idx); }
template<>
inline uint16_t duk_require(duk_context *ctx, duk_idx_t idx)
{ return (uint16_t)duk_require_uint(ctx, idx); }
template<>
inline double duk_require(duk_context *ctx, duk_idx_t idx)
{ return (double)duk_require_number(ctx, idx); }
template<>
inline float duk_require(duk_context *ctx, duk_idx_t idx)
{ return (float)duk_require_number(ctx, idx); }
template<>
inline const char *duk_require(duk_context *ctx, duk_idx_t idx) {
	if (auto lpsz = duk_require_string(ctx, idx))
		return lpsz;
	duk_type_error(ctx, "Cannot convert from_js const char *");
	return nullptr;
}
#pragma endregion

#pragma region Special parameter tags for function reflection

enum class tag_type {
	strict_type,
	optional_type,
};
template<tag_type> struct duk_tag;

template<class AnyType>
struct remove_tags_t { using type = AnyType; };
template<class AnyType>
using remove_tags = typename remove_tags_t<AnyType>::type;
template<class AnyType>
using remove_tag = typename remove_tags_t<AnyType>::removed;

template<class AnyType>
inline AnyType duk_from_stack(duk_context *ctx, duk_idx_t idx)
{ return duk_get<AnyType>(ctx, idx); }

// --- strict tag ---

template<class AnyType>
struct strict {};

template<class AnyType>
struct remove_tags_t<strict<AnyType>> : remove_tags_t<AnyType> { using removed = AnyType; };

template<class AnyType>
constexpr bool is_strict_para = false;
template<class AnyType>
constexpr bool is_strict_para<strict<AnyType>> = true;

template<class AnyType>
concept strict_para = is_strict_para<AnyType>;	

template<strict_para AnyType>
inline remove_tags<AnyType> duk_from_stack(duk_context *ctx, duk_idx_t idx)
{ return duk_require<remove_tags<AnyType>>(ctx, idx); }

// --- optional tag ---

template<class AnyType, AnyType def_val>
struct optional { static constexpr AnyType default_value = def_val; };

template<class AnyType, AnyType def_val>
struct remove_tags_t<optional<AnyType, def_val>> : remove_tags_t<AnyType> { using removed = AnyType; };

template<class AnyType>
constexpr bool is_optional_para = false;
template<class AnyType, AnyType def_val>
constexpr bool is_optional_para<optional<AnyType, def_val>> = true;

template<class AnyType>
concept optional_para = is_optional_para<AnyType>;

template<optional_para AnyType>
inline remove_tags<AnyType> duk_from_stack(duk_context *ctx, duk_idx_t idx) {
	if (duk_is_undefined(ctx, idx))
		return AnyType::default_value;
	return duk_from_stack<remove_tags<AnyType>>(ctx, idx);
}

#pragma endregion

#pragma endregion

template<class...Para>
concept no_lifecycle_conversions = (WX::DestructorEffectless<remove_tags<Para>> || ...);
template<class...Para>
concept lifecycle_conversions = (WX::DestructorEffective<remove_tags<Para>> || ...);

// --- internal invokers ---
template<class RetType, no_lifecycle_conversions...Args, size_t...ind>
inline duk_ret_t duk_invoke(duk_context *ctx, WX::FunctionType auto f, duk_idx_t arg_base, std::index_sequence<ind...>) {
	if constexpr (std::is_void_v<RetType>)
		 return (f(duk_from_stack<Args>(ctx, arg_base + ind)...), 0);
	else return duk_push_c<RetType>(ctx, f(duk_from_stack<Args>(ctx, arg_base + ind)...));
}
template<class AnyClass, class RetType, no_lifecycle_conversions...Args, size_t...ind>
inline duk_ret_t duk_invoke(duk_context *ctx, AnyClass *pClass, WX::MethodType auto f, duk_idx_t arg_base, std::index_sequence<ind...>) {
	if constexpr (std::is_void_v<RetType>)
		 return ((pClass->*f)(duk_from_stack<Args>(ctx, arg_base + ind)...), 0);
	else return duk_push_c<RetType>(ctx, (pClass->*f)(duk_from_stack<Args>(ctx, arg_base + ind)...));
}

template<class AnyClass, no_lifecycle_conversions...Args, size_t...ind>
inline AnyClass *duk_invoke_constructor(duk_context *ctx, duk_idx_t arg_base, std::index_sequence<ind...>)
{ return new AnyClass(f(duk_from_stack<Args>(ctx, arg_base + ind)...)); }
template<class AnyClass, no_lifecycle_conversions...Args, size_t...ind>
inline AnyClass *duk_invoke_constructor_(duk_context *ctx, void *pMem, duk_idx_t arg_base, std::index_sequence<ind...>)
{ return new(pMem) AnyClass(f(duk_from_stack<Args>(ctx, arg_base + ind)...)); }

// --- native invokers ---
template<bool is_strict = false, class RetType, class...Args>
inline duk_ret_t duk_invoke(duk_context *ctx, RetType(*f)(Args...), duk_idx_t arg_base = 0) {
	if constexpr (is_strict)
		 return duk_invoke<RetType, strict<Args>...>(ctx, f, arg_base, std::index_sequence_for<Args...>{});
	else return duk_invoke<RetType, Args...>(ctx, f, arg_base, std::index_sequence_for<Args...>{});
}
template<bool is_strict = false, class AnyClass, class RetType, class...Args>
inline duk_ret_t duk_invoke(duk_context *ctx, AnyClass *pClass, RetType(AnyClass:: *f)(Args...), duk_idx_t arg_base = 0) {
	if constexpr (is_strict)
		 return duk_invoke<AnyClass, RetType, strict<Args>...>(ctx, pClass, f, arg_base, std::index_sequence_for<Args...>{});
	else return duk_invoke<AnyClass, RetType, Args...>(ctx, pClass, f, arg_base, std::index_sequence_for<Args...>{});
}

template<bool is_strict = false, class AnyClass, class...Args>
inline void duk_invoke_constructor_native(duk_context *ctx, duk_idx_t obj_base, duk_idx_t arg_base = 0) {
	if constexpr (is_strict)
		 duk_set_c_pointer(ctx, duk_invoke_constructor<AnyClass, strict<Args>...>(ctx, arg_base, std::index_sequence_for<Args...>{}), obj_base);
	else duk_set_c_pointer(ctx, duk_invoke_constructor<AnyClass, Args...>(ctx, arg_base,  std::index_sequence_for<Args...>{}), obj_base);
}
template<bool is_strict = false, class AnyClass, class...Args>
inline void duk_invoke_constructor(duk_context *ctx, duk_idx_t obj_base, duk_idx_t arg_base = 0) {
	auto pInst = duk_set_c_instance<AnyClass>(ctx, obj_base);
	if constexpr (is_strict)
		 duk_invoke_constructor<AnyClass, strict<Args>...>(ctx, pInst, arg_base, std::index_sequence_for<Args...>{});
	else duk_invoke_constructor<AnyClass, Args...>(ctx, pInst, arg_base,  std::index_sequence_for<Args...>{});
}

// --- wrapped invokers ---
template<WX::FunctionAllType auto fn, bool is_strict = false>
duk_ret_t duk_invoke_wrapped_of(duk_context *ctx) {
	return duk_invoke<is_strict>(ctx, fn);
}
//template<WX::MethodType auto fn, bool is_strict = false>
//duk_ret_t duk_invoke_wrapped_of(duk_context *ctx) {
//	auto pParent = duk_get_c_auto<WX::MethodParentOf<decltype(fn)>>(ctx);
//	return duk_invoke<is_strict>(ctx, pParent, fn);
//}

template<class AnyClass, bool is_strict, class...Args>
duk_ret_t duk_constructor_wrapped_of(duk_context *ctx) {
	if (duk_always_construct(ctx))
		return 1;
	duk_push_this(ctx);
	auto pInstance = duk_set_c_instance<AnyClass>(ctx);
	duk_always_construct<Args...>(ctx, pInstance);
}

// --- function pushers and adders ---
template<WX::FunctionAllType auto fn, bool is_strict = false>
void duk_push_function(duk_context *ctx) {
	duk_push_c_function(ctx, duk_invoke_wrapped_of<fn, is_strict>, (duk_idx_t)WX::ArgCountOf(fn));
}
template<WX::FunctionAllType auto fn, bool is_strict = false>
void duk_add_function(duk_context *ctx, const char *name, duk_idx_t idx = -1) {
	duk_push_string(ctx, name);
	duk_push_function<fn, is_strict>(ctx);
	if (idx < 0) idx -= 2;
	duk_def_prop(ctx, idx, DUK_DEFPROP_PUBLIC_CONST);
}

#pragma endregion

class Context {
public:
	class Heap {
	public:
		virtual void *Alloc(duk_size_t size) = 0;
		virtual void *Realloc(void *ptr, duk_size_t size) = 0;
		virtual void Free(void *ptr) = 0;
	private:
		friend class Context;
		static void *CAlloc(void *udata, duk_size_t size) {
			auto pThis = reinterpret_cast<Context *>(udata);
			return pThis->pHeap->Alloc(size);
		}
		static void *CRealloc(void *udata, void *ptr, duk_size_t size) {
			auto pThis = reinterpret_cast<Context *>(udata);
			return pThis->pHeap->Realloc(ptr, size);
		}
		static void CFree(void *udata, void *ptr) {
			auto pThis = reinterpret_cast<Context *>(udata);
			pThis->pHeap->Free(ptr);
		}
	} *pHeap = nullptr;
private:
	duk_context *ctx = nullptr;
public:
	Context() : 
		ctx(duk_create_heap(nullptr, nullptr, nullptr, this, CFatal)) {}
	Context(std::nullptr_t) {}
	Context(Heap &heap) :
		pHeap(&heap),
		ctx(duk_create_heap(
			Heap::CAlloc,
			Heap::CRealloc,
			Heap::CFree,
			this,
			CFatal)) {}
	~Context() { Destroy(); }
public:
	void Destroy() {
		if (ctx) 
			duk_destroy_heap(ctx);
		ctx = nullptr;
	}
protected:
	virtual void OnFatal(const char *msg) {}
private:
	static void CFatal(void *udata, const char *msg) {
		auto pThis = reinterpret_cast<Context *>(udata);
		pThis->OnFatal(msg);
	}
public:
	inline operator duk_context *() { return ctx; }
	inline operator const duk_context *() const { return ctx; }
};
using Heap = Context::Heap;

}
