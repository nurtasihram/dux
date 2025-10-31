#include "dux.h"
#include "wx_type"

using namespace WX;

import dux;

using namespace Dux;

#include "wx_console"

void print(const char *str) {
	Console.LogA(CString(str, 1024), '\n');
}

static duk_ret_t load_type(duk_context *ctx) {

	return 0;
}

duk_ret_t load_dux(duk_context *ctx, void *) {
//	if (auto res = load_type(ctx))
///		return res;
	AddFunction(ctx, "print", print);
	Console.Log(duk_get_top(ctx), '\n');
	duk_push_this(ctx);
	Console.Log(duk_get_top(ctx), '\n');
	duk_pop(ctx);
	Console.Log(duk_get_top(ctx), '\n');
	return 0;
}

//template<>
//std::string ToNative<std::string>(duk_context *ctx, duk_idx_t idx) {
//	duk_size_t len = 0;
//	if (auto lpsz = duk_to_lstring(ctx, idx, &len))
//		return std::string(lpsz, len);
//	duk_type_error(ctx, "Cannot convert to std::string");
//	duk_throw(ctx);
//}
//template<>
//WX::StringA ToNative<WX::StringA>(duk_context *ctx, duk_idx_t idx) {
//	duk_size_t len = 0;
//	if (auto lpsz = duk_to_lstring(ctx, idx, &len))
//		return +WX::CString(lpsz, (size_t)len);
//	duk_type_error(ctx, "Cannot convert to WX::StringA");
//	duk_throw(ctx);
//}
