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

static VALUE id_in_head, id_in_index, id_in_config, id_in_workdir;
static VALUE id_index_added, id_index_deleted, id_index_modified;
static VALUE id_wd_uninitialized, id_wd_added, id_wd_deleted, id_wd_modified;
static VALUE id_wd_index_modified, id_wd_wd_modified, id_wd_untracked;

VALUE init_status_list(void)
{
	VALUE status_list = rb_ary_new2(14);
	rb_ary_push(status_list,
		id_in_head            = CSTR2SYM("in_head")
	);
	rb_ary_push(status_list,
		id_in_index           = CSTR2SYM("in_index")
	);
	rb_ary_push(status_list,
		id_in_config          = CSTR2SYM("in_config")
	);
	rb_ary_push(status_list,
		id_in_workdir         = CSTR2SYM("in_workdir")
	);
	rb_ary_push(status_list,
		id_index_added        = CSTR2SYM("index_added")
	);
	rb_ary_push(status_list,
		id_index_deleted      = CSTR2SYM("index_deleted")
	);
	rb_ary_push(status_list,
		id_index_modified     = CSTR2SYM("index_modified")
	);
	rb_ary_push(status_list,
		id_wd_uninitialized   = CSTR2SYM("workdir_uninitialized")
	);
	rb_ary_push(status_list,
		id_wd_added           = CSTR2SYM("workdir_added")
	);
	rb_ary_push(status_list,
		id_wd_deleted         = CSTR2SYM("workdir_deleted")
	);
	rb_ary_push(status_list,
		id_wd_modified        = CSTR2SYM("workdir_modified")
	);
	rb_ary_push(status_list,
		id_wd_index_modified  = CSTR2SYM("workdir_index_modified")
	);
	rb_ary_push(status_list,
		id_wd_wd_modified     = CSTR2SYM("workdir_workdir_modified")
	);
	rb_ary_push(status_list,
		id_wd_untracked       = CSTR2SYM("workdir_untracked")
	);

	return status_list;
}

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


static VALUE submodule_status_flags_to_rb(unsigned int flags)
{
	VALUE rb_flags = rb_ary_new();

	if (flags & GIT_SUBMODULE_STATUS_IN_HEAD)
		rb_ary_push(rb_flags, id_in_head);

	if (flags & GIT_SUBMODULE_STATUS_IN_INDEX)
		rb_ary_push(rb_flags, id_in_index);

	if (flags & GIT_SUBMODULE_STATUS_IN_CONFIG)
		rb_ary_push(rb_flags, id_in_config);

	if (flags & GIT_SUBMODULE_STATUS_IN_WD)
		rb_ary_push(rb_flags, id_in_workdir);

	if (flags & GIT_SUBMODULE_STATUS_INDEX_ADDED)
		rb_ary_push(rb_flags, id_index_added);

	if (flags & GIT_SUBMODULE_STATUS_INDEX_DELETED)
		rb_ary_push(rb_flags, id_index_deleted);

	if (flags & GIT_SUBMODULE_STATUS_INDEX_MODIFIED)
		rb_ary_push(rb_flags, id_index_modified);

	if (flags & GIT_SUBMODULE_STATUS_WD_UNINITIALIZED)
		rb_ary_push(rb_flags, id_wd_uninitialized);

	if (flags & GIT_SUBMODULE_STATUS_WD_ADDED)
		rb_ary_push(rb_flags, id_wd_added);

	if (flags & GIT_SUBMODULE_STATUS_WD_DELETED)
		rb_ary_push(rb_flags, id_wd_deleted);

	if (flags & GIT_SUBMODULE_STATUS_WD_MODIFIED)
		rb_ary_push(rb_flags, id_wd_modified);

	if (flags & GIT_SUBMODULE_STATUS_WD_INDEX_MODIFIED)
		rb_ary_push(rb_flags, id_wd_index_modified);

	if (flags & GIT_SUBMODULE_STATUS_WD_WD_MODIFIED)
		rb_ary_push(rb_flags, id_wd_wd_modified);

	if (flags & GIT_SUBMODULE_STATUS_WD_UNTRACKED)
		rb_ary_push(rb_flags, id_wd_untracked);

	return rb_flags;
}

static VALUE rb_git_submodule_status(VALUE self)
{
	git_submodule *submodule;
	unsigned int flags;

	Data_Get_Struct(self, git_submodule, submodule);

	rugged_exception_check(
		git_submodule_status(&flags, submodule)
	);

	return submodule_status_flags_to_rb(flags);

}

/*
 *  call-seq:
 *    submodule.add_to_index(write_index = true) -> nil
 *
 *  Add current submodule +HEAD+ commit to the index of superproject.
 *
 *  +write_index+ (optional, default +true+) - if this should immediately write
 *  the index file. If you pass this as +false+, you will have to get the
 *  Rugged::Repository#index and explicitly call Rugged::Index#write on it to
 *  save the change.
 */
static VALUE rb_git_submodule_add_to_index(int argc, VALUE *argv, VALUE self)
{
	git_submodule *submodule;
	VALUE rb_write_index;
	int write_index;

	Data_Get_Struct(self, git_submodule, submodule);

	rb_scan_args(argc, argv, "01", &rb_write_index);

	if(NIL_P(rb_write_index))
		write_index = 1;
	else
		write_index = rugged_parse_bool(rb_write_index);

	rugged_exception_check(
		git_submodule_add_to_index(submodule, write_index)
	);

	return Qnil;
}

/*
 *  call-seq:
 *    submodule.reload -> nil
 *
 *  Reread submodule info from config, index, and +HEAD+.
 *
 *  Call this to reread cached submodule information for this submodule if
 *  you have reason to believe that it has changed.
 */
static VALUE rb_git_submodule_reload(VALUE self)
{
	git_submodule *submodule;
	Data_Get_Struct(self, git_submodule, submodule);

	rugged_exception_check(
		git_submodule_reload(submodule)
	);

	return Qnil;
}


void Init_rugged_submodule(void)
{
	VALUE status_list = init_status_list();
	rb_cRuggedSubmodule = rb_define_class_under(rb_mRugged, "Submodule", rb_cObject);
	rb_define_const(rb_cRuggedSubmodule, "STATUS_LIST", status_list);

	rb_define_singleton_method(rb_cRuggedSubmodule, "lookup", rb_git_submodule_lookup, 2);

	rb_define_method(rb_cRuggedSubmodule, "status", rb_git_submodule_status, 0);
	rb_define_method(rb_cRuggedSubmodule, "add_to_index", rb_git_submodule_add_to_index, -1);
	rb_define_method(rb_cRuggedSubmodule, "reload", rb_git_submodule_reload, 0);
}
