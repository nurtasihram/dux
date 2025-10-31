module;

#include "duktape.h"

#include <stdint.h>
#include <typeinfo>
#include <memory>

export module dux;


export namespace Dux {

#pragma region duktape HAL helpers (reference named of duk_xxx)

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
	auto pC = duk_to_pointer(ctx, -1); // pC = object(idx)["__c"]
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
inline AnyClass *duk_put_c_instance(duk_context *ctx, duk_idx_t idx = -3) {
	return (AnyClass *)duk_put_c_instance_nassert(ctx, sizeof(AnyClass), idx);
}
template<class AnyClass>
AnyClass *duk_get_c_instance(duk_context *ctx, duk_idx_t idx = -1) {
	duk_size_t size = 0;
	auto pInstance = (AnyClass *)duk_get_c_instance_nassert(ctx, &size, idx);
	if (!pInstance) {
		duk_error(ctx, DUK_ERR_TYPE_ERROR,
				  "C instance of %s is null",
				  typeid(AnyClass).name());
		duk_throw(ctx);
	}
	if (size != sizeof(AnyClass)) {
		duk_error(ctx, DUK_ERR_TYPE_ERROR,
				  "C instance size %d of %s invalid, should be %d",
				  (int)size, typeid(AnyClass).name(), (int)sizeof(AnyClass));
		duk_throw(ctx);
	}
	return pInstance;
}

template<class AnyClass>
inline void duk_set_c_pointer(duk_context *ctx, AnyClass *pInstance, duk_idx_t idx = -3) {
	duk_set_c_pointer_nassert(ctx, (void *)pInstance, idx);
}
template<class AnyClass>
AnyClass *duk_get_c_pointer(duk_context *ctx, duk_idx_t idx = -1) {
	auto pInstance = (AnyClass *)duk_get_c_pointer_nassert(ctx, idx);
	if (!pInstance) {
		duk_error(ctx, DUK_ERR_TYPE_ERROR,
				  "C pointer of %s is null",
				  typeid(AnyClass).name());
		duk_throw(ctx);
	}
	return pInstance;
}

#pragma endregion

#pragma region Conversion from C++ to JS
template<class AnyType>
void duk_push_c(duk_context *ctx, AnyType val);
template<>
void duk_push_c(duk_context *ctx, std::nullptr_t)
{ duk_push_null(ctx); }
template<>
void duk_push_c<bool>(duk_context *ctx, bool val)
{ duk_push_boolean(ctx, val); }
void duk_push_c(duk_context *ctx, int32_t val)
{ duk_push_int(ctx, val); }
void duk_push_c(duk_context *ctx, int16_t val)
{ duk_push_int(ctx, val); }
template<>
void duk_push_c(duk_context *ctx, uint32_t val)
{ duk_push_uint(ctx, val); }
template<>
void duk_push_c(duk_context *ctx, uint16_t val)
{ duk_push_uint(ctx, val); }
template<>
void duk_push_c(duk_context *ctx, double val)
{ duk_push_number(ctx, val); }
template<>
void duk_push_c(duk_context *ctx, float val)
{ duk_push_number(ctx, val); }
template<>
void duk_push_c(duk_context *ctx, const char *val)
{ duk_push_string(ctx, val); }

template<class... Args>
void duk_push_c(duk_context *ctx, Args... args)
{ (duk_push_c(ctx, args), ...); }
#pragma endregion

#pragma region Conversion from JS to C++
template<class AnyType>
AnyType duk_get(duk_context *ctx, duk_idx_t idx);
template<>
std::nullptr_t duk_get(duk_context *ctx, duk_idx_t idx) {
	duk_to_null(ctx, idx);
	return nullptr;
}
template<>
bool duk_get(duk_context *ctx, duk_idx_t idx)
{ return duk_to_boolean(ctx, idx) != 0; }
template<>
int32_t duk_get(duk_context *ctx, duk_idx_t idx)
{ return duk_to_int(ctx, idx); }
template<>
int16_t duk_get(duk_context *ctx, duk_idx_t idx)
{ return (int16_t)duk_to_int(ctx, idx); }
template<>
uint32_t duk_get(duk_context *ctx, duk_idx_t idx)
{ return duk_to_uint32(ctx, idx); }
template<>
uint16_t duk_get(duk_context *ctx, duk_idx_t idx)
{ return duk_to_uint16(ctx, idx); }
template<>
double duk_get(duk_context *ctx, duk_idx_t idx)
{ return duk_to_number(ctx, idx); }
template<>
float duk_get(duk_context *ctx, duk_idx_t idx)
{ return (float)duk_to_number(ctx, idx); }
template<>
const char *duk_get(duk_context *ctx, duk_idx_t idx) {
	if (auto lpsz = duk_to_string(ctx, idx))
		return lpsz;
	duk_type_error(ctx, "Cannot convert from_js const char *");
	duk_throw(ctx);
}

template<class... Args>
void duk_get(duk_context *ctx, duk_idx_t startIdx, Args &... args)
{ ((args = duk_get<Args>(ctx, startIdx++)), ...); }
#pragma endregion

#pragma region c function (smart native function pointer)

//!!!!!!!!!!!! - TODO: add force function not for constructors - !!!!!!!!!!!!//

class c_callable_base {
public:
	virtual ~c_callable_base() = default;
	virtual duk_ret_t call(duk_context *ctx, duk_idx_t nargs) = 0;
public:
	static duk_ret_t duk_c_reflect(duk_context *ctx) {
		auto nargs = duk_get_top(ctx);
		// ...
		duk_push_current_function(ctx);
		// ... current_function
		auto nf = duk_get_c_pointer<c_callable_base>(ctx); // ... current_function (. __c)
		// ... current_function
		duk_pop(ctx);
		// ...
		return nf->call(ctx, nargs);
	}
};

template<class RetType, class... Args>
class c_function : public c_callable_base {
	using FnType = RetType(*)(Args...);
	static constexpr size_t nArgs = sizeof...(Args);
	static constexpr duk_ret_t duk_ret = std::is_void_v<RetType> ? 0 : 1;
	FnType fn;
public:
	c_function(FnType fn) : fn(fn) {}
public:
	template<duk_idx_t...ind>
	void reflect(duk_context *ctx, std::index_sequence<ind...>) {
		if (!fn) {
			duk_error(ctx, DUK_ERR_REFERENCE_ERROR,
					  "C callable pointer is null");
			duk_throw(ctx);
		}
		if constexpr (duk_ret) {
			if constexpr (nArgs) {
				auto &&ret = fn(duk_get<Args>(ctx, (duk_idx_t)(ind))...);
				duk_push_c<RetType>(ctx, ret);
			}
			else {
				auto &&ret = fn();
				duk_push_c<RetType>(ctx, ret);
			}
		}
		else if constexpr (nArgs)
			 fn(duk_get<Args>(ctx, (duk_idx_t)(ind))...);
		else fn();
	}
	duk_ret_t call(duk_context *ctx, duk_idx_t nargs) override {
		if (nargs != nArgs) {
			duk_error(ctx, DUK_ERR_TYPE_ERROR,
					  "Invalid argument count, expected %d but got %d",
					  (int)nArgs, (int)nargs);
			duk_throw(ctx);
		}
		reflect(ctx, std::make_index_sequence<nArgs>{});
		return duk_ret;
	}
};

template<class AnyClass, class RetType, class... Args>
class c_method : public c_callable_base {
	using FnType = RetType(AnyClass:: *)(Args...);
	static constexpr size_t nArgs = sizeof...(Args);
	static constexpr duk_ret_t duk_ret = std::is_void_v<RetType> ? 0 : 1;
	FnType fn;
public:
	c_method(FnType fn) : fn(fn) {}
public:
	template<duk_idx_t...ind>
	void reflect(duk_context *ctx, AnyClass *pThis, std::index_sequence<ind...>) {
		if (!fn) {
			duk_error(ctx, DUK_ERR_REFERENCE_ERROR,
					  "C method pointer is null");
			duk_throw(ctx);
		}
		if constexpr (duk_ret) {
			if constexpr (nArgs) {
				auto &&ret = (pThis->*fn)(duk_get<Args>(ctx, (duk_idx_t)(ind))...);
				duk_push_c<RetType>(ctx, ret);
			}
			else {
				auto &&ret = (pThis->*fn)();
				duk_push_c<RetType>(ctx, ret);
			}
		}
		else if constexpr (nArgs)
			 (pThis->*fn)(duk_get<Args>(ctx, (duk_idx_t)(ind))...);
		else (pThis->*fn)();
	}
	duk_ret_t call(duk_context *ctx, duk_idx_t nargs) override {
		if (nargs != nArgs) {
			duk_error(ctx, DUK_ERR_TYPE_ERROR,
					  "Invalid argument count, expected %d but got %d",
					  (int)(nArgs + 1), (int)nargs);
			duk_throw(ctx);
		}
		// ...
		duk_push_this(ctx);
		// ... this
		auto pThis = duk_get_c_pointer<AnyClass>(ctx); // ... this (. __c)
		// ... this
		duk_pop(ctx);
		// ...
		reflect(ctx, pThis, std::make_index_sequence<nArgs>{});
		return duk_ret;
	}
};

void duk_push_c_callable(duk_context *ctx, c_callable_base *pFunction, duk_idx_t nargs, const char *name = nullptr) {
	if (!pFunction) {
		duk_error(ctx, DUK_ERR_REFERENCE_ERROR,
				  "C callable pointer is null");
		duk_throw(ctx);
	}
	// ... 
	duk_push_c_function(ctx, c_callable_base::duk_c_reflect, nargs);
	// ... c_function
	duk_set_c_pointer(ctx, pFunction); // c_function.$__c = ptr
	// ... c_function
	duk_set_destructor(ctx, [](duk_context *ctx) {
		auto nf = duk_get_c_pointer<c_callable_base>(ctx);
		if (nf) delete nf;
		return 0;
	}); // c_function.$destructor = lambda
	// ... c_function
	if (name)
		duk_set_self_name(ctx, name); // c_function.name = name
	// ... c_function
}
template<>
void duk_push_c(duk_context *ctx, c_callable_base *pFunction)
{ duk_push_c_callable(ctx, pFunction, DUK_VARARGS); }

template<class AnyType>
concept c_callable_type =
	std::is_function_v<std::remove_pointer_t<AnyType>> ||
	std::is_member_function_pointer_v<AnyType>;

class c_callable {
	mutable c_callable_base *pFunction;
	duk_idx_t nargs;
public:
	c_callable(const c_callable &c) : pFunction(c.pFunction) { c.pFunction = nullptr; }
	template<class RetType, class... Args>
	c_callable(RetType(*pfn)(Args...)) :
		pFunction(new c_function<RetType, Args...>(pfn)),
		nargs(sizeof...(Args)) {}
	template<class AnyClass, class RetType, class... Args>
	c_callable(RetType(AnyClass:: *pmd)(Args...)) :
		pFunction(new c_method<AnyClass, RetType, Args...>(pmd)),
		nargs(sizeof...(Args)) {}
	~c_callable() {
		if (pFunction) delete pFunction;
		pFunction = nullptr;
	}
public:
	inline duk_idx_t NArgs() const { return nargs; }
public:
	inline operator bool() const { return pFunction; }
	inline operator c_callable_base *() {
		auto pFunction = this->pFunction;
		this->pFunction = nullptr;
		return pFunction;
	}
};
inline void duk_push_c_callable(duk_context *ctx, c_callable callable, const char *name = nullptr)
{ duk_push_c_callable(ctx, callable, callable.NArgs()); }
template<c_callable_type AnyFunc>
inline void duk_push_c_callable_expose(duk_context *ctx, AnyFunc fn)
{ duk_push_c_callable(ctx, fn, typeid(AnyFunc).name()); }
template<c_callable_type AnyFunc>
inline void duk_push_c(duk_context *ctx, AnyFunc fn)
{ duk_push_c_callable(ctx, fn); }

#pragma endregion
	
namespace property {
	struct get_set;
	struct get : private c_callable {
		get(c_callable getter) : c_callable(getter) {}
		get_set set(c_callable setter);
		using c_callable::operator c_callable_base *;
	};
	struct set : private c_callable {
		set(c_callable setter) : c_callable(setter) {}
		get_set get(c_callable getter);
		using c_callable::operator c_callable_base *;
	};
	struct get_set : private get, private set {
		get_set(property::get getter, property::set setter) : property::get(getter), property::set(setter) {}
		inline operator c_callable_base *() {
			if (c_callable_base *getter = (property::get)(*this)) return getter;
			if (c_callable_base *setter = (property::set)(*this)) return setter;
			return nullptr;
		}
	};
	inline get_set get::set(c_callable setter) { return{ *this, setter }; }
	inline get_set set::get(c_callable getter) { return{ getter, *this }; }
}

class key {
	duk_context *ctx;
	const char *name;
	duk_idx_t idx;
public:
	key(duk_context *ctx, const char *name, duk_idx_t idx) : ctx(ctx), name(name), idx(idx) {}
public:
	template<class AnyType>
	void operator=(AnyType val) {
		// ... object(idx) ...
		duk_push_string(ctx, name);
		// ... object(idx) ... name
		duk_push_c(ctx, val);
		// ... object(idx) ... name value
		duk_def_prop(ctx, -3, DUK_DEFPROP_PUBLIC_CONST); // object(idx).name = value
		// ... object(idx) ...
	}
};

#pragma region InStack
class InStack {
protected:
	duk_context *ctx;
	duk_idx_t idx;
public:
	InStack(duk_context *ctx, duk_idx_t idx) : ctx(ctx), idx(idx) {}
public:
	inline bool IsUndefined() { return duk_is_undefined(ctx, idx) != 0; }
	inline bool IsNull() { return duk_is_null(ctx, idx) != 0; }
	inline bool IsObject() { return duk_is_object(ctx, idx) != 0; }
	inline bool IsBoolean() { return duk_is_boolean(ctx, idx) != 0; }
	inline bool IsNumber() { return duk_is_number(ctx, idx) != 0; }
	inline bool IsString() { return duk_is_string(ctx, idx) != 0; }
	inline bool IsArray() { return duk_is_array(ctx, idx) != 0; }
	inline bool IsFunction() { return duk_is_function(ctx, idx) != 0; }
	inline bool IsCFunction() { return duk_is_c_function(ctx, idx) != 0; }
	inline bool IsBuffer() { return duk_is_buffer(ctx, idx) != 0; }
	inline bool IsBufferData() { return duk_is_buffer_data(ctx, idx) != 0; }
	inline bool IsPointer() { return duk_is_pointer(ctx, idx) != 0; }
	inline bool IsDynamicBuffer() { return duk_is_dynamic_buffer(ctx, idx) != 0; }
	inline bool IsFixedBuffer() { return duk_is_fixed_buffer(ctx, idx) != 0; }
	inline bool IsExternalBuffer() { return duk_is_external_buffer(ctx, idx) != 0; }
	inline bool IsConstructable() { return duk_is_constructable(ctx, idx) != 0; }
public:
	inline key operator[](const char *name) { return{ ctx, name, idx }; }
};

class Object : protected InStack {
public:
	Object(duk_context *ctx, duk_idx_t idx) : InStack(ctx, idx) {}
};

class Boolean : protected Object {
public:
	Boolean(duk_context *ctx, duk_idx_t idx) : Object(ctx, idx) {}
public:
	inline operator bool() { return duk_to_boolean(ctx, idx) != 0; }
};

class Number : protected Object {
public:
	Number(duk_context *ctx, duk_idx_t idx) : Object(ctx, idx) {}
public:
	inline operator double() { return duk_to_number(ctx, idx); }
	inline operator float() { return (float)duk_to_number(ctx, idx); }
	inline operator int32_t() { return duk_to_int(ctx, idx); }
	inline operator int16_t() { return (int16_t)duk_to_int(ctx, idx); }
	inline operator uint32_t() { return duk_to_uint32(ctx, idx); }
	inline operator uint16_t() { return (uint16_t)duk_to_uint32(ctx, idx); }
};

class String : protected Object {
public:
	String(duk_context *ctx, duk_idx_t idx) : Object(ctx, idx) {}
public:
	inline size_t Length() { return duk_get_length(ctx, idx); }
	inline size_t Bytes() {
		duk_size_t size = 0;
		duk_get_lstring(ctx, idx, &size);
		return size;
	}
public:
	inline operator const char *() { return duk_to_string(ctx, idx); }
};

class Array : protected Object {
public:
	Array(duk_context *ctx, duk_idx_t idx) : Object(ctx, idx) {}
public:
	inline size_t Length() { return duk_get_length(ctx, idx); }
public:
	inline Object operator[](duk_size_t index) {
		// ... object(idx) ...
		duk_get_prop_index(ctx, idx, index); // object(idx)[index]
		// ... object(idx) ... object(idx)[index]
		return{ ctx, duk_get_top_index(ctx) - 1 };
	}
};

class This : public Object {
public:
	This(duk_context *ctx, duk_idx_t idx) : Object(ctx, idx) {}
public:
public:
	inline key operator[](const char *name) { return{ ctx, name, idx }; }
};
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

//	inline Key operator[](const char *name) { return Key(ctx, name); }
};
using Heap = Context::Heap;

}
