#include "wx_type"
#include "duktape.h"


import dux;

using namespace Dux;
using namespace WX;

#include "wx_console"

template<class>
class Wrapper;
template<class...Args>
class Wrapper<TypeList<Args...>> {
	using ArgsList = TypeList<Args...>;
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
template<auto fn, class Ret, class Args, class Own, NatType type>
duk_ret_t duk_static_function_of(duk_context *ctx) {
	auto &&invoker = Wrapper<Args>::from(ctx);
	if_c (std::is_void_v<Own>) {
		if_c (std::is_void_v<Ret>)
			invoker.invoke(fn);
		else {
			duk_push_c(ctx, invoker.invoke(fn));
			return 1;
		}
	} else {
		duk_push_this(ctx);
		Own *pOwn;
		if_c (type == NatType::Pointer)
			pOwn = duk_get_c_pointer<Own>(ctx);
		else 
			pOwn = duk_get_c_instance<Own>(ctx);
		if_c (std::is_void_v<Ret>)
			invoker.invoke(pOwn, fn);
		else {
			duk_push_c(ctx, invoker.invoke(pOwn, fn));
			return 1;
		}
}
	return 0;
}

template<auto fn, NatType type = NatType::Static>
void duk_push_function(duk_context *ctx) {
	using Inf = functionof<decltype(fn)>;
	using Ret = typename Inf::Return;
	using Args = typename Inf::ArgsList;
	using Own = typename Inf::Belong;
	constexpr auto nargs = Inf::is_ellipsis ? DUK_VARARGS : Args::Length;
	duk_push_c_function(
		ctx,
		duk_static_function_of<fn, Ret, Args, Own, type>,
		nargs);
}

template<auto fn, NatType type>
void duk_add_function(duk_context *ctx, const char *name, duk_idx_t idx = -1) {
	duk_push_string(ctx, name);
	duk_push_function<fn, type>(ctx);
	if (idx < 0) idx -= 2;
	duk_def_prop(ctx, idx, DUK_DEFPROP_PUBLIC_CONST);
}

namespace nDux {

class Context;

class Object {
	duk_context *ctx;
	duk_idx_t idx;
private:
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
	template<class AnyNative>
	static duk_ret_t construtor(duk_context *ctx) {
		if (duk_always_construct(ctx))
			return 1;

		return 0;
	}
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
	Console.LogA(CString(str, 1024), sum, '\n');
}

duk_ret_t load_dux(duk_context *ctx, void *) {
	nDux::Context c = ctx;
	c.global()
		.add<print>("print");
	return 0;
}
