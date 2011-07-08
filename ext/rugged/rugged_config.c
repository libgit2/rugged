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

extern VALUE rb_mRugged;
VALUE rb_cRuggedConfig;

void rb_git_config__free(git_config *config)
{
	git_config_free(config);
}

static VALUE rugged_config__alloc(VALUE klass, git_config *cfg)
{
	return Data_Wrap_Struct(klass, NULL, rb_git_config__free, cfg);
}

VALUE rugged_config_new(git_config *cfg)
{
	return rugged_config__alloc(rb_cRuggedConfig, cfg);
}

static VALUE rb_git_config_allocate(VALUE klass)
{
	return rugged_config__alloc(klass, NULL);
}

static VALUE rb_git_config_init(VALUE self, VALUE rb_path)
{
	git_config *config = NULL;
	int error, i;

	if (TYPE(rb_path) == T_ARRAY) {
		error = git_config_new(&config);
		rugged_exception_check(error);

		for (i = 0; i < RARRAY_LEN(rb_path); ++i) {
			VALUE f = rb_ary_entry(rb_path, i);
			Check_Type(f, T_STRING);
			error = git_config_add_file_ondisk(config, StringValueCStr(f), i + 1);
			rugged_exception_check(error);
		}
	} else if (TYPE(rb_path) == T_STRING) {
		error = git_config_open_ondisk(&config, StringValueCStr(rb_path));
		rugged_exception_check(error);
	} else {
		rb_raise(rb_eTypeError, "Expecting a filename or an array of filenames");
	}

	DATA_PTR(self) = config;
	return Qnil;
}

static VALUE rb_git_config_get(VALUE self, VALUE rb_key)
{
	git_config *config;
	const char *value;
	int error;

	Data_Get_Struct(self, git_config, config);
	Check_Type(rb_key, T_STRING);

	error = git_config_get_string(config, StringValueCStr(rb_key), &value);
	if (error == GIT_ENOTFOUND)
		return Qnil;

	rugged_exception_check(error);
	return rugged_str_new2(value, NULL);
}

static VALUE rb_git_config_store(VALUE self, VALUE rb_key, VALUE rb_val)
{
	git_config *config;
	const char *key;
	int error;

	Data_Get_Struct(self, git_config, config);
	Check_Type(rb_key, T_STRING);

	key = StringValueCStr(rb_key);

	switch (TYPE(rb_val)) {
	case T_STRING:
		error = git_config_set_string(config, key, StringValueCStr(rb_val));
		break;

	case T_TRUE:
	case T_FALSE:
		error = git_config_set_bool(config, key, (rb_val == Qtrue));
		break;

	case T_FIXNUM:
		error = git_config_set_int(config, key, FIX2INT(rb_val));
		break;

	default:
		rb_raise(rb_eTypeError,
			"Invalid value; config files can only store string, bool or int keys");
	}

	rugged_exception_check(error);
	return Qnil;
}

static int cb_config__each_key(const char *key, const char *value, void *opaque)
{
	rb_funcall((VALUE)opaque, rb_intern("call"), 1, rugged_str_new2(key, NULL));
	return GIT_SUCCESS;
}

static int cb_config__each_pair(const char *key, const char *value, void *opaque)
{
	rb_funcall((VALUE)opaque, rb_intern("call"), 2,
		rugged_str_new2(key, NULL),
		rugged_str_new2(value, NULL));

	return GIT_SUCCESS;
}

static int cb_config__to_hash(const char *key, const char *value, void *opaque)
{
	rb_hash_aset((VALUE)opaque,
		rugged_str_new2(key, NULL),
		rugged_str_new2(value, NULL));

	return GIT_SUCCESS;
}

static VALUE rb_git_config_each_key(VALUE self)
{
	git_config *config;
	int error;

	Data_Get_Struct(self, git_config, config);

	if (!rb_block_given_p())
		return rb_funcall(self, rb_intern("to_enum"), 1, CSTR2SYM("each_key"));

	error = git_config_foreach(config, &cb_config__each_key, (void *)rb_block_proc());
	rugged_exception_check(error);
	return Qnil;
}

static VALUE rb_git_config_each_pair(VALUE self)
{
	git_config *config;
	int error;

	Data_Get_Struct(self, git_config, config);

	if (!rb_block_given_p())
		return rb_funcall(self, rb_intern("to_enum"), 1, CSTR2SYM("each_pair"));

	error = git_config_foreach(config, &cb_config__each_pair, (void *)rb_block_proc());
	rugged_exception_check(error);
	return Qnil;
}

static VALUE rb_git_config_to_hash(VALUE self)
{
	git_config *config;
	int error;
	VALUE hash;

	Data_Get_Struct(self, git_config, config);
	hash = rb_hash_new();

	error = git_config_foreach(config, &cb_config__to_hash, (void *)hash);
	rugged_exception_check(error);
	return hash;
}

static VALUE rb_git_config_open_global(VALUE self)
{
	git_config *cfg;
	int error;

	error = git_config_open_global(&cfg);
	rugged_exception_check(error);

	return rugged_config_new(cfg);
}

void Init_rugged_config()
{
	/*
	 * Config
	 */
	rb_cRuggedConfig = rb_define_class_under(rb_mRugged, "Config", rb_cObject);
	rb_define_alloc_func(rb_cRuggedConfig, rb_git_config_allocate);
	rb_define_method(rb_cRuggedConfig, "initialize", rb_git_config_init, 1);

	rb_define_method(rb_cRuggedConfig, "store", rb_git_config_store, 2);
	rb_define_method(rb_cRuggedConfig, "[]=", rb_git_config_store, 2);

	rb_define_method(rb_cRuggedConfig, "get", rb_git_config_get, 1);
	rb_define_method(rb_cRuggedConfig, "[]", rb_git_config_get, 1);

	rb_define_method(rb_cRuggedConfig, "each_key", rb_git_config_each_key, 0);
	rb_define_method(rb_cRuggedConfig, "each_pair", rb_git_config_each_pair, 0);
	rb_define_method(rb_cRuggedConfig, "each", rb_git_config_each_pair, 0);
	rb_define_method(rb_cRuggedConfig, "to_hash", rb_git_config_to_hash, 0);

	rb_define_singleton_method(rb_cRuggedConfig, "open_global", rb_git_config_open_global, 0);
}
