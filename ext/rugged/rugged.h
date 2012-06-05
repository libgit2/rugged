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

extern VALUE rb_eRuggedError;
extern VALUE rb_eRuggedOsError;
extern VALUE rb_eRuggedInvalidError;
extern VALUE rb_eRuggedRefError;
extern VALUE rb_eRuggedZlibError;
extern VALUE rb_eRuggedRepoError;
extern VALUE rb_eRuggedConfigError;
extern VALUE rb_eRuggedRegexError;
extern VALUE rb_eRuggedOdbError;
extern VALUE rb_eRuggedIndexError;
extern VALUE rb_eRuggedObjError;
extern VALUE rb_eRuggedNetError;
extern VALUE rb_eRuggedTagError;
extern VALUE rb_eRuggedTreeError;
extern VALUE rb_eRuggedIndexerError;

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
	rb_iv_get(object, "@owner");
}

static inline void rugged_exception_raise(int errorcode)
{
	VALUE err_klass;
	VALUE err_obj;
	const git_error *error;

	error = giterr_last();

	if (error) {
		switch(error->klass) {
			case GITERR_NOMEMORY:
				err_klass = rb_eNoMemError;
				break;
			case GITERR_OS:
				err_klass = rb_eRuggedOsError;
				break;
			case GITERR_INVALID:
				err_klass = rb_eRuggedInvalidError;
				break;
			case GITERR_REFERENCE:
				err_klass = rb_eRuggedRefError;
				break;
			case GITERR_ZLIB:
				err_klass = rb_eRuggedZlibError;
				break;
			case GITERR_REPOSITORY:
				err_klass = rb_eRuggedRepoError;
				break;
			case GITERR_CONFIG:
				err_klass = rb_eRuggedConfigError;
				break;
			case GITERR_REGEX:
				err_klass = rb_eRuggedRegexError;
				break;
			case GITERR_ODB:
				err_klass = rb_eRuggedOdbError;
				break;
			case GITERR_INDEX:
				err_klass = rb_eRuggedIndexError;
				break;
			case GITERR_OBJECT:
				err_klass = rb_eRuggedObjError;
				break;
			case GITERR_NET:
				err_klass = rb_eRuggedNetError;
				break;
			case GITERR_TAG:
				err_klass = rb_eRuggedTagError;
				break;
			case GITERR_TREE:
				err_klass = rb_eRuggedTreeError;
				break;
			case GITERR_INDEXER:
				err_klass = rb_eRuggedIndexerError;
				break;
			default:
				err_klass = rb_eRuggedError;
		}

		err_obj = rb_exc_new2(err_klass, error->message);
		giterr_clear();
		rb_exc_raise(err_obj);
	}
}

static inline void rugged_exception_check(int errorcode)
{
	VALUE err_obj;
	const git_error *error;

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

static VALUE rugged_str_repoenc(const char *str, long len, VALUE rb_repo)
{
	VALUE rb_enc = rb_iv_get(rb_repo, "@encoding");
	return rb_enc_str_new(str, len, NIL_P(rb_enc) ? NULL : rb_to_encoding(rb_enc));
}
#else
#	define rugged_str_new(str, len, rb_enc)  rb_str_new(str, len)
#	define rugged_str_new2(str, rb_enc) rb_str_new2(str)
#	define rugged_str_repoenc(str, len, repo) rb_str_new(str, len)
#	define rugged_str_ascii(str, len) rb_str_new(str, len)
#endif

static inline VALUE rugged_create_oid(const git_oid *oid)
{
	char out[40];
	git_oid_fmt(out, oid);
	return rugged_str_new(out, 40, NULL);
}

#endif
