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
VALUE rb_cRuggedRefdb;

VALUE rugged_refdb_new(VALUE klass, VALUE owner, git_refdb *refdb)
{
	VALUE rb_refdb = Data_Wrap_Struct(klass, NULL, git_refdb_free, refdb);
	rugged_set_owner(rb_refdb, owner);
	return rb_refdb;
}

/*
 *  call-seq:
 *    Refdb.new(repository) -> refdb
 *
 *  Create a new reference database with no backends.
 *
 *  Before the reference database can be used, a backend
 *  needs to be assigned using `Refdb#backend=`.
 */
static VALUE rb_git_refdb_new(VALUE klass, VALUE rb_repo) {
	git_refdb *refdb;
	git_repository *repo;

	Data_Get_Struct(rb_repo, git_repository, repo);

	rugged_exception_check(git_refdb_new(&refdb, repo));

	return rugged_refdb_new(klass, Qnil, refdb);
}

/*
 *  call-seq:
 *    Refdb.open(repository) -> refdb
 *
 *  Create a new reference database with the default filesystem
 *  backend.
 */
static VALUE rb_git_refdb_open(VALUE klass, VALUE rb_repo) {
	git_refdb *refdb;
	git_repository *repo;

	Data_Get_Struct(rb_repo, git_repository, repo);

	rugged_exception_check(git_refdb_open(&refdb, repo));

	return rugged_refdb_new(klass, Qnil, refdb);
}

/*
 *  call-seq:
 *    refdb.backend = backend
 *
 *  Set the backend to be used by the reference db.
 *
 *  A backend can only be assigned once, and becomes unusable from that
 *  point on. Trying to assign a backend a second time will raise an
 *  exception.
 */
static VALUE rb_git_refdb_set_backend(VALUE self, VALUE rb_backend)
{
	git_refdb *refdb;
	git_refdb_backend *backend;

	Data_Get_Struct(self, git_refdb, refdb);
	Data_Get_Struct(rb_backend, git_refdb_backend, backend);

	if (!backend)
		rb_exc_raise(rb_exc_new2(rb_eRuntimeError, "Can not reuse refdb backend instances"));

	rugged_exception_check(git_refdb_set_backend(refdb, backend));

	// libgit2 has taken ownership of the backend, so we should make sure
	// we don't try to free it.
	((struct RData *)rb_backend)->data = NULL;

	return Qnil;
}

/*
 *  call-seq:
 *    refdb.compress -> nil
 */
static VALUE rb_git_refdb_compress(VALUE self)
{
	git_refdb *refdb;

	Data_Get_Struct(self, git_refdb, refdb);

	rugged_exception_check(git_refdb_compress(refdb));

	return Qnil;
}

void Init_rugged_refdb(void)
{
	rb_cRuggedRefdb = rb_define_class_under(rb_mRugged, "Refdb", rb_cObject);

	rb_define_singleton_method(rb_cRuggedRefdb, "new", rb_git_refdb_new, 1);
	rb_define_singleton_method(rb_cRuggedRefdb, "open", rb_git_refdb_open, 1);

	rb_define_method(rb_cRuggedRefdb, "backend=", rb_git_refdb_set_backend, 1);
	rb_define_method(rb_cRuggedRefdb, "compress", rb_git_refdb_compress, 0);
}
