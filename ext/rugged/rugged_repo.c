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
extern VALUE rb_cRuggedIndex;
extern VALUE rb_cRuggedConfig;
extern VALUE rb_cRuggedBackend;

VALUE rb_cRuggedRepo;
VALUE rb_cRuggedOdbObject;

/*
 *	call-seq:
 *		odb_obj.oid -> hex_oid
 *
 *	Return the Object ID (a 40 character SHA1 hash) for this raw
 *	object.
 *
 *		odb_obj.oid #=> "d8786bfc97485e8d7b19b21fb88c8ef1f199fc3f"
 */
static VALUE rb_git_odbobj_oid(VALUE self)
{
	git_odb_object *obj;
	Data_Get_Struct(self, git_odb_object, obj);
	return rugged_create_oid(git_odb_object_id(obj));
}

/*
 *	call-seq:
 *		odb_obj.data -> buffer
 *
 *	Return an ASCII buffer with the raw bytes that form the Git object.
 *
 *		odb_obj.data #=> "tree 87ebee8367f9cc5ac04858b3bd5610ca74f04df9\n"
 *		             #=> "parent 68d041ee999cb07c6496fbdd4f384095de6ca9e1\n"
 *		             #=> "author Vicent Martí <tanoku@gmail.com> 1326863045 -0800\n"
 *		             #=> ...
 */
static VALUE rb_git_odbobj_data(VALUE self)
{
	git_odb_object *obj;
	Data_Get_Struct(self, git_odb_object, obj);
	return rugged_str_ascii(git_odb_object_data(obj), git_odb_object_size(obj));
}

/*
 *	call-seq:
 *		odb_obj.size -> size
 *
 *	Return the size in bytes of the Git object after decompression. This is
 *	also the size of the +obj.data+ buffer.
 *
 *		odb_obj.size #=> 231
 */
static VALUE rb_git_odbobj_size(VALUE self)
{
	git_odb_object *obj;
	Data_Get_Struct(self, git_odb_object, obj);
	return INT2FIX(git_odb_object_size(obj));
}

/*
 *	call-seq:
 *		odb_obj.type -> Symbol
 *
 *	Return a Ruby symbol representing the basic Git type of this object.
 *	Possible values are +:tree+, +:blob+, +:commit+ and +:tag+
 *
 *		odb_obj.type #=> :tag
 */
static VALUE rb_git_odbobj_type(VALUE self)
{
	git_odb_object *obj;
	Data_Get_Struct(self, git_odb_object, obj);
	return rugged_otype_new(git_odb_object_type(obj));
}

void rb_git__odbobj_free(void *obj)
{
	git_odb_object_free((git_odb_object *)obj);
}

VALUE rugged_raw_read(git_repository *repo, const git_oid *oid)
{
	git_odb *odb;
	git_odb_object *obj;

	int error;

	error = git_repository_odb(&odb, repo);
	rugged_exception_check(error);

	error = git_odb_read(&obj, odb, oid);
	rugged_exception_check(error);

	git_odb_free(odb);

	return Data_Wrap_Struct(rb_cRuggedOdbObject, NULL, rb_git__odbobj_free, obj);
}

void rb_git_repo__free(git_repository *repo)
{
	git_repository_free(repo);
}

static VALUE rugged_repo_new(VALUE klass, git_repository *repo)
{
	VALUE rb_repo = Data_Wrap_Struct(klass, NULL, &rb_git_repo__free, repo);

#ifdef HAVE_RUBY_ENCODING_H
	/* TODO: set this properly */
	rb_iv_set(rb_repo, "@encoding",
		rb_enc_from_encoding(rb_filesystem_encoding()));
#endif

	rb_iv_set(rb_repo, "@config", Qnil);
	rb_iv_set(rb_repo, "@index", Qnil);

	return rb_repo;
}

/*
 *	call-seq:
 *		Rugged::Repository.new(path) -> repository
 *
 *	Open a Git repository in the given +path+ and return a +Repository+ object
 *	representing it. An exception will be thrown if +path+ doesn't point to a
 *	valid repository. If you need to create a repository from scratch, use
 *	+Rugged::Repository.init+ instead.
 *
 *	The +path+ must point to the actual folder (+.git+) of a Git repository.
 *	If you're unsure of where is this located, use +Rugged::Repository.discover+
 *	instead.
 *
 *		Rugged::Repository.new('~/test/.git') #=> #<Rugged::Repository:0x108849488>
 */
static VALUE rb_git_repo_new(VALUE klass, VALUE rb_path)
{
	git_repository *repo;
	int error = 0;

	Check_Type(rb_path, T_STRING);

	error = git_repository_open(&repo, StringValueCStr(rb_path));
	rugged_exception_check(error);

	return rugged_repo_new(klass, repo);
}

/*
 *	call-seq:
 *		Rugged::Repository.init_at(path, is_bare) -> repository
 *
 *	Initialize a Git repository in +path+. This implies creating all the
 *	necessary files on the FS, or re-initializing an already existing
 *	repository if the files have already been created.
 *
 *	The +is_bare+ attribute specifies whether the Repository should be
 *	created on disk as bare or not. Bare repositories have no working
 *	directory and are created in the root of +path+. Non-bare repositories
 *	are created in a +.git+ folder and use +path+ as working directory.
 *
 *		Rugged::Repository.init_at('~/repository') #=> #<Rugged::Repository:0x108849488>
 */
static VALUE rb_git_repo_init_at(VALUE klass, VALUE path, VALUE rb_is_bare)
{
	git_repository *repo;
	int error, is_bare;

	is_bare = rugged_parse_bool(rb_is_bare);
	Check_Type(path, T_STRING);

	error = git_repository_init(&repo, StringValueCStr(path), is_bare);
	rugged_exception_check(error);

	return rugged_repo_new(klass, repo);
}

#define RB_GIT_REPO_OWNED_GET(_klass, _object) \
	VALUE rb_data = rb_iv_get(self, "@" #_object); \
	if (NIL_P(rb_data)) { \
		git_repository *repo; \
		git_##_object *data; \
		int error; \
		Data_Get_Struct(self, git_repository, repo); \
		error = git_repository_##_object(&data, repo); \
		rugged_exception_check(error); \
		rb_data = rugged_##_object##_new(_klass, self, data); \
		rb_iv_set(self, "@" #_object, rb_data); \
	} \
	return rb_data; \

#define RB_GIT_REPO_OWNED_SET(_klass, _object) \
	VALUE rb_old_data; \
	git_repository *repo; \
	git_##_object *data; \
	if (!rb_obj_is_kind_of(rb_data, _klass))\
		rb_raise(rb_eTypeError, \
			"The given object is not a Rugged::" #_object); \
	if (!NIL_P(rugged_owner(rb_data))) \
		rb_raise(rb_eRuntimeError, \
			"The given object is already owned by another repository"); \
	Data_Get_Struct(self, git_repository, repo); \
	Data_Get_Struct(rb_data, git_##_object, data); \
	git_repository_set_##_object(repo, data); \
	rb_old_data = rb_iv_get(self, "@" #_object); \
	if (!NIL_P(rb_old_data)) rugged_set_owner(rb_old_data, Qnil); \
	rugged_set_owner(rb_data, self); \
	rb_iv_set(self, "@" #_object, rb_data); \
	return Qnil; \


/*
 *	call-seq:
 *		repo.index = idx
 *
 *	Set the index for this +Repository+. +idx+ must be a instance of
 *	+Rugged::Index+. This index will be used internally by all
 *	operations that use the Git index on +repo+.
 *
 *	Note that it's not necessary to set the +index+ for any repository;
 *	by default repositories are loaded with the index file that can be
 *	located on the +.git+ folder in the filesystem.
 */
static VALUE rb_git_repo_set_index(VALUE self, VALUE rb_data)
{
	RB_GIT_REPO_OWNED_SET(rb_cRuggedIndex, index);
}

static VALUE rb_git_repo_get_index(VALUE self)
{
	RB_GIT_REPO_OWNED_GET(rb_cRuggedIndex, index);
}

/*
 *	call-seq:
 *		repo.config = cfg
 *
 *	Set the configuration file for this +Repository+. +cfg+ must be a instance of
 *	+Rugged::Config+. This config file will be used internally by all
 *	operations that need to lookup configuration settings on +repo+.
 *
 *	Note that it's not necessary to set the +config+ for any repository;
 *	by default repositories are loaded with their relevant config files
 *	on the filesystem, and the corresponding global and system files if
 *	they can be found.
 */
static VALUE rb_git_repo_set_config(VALUE self, VALUE rb_data)
{
	RB_GIT_REPO_OWNED_SET(rb_cRuggedConfig, config);
}

static VALUE rb_git_repo_get_config(VALUE self)
{
	RB_GIT_REPO_OWNED_GET(rb_cRuggedConfig, config);
}

/*
 *	call-seq:
 *		repo.include?(oid) -> true or false
 *		repo.exists?(oid) -> true or false
 *
 *	Return whether an object with the given SHA1 OID (represented as
 *	a 40-character string) exists in the repository.
 *
 *		repo.include?("d8786bfc97485e8d7b19b21fb88c8ef1f199fc3f") #=> true
 */
static VALUE rb_git_repo_exists(VALUE self, VALUE hex)
{
	git_repository *repo;
	git_odb *odb;
	git_oid oid;
	int error;
	VALUE rb_result;

	Data_Get_Struct(self, git_repository, repo);
	Check_Type(hex, T_STRING);

	error = git_repository_odb(&odb, repo);
	rugged_exception_check(error);

	error = git_oid_fromstr(&oid, StringValueCStr(hex));
	rugged_exception_check(error);

	rb_result = git_odb_exists(odb, &oid) ? Qtrue : Qfalse;
	git_odb_free(odb);

	return rb_result;
}

static VALUE rb_git_repo_read(VALUE self, VALUE hex)
{
	git_repository *repo;
	git_oid oid;
	int error;

	Data_Get_Struct(self, git_repository, repo);
	Check_Type(hex, T_STRING);

	error = git_oid_fromstr(&oid, StringValueCStr(hex));
	rugged_exception_check(error);

	return rugged_raw_read(repo, &oid);
}

/*
 *	call-seq:
 *		Repository.hash(buffer, type) -> oid
 *
 *	Hash the contents of +buffer+ as raw bytes (ignoring any encoding
 *	information) and adding the relevant header corresponding to +type+,
 *	and return a hex string representing the result from the hash.
 *
 *		Repository.hash('hello world', :commit) #=> "de5ba987198bcf2518885f0fc1350e5172cded78"
 *
 *		Repository.hash('hello_world', :tag) #=> "9d09060c850defbc7711d08b57def0d14e742f4e"
 */
static VALUE rb_git_repo_hash(VALUE self, VALUE rb_buffer, VALUE rb_type)
{
	int error;
	git_oid oid;

	Check_Type(rb_buffer, T_STRING);

	error = git_odb_hash(&oid,
		RSTRING_PTR(rb_buffer),
		RSTRING_LEN(rb_buffer),
		rugged_otype_get(rb_type)
	);
	rugged_exception_check(error);

	return rugged_create_oid(&oid);
}

/*
 *	call-seq:
 *		Repository.hash_file(path, type) -> oid
 *
 *	Hash the contents of the file pointed at by +path+, assuming
 *	that it'd be stored in the ODB with the given +type+, and return
 *	a hex string representing the SHA1 OID resulting from the hash.
 *
 *		Repository.hash_file('foo.txt', :commit) #=> "de5ba987198bcf2518885f0fc1350e5172cded78"
 *
 *		Repository.hash_file('foo.txt', :tag) #=> "9d09060c850defbc7711d08b57def0d14e742f4e"
 */
static VALUE rb_git_repo_hashfile(VALUE self, VALUE rb_path, VALUE rb_type)
{
	int error;
	git_oid oid;

	Check_Type(rb_path, T_STRING);

	error = git_odb_hashfile(&oid,
		StringValueCStr(rb_path),
		rugged_otype_get(rb_type)
	);
	rugged_exception_check(error);

	return rugged_create_oid(&oid);
}

static VALUE rb_git_repo_write(VALUE self, VALUE rb_buffer, VALUE rub_type)
{
	git_repository *repo;
	git_odb_stream *stream;

	git_odb *odb;
	git_oid oid;
	int error;

	git_otype type;

	Data_Get_Struct(self, git_repository, repo);

	error = git_repository_odb(&odb, repo);
	rugged_exception_check(error);

	type = rugged_otype_get(rub_type);

	error = git_odb_open_wstream(&stream, odb, RSTRING_LEN(rb_buffer), type);
	rugged_exception_check(error);

	git_odb_free(odb);

	error = stream->write(stream, RSTRING_PTR(rb_buffer), RSTRING_LEN(rb_buffer));
	rugged_exception_check(error);

	error = stream->finalize_write(&oid, stream);
	rugged_exception_check(error);

	return rugged_create_oid(&oid);
}

#define RB_GIT_REPO_GETTER(method) \
	git_repository *repo; \
	int error; \
	Data_Get_Struct(self, git_repository, repo); \
	error = git_repository_##method(repo); \
	rugged_exception_check(error); \
	return error ? Qtrue : Qfalse; \

/*
 *	call-seq:
 *		repo.bare? -> true or false
 *
 *	Return whether a repository is bare or not. A bare repository has no
 *	working directory.
 */
static VALUE rb_git_repo_is_bare(VALUE self)
{
	RB_GIT_REPO_GETTER(is_bare);
}


/*
 *	call-seq:
 *		repo.empty? -> true or false
 *
 *	Return whether a repository is empty or not. An empty repository has just
 *	been initialized and has no commits yet.
 */
static VALUE rb_git_repo_is_empty(VALUE self)
{
	RB_GIT_REPO_GETTER(is_empty);
}

/*
 *	call-seq:
 *		repo.head_detached? -> true or false
 *
 *	Return whether the +HEAD+ of a repository is detached or not.
 */
static VALUE rb_git_repo_head_detached(VALUE self)
{
	RB_GIT_REPO_GETTER(head_detached);
}

/*
 *	call-seq:
 *		repo.head_orphan? -> true or false
 *
 *	Return whether the +HEAD+ of a repository is orphaned or not.
 */
static VALUE rb_git_repo_head_orphan(VALUE self)
{
	RB_GIT_REPO_GETTER(head_orphan);
}

/*
 *	call-seq:
 *		repo.path -> path
 *
 *	Return the full, normalized path to this repository. For non-bare repositories,
 *	this is the path of the actual +.git+ folder, not the working directory.
 *
 *		repo.path #=> "/home/foo/workthing/.git"
 */
static VALUE rb_git_repo_path(VALUE self)
{
	git_repository *repo;
	Data_Get_Struct(self, git_repository, repo);
	return rugged_str_new2(git_repository_path(repo), NULL);
}

/*
 *	call-seq:
 *		repo.workdir -> path or nil
 *
 *	Return the working directory for this repository, or +nil+ if
 *	the repository is bare.
 *
 *		repo1.bare? #=> false
 *		repo1.workdir # => "/home/foo/workthing/"
 *
 *		repo2.bare? #=> true
 *		repo2.workdir #=> nil
 */
static VALUE rb_git_repo_workdir(VALUE self)
{
	git_repository *repo;
	const char *workdir;

	Data_Get_Struct(self, git_repository, repo);
	workdir = git_repository_workdir(repo);

	return workdir ? rugged_str_new2(workdir, NULL) : Qnil;
}

/*
 *	call-seq:
 *		repo.workdir = path
 *
 *	Sets the working directory of +repo+ to +path+. All internal
 *	operations on +repo+ that affect the working directory will
 *	instead use +path+.
 *
 *	The +workdir+ can be set on bare repositories to temporarily
 *	turn them into normal repositories.
 *
 *		repo.bare? #=> true
 *		repo.workdir = "/tmp/workdir"
 *		repo.bare? #=> false
 *		repo.checkout
 */
static VALUE rb_git_repo_set_workdir(VALUE self, VALUE rb_workdir)
{
	git_repository *repo;

	Data_Get_Struct(self, git_repository, repo);
	Check_Type(rb_workdir, T_STRING);

	rugged_exception_check(
		git_repository_set_workdir(repo, StringValueCStr(rb_workdir), 0)
	);

	return Qnil;
}

/*
 *	call-seq:
 *		Repository.discover(path = nil, across_fs = true) -> repository
 *
 *	Traverse +path+ upwards until a Git working directory with a +.git+
 *	folder has been found, open it and return it as a +Repository+
 *	object.
 *
 *	If +path+ is +nil+, the current working directory will be used as
 *	a starting point.
 *
 *	If +across_fs+ is +true+, the traversal won't stop when reaching
 *	a different device than the one that contained +path+ (only applies
 *	to UNIX-based OSses).
 */
static VALUE rb_git_repo_discover(int argc, VALUE *argv, VALUE self)
{
	VALUE rb_path, rb_across_fs;
	char repository_path[GIT_PATH_MAX];
	int error, across_fs = 0;

	rb_scan_args(argc, argv, "02", &rb_path, &rb_across_fs);

	if (NIL_P(rb_path)) {
		VALUE rb_dir = rb_const_get(rb_cObject, rb_intern("Dir"));
		rb_path = rb_funcall(rb_dir, rb_intern("pwd"), 0);
	}

	if (!NIL_P(rb_across_fs)) {
		across_fs = rugged_parse_bool(rb_across_fs);
	}

	Check_Type(rb_path, T_STRING);

	error = git_repository_discover(
		repository_path, GIT_PATH_MAX,
		StringValueCStr(rb_path),
		across_fs,
		NULL
	);

	rugged_exception_check(error);
	return rugged_str_new2(repository_path, NULL);
}

static VALUE flags_to_rb(unsigned int flags)
{
	VALUE rb_flags = rb_ary_new();

	if (flags & GIT_STATUS_INDEX_NEW)
		rb_ary_push(rb_flags, CSTR2SYM("index_new"));

	if (flags & GIT_STATUS_INDEX_MODIFIED)
		rb_ary_push(rb_flags, CSTR2SYM("index_modified"));

	if (flags & GIT_STATUS_INDEX_DELETED)
		rb_ary_push(rb_flags, CSTR2SYM("index_deleted"));

	if (flags & GIT_STATUS_WT_NEW)
		rb_ary_push(rb_flags, CSTR2SYM("worktree_new"));

	if (flags & GIT_STATUS_WT_MODIFIED)
		rb_ary_push(rb_flags, CSTR2SYM("worktree_modified"));

	if (flags & GIT_STATUS_WT_DELETED)
		rb_ary_push(rb_flags, CSTR2SYM("worktree_deleted"));

	return rb_flags;
}

static int rugged__status_cb(const char *path, unsigned int flags, void *payload)
{
	rb_funcall((VALUE)payload, rb_intern("call"), 2,
		rugged_str_new2(path, NULL),
		flags_to_rb(flags)
	);

	return GIT_OK;
}

/*
 *	call-seq:
 *		repo.status { |status_data| block }
 *		repo.status(path) -> status_data
 *
 *	Returns the status for one or more files in the working directory
 *	of the repository. This is equivalent to the +git status+ command.
 *
 *	The returned +status_data+ is always an array containing one or more
 *	status flags as Ruby symbols. Possible flags are:
 *
 *	- +:index_new+: the file is new in the index
 *	- +:index_modified+: the file has been modified in the index
 *	- +:index_deleted+: the file has been deleted from the index
 *	- +:worktree_new+: the file is new in the working directory
 *	- +:worktree_modified+: the file has been modified in the working directory
 *	- +:worktree_deleted+: the file has been deleted from the working directory
 *
 *	If a +block+ is given, status information will be gathered for every
 *	single file on the working dir. The +block+ will be called with the
 *	status data for each file.
 *
 *		repo.status { |status_data| puts status_data.inspect }
 *
 *	results in, for example:
 *
 *		[:index_new, :worktree_new]
 *		[:worktree_modified]
 *
 *	If a +path+ is given instead, the function will return the +status_data+ for
 *	the file pointed to by path, or raise an exception if the path doesn't exist.
 *
 *	+path+ must be relative to the repository's working directory.
 *
 *		repo.status('src/diff.c') #=> [:index_new, :worktree_new]
 */
static VALUE rb_git_repo_status(int argc, VALUE *argv, VALUE self)
{
	int error;
	VALUE rb_path;
	git_repository *repo;

	Data_Get_Struct(self, git_repository, repo);

	if (rb_scan_args(argc, argv, "01", &rb_path) == 1) {
		unsigned int flags;
		Check_Type(rb_path, T_STRING);
		error = git_status_file(&flags, repo, StringValueCStr(rb_path));
		rugged_exception_check(error);

		return flags_to_rb(flags);
	}

	if (!rb_block_given_p())
		rb_raise(rb_eRuntimeError,
			"A block was expected for iterating through "
			"the repository contents.");

	error = git_status_foreach(
		repo,
		&rugged__status_cb,
		(void *)rb_block_proc()
	);

	rugged_exception_check(error);
	return Qnil;
}

void Init_rugged_repo()
{
	rb_cRuggedRepo = rb_define_class_under(rb_mRugged, "Repository", rb_cObject);

	rb_define_singleton_method(rb_cRuggedRepo, "new", rb_git_repo_new, 1);
	rb_define_singleton_method(rb_cRuggedRepo, "hash",   rb_git_repo_hash,  2);
	rb_define_singleton_method(rb_cRuggedRepo, "hash_file",   rb_git_repo_hashfile,  2);
	rb_define_singleton_method(rb_cRuggedRepo, "init_at", rb_git_repo_init_at, 2);
	rb_define_singleton_method(rb_cRuggedRepo, "discover", rb_git_repo_discover, -1);

	rb_define_method(rb_cRuggedRepo, "exists?", rb_git_repo_exists, 1);
	rb_define_method(rb_cRuggedRepo, "include?", rb_git_repo_exists, 1);

	rb_define_method(rb_cRuggedRepo, "read",   rb_git_repo_read,   1);
	rb_define_method(rb_cRuggedRepo, "write",  rb_git_repo_write,  2);
	rb_define_method(rb_cRuggedRepo, "path",  rb_git_repo_path, 0);
	rb_define_method(rb_cRuggedRepo, "workdir",  rb_git_repo_workdir, 0);
	rb_define_method(rb_cRuggedRepo, "workdir=",  rb_git_repo_set_workdir, 1);
	rb_define_method(rb_cRuggedRepo, "status",  rb_git_repo_status,  -1);

	rb_define_method(rb_cRuggedRepo, "index",  rb_git_repo_get_index,  0);
	rb_define_method(rb_cRuggedRepo, "index=",  rb_git_repo_set_index,  1);
	rb_define_method(rb_cRuggedRepo, "config",  rb_git_repo_get_config,  0);
	rb_define_method(rb_cRuggedRepo, "config=",  rb_git_repo_set_config,  1);

	rb_define_method(rb_cRuggedRepo, "bare?",  rb_git_repo_is_bare,  0);
	rb_define_method(rb_cRuggedRepo, "empty?",  rb_git_repo_is_empty,  0);

	rb_define_method(rb_cRuggedRepo, "head_detached?",  rb_git_repo_head_detached,  0);
	rb_define_method(rb_cRuggedRepo, "head_orphan?",  rb_git_repo_head_orphan,  0);

	rb_cRuggedOdbObject = rb_define_class_under(rb_mRugged, "OdbObject", rb_cObject);
	rb_define_method(rb_cRuggedOdbObject, "data",  rb_git_odbobj_data,  0);
	rb_define_method(rb_cRuggedOdbObject, "len",  rb_git_odbobj_size,  0);
	rb_define_method(rb_cRuggedOdbObject, "type",  rb_git_odbobj_type,  0);
	rb_define_method(rb_cRuggedOdbObject, "oid",  rb_git_odbobj_oid,  0);
}
