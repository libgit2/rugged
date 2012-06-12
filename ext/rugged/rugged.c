/*
 * The MIT License
 *
 * Copyright (c) 2011 GitHub, Inc
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "rugged.h"

const char *RUGGED_ERROR_NAMES[] = {
	NULL, /* GITERR_NOMEMORY, */
	"OSError", /* GITERR_OS, */
	"InvalidError", /* GITERR_INVALID, */
	"ReferenceError", /* GITERR_REFERENCE, */
	"ZlibError", /* GITERR_ZLIB, */
	"RepositoryError", /* GITERR_REPOSITORY, */
	"ConfigError", /* GITERR_CONFIG, */
	"RegexError", /* GITERR_REGEX, */
	"OdbError", /* GITERR_ODB, */
	"IndexError", /* GITERR_INDEX, */
	"ObjectError", /* GITERR_OBJECT, */
	"NetworkError" /* GITERR_NET, */
	"TagError", /* GITERR_TAG, */
	"TreeError", /* GITERR_TREE, */
	"IndexerError", /* GITERR_INDEXER, */
};

#define RUGGED_ERROR_COUNT (int)((sizeof(RUGGED_ERROR_NAMES)/sizeof(RUGGED_ERROR_NAMES[0])))

VALUE rb_mRugged;
VALUE rb_eRuggedError;
VALUE rb_eRuggedErrors[RUGGED_ERROR_COUNT];

static VALUE rb_mShutdownHook;

/*
 *	call-seq:
 *		Rugged.hex_to_raw(oid) -> raw_buffer
 *
 *	Turn a string of 40 hexadecimal characters into the buffer of
 *	20 bytes it represents.
 *
 *		Rugged.hex_to_raw('d8786bfc97485e8d7b19b21fb88c8ef1f199fc3f')
 *		#=> "\330xk\374\227H^\215{\031\262\037\270\214\216\361\361\231\374?"
 */
static VALUE rb_git_hex_to_raw(VALUE self, VALUE hex)
{
	git_oid oid;

	Check_Type(hex, T_STRING);
	rugged_exception_check(git_oid_fromstr(&oid, StringValueCStr(hex)));

	return rugged_str_ascii((const char *)oid.id, 20);
}

/*
 *	call-seq:
 *		Rugged.raw_to_hex(buffer) -> hex_oid
 *
 *	Turn a buffer of 20 bytes (representing a SHA1 OID) into its
 *	readable hexadecimal representation.
 *
 *		Rugged.raw_to_hex("\330xk\374\227H^\215{\031\262\037\270\214\216\361\361\231\374?")
 *		#=> "d8786bfc97485e8d7b19b21fb88c8ef1f199fc3f"
 */
static VALUE rb_git_raw_to_hex(VALUE self, VALUE raw)
{
	git_oid oid;
	char out[40];

	Check_Type(raw, T_STRING);

	if (RSTRING_LEN(raw) != GIT_OID_RAWSZ)
		rb_raise(rb_eTypeError, "Invalid buffer size for an OID");

	git_oid_fromraw(&oid, (const unsigned char *)RSTRING_PTR(raw));
	git_oid_fmt(out, &oid);

	return rugged_str_new(out, 40, NULL);
}

static VALUE minimize_cb(VALUE rb_oid, git_oid_shorten *shortener)
{
	Check_Type(rb_oid, T_STRING);
	git_oid_shorten_add(shortener, RSTRING_PTR(rb_oid));
	return Qnil;
}

static VALUE minimize_yield(VALUE rb_oid, VALUE *data)
{
	rb_funcall(data[0], rb_intern("call"), 1,
		rb_str_substr(rb_oid, 0, FIX2INT(data[1])));
	return Qnil;
}

/*
 *	call-seq:
 *		Rugged.minimize_oid(oid_iterator, min_length = 7) { |short_oid| block }
 *		Rugged.minimize_oid(oid_iterator, min_length = 7) -> min_length
 *
 *	Iterate through +oid_iterator+, which should yield any number of SHA1 OIDs
 *	(represented as 40-character hexadecimal strings), and tries to minify them.
 *
 *	Minifying a set of a SHA1 strings means finding the shortest root substring
 *	for each string that uniquely identifies it.
 *
 *	If no +block+ is given, the function will return the minimal length as an
 *	integer value:
 *
 *		oids = [
 *			'd8786bfc974aaaaaaaaaaaaaaaaaaaaaaaaaaaaa',
 *			'd8786bfc974bbbbbbbbbbbbbbbbbbbbbbbbbbbbb',
 *			'd8786bfc974ccccccccccccccccccccccccccccc',
 *			'68d041ee999cb07c6496fbdd4f384095de6ca9e1',
 *		]
 *
 *		Rugged.minimize_oids(oids) #=> 12
 *
 *	If a +block+ is given, it will be called with each OID from +iterator+
 *	in its minified form:
 *
 *		Rugged.minimize_oids(oids) { |oid| puts oid }
 *
 *	produces:
 *
 *		d8786bfc974a
 *		d8786bfc974b
 *		d8786bfc974c
 *		68d041ee999c
 *
 *	The optional +min_length+ argument allows you to specify a lower bound for
 *	the minified strings; returned strings won't be shorter than the given value,
 *	even if they would still be uniquely represented.
 *
 *		Rugged.minimize_oids(oids, 18) #=> 18
 */
static VALUE rb_git_minimize_oid(int argc, VALUE *argv, VALUE self)
{
	git_oid_shorten *shrt;
	int length, minlen = 7;
	VALUE rb_enum, rb_minlen, rb_block;

	rb_scan_args(argc, argv, "11&", &rb_enum, &rb_minlen, &rb_block);

	if (!NIL_P(rb_minlen)) {
		Check_Type(rb_minlen, T_FIXNUM);
		minlen = FIX2INT(rb_minlen);
	}

	if (!rb_respond_to(rb_enum, rb_intern("each")))
		rb_raise(rb_eTypeError, "Expecting an Enumerable instance");

	shrt = git_oid_shorten_new(minlen);

	rb_iterate(rb_each, rb_enum, &minimize_cb, (VALUE)shrt);
	length = git_oid_shorten_add(shrt, NULL);
	rugged_exception_check(length);

	if (RTEST(rb_block)) {
		VALUE yield_data[2];

		yield_data[0] = rb_block;
		yield_data[1] = INT2FIX(length);

		rb_iterate(rb_each, rb_enum, &minimize_yield, (VALUE)yield_data);
		git_oid_shorten_free(shrt);
		return Qnil;
	}

	git_oid_shorten_free(shrt);
	return INT2FIX(length);
}

static void cleanup_cb(void *unused)
{
	(void)unused;
	git_threads_shutdown();
}

void rugged_exception_raise(int errorcode)
{
	VALUE err_klass = rb_eRuggedError;
	VALUE err_obj;
	const git_error *error;

	error = giterr_last();

	if (!error)
		return;

	if (error->klass >= 0 && error->klass < RUGGED_ERROR_COUNT)
		err_klass = rb_eRuggedErrors[error->klass];

	err_obj = rb_exc_new2(err_klass, error->message);
	giterr_clear();
	rb_exc_raise(err_obj);
}

void Init_rugged()
{
	rb_mRugged = rb_define_module("Rugged");

	/* Initialize the Error classes */
	{
		int i;

		rb_eRuggedError = rb_define_class_under(rb_mRugged, "Error", rb_eStandardError);
		rb_eRuggedErrors[0] = rb_eNoMemError;

		for (i = 1; i < RUGGED_ERROR_COUNT; ++i) {
			rb_eRuggedErrors[i] = rb_define_class_under(rb_mRugged, RUGGED_ERROR_NAMES[i], rb_eRuggedError);
		}
	}

	rb_define_module_function(rb_mRugged, "hex_to_raw", rb_git_hex_to_raw, 1);
	rb_define_module_function(rb_mRugged, "raw_to_hex", rb_git_raw_to_hex, 1);
	rb_define_module_function(rb_mRugged, "minimize_oid", rb_git_minimize_oid, -1);

	Init_rugged_object();
	Init_rugged_commit();
	Init_rugged_tree();
	Init_rugged_tag();
	Init_rugged_blob();

	Init_rugged_index();
	Init_rugged_repo();
	Init_rugged_revwalk();
	Init_rugged_reference();
	Init_rugged_config();
	Init_rugged_remote();

	/* Constants */
	rb_define_const(rb_mRugged, "SORT_NONE", INT2FIX(0));
	rb_define_const(rb_mRugged, "SORT_TOPO", INT2FIX(1));
	rb_define_const(rb_mRugged, "SORT_DATE", INT2FIX(2));
	rb_define_const(rb_mRugged, "SORT_REVERSE", INT2FIX(4));

	/* Initialize libgit2 */
	git_threads_init();

	/* Hook a global object to cleanup the library
	 * on shutdown */
	rb_mShutdownHook = Data_Wrap_Struct(rb_cObject, NULL, &cleanup_cb, NULL);
	rb_global_variable(&rb_mShutdownHook);
}

