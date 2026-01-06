#include "wx_type"
#include "duktape.h"


import dux;

using namespace Dux;
using namespace WX;

#include "wx_console"

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
	inline auto &method(const char *name) reflect_to_self(duk_add_function<fn>(ctx, name, idx));
	template<auto fn>
	inline auto &method_strict(const char *name) reflect_to_self(duk_add_function<fn, true>(ctx, name, idx));
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
		.method_strict<print>("print");
	//	static_assert(WX::IsTypePointer<int Context:: *>);
	return 0;
}
