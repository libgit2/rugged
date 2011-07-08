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

VALUE rb_mRugged;

static VALUE rb_git_hex_to_raw(VALUE self, VALUE hex)
{
	git_oid oid;

	Check_Type(hex, T_STRING);
	rugged_exception_check(git_oid_fromstr(&oid, StringValueCStr(hex)));
	return rugged_str_ascii(oid.id, 20);
}

static VALUE rb_git_raw_to_hex(VALUE self, VALUE raw)
{
	git_oid oid;
	char out[40];

	Check_Type(raw, T_STRING);
	git_oid_fromraw(&oid, StringValueCStr(raw));
	git_oid_fmt(out, &oid);

	return rugged_str_new(out, 40, NULL);
}

static VALUE rb_git_type_to_string(VALUE self, VALUE type)
{
	const char *str;

	Check_Type(type, T_FIXNUM);
	git_otype t = (git_otype)FIX2INT(type);
	str = git_object_type2string(t);
	return str ? rugged_str_new2(str, NULL) : Qfalse;
}

static VALUE rb_git_string_to_type(VALUE self, VALUE string_type)
{
	Check_Type(string_type, T_STRING);
	return INT2FIX(git_object_string2type(StringValueCStr(string_type)));
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
		return Qnil;
	}

	return INT2FIX(length);
}

void Init_rugged()
{
	rb_mRugged = rb_define_module("Rugged");

	rb_define_module_function(rb_mRugged, "hex_to_raw", rb_git_hex_to_raw, 1);
	rb_define_module_function(rb_mRugged, "raw_to_hex", rb_git_raw_to_hex, 1);
	rb_define_module_function(rb_mRugged, "type_to_string", rb_git_type_to_string, 1);
	rb_define_module_function(rb_mRugged, "string_to_type", rb_git_string_to_type, 1);
	rb_define_module_function(rb_mRugged, "minimize_oid", rb_git_minimize_oid, -1);

	Init_rugged_signature();

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

	Init_rugged_backend();

	/* Constants */
	rb_define_const(rb_mRugged, "SORT_NONE", INT2FIX(0));
	rb_define_const(rb_mRugged, "SORT_TOPO", INT2FIX(1));
	rb_define_const(rb_mRugged, "SORT_DATE", INT2FIX(2));
	rb_define_const(rb_mRugged, "SORT_REVERSE", INT2FIX(4));

	rb_define_const(rb_mRugged, "OBJ_ANY", INT2FIX(GIT_OBJ_ANY));
	rb_define_const(rb_mRugged, "OBJ_BAD", INT2FIX(GIT_OBJ_BAD));
	rb_define_const(rb_mRugged, "OBJ_COMMIT", INT2FIX(GIT_OBJ_COMMIT));
	rb_define_const(rb_mRugged, "OBJ_TREE", INT2FIX(GIT_OBJ_TREE));
	rb_define_const(rb_mRugged, "OBJ_BLOB", INT2FIX(GIT_OBJ_BLOB));
	rb_define_const(rb_mRugged, "OBJ_TAG", INT2FIX(GIT_OBJ_TAG));

	rb_define_const(rb_mRugged, "REF_OID", INT2FIX(GIT_REF_OID));
	rb_define_const(rb_mRugged, "REF_SYMBOLIC", INT2FIX(GIT_REF_SYMBOLIC));
}

