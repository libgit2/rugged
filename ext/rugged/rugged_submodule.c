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
		id_index_added        = CSTR2SYM("added_to_index")
	);
	rb_ary_push(status_list,
		id_index_deleted      = CSTR2SYM("deleted_from_index")
	);
	rb_ary_push(status_list,
		id_index_modified     = CSTR2SYM("modified_in_index")
	);
	rb_ary_push(status_list,
		id_wd_uninitialized   = CSTR2SYM("uninitialized")
	);
	rb_ary_push(status_list,
		id_wd_added           = CSTR2SYM("added_to_workdir")
	);
	rb_ary_push(status_list,
		id_wd_deleted         = CSTR2SYM("deleted_from_workdir")
	);
	rb_ary_push(status_list,
		id_wd_modified        = CSTR2SYM("modified_in_workdir")
	);
	rb_ary_push(status_list,
		id_wd_index_modified  = CSTR2SYM("dirty_workdir_index")
	);
	rb_ary_push(status_list,
		id_wd_wd_modified     = CSTR2SYM("modified_files_in_workdir")
	);
	rb_ary_push(status_list,
		id_wd_untracked       = CSTR2SYM("untracked_files_in_workdir")
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

/*
 *  call-seq:
 *    submodule.status -> array
 *
 *  Returns an +array+ with the status flags for a submodule.
 *
 *  Depending on the Submodule#ignore property of the submodule, some of
 *  the flags may never be returned because they indicate changes that are
 *  supposed to be ignored.
 *
 *  \Submodule info is contained in 4 places: the +HEAD+ tree, the index,
 *  config files (both +.git/config+ and +.gitmodules+), and the working
 *  directory. Any or all of those places might be missing information about
 *  the submodule depending on what state the repository is in. We consider
 *  all four places to build the combination of status flags.
 *
 *  There are four values that are not really status, but give basic info
 *  about what sources of submodule data are available. These will be
 *  returned even if ignore is set to +:all+.
 *
 *  [:in_head]
 *    superproject +HEAD+ contains submodule
 *  [:in_index]
 *    superproject index contains submodule
 *  [:in_config]
 *    superproject +.gitmodules+ has submodule
 *  [:in_workdir]
 *    superproject workdir has submodule
 *
 *  The following values will be returned so long as ignore is not +:all+.
 *
 *  [:added_to_index]
 *    submodule is in index, not in +HEAD+
 *  [:deleted_from_index]
 *    submodule is in +HEAD+, not in index
 *  [:modified_in_index]
 *    submodule in index and +HEAD+ don't match
 *  [:uninitialized]
 *    submodule in workdir is not initialized
 *  [:added_to_workdir]
 *    submodule is in workdir, not index
 *  [:deleted_from_workdir]
 *    submodule is in index, not workdir
 *  [:modified_in_workdir]
 *    submodule in index and workdir +HEAD+ don't match
 *
 *  The following can only be returned if ignore is +:none+ or +:untracked+.
 *
 *  [:dirty_workdir_index]
 *    submodule workdir index is dirty
 *  [:modified_files_in_workdir]
 *    submodule workdir has modified files
 *
 *  Lastly, the following will only be returned for ignore +:none+.
 *
 *  [:untracked_files_in_workdir]
 *    submodule workdir contains untracked files
 */
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

#define RB_GIT_STRING_GETTER(_klass, _attribute) \
	git_##_klass *object; \
	const char * value; \
	Data_Get_Struct(self, git_##_klass, object); \
	value = git_##_klass##_##_attribute(object); \
	return value ? rb_str_new_utf8(value) : Qnil; \

/*
 *  call-seq:
 *    submodule.name -> string
 *
 *  Returns the name of the submodule.
 */
static VALUE rb_git_submodule_name(VALUE self)
{
	RB_GIT_STRING_GETTER(submodule, name);
}

/*
 *  call-seq:
 *    submodule.url -> string
 *
 *  Returns the URL of the submodule.
 */
static VALUE rb_git_submodule_url(VALUE self)
{
	RB_GIT_STRING_GETTER(submodule, url);
}

/*
 *  call-seq:
 *    submodule.path -> string
 *
 *  Returns the path of the submodule.
 *
 *  The +path+ is almost always the same as the Submodule#name,
 *  but the two are actually not required to match.
 */
static VALUE rb_git_submodule_path(VALUE self)
{
	RB_GIT_STRING_GETTER(submodule, path);
}

#define RB_GIT_OID_GETTER(_klass, _attribute) \
	git_##_klass *object; \
	const git_oid * oid; \
	Data_Get_Struct(self, git_##_klass, object); \
	oid = git_##_klass##_##_attribute(object); \
	return oid ? rugged_create_oid(oid) : Qnil; \

/*
 *  call-seq:
 *    submodule.head_oid -> string or nil
 *
 *  Returns the OID for the submodule in the current +HEAD+ tree or +nil+
 *  if the submodule is not in the +HEAD+.
 */
static VALUE rb_git_submodule_head_id(VALUE self)
{
	RB_GIT_OID_GETTER(submodule, head_id);
}

/*
 *  call-seq:
 *    submodule.index_oid -> string or nil
 *
 *  Returns the OID for the submodule in the index or +nil+ if the submodule
 *  is not in the index.
 */
static VALUE rb_git_submodule_index_id(VALUE self)
{
	RB_GIT_OID_GETTER(submodule, index_id);
}

/*
 *  call-seq:
 *    submodule.workdir_oid -> string or nil
 *
 *  Returns the OID for the submodule in the current working directory or
 *  +nil+ of the submodule is not checked out.
 *
 *  This returns the OID that corresponds to looking up +HEAD+ in the checked
 *  out submodule.  If there are pending changes in the index or anything
 *  else, this won't notice that.  You should call Submodule#status
 *  for a more complete picture about the state of the working directory.
 */
static VALUE rb_git_submodule_wd_id(VALUE self)
{
	RB_GIT_OID_GETTER(submodule, wd_id);
}


void Init_rugged_submodule(void)
{
	VALUE status_list = init_status_list();
	rb_cRuggedSubmodule = rb_define_class_under(rb_mRugged, "Submodule", rb_cObject);
	rb_define_const(rb_cRuggedSubmodule, "STATUS_LIST", status_list);

	rb_define_singleton_method(rb_cRuggedSubmodule, "lookup", rb_git_submodule_lookup, 2);

	rb_define_method(rb_cRuggedSubmodule, "name", rb_git_submodule_name, 0);
	rb_define_method(rb_cRuggedSubmodule, "url", rb_git_submodule_url, 0);
	rb_define_method(rb_cRuggedSubmodule, "path", rb_git_submodule_path, 0);

	rb_define_method(rb_cRuggedSubmodule, "head_oid", rb_git_submodule_head_id, 0);
	rb_define_method(rb_cRuggedSubmodule, "index_oid", rb_git_submodule_index_id, 0);
	rb_define_method(rb_cRuggedSubmodule, "workdir_oid", rb_git_submodule_wd_id, 0);

	rb_define_method(rb_cRuggedSubmodule, "status", rb_git_submodule_status, 0);
	rb_define_method(rb_cRuggedSubmodule, "add_to_index", rb_git_submodule_add_to_index, -1);
	rb_define_method(rb_cRuggedSubmodule, "reload", rb_git_submodule_reload, 0);
}
