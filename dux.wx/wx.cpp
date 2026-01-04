#include "wx_type"
#include "duktape.h"


import dux;

using namespace Dux;
using namespace WX;

#include "wx_console"

template<class>
class WrapArgs;
template<class...Args>
class WrapArgs<ArgsList<Args...>> {
	using ArgsList = ArgsList<Args...>;
	template<size_t...ind>
	static inline ArgsList from(duk_context *ctx, std::index_sequence<ind...>)
	{ return { duk_get<Args>(ctx, ind)... }; }
public:
	static inline ArgsList from(duk_context *ctx)
	{ return from(ctx, std::index_sequence_for<Args...>{}); }
};

enum class NatType {
	Static,
	Pointer,
	Buffer
};

template<auto fn>
struct WrapFunc {
	using Detail = functionof<decltype(fn)>;
	using Return = typename Detail::Return;
	using ArgsList = typename Detail::ArgsList;
	using Parent = typename Detail::Parent;
	static constexpr bool is_ellipsis = Detail::is_ellipsis;
	static constexpr bool is_method = Detail::is_method;
	static constexpr bool is_static = Detail::is_static;
	static constexpr auto nargs = Detail::nargs;
public:
	template<NatType type>
	static duk_ret_t native(duk_context *ctx) {
		auto &&invoker = WrapArgs<ArgsList>::from(ctx);
		if_c (is_static) {
			if_c (std::is_void_v<Return>)
				invoker.invoke(fn);
			else {
				duk_push_c(ctx, invoker.invoke(fn));
				return 1;
			}
		} else {
			duk_push_this(ctx);
			Parent *pParent;
			if_c (type == NatType::Pointer)
				pParent = duk_get_c_pointer<Parent>(ctx);
			else 
				pParent = duk_get_c_instance<Parent>(ctx);
			if_c (std::is_void_v<Return>)
				invoker.invoke(pParent, fn);
			else {
				duk_push_c(ctx, invoker.invoke(pParent, fn));
				return 1;
			}
		}
		return 0;
	}

};
template<auto fn, NatType type = NatType::Static>
void duk_push_function(duk_context *ctx) {
	using Wrap = WrapFunc<fn>;
	duk_push_c_function(
		ctx,  Wrap::template native<type>, 
		Wrap::is_ellipsis ? DUK_VARARGS : Wrap::nargs);
}
template<auto fn, NatType type>
void duk_add_function(duk_context *ctx, const char *name, duk_idx_t idx = -1) {
	duk_push_string(ctx, name);
	duk_push_function<fn, type>(ctx);
	if (idx < 0) idx -= 2;
	duk_def_prop(ctx, idx, DUK_DEFPROP_PUBLIC_CONST);
}

template<class AnyType>
struct WrapConstructor {
	template<class... ArgsLists>
		requires ConstructorsOf<AnyType, ArgsLists...>::any_ways
	static duk_ret_t native(duk_context *ctx) {
		auto nargs = duk_get_top(ctx);
		if (duk_is_constructor_call(ctx)) { // new XXX(...)	==> new AnyType(...)
			
		} else { // XXX(...) ===> AnyType(...)
		}
	}
};

namespace nDux {

class Context;

class Object {
	duk_context *ctx;
	duk_idx_t idx;
protected:
	friend class Context;
	Object(duk_context *ctx, duk_idx_t idx) : ctx(ctx), idx(idx) {}
public:
	~Object() {
		duk_remove(ctx, idx);
	}
	template<auto fn>
	inline auto &method(const char *name) reflect_to_self(duk_add_function<fn, NatType::Static>(ctx, name, idx));
};

class JStruct : public Object {
	const char *name;
private:
	friend class Context;
	JStruct(duk_context *ctx, duk_idx_t idx) : Object(ctx, idx) {}
public:
	template<auto fn>
	inline auto &method(const char *name) reflect_to_self(duk_add_function<fn, NatType::Buffer>(ctx, name, idx));
};

class Context {
	duk_context *ctx;
public:
	Context(duk_context *ctx) : ctx(ctx) {}
public:
	inline Object global() {
		duk_push_global_object(ctx);
		return{ ctx, -1 };
	}
};

}

void print(const char *str, int sum) {
	while (sum--)
		Console.Write(CString(str, 1024));
	Console.Write('\n');
}

duk_ret_t load_dux(duk_context *ctx, void *) {
	nDux::Context c = ctx;
	c.global()
		.method<print>("print");
	return 0;
}
