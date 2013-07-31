/*
 * The MIT License
 *
 * Copyright (c) 2013 GitHub, Inc
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
VALUE rb_cRuggedSubmodule;

VALUE rugged_submodule_new(VALUE klass, VALUE owner, git_submodule *submodule)
{
	VALUE rb_submodule = Data_Wrap_Struct(klass, NULL, NULL, submodule);
	rugged_set_owner(rb_submodule, owner);
	return rb_submodule;
}

/*
 *  call-seq:
 *    Submodule.lookup(repository, name) -> submodule or nil
 *
 *  Lookup +submodule+ by +name+ or +path+ (they are usually the same) in
 *  +repository+.
 *
 *  Returns +nil+ if submodule does not exist.
 *
 *  Raises <tt>Rugged::SubmoduleError</tt> if submodule exists only in working
 *  directory (i.e. there is a subdirectory that is a valid self-contained git
 *  repository) and is not mentioned in the +HEAD+, the index and the config.
 */
static VALUE rb_git_submodule_lookup(VALUE klass, VALUE rb_repo, VALUE rb_name)
{
	git_repository *repo;
	git_submodule *submodule;
	int error;

	rugged_check_repo(rb_repo);
	Data_Get_Struct(rb_repo, git_repository, repo);

	Check_Type(rb_name, T_STRING);

	error = git_submodule_lookup(
			&submodule,
			repo,
			StringValueCStr(rb_name)
		);

	if (error == GIT_ENOTFOUND)
		return Qnil;

	rugged_exception_check(error);

	return rugged_submodule_new(klass, rb_repo, submodule);
}

void Init_rugged_submodule(void)
{
	rb_cRuggedSubmodule = rb_define_class_under(rb_mRugged, "Submodule", rb_cObject);

	rb_define_singleton_method(rb_cRuggedSubmodule, "lookup", rb_git_submodule_lookup, 2);
}
