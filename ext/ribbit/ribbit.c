#include "ruby.h"
#include <git/commit.h>
#include <git/common.h>
#include <git/errors.h>
#include <git/index.h>
#include <git/odb.h>
#include <git/oid.h>
#include <git/revwalk.h>
#include <git/repository.h>
#include <git/zlib.h>

static VALUE rb_git_createobject(git_repository_object *object);

static VALUE rb_cRibbit;

/*
 * Ribbit Lib
 */

static VALUE rb_cRibbitLib;

static VALUE rb_git_hex_to_raw(VALUE self, VALUE hex)
{
	git_oid oid;

	Check_Type(hex, T_STRING);
	git_oid_mkstr(&oid, RSTRING_PTR(hex));
	return rb_str_new((&oid)->id, 20);
}

static VALUE rb_git_raw_to_hex(VALUE self, VALUE raw)
{
	git_oid oid;
	char out[40];

	Check_Type(raw, T_STRING);
	git_oid_mkraw(&oid, RSTRING_PTR(raw));
	git_oid_fmt(out, &oid);
	return rb_str_new(out, 40);
}

static VALUE rb_git_type_to_string(VALUE self, VALUE type)
{
	const char *str;

	Check_Type(type, T_FIXNUM);
	str = git_obj_type_to_string(type);
	return str ? rb_str_new2(str) : Qfalse;
}

static VALUE rb_git_string_to_type(VALUE self, VALUE string_type)
{
	Check_Type(string_type, T_STRING);
	return INT2FIX(git_obj_string_to_type(RSTRING_PTR(string_type)));
}


/*
 * Ribbit Object Database
 */

static VALUE rb_cRibbitRepo;

static VALUE rb_git_repo_allocate(VALUE klass)
{
	git_repository *repo = NULL;
	return Data_Wrap_Struct(klass, NULL, NULL, repo);
}

static VALUE rb_git_repo_init(VALUE self, VALUE path)
{
	git_odb *odb;
	git_repository *repo;

	git_odb_open(&odb, RSTRING_PTR(path));
	repo = git_repository_alloc(odb);

	if (repo == NULL)
		rb_raise(rb_eRuntimeError, "Failed to allocate new Git repository");

	DATA_PTR(self) = repo;
	return Qnil;
}

static VALUE rb_git_repo_exists(VALUE self, VALUE hex)
{
	git_repository *repo;
	git_odb *odb;
	git_oid oid;

	Data_Get_Struct(self, git_repository, repo);

	odb = git_repository_database(repo);
	git_oid_mkstr(&oid, RSTRING_PTR(hex));

	return git_odb_exists(odb, &oid) ? Qtrue : Qfalse;
}

static VALUE rb_git_repo_read(VALUE self, VALUE hex)
{
	git_repository *repo;
	git_odb *odb;
	git_oid oid;
	git_obj obj;

	Data_Get_Struct(self, git_repository, repo);
	odb = git_repository_database(repo);

	git_oid_mkstr(&oid, RSTRING_PTR(hex));

	if (git_odb_read(&obj, odb, &oid) == GIT_SUCCESS) {
		VALUE ret_arr = rb_ary_new();

		unsigned char *data = (&obj)->data;
		rb_ary_store(ret_arr, 0, rb_str_new2(data));

		rb_ary_store(ret_arr, 1, INT2FIX((int)(&obj)->len));

		const char *str_type;
		git_otype git_type = (&obj)->type;
		str_type = git_obj_type_to_string(git_type);
		rb_ary_store(ret_arr, 2, rb_str_new2(str_type));
		return ret_arr;
	}

	return Qfalse;
}

static VALUE rb_git_repo_obj_hash(VALUE self, VALUE content, VALUE type)
{
	git_obj obj;
	git_oid oid;

	(&obj)->data = RSTRING_PTR(content);
	(&obj)->len  = RSTRING_LEN(content);
	(&obj)->type = git_obj_string_to_type(RSTRING_PTR(type));

	if (git_obj_hash(&oid, &obj) == GIT_SUCCESS) {
		char out[40];
		git_oid_fmt(out, &oid);
		return rb_str_new(out, 40);
	}

	return Qfalse;
}

static VALUE rb_git_repo_write(VALUE self, VALUE content, VALUE type)
{
	git_repository *repo;
	git_odb *odb;
	git_obj obj;
	git_oid oid;

	Data_Get_Struct(self, git_repository, repo);
	odb = git_repository_database(repo);

	(&obj)->data = RSTRING_PTR(content);
	(&obj)->len  = RSTRING_LEN(content);
	(&obj)->type = git_obj_string_to_type(RSTRING_PTR(type));

	git_obj_hash(&oid, &obj);

	if (git_odb_write(&oid, odb, &obj) == GIT_SUCCESS) {
		char out[40];
		git_oid_fmt(out, &oid);
		return rb_str_new(out, 40);
	}

	return Qfalse;
}

static VALUE rb_git_repo_lookup(int argc, VALUE *argv, VALUE self)
{
	git_repository *repo;
	git_otype type;
	git_repository_object *obj;
	git_oid oid;

	VALUE rb_type, rb_sha;

	Data_Get_Struct(self, git_repository, repo);

	rb_scan_args(argc, argv, "11", &rb_sha, &rb_type);

	type = NIL_P(rb_type) ? GIT_OBJ_ANY : FIX2INT(rb_type);
	git_oid_mkstr(&oid, RSTRING_PTR(rb_sha));

	obj = git_repository_lookup(repo, &oid, type);

	return obj ? rb_git_createobject(obj) : Qnil;
}

/*
 * Ribbit Object
 */ 

static VALUE rb_cRibbitObject;
static VALUE rb_cRibbitCommit;

static VALUE rb_git_createobject(git_repository_object *object)
{
	git_otype type;
	char sha1[40];
	VALUE obj, klass;

	type = git_repository_object_type(object);

	switch (type)
	{
		case GIT_OBJ_COMMIT:
			klass = rb_cRibbitCommit;
			break;

		case GIT_OBJ_TAG:
		case GIT_OBJ_TREE:
		case GIT_OBJ_BLOB:
			klass = rb_cRibbitObject;
			break;

		default:
			return Qnil;
	}

	obj = Data_Wrap_Struct(klass, NULL, NULL, object);

	git_oid_fmt(sha1, git_repository_object_id(object));

	rb_iv_set(obj, "@type", rb_str_new2(git_obj_type_to_string(type)));
	rb_iv_set(obj, "@sha", rb_str_new(sha1, 40));

	return obj;
}

static VALUE rb_git_object_allocate(VALUE klass)
{
	git_repository_object *object = NULL;
	return Data_Wrap_Struct(klass, NULL, NULL, object);
}

static VALUE rb_git_object_init(VALUE self, VALUE rb_repo, VALUE hex, VALUE type)
{
	rb_iv_set(self, "@sha", hex);
	rb_iv_set(self, "repo", rb_repo);
	rb_iv_set(self, "@type", type);
}

static VALUE rb_git_object_read_raw(VALUE self)
{
	return rb_git_repo_read(rb_iv_get(self, "repo"), rb_iv_get(self, "@sha"));
}

/*
 * Ribbit Commit
 */ 

static VALUE rb_git_commit_allocate(VALUE klass)
{
	git_commit *commit = NULL;
	return Data_Wrap_Struct(klass, NULL, NULL, commit);
}

static VALUE rb_git_commit_init(int argc, VALUE *argv, VALUE self)
{
	git_repository *repo;
	git_commit *commit;
	VALUE rb_repo, hex;

	rb_scan_args(argc, argv, "11", &rb_repo, &hex);
	rb_git_object_init(self, rb_repo, hex, rb_str_new2("commit"));
	Data_Get_Struct(rb_repo, git_repository, repo);

	if (NIL_P(hex)) {

		/* TODO; create new commit object */
	
	} else {
		git_oid oid;

		git_oid_mkstr(&oid, RSTRING_PTR(hex));
		commit = git_commit_lookup(repo, &oid);

		if (commit == NULL)
			rb_raise(rb_eRuntimeError, "Commit not found");
	}

	DATA_PTR(self) = commit;
}

static VALUE rb_git_commit_message(VALUE self)
{
	git_commit *commit;
	const char *str_type;

	Data_Get_Struct(self, git_commit, commit);
	str_type = git_commit_message(commit);

	return rb_str_new2(str_type);
}

static VALUE rb_git_commit_message_short(VALUE self)
{
	git_commit *commit;
	const char *str_type;

	Data_Get_Struct(self, git_commit, commit);
	str_type = git_commit_message_short(commit);

	return rb_str_new2(str_type);
}

static VALUE rb_git_person2hash(const git_person *person)
{
	VALUE rb_person;

	if (person == NULL)
		return Qnil;

	rb_person = rb_hash_new();
	rb_hash_aset(rb_person, rb_str_new2("name"), rb_str_new2(person->name));
	rb_hash_aset(rb_person, rb_str_new2("email"), rb_str_new2(person->email));
	rb_hash_aset(rb_person, rb_str_new2("time"), ULONG2NUM(person->time));

	return rb_person;
}

static VALUE rb_git_commit_committer(VALUE self)
{
	git_commit *commit;
	const git_person *person;

	Data_Get_Struct(self, git_commit, commit);
	person = git_commit_committer(commit);

	return rb_git_person2hash(person);
}

static VALUE rb_git_commit_author(VALUE self)
{
	git_commit *commit;
	const git_person *person;

	Data_Get_Struct(self, git_commit, commit);
	person = git_commit_author(commit);

	return rb_git_person2hash(person);
}

static VALUE rb_git_commit_time(VALUE self)
{
	git_commit *commit;
	Data_Get_Struct(self, git_commit, commit);

	return ULONG2NUM(git_commit_time(commit));
}

static VALUE rb_git_commit_tree(VALUE self)
{
	git_commit *commit;
	const git_tree *tree;
	Data_Get_Struct(self, git_commit, commit);

	tree = git_commit_tree(commit);

	return tree ? rb_git_createobject((git_repository_object *)tree) : Qnil;
}


/*
 * Ribbit Revwalking
 */ 

static VALUE rb_cRibbitWalker;

static VALUE rb_git_walker_allocate(VALUE klass)
{
	git_revwalk *walker = NULL;
	return Data_Wrap_Struct(klass, NULL, NULL, walker);
}

static VALUE rb_git_walker_init(VALUE self, VALUE rb_repo)
{
	git_repository *repo;
	Data_Get_Struct(rb_repo, git_repository, repo);
	DATA_PTR(self) = git_revwalk_alloc(repo);
	rb_iv_set(self, "repo", rb_repo);
	return Qnil;
}


static VALUE rb_git_walker__push(VALUE self, VALUE rb_commit, int hide)
{
	git_revwalk *walk;
	git_commit *commit;

	Data_Get_Struct(self, git_revwalk, walk);

	if (TYPE(rb_commit) == T_STRING) {
		git_repository *repo;
		git_oid oid;

		Data_Get_Struct(rb_iv_get(self, "repo"), git_repository, repo);

		git_oid_mkstr(&oid, RSTRING_PTR(rb_commit));
		commit = git_commit_lookup(repo, &oid);

	} else if (TYPE(rb_commit) == T_DATA) {
		Data_Get_Struct(rb_commit, git_commit, commit);
	}

	if (commit == NULL)
		rb_raise(rb_eRuntimeError, "Invalid commit");

	if (hide)
		git_revwalk_hide(walk, commit);
	else
		git_revwalk_push(walk, commit);

	return Qnil;
}

static VALUE rb_git_walker_next(VALUE self)
{
	git_revwalk *walk;
	git_commit *commit;

	Data_Get_Struct(self, git_revwalk, walk);
	commit = git_revwalk_next(walk);

	return commit ? rb_git_createobject((git_repository_object*)commit) : Qfalse;
}

static VALUE rb_git_walker_push(VALUE self, VALUE rb_commit)
{
	return rb_git_walker__push(self, rb_commit, 0);
}

static VALUE rb_git_walker_hide(VALUE self, VALUE rb_commit)
{
	return rb_git_walker__push(self, rb_commit, 1);
}

static VALUE rb_git_walker_sorting(VALUE self, VALUE ruby_sort_mode)
{
	git_revwalk *walk;
	Data_Get_Struct(self, git_revwalk, walk);
	git_revwalk_sorting(walk, FIX2INT(ruby_sort_mode));
	return Qnil;
}

static VALUE rb_git_walker_reset(VALUE self)
{
	git_revwalk *walk;
	Data_Get_Struct(self, git_revwalk, walk);
	git_revwalk_reset(walk);
	return Qnil;
}

/*
 * Ribbit Init Call
 */

void Init_ribbit()
{
	rb_cRibbit = rb_define_class("Ribbit", rb_cObject);
	rb_cRibbitLib = rb_define_class_under(rb_cRibbit, "Lib", rb_cObject);

	rb_define_module_function(rb_cRibbitLib, "hex_to_raw", rb_git_hex_to_raw, 1);
	rb_define_module_function(rb_cRibbitLib, "raw_to_hex", rb_git_raw_to_hex, 1);
	rb_define_module_function(rb_cRibbitLib, "type_to_string", rb_git_type_to_string, 1);
	rb_define_module_function(rb_cRibbitLib, "string_to_type", rb_git_string_to_type, 1);

	rb_cRibbitRepo = rb_define_class_under(rb_cRibbit, "Repository", rb_cObject);
	rb_define_alloc_func(rb_cRibbitRepo, rb_git_repo_allocate);
	rb_define_method(rb_cRibbitRepo, "initialize", rb_git_repo_init, 1);
	rb_define_method(rb_cRibbitRepo, "exists", rb_git_repo_exists, 1);
	rb_define_method(rb_cRibbitRepo, "hash",   rb_git_repo_obj_hash,  2);
	rb_define_method(rb_cRibbitRepo, "read",   rb_git_repo_read,   1);
	rb_define_method(rb_cRibbitRepo, "write",  rb_git_repo_write,  2);
	rb_define_method(rb_cRibbitRepo, "lookup",  rb_git_repo_lookup,  -1);

	rb_cRibbitObject = rb_define_class_under(rb_cRibbit, "Object", rb_cObject);
	rb_define_alloc_func(rb_cRibbitObject, rb_git_object_allocate);
	rb_define_method(rb_cRibbitObject, "read_raw", rb_git_object_read_raw, 0);
	rb_attr(rb_cRibbitObject, rb_intern("sha"), Qtrue, Qtrue, Qtrue);
	rb_attr(rb_cRibbitObject, rb_intern("type"), Qtrue, Qfalse, Qtrue);

	rb_cRibbitCommit = rb_define_class_under(rb_cRibbit, "Commit", rb_cRibbitObject);
	rb_define_alloc_func(rb_cRibbitCommit, rb_git_commit_allocate);
	rb_define_method(rb_cRibbitCommit, "initialize", rb_git_commit_init, -1);
	rb_define_method(rb_cRibbitCommit, "message", rb_git_commit_message, 0);
	rb_define_method(rb_cRibbitCommit, "message_short", rb_git_commit_message_short, 0);
	rb_define_method(rb_cRibbitCommit, "time", rb_git_commit_time, 0);
	rb_define_method(rb_cRibbitCommit, "committer", rb_git_commit_committer, 0);
	rb_define_method(rb_cRibbitCommit, "author", rb_git_commit_author, 0);
	rb_define_method(rb_cRibbitCommit, "tree", rb_git_commit_tree, 0);

	rb_cRibbitWalker = rb_define_class_under(rb_cRibbit, "Walker", rb_cObject);
	rb_define_alloc_func(rb_cRibbitWalker, rb_git_walker_allocate);
	rb_define_method(rb_cRibbitWalker, "initialize", rb_git_walker_init, 1);
	rb_define_method(rb_cRibbitWalker, "push", rb_git_walker_push, 1);
	rb_define_method(rb_cRibbitWalker, "next", rb_git_walker_next, 0);
	rb_define_method(rb_cRibbitWalker, "hide", rb_git_walker_hide, 1);
	rb_define_method(rb_cRibbitWalker, "reset", rb_git_walker_reset, 0);
	rb_define_method(rb_cRibbitWalker, "sorting", rb_git_walker_sorting, 1);

	rb_define_const(rb_cRibbit, "SORT_NONE", INT2FIX(0));
	rb_define_const(rb_cRibbit, "SORT_TOPO", INT2FIX(1));
	rb_define_const(rb_cRibbit, "SORT_DATE", INT2FIX(2));
	rb_define_const(rb_cRibbit, "SORT_REVERSE", INT2FIX(4));

	rb_define_const(rb_cRibbit, "OBJ_ANY", INT2FIX(GIT_OBJ_ANY));
	rb_define_const(rb_cRibbit, "OBJ_BAD", INT2FIX(GIT_OBJ_BAD));
	rb_define_const(rb_cRibbit, "OBJ_COMMIT", INT2FIX(GIT_OBJ_COMMIT));
	rb_define_const(rb_cRibbit, "OBJ_TREE", INT2FIX(GIT_OBJ_TREE));
	rb_define_const(rb_cRibbit, "OBJ_BLOB", INT2FIX(GIT_OBJ_BLOB));
	rb_define_const(rb_cRibbit, "OBJ_TAG", INT2FIX(GIT_OBJ_TAG));
}

