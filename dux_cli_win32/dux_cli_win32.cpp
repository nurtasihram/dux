#include <iostream>

#include "wx_console"
#include "wx_realtime"
#include "wx_window"

#include "wx_main"

#include "dux.h"

using namespace WX;

import dux;

class DuxContext : public Dux::Context {
public:
	DuxContext(Dux::Heap & heap) : Dux::Context(heap) {}
public:
	void OnFatal(const char *msg) override {
		throw duk_exception{ msg };
	}
};

class Win32HeapDux : public Dux::Heap {
	int nAlloc = 0;
	WX::Heap heap = WX::Heap::Create();
public:
	Win32HeapDux() {}
protected:
	void *Alloc(duk_size_t size) override {
		if (!size) return O;
		nAlloc++;
		return heap.Alloc(size, HeapAllocFlag::GenerateExceptions);
	}
	void *Realloc(void *ptr, duk_size_t size) override {
		if (!ptr) return Alloc(size);
		if (!size) {
			Free(ptr);
			return O;
		}
		return heap.Realloc(ptr, size);
	}
	void Free(void *ptr) override {
		if (!ptr) return;
		nAlloc--;
		heap.Free(ptr);
	}
public:
	void PrintInfo() const {
		auto &&sum = heap.Summaries();
		Console.Log(
			T(  "   Allocated: "), sum.Allocated(),
			T("\n   Committed: "), sum.Committed(),
			T("\n   Count: "), nAlloc, T("\n"));
	}
};

#pragma region Memory Management
static duk_ret_t print_mem(duk_context * = O) {
	//heap.PrintInfo();
	return 0;
}
#pragma endregion

static bool bEcho = true;
static bool bExit = false;
static bool bCmdl = false;
static bool bReset = false;

Event proc_ok = Event::Create().AutoReset();
static constexpr auto WX_DUK_ON_CMD = WM_USER + 1;

static UINT duk_cmdl_count = 0;

static duk_ret_t cmd_exe(duk_context *ctx) {
	auto lpszCode = duk_require_string(ctx, 0);
	duk_size_t szCode = duk_get_length(ctx, 0);
	duk_compile_lstring(ctx, DUK_COMPILE_SHEBANG, lpszCode, szCode);
	duk_push_global_object(ctx);  /* 'this' binding */
	duk_call_method(ctx, 0);
	if (bEcho && !duk_is_undefined(ctx, -1)) {
		duk_to_string(ctx, -1);
		auto len = duk_get_length(ctx, -1);
		auto lpsz = duk_get_string(ctx, -1);
		Console.LogA(CString(len, lpsz), '\n');
		duk_pop(ctx);
	}
	return 0;
}

static duk_ret_t cmd_prc(duk_context *ctx) {
	++duk_cmdl_count;
	Console.Log(T("\n -- JavaScript --\n"));
	proc_ok.Set();
	for (;;) {
		Msg msg;
		try {
			while (msg.Get()) {
				if (msg.Window()) {
					msg.Translate();
					msg.Dispatch();
				}
				elif(msg.ID() == WX_DUK_ON_CMD) {
					duk_get_global_string(ctx, "cmd_exe");
					duk_push_string(ctx, msg.ParamW<LPCSTR>());
					auto rc = duk_pcall(ctx, 1);
					if (rc != DUK_EXEC_SUCCESS) {
						duk_dup(ctx, -1);
						duk_put_global_string(ctx, "LastError");
						duk_errout(ctx, "%s\n", duk_safe_to_stacktrace(ctx, -1));
					}
					if (bReset) {
						Console.Log(T("reset\n"));
						--duk_cmdl_count;
						return 0;
					}
					if (bExit) {
						Console.Log(T("exit\n\n"));
						--duk_cmdl_count;
						return 0;
					}
					if (bCmdl) {
						bCmdl = false;
						do {
							bReset = false;
							duk_get_global_string(ctx, "cmd_prc");
							duk_call(ctx, 0);
							duk_pop(ctx);
						} while (bReset);
						continue;
					}
					proc_ok.Set();
				}
			}
			--duk_cmdl_count;
			proc_ok.Set();
			return 0;
		} catch (duk_exception err) {
			duk_errout(ctx, "Duktape Exception: \n%s", err.msg);
			proc_ok.Set();
		} catch (Exception err) {
			duk_errout(ctx, "WX Exception: \n%s", (LPCSTR)err.toStringA());
			proc_ok.Set();
		} catch (const std::exception &err) {
			duk_errout(ctx, "C++ Exception: \n%s", err.what());
			proc_ok.Set();
		} catch (...) {
			Console.Err(T("Other exception\n"));
			proc_ok.Set();
			bExit = true;
			return 0;
		}
	}
}

duk_ret_t load_duk_cmdl(duk_context *ctx, void *) {
	duk_push_global_object(ctx);
	duk_add_prop(
		ctx, "echo",
		duk_set {
			bEcho = duk_to_boolean(ctx, 0);
			return 0;
		},
		duk_get {
			duk_push_boolean(ctx, bEcho);
			return 1;
		}
	);
	duk_add_prop_r(
		ctx, "exit",
		duk_get {
			bExit = true;
			return 0;
		});
	duk_add_prop_r(
		ctx, "cmdl",
		duk_get {
			bCmdl = true;
			return 0;
		});
	duk_add_prop_r(
		ctx, "reset",
		duk_get {
			bReset = true;
			return 0;
		});
	duk_add_prop_r(
		ctx, "clear",
		duk_get {
			Console.Clear();
			return 0;
		});
	duk_push_undefined(ctx);
	duk_put_prop_string(ctx, -2, "LastError");
	duk_add_method(ctx, "print_mem", 0, print_mem);
	duk_add_method(ctx, "cmd_prc", 0, cmd_prc);
	duk_add_method(ctx, "cmd_exe", 1, cmd_exe);
	return 0;
}

class BaseOf_Thread(CommandProcThread) {
	SFINAE_Thread(CommandProcThread);
private:
	Dux::Context &ctx;
public:
	CommandProcThread(Dux::Context & ctx) : ctx(ctx) {}
protected:
	inline void OnRun() {
		do {
			bReset = false;
			duk_get_global_string(ctx, "cmd_prc");
			duk_call(ctx, 0);
			duk_pop(ctx);
			duk_gc(ctx, 0);
		} while (bReset);
		proc_ok.Set();
	}
};

void commandline(Dux::Context &ctx) {
	duk_push_global_object(ctx);

	duk_load_library(ctx, load_dux);
	duk_load_library(ctx, load_duk_cmdl);

//	duk_eval_string(ctx, "Console.Reopen()");
	Console.Log(T("\n - Duktape symbols loaded -\n"));
	print_mem();

	CommandProcThread cmd_prc_thr = ctx;

	assertl(cmd_prc_thr.Create());
	proc_ok.Wait();

	while (duk_cmdl_count) {
		// print prompt
		auto &&cur_pos = Console.CursorPosition();
		++cur_pos.x;
		Console.Fill(T('>'), duk_cmdl_count, cur_pos);
		cur_pos.x += duk_cmdl_count + 1;
		Console.CursorPosition(cur_pos);
		// read input
		char js_code[1024]{ 0 };
		std::cin.getline(js_code, sizeof(js_code));
		// process input
		if (!cmd_prc_thr.StillActive()) {
			Console.Log(T("Message procedurer had exited\n"));
			break;
		}
		cmd_prc_thr.Post(WX_DUK_ON_CMD, js_code);
		proc_ok.Wait();
	}
}

void DuxCLI(Win32HeapDux &heap) {
	DuxContext ctx = heap;

	Console.Log(T("\n - Duktape heap created -\n"));
	heap.PrintInfo();

	commandline(ctx);
}

int WxMain() {
	Console.Title(T("JavaScript CommandLine Interface"));

	Console.Log(T("\n -- Duktape x WindowX --\n\n"));

	duk_errout = [](duk_context *ctx, const char *err_fmt, ...) {
		va_list args;
		va_start(args, err_fmt);
		char buffer[2048];
		vsnprintf_s(buffer, sizeof(buffer), err_fmt, args);
		va_end(args);
		auto attr = Console.Attributes();
		Console.Attributes((attr - ConsoleColor::Foreground) + ConsoleColor::Red);
		Console.ErrA(CString(buffer, CountOf(buffer)), " \n");
		Console.Attributes(attr);
	};

	Win32HeapDux heap;

	DuxCLI(heap);

	Console.Log(T("\n - Duktape destroyed -\n"));
	heap.PrintInfo();

	return 0;
}
