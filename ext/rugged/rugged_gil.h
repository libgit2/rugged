#ifndef __H_RUGGED_GIL__
#define __H_RUGGED_GIL__

/*
 * These wrappers are inspired from:
 * https://github.com/torvalds/linux/blob/master/include/linux/syscalls.h
 */

#define __RUGGED_DECL1(t1, a1)         t1 a1
#define __RUGGED_DECL2(t2, a2, ...)    t2 a2,   __RUGGED_DECL1(__VA_ARGS__)
#define __RUGGED_DECL3(t3, a3, ...)    t3 a3,   __RUGGED_DECL2(__VA_ARGS__)
#define __RUGGED_DECL4(t4, a4, ...)    t4 a4,   __RUGGED_DECL3(__VA_ARGS__)
#define __RUGGED_DECL5(t5, a5, ...)    t5 a5,   __RUGGED_DECL4(__VA_ARGS__)
#define __RUGGED_DECL6(t6, a6, ...)    t6 a6,   __RUGGED_DECL5(__VA_ARGS__)
#define __RUGGED_DECL7(t7, a7, ...)    t7 a7,   __RUGGED_DECL6(__VA_ARGS__)
#define __RUGGED_DECL8(t8, a8, ...)    t8 a8,   __RUGGED_DECL7(__VA_ARGS__)
#define __RUGGED_DECL9(t9, a9, ...)    t9 a9,   __RUGGED_DECL8(__VA_ARGS__)
#define __RUGGED_DECL10(t10, a10, ...) t10 a10, __RUGGED_DECL9(__VA_ARGS__)

#define __RUGGED_ARGS1(t1, a1)         a1
#define __RUGGED_ARGS2(t2, a2, ...)    a2,  __RUGGED_ARGS1(__VA_ARGS__)
#define __RUGGED_ARGS3(t3, a3, ...)    a3,  __RUGGED_ARGS2(__VA_ARGS__)
#define __RUGGED_ARGS4(t4, a4, ...)    a4,  __RUGGED_ARGS3(__VA_ARGS__)
#define __RUGGED_ARGS5(t5, a5, ...)    a5,  __RUGGED_ARGS4(__VA_ARGS__)
#define __RUGGED_ARGS6(t6, a6, ...)    a6,  __RUGGED_ARGS5(__VA_ARGS__)
#define __RUGGED_ARGS7(t7, a7, ...)    a7,  __RUGGED_ARGS6(__VA_ARGS__)
#define __RUGGED_ARGS8(t8, a8, ...)    a8,  __RUGGED_ARGS7(__VA_ARGS__)
#define __RUGGED_ARGS9(t9, a9, ...)    a9,  __RUGGED_ARGS8(__VA_ARGS__)
#define __RUGGED_ARGS10(t10, a10, ...) a10, __RUGGED_ARGS9(__VA_ARGS__)

#define __RUGGED_STRUCT_DECL1(t1, a1)         t1 a1;
#define __RUGGED_STRUCT_DECL2(t2, a2, ...)    t2 a2;   __RUGGED_STRUCT_DECL1(__VA_ARGS__)
#define __RUGGED_STRUCT_DECL3(t3, a3, ...)    t3 a3;   __RUGGED_STRUCT_DECL2(__VA_ARGS__)
#define __RUGGED_STRUCT_DECL4(t4, a4, ...)    t4 a4;   __RUGGED_STRUCT_DECL3(__VA_ARGS__)
#define __RUGGED_STRUCT_DECL5(t5, a5, ...)    t5 a5;   __RUGGED_STRUCT_DECL4(__VA_ARGS__)
#define __RUGGED_STRUCT_DECL6(t6, a6, ...)    t6 a6;   __RUGGED_STRUCT_DECL5(__VA_ARGS__)
#define __RUGGED_STRUCT_DECL7(t7, a7, ...)    t7 a7;   __RUGGED_STRUCT_DECL6(__VA_ARGS__)
#define __RUGGED_STRUCT_DECL8(t8, a8, ...)    t8 a8;   __RUGGED_STRUCT_DECL7(__VA_ARGS__)
#define __RUGGED_STRUCT_DECL9(t9, a9, ...)    t9 a9;   __RUGGED_STRUCT_DECL8(__VA_ARGS__)
#define __RUGGED_STRUCT_DECL10(t10, a10, ...) t10 a10; __RUGGED_STRUCT_DECL9(__VA_ARGS__)

#define __RUGGED_STRUCT_ARGS1(t1, a1)         args->a1
#define __RUGGED_STRUCT_ARGS2(t2, a2, ...)    args->a2,  __RUGGED_STRUCT_ARGS1(__VA_ARGS__)
#define __RUGGED_STRUCT_ARGS3(t3, a3, ...)    args->a3,  __RUGGED_STRUCT_ARGS2(__VA_ARGS__)
#define __RUGGED_STRUCT_ARGS4(t4, a4, ...)    args->a4,  __RUGGED_STRUCT_ARGS3(__VA_ARGS__)
#define __RUGGED_STRUCT_ARGS5(t5, a5, ...)    args->a5,  __RUGGED_STRUCT_ARGS4(__VA_ARGS__)
#define __RUGGED_STRUCT_ARGS6(t6, a6, ...)    args->a6,  __RUGGED_STRUCT_ARGS5(__VA_ARGS__)
#define __RUGGED_STRUCT_ARGS7(t7, a7, ...)    args->a7,  __RUGGED_STRUCT_ARGS6(__VA_ARGS__)
#define __RUGGED_STRUCT_ARGS8(t8, a8, ...)    args->a8,  __RUGGED_STRUCT_ARGS7(__VA_ARGS__)
#define __RUGGED_STRUCT_ARGS9(t9, a9, ...)    args->a9,  __RUGGED_STRUCT_ARGS8(__VA_ARGS__)
#define __RUGGED_STRUCT_ARGS10(t10, a10, ...) args->a10, __RUGGED_STRUCT_ARGS9(__VA_ARGS__)

#define RUGGED_WITHOUT_GIL1(ret_type, name, ...)  __RUGGED_WITHOUT_GILx(1,  ret_type, name, __VA_ARGS__)
#define RUGGED_WITHOUT_GIL2(ret_type, name, ...)  __RUGGED_WITHOUT_GILx(2,  ret_type, name, __VA_ARGS__)
#define RUGGED_WITHOUT_GIL3(ret_type, name, ...)  __RUGGED_WITHOUT_GILx(3,  ret_type, name, __VA_ARGS__)
#define RUGGED_WITHOUT_GIL4(ret_type, name, ...)  __RUGGED_WITHOUT_GILx(4,  ret_type, name, __VA_ARGS__)
#define RUGGED_WITHOUT_GIL5(ret_type, name, ...)  __RUGGED_WITHOUT_GILx(5,  ret_type, name, __VA_ARGS__)
#define RUGGED_WITHOUT_GIL6(ret_type, name, ...)  __RUGGED_WITHOUT_GILx(6,  ret_type, name, __VA_ARGS__)
#define RUGGED_WITHOUT_GIL7(ret_type, name, ...)  __RUGGED_WITHOUT_GILx(7,  ret_type, name, __VA_ARGS__)
#define RUGGED_WITHOUT_GIL8(ret_type, name, ...)  __RUGGED_WITHOUT_GILx(8,  ret_type, name, __VA_ARGS__)
#define RUGGED_WITHOUT_GIL9(ret_type, name, ...)  __RUGGED_WITHOUT_GILx(9,  ret_type, name, __VA_ARGS__)
#define RUGGED_WITHOUT_GIL10(ret_type, name, ...) __RUGGED_WITHOUT_GILx(10, ret_type, name, __VA_ARGS__)

/*
 * Note: We are using the unblocking argument to be NULL.
 * Maybe we should use RUBY_UBF_IO. Not sure what the semantics are.
 */

extern int rugged_without_gil_enabled;
extern void rugged_gc_disable(void);
extern void rugged_gc_enable(void);

#ifndef __RUGGED_WITHOUT_GILx
#define __RUGGED_WITHOUT_GILx(x, ret_type, name, ...)					\
											\
struct name##_without_gil_args { __RUGGED_STRUCT_DECL##x(__VA_ARGS__) };		\
extern ret_type __##name##_without_gil(void* _args);					\
											\
static inline ret_type name##_without_gil(__RUGGED_DECL##x(__VA_ARGS__))		\
{											\
	struct name##_without_gil_args args = { __RUGGED_ARGS##x(__VA_ARGS__) };	\
	ret_type value;									\
											\
	if (rugged_without_gil_enabled) {						\
		rugged_gc_disable();							\
		value = (ret_type)rb_thread_call_without_gvl(				\
				__##name##_without_gil, &args, NULL, 0);		\
		rugged_gc_enable();							\
	} else										\
		value = name(__RUGGED_ARGS##x(__VA_ARGS__));				\
											\
	return value;									\
}

#endif

#endif
