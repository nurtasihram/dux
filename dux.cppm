module;

#include "duktape.h"

#include <stdint.h>
#include <typeinfo>
#include <memory>

export module dux;

export namespace Dux {

#pragma region Duktape HAL Helpers

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

void *duk_put_c_instance_nassert(duk_context *ctx, duk_size_t size, duk_idx_t idx = -3) {
	// ... object(idx) ...
	duk_push_string(ctx, "__c");
	// ... object(idx) ... "__c"
	auto pC = duk_push_fixed_buffer(ctx, size);
	// ... object(idx) ... "__c" buffer
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

void duk_put_c_pointer_nassert(duk_context *ctx, void *ptr, duk_idx_t idx = -3) {
	// ... object(idx) ...
	duk_push_string(ctx, "__c");
	// ... object(idx) ... "__c"
	duk_push_pointer(ctx, ptr);
	// ... object(idx) ... "__c" ptr
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

void duk_set_destructor(duk_context *ctx, duk_c_function destructor, duk_idx_t idx = -2) {
	// ... object(idx) ...
	duk_push_c_function(ctx, destructor, 1);
	// ... object(idx) ... destructor
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
inline void duk_put_c_pointer(duk_context *ctx, AnyClass *pInstance, duk_idx_t idx = -3) {
	duk_put_c_pointer_nassert(ctx, (void *)pInstance, idx);
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

#pragma region FromC
template<class InType>
struct from {};
template<>
struct from<void> {
	static void to(duk_context *ctx, ...) {}
};
template<>
struct from<std::nullptr_t> {
	static void to(duk_context *ctx, std::nullptr_t) {
		duk_push_null(ctx);
	}
};
template<>
struct from<bool> {
	static void to(duk_context *ctx, bool val) {
		duk_push_boolean(ctx, val);
	}
};
template<>
struct from<int32_t> {
	static void to(duk_context *ctx, int32_t val) {
		duk_push_int(ctx, val);
	}
};
template<>
struct from<int16_t> {
	static void to(duk_context *ctx, int16_t val) {
		duk_push_int(ctx, val);
	}
};
template<>
struct from<uint32_t> {
	static void to(duk_context *ctx, uint32_t val) {
		duk_push_uint(ctx, val);
	}
};
template<>
struct from<uint16_t> {
	static void to(duk_context *ctx, uint16_t val) {
		duk_push_uint(ctx, val);
	}
};
template<>
struct from<double> {
	static void to(duk_context *ctx, double val) {
		duk_push_number(ctx, val);
	}
};
template<>
struct from<float> {
	static void to(duk_context *ctx, float val) {
		duk_push_number(ctx, val);
	}
};
template<>
struct from<const char *> {
	static void to(duk_context *ctx, const char *val) {
		duk_push_string(ctx, val);
	}
};

template<class AnyType>
concept FromC = requires {
	from<AnyType>::to(
		std::declval<duk_context *>(),
		std::declval<std::conditional_t<std::is_same_v<AnyType, void>, int, AnyType>>()
	);
};
#pragma endregion

#pragma region ToC
template<class OutType>
struct to {};
template<>
struct to<std::nullptr_t> {
	static std::nullptr_t from(duk_context *ctx, duk_idx_t idx) {
		duk_to_null(ctx, idx);
		return nullptr;
	}
};
template<>
struct to<bool> {
	static bool from(duk_context *ctx, duk_idx_t idx) {
		return duk_to_boolean(ctx, idx) != 0;
	}
};
template<>
struct to<int32_t> {
	static int32_t from(duk_context *ctx, duk_idx_t idx) {
		return duk_to_int(ctx, idx);
	}
};
template<>
struct to<int16_t> {
	static int16_t from(duk_context *ctx, duk_idx_t idx) {
		return (int16_t)duk_to_int(ctx, idx);
	}
};
template<>
struct to<uint32_t> {
	static uint32_t from(duk_context *ctx, duk_idx_t idx) {
		return duk_to_uint32(ctx, idx);
	}
};
template<>
struct to<uint16_t> {
	static uint16_t from(duk_context *ctx, duk_idx_t idx) {
		return duk_to_uint16(ctx, idx);
	}
};
template<>
struct to<double> {
	static double from(duk_context *ctx, duk_idx_t idx) {
		return duk_to_number(ctx, idx);
	}
};
template<>
struct to<float> {
	static float from(duk_context *ctx, duk_idx_t idx) {
		return (float)duk_to_number(ctx, idx);
	}
};
template<>
struct to<const char *> {
	static const char *from(duk_context *ctx, duk_idx_t idx) {
		if (auto lpsz = duk_to_string(ctx, idx))
			return lpsz;
		duk_type_error(ctx, "Cannot convert to const char *");
		duk_throw(ctx);
	}
};

template<class AnyType>
concept ToC = requires {
	{ to<AnyType>::from(std::declval<duk_context *>(), std::declval<duk_idx_t>()) } -> std::convertible_to<AnyType>;
};
#pragma endregion

#pragma region CFunction

//!!!!!!!!!!!! - TODO: add force function not for constructors - !!!!!!!!!!!!//

class CFunctionBase {
public:
	virtual ~CFunctionBase() = default;
	virtual duk_ret_t call(duk_context *ctx, duk_idx_t nargs) = 0;
public:
	static duk_ret_t duk_c_reflect(duk_context *ctx) {
		auto nargs = duk_get_top(ctx);
		// ...
		duk_push_current_function(ctx);
		// ... current_function
		auto nf = duk_get_c_pointer<CFunctionBase>(ctx); // ... current_function (. __c)
		// ... current_function
		duk_pop(ctx);
		// ...
		return nf->call(ctx, nargs);
	}
};

template<FromC RetType, ToC... Args>
class CFunctionPtr : public CFunctionBase {
	using FnType = RetType(*)(Args...);
	static constexpr size_t nArgs = sizeof...(Args);
	static constexpr duk_ret_t duk_ret = std::is_void_v<RetType> ? 0 : 1;
	FnType fn;
public:
	CFunctionPtr(FnType fn)
		: fn(fn) {}
public:
	template<duk_idx_t...ind>
	void reflect(duk_context *ctx, std::index_sequence<ind...>) {
		if (!fn) {
			duk_error(ctx, DUK_ERR_REFERENCE_ERROR,
					  "C function pointer is null");
			duk_throw(ctx);
		}
		if constexpr (duk_ret) {
			if constexpr (nArgs) {
				auto &&ret = fn(to<Args>::from(ctx, (duk_idx_t)(ind))...);
				PushC<RetType>(ctx, ret);
			}
			else {
				auto &&ret = fn();
				PushC<RetType>(ctx, ret);
			}
		}
		else if constexpr (nArgs)
			fn(to<Args>::from(ctx, (duk_idx_t)(ind))...);
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

template<class AnyClass, FromC RetType, ToC... Args>
class CMethodPtr : public CFunctionBase {
	using FnType = RetType(AnyClass:: *)(Args...);
	static constexpr size_t nArgs = sizeof...(Args);
	static constexpr duk_ret_t duk_ret = std::is_void_v<RetType> ? 0 : 1;
	FnType fn;
public:
	CMethodPtr(FnType fn)
		: fn(fn) {}
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
				auto &&ret = (pThis->*fn)(to<Args>::from(ctx, (duk_idx_t)(ind))...);
				PushC<RetType>(ctx, ret);
			}
			else {
				auto &&ret = (pThis->*fn)();
				PushC<RetType>(ctx, ret);
			}
		}
		else if constexpr (nArgs)
			(pThis->*fn)(to<Args>::from(ctx, (duk_idx_t)(ind))...);
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

void AddFunction(duk_context *ctx, const char *name, duk_idx_t nargs, CFunctionBase *ptr, duk_idx_t idx = -3) {
	// ... object(idx) ...
	duk_push_string(ctx, name);
	// ... object(idx) ... name
	duk_push_c_function(ctx, CFunctionBase::duk_c_reflect, nargs);
	// ... object(idx) ... name c_function
	duk_put_c_pointer(ctx, ptr); // c_function.__c = ptr
	// ... object(idx) ... name c_function
	duk_set_destructor(ctx, [](duk_context *ctx) {
		auto nf = duk_get_c_pointer<CFunctionBase>(ctx);
		if (nf) delete nf;
		return 0;
	}); // c_function.$destructor = lambda
	// ... object(idx) ... name c_function
	duk_push_string(ctx, "name");
	// ... object(idx) ... name c_function "name"
	duk_push_string(ctx, name);
	// ... object(idx) ... name c_function "name" name
	duk_def_prop(ctx, -3, DUK_DEFPROP_PUBLIC_CONST); // c_function["name"] = name
	// ... object(idx) ... name c_function
	duk_def_prop(ctx, -3, DUK_DEFPROP_PUBLIC_CONST); // object(idx)[name] = c_function
	// ... object(idx) ...
}

template<class RetType, class... Args>
void AddFunction(duk_context *ctx, const char *name, RetType(*pfn)(Args...), duk_idx_t idx = -3)
{ AddFunction(ctx, name, sizeof...(Args), new CFunctionPtr(pfn), idx); }

template<class AnyClass, class RetType, class... Args>
void AddMethod(duk_context *ctx, const char *name, RetType(AnyClass:: *pmd)(Args...), duk_idx_t idx = -3)
{ AddFunction(ctx, name, sizeof...(Args), new CMethodPtr(pmd), idx); }
#pragma endregion

class IProperty {
	CFunctionBase *pGetter;
	CFunctionBase *pSetter;
	class R {
		friend class IProperty;
		CFunctionBase *pGetter;
	public:
		~R() {
			if (pGetter) delete pGetter;
		}
		template<FromC RetType, ToC ArgType>
		inline IProperty set(RetType(*pfnSetter)(ArgType)) {
			return{ pGetter, new CFunctionPtr((void(*)(ArgType))pfnSetter) };
		}
		template<class AnyClass, FromC RetType, ToC ArgType>
		inline IProperty set(RetType(AnyClass:: *pmdSetter)(ArgType)) {
			return{ pGetter, new CMethodPtr((void(AnyClass:: *)(ArgType))pmdSetter) };
		}
	public:
		inline CFunctionBase *__getter() {
			auto pGetter = this->pGetter;
			this->pGetter = nullptr;
			return pGetter;
		}
	};
	class W {
		friend class IProperty;
		CFunctionBase *pSetter;
	public:
		~W() {
			if (pSetter) delete pSetter;
		}
		template<FromC RetType>
		inline IProperty get(RetType(*pfnGetter)()) {
			return{ new CFunctionPtr(pfnGetter), pSetter };
		}
		template<class AnyClass, FromC RetType>
		inline IProperty get(RetType(AnyClass:: *pmdGetter)()) {
			return{ new CMethodPtr(pmdGetter), pSetter };
		}
	public:
		inline CFunctionBase *__setter() {
			auto pSetter = this->pSetter;
			this->pSetter = nullptr;
			return pSetter;
		}
	};
public:
	IProperty(CFunctionBase *pGetter = nullptr, CFunctionBase *pSetter = nullptr)
		: pGetter(pGetter), pSetter(pSetter) {}
	~IProperty() {
		if (pGetter) delete pGetter;
		if (pSetter) delete pSetter;
	}
public:
	template<FromC RetType, ToC ArgType>
	inline W set(RetType(*pfnSetter)(ArgType)) {
		return new CFunctionPtr((void(*)(ArgType))pfnSetter);
	}
	template<class AnyClass, FromC RetType, ToC ArgType>
	inline W set(RetType(AnyClass:: *pmdSetter)(ArgType)) {
		return new CMethodPtr((void(AnyClass:: *)(ArgType))pmdSetter);
	}
	template<FromC RetType>
	inline R get(RetType(*pfnGetter)()) {
		return new CFunctionPtr(pfnGetter);
	}
	template<class AnyClass, FromC RetType>
	inline R get(RetType(AnyClass:: *pmdGetter)()) {
		return new CMethodPtr(pmdGetter);
	}
public:
	inline CFunctionBase *__getter() {
		auto pGetter = this->pGetter;
		this->pGetter = nullptr;
		return pGetter;
	}
	inline CFunctionBase *__setter() {
		auto pSetter = this->pSetter;
		this->pSetter = nullptr;
		return pSetter;
	}
} inline Property;

class Key {
	duk_context *ctx;
	const char *name;
	duk_idx_t idx;
public:
	Key(duk_context *ctx, const char *name, duk_idx_t idx) : ctx(ctx), name(name), idx(idx) {}
public:
	template<FromC RetType, ToC... Args>
	void operator=(RetType(*fn)(Args...)) {
		// ... object(idx) ...
		AddFunction(ctx, name, fn); // object(idx).name = function
		// ...object(idx) ...
	}
	template<class AnyClass, FromC RetType, ToC... Args>
	void operator=(RetType(AnyClass::*fn)(Args...)) {
		// ... object(idx) ...
		AddMethod(ctx, name, fn); // object(idx).name = function
		// ... object(idx) ...
	}
	template<FromC AnyType>
	void operator=(AnyType val) {
		// ... object(idx) ...
		duk_push_string(ctx, name);
		// ... object(idx) ... name
		from<AnyType>::to(ctx, val);
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
	inline Key operator[](const char *name) { return{ ctx, name, idx }; }
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
	inline Key operator[](const char *name) { return{ ctx, name, idx }; }
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
