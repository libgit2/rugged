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

#ifndef __H_RUGGED_BINDINGS__
#define __H_RUGGED_BINDINGS__

// tell rbx not to use it's caching compat layer
// by doing this we're making a promize to RBX that
// we'll never modify the pointers we get back from RSTRING_PTR
#define RSTRING_NOT_MODIFIED

#include <ruby.h>

#ifdef HAVE_RUBY_ENCODING_H
#	include <ruby/encoding.h>
#endif

#include <assert.h>
#include <git2.h>
#include <git2/odb_backend.h>

#define CSTR2SYM(s) (ID2SYM(rb_intern((s))))

/*
 * Initialization functions
 */
void Init_rugged_object();
void Init_rugged_commit();
void Init_rugged_tree();
void Init_rugged_tag();
void Init_rugged_blob();
void Init_rugged_index();
void Init_rugged_repo();
void Init_rugged_revwalk();
void Init_rugged_reference();
void Init_rugged_config();
void Init_rugged_remote();

VALUE rb_git_object_init(git_otype type, int argc, VALUE *argv, VALUE self);

VALUE rugged_raw_read(git_repository *repo, const git_oid *oid);

VALUE rugged_signature_new(const git_signature *sig, const char *encoding_name);

VALUE rugged_index_new(VALUE klass, VALUE owner, git_index *index);
VALUE rugged_object_new(VALUE owner, git_object *object);
VALUE rugged_config_new(VALUE klass, VALUE owner, git_config *cfg);

VALUE rugged_otype_new(git_otype t);
git_otype rugged_otype_get(VALUE rb_type);

git_signature *rugged_signature_get(VALUE rb_person);
git_object *rugged_object_load(git_repository *repo, VALUE object_value, git_otype type);

static inline void rugged_set_owner(VALUE object, VALUE owner)
{
	rb_iv_set(object, "@owner", owner);
}

static inline VALUE rugged_owner(VALUE object)
{
	return rb_iv_get(object, "@owner");
}

extern void rugged_exception_raise(int errorcode);

static inline void rugged_exception_check(int errorcode)
{
	if (errorcode < 0)
		rugged_exception_raise(errorcode);
}

static inline int rugged_parse_bool(VALUE boolean)
{
	if (TYPE(boolean) != T_TRUE && TYPE(boolean) != T_FALSE)
		rb_raise(rb_eTypeError, "Expected boolean value");

	return boolean ? 1 : 0;
}

/* support for string encodings in 1.9 */
#ifdef HAVE_RUBY_ENCODING_H
#	define rugged_str_new(str, len, enc) rb_enc_str_new(str, len, enc)
#	define rugged_str_new2(str, enc) rb_enc_str_new(str, strlen(str), enc)
#	define rugged_str_ascii(str, len) rb_enc_str_new(str, len, rb_ascii8bit_encoding());

#else
#	define rugged_str_new(str, len, rb_enc)  rb_str_new(str, len)
#	define rugged_str_new2(str, rb_enc) rb_str_new2(str)
#	define rugged_str_ascii(str, len) rb_str_new(str, len)
#endif

static inline VALUE rugged_create_oid(const git_oid *oid)
{
	char out[40];
	git_oid_fmt(out, oid);
	return rugged_str_new(out, 40, NULL);
}

#endif
