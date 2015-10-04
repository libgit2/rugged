/*
 * The MIT License
 *
 * Copyright (c) 2015 GitHub, Inc
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
VALUE rb_cRuggedOdb;

VALUE rugged_odb_new(VALUE klass, VALUE owner, git_odb *odb)
{
	VALUE rb_odb = Data_Wrap_Struct(klass, NULL, &git_odb_free, odb);

	rugged_set_owner(rb_odb, owner);

	return rb_odb;
}

/*
 *  call-seq:
 *    Odb.new() -> odb
 *
 *  Create a new object database with no backend.
 *
 *  Before the object database can be used, backends
 *  needs to be assigned using `Odb#add_backend`.
 */
static VALUE rb_git_odb_new(VALUE klass) {
	git_odb *odb;

	rugged_exception_check(git_odb_new(&odb));

	return rugged_odb_new(klass, Qnil, odb);
}

/*
 *  call-seq:
 *    Odb.open(dir) -> odb
 *
 *  Create a new object database with the default filesystem
 *  backends.
 *
 *  `dir` needs to point to the objects folder to be used
 *  by the filesystem backends.
 */
static VALUE rb_git_odb_open(VALUE klass, VALUE rb_path) {
	git_odb *odb;

	rugged_exception_check(git_odb_open(&odb, StringValueCStr(rb_path)));

	return rugged_odb_new(klass, Qnil, odb);
}

/*
 *  call-seq:
 *    odb.add_backend(backend, priority) -> odb
 *
 *  Add a backend to be used by the object db.
 *
 *  A backend can only be assigned once, and becomes unusable from that
 *  point on. Trying to assign a backend a second time will raise an
 *  exception.
 */
static VALUE rb_git_odb_add_backend(VALUE self, VALUE rb_backend, VALUE rb_priority)
{
	git_odb *odb;
	git_odb_backend *backend;

	Data_Get_Struct(self, git_odb, odb);
	Data_Get_Struct(rb_backend, git_odb_backend, backend);

	if (!backend)
		rb_exc_raise(rb_exc_new2(rb_eRuntimeError, "Can not reuse odb backend instances"));

	rugged_exception_check(git_odb_add_backend(odb, backend, NUM2INT(rb_priority)));

	// libgit2 has taken ownership of the backend, so we should make sure
	// we don't try to free it.
	((struct RData *)rb_backend)->data = NULL;

	return self;
}

static int cb_odb__each(const git_oid *id, void *data)
{
	char out[40];
	struct rugged_cb_payload *payload = data;

	git_oid_fmt(out, id);
	rb_protect(rb_yield, rb_str_new(out, 40), &payload->exception);

	return payload->exception ? GIT_ERROR : GIT_OK;
}

static VALUE rb_git_odb_each(VALUE self)
{
	git_odb *odb;
	int error;
	struct rugged_cb_payload payload = { self, 0 };

	Data_Get_Struct(self, git_odb, odb);

	error = git_odb_foreach(odb, &cb_odb__each, &payload);

	if (payload.exception)
		rb_jump_tag(payload.exception);
	rugged_exception_check(error);

	return Qnil;
}

void Init_rugged_odb(void)
{
	rb_cRuggedOdb = rb_define_class_under(rb_mRugged, "Odb", rb_cObject);

	rb_define_singleton_method(rb_cRuggedOdb, "new", rb_git_odb_new, 0);
	rb_define_singleton_method(rb_cRuggedOdb, "open", rb_git_odb_open, 1);

	rb_define_method(rb_cRuggedOdb, "add_backend", rb_git_odb_add_backend, 2);
	rb_define_method(rb_cRuggedOdb, "each", rb_git_odb_each, 0);
}
