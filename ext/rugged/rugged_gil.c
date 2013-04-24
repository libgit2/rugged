#include <ruby.h>

#ifdef HAVE_RUBY_ENCODING_H
#	include <ruby/encoding.h>
#endif

#include <assert.h>
#include <git2.h>
#include <git2/odb_backend.h>

int rugged_without_gil_enabled;

/*
 * When running the rugged_gc_enable()/rugged_gc_disable() function, the GIL
 * is held, therefore we don't need atomic increment/decrements.
 *
 * XXX We are no longer safe if the user plays with GC.disable while we are
 * doing some work: we will reenable the GC after the last *_without_gil call,
 * without knowing that the user wanted to keep the GC disabled.
 * We are certainly not safe if the user calls GC.enable from another thread.
 * A solution would be to to override GC.enable/GC.disable to use
 * rugged_gc_disable()/rugged_gc_enable().
 *
 * Also, if there is always one thread doing some git work at any point in
 * time, the GC will never be reenabled, leading to starvation, and memory
 * exhaustion.
 *
 * The user still need to use "Rugged.without_gil = true" to be in that
 * situation, so that's okay for now.
 */

static int rugged_gc_disable_count;

void rugged_gc_disable(void)
{
	if (rugged_gc_disable_count++ == 0) {
		rb_gc_disable();
	}
}

void rugged_gc_enable(void)
{
	if (--rugged_gc_disable_count == 0) {
		rb_gc_enable();
	}
}

#define __RUGGED_WITHOUT_GILx(x, ret_type, name, ...)					\
											\
struct name##_without_gil_args { __RUGGED_STRUCT_DECL##x(__VA_ARGS__) };		\
											\
ret_type __##name##_without_gil(void* _args)						\
{											\
	struct name##_without_gil_args* args = _args;					\
	return name(__RUGGED_STRUCT_ARGS##x(__VA_ARGS__));				\
}											\

/* Generate all the __*_without_gil trampoline methods */
#include "rugged_gil.h"
#include "rugged_gil.def"
