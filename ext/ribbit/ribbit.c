#include "ruby.h"
#include <assert.h>

#include <git/commit.h>
#include <git/tag.h>
#include <git/common.h>
#include <git/errors.h>
#include <git/index.h>
#include <git/odb.h>
#include <git/oid.h>
#include <git/revwalk.h>
#include <git/repository.h>
#include <git/zlib.h>

static VALUE rb_git_createobject(git_object *object);

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
	git_rawobj obj;

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
	git_rawobj obj;
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
	git_rawobj obj;
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
	git_object *obj;
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
static VALUE rb_cRibbitTag;
static VALUE rb_cRibbitTree;
static VALUE rb_cRibbitTreeEntry;

static git_object *rb_git_get_C_object(VALUE parent, VALUE object_value, git_otype type)
{
	git_object *object = NULL;

	if (TYPE(object_value) == T_STRING) {
		git_repository *repo;
		git_oid oid;

		Data_Get_Struct(rb_iv_get(parent, "repo"), git_repository, repo);

		git_oid_mkstr(&oid, RSTRING_PTR(object_value));
		object = git_repository_lookup(repo, &oid, type);

		if (object == NULL)
			rb_raise(rb_eRuntimeError, "Failed to lookup object in the repository");

	} else if (rb_obj_is_kind_of(object_value, rb_cRibbitObject)) {
		Data_Get_Struct(object_value, git_object, object);

		if (type != GIT_OBJ_ANY && git_object_type(object) != type)
			rb_raise(rb_eRuntimeError, "Object is not of the required type");
	} else {
		rb_raise(rb_eTypeError, "Invalid GIT object; an object reference must be a SHA1 id or an object itself");
	}

	assert(object);
	return object;
}

static VALUE rb_git_createobject(git_object *object)
{
	git_otype type;
	char sha1[40];
	VALUE obj, klass;

	type = git_object_type(object);

	switch (type)
	{
		case GIT_OBJ_COMMIT:
			klass = rb_cRibbitCommit;
			break;

		case GIT_OBJ_TAG:
			klass = rb_cRibbitTag;
			break;

		case GIT_OBJ_TREE:
			klass = rb_cRibbitTree;
			break;

		case GIT_OBJ_BLOB:
		default:
			klass = rb_cRibbitObject;
			break;
	}

	obj = Data_Wrap_Struct(klass, NULL, NULL, object);

	git_oid_fmt(sha1, git_object_id(object));

	rb_iv_set(obj, "@type", rb_str_new2(git_obj_type_to_string(type)));
	rb_iv_set(obj, "@sha", rb_str_new(sha1, 40));

	return obj;
}

static VALUE rb_git_object_allocate(VALUE klass)
{
	git_object *object = NULL;
	return Data_Wrap_Struct(klass, NULL, NULL, object);
}

static VALUE rb_git_object_equal(VALUE self, VALUE other)
{
	git_object *a, *b;

	if (!rb_obj_is_kind_of(other, rb_cRibbitObject))
		return Qfalse;

	Data_Get_Struct(self, git_object, a);
	Data_Get_Struct(other, git_object, b);

	return git_oid_cmp(git_object_id(a), git_object_id(b)) == 0 ? Qtrue : Qfalse;
}

static VALUE rb_git_object_sha_GET(VALUE self)
{
	git_object *object;
	char hex[40];

	Data_Get_Struct(self, git_object, object);

	git_oid_fmt(hex, git_object_id(object));
	return rb_str_new(hex, 40);
}

static VALUE rb_git_object_type_GET(VALUE self)
{
	git_object *object;
	Data_Get_Struct(self, git_object, object);

	return rb_str_new2(git_obj_type_to_string(git_object_type(object)));
}

static VALUE rb_git_object_init(git_otype type, int argc, VALUE *argv, VALUE self)
{
	git_repository *repo;
	git_object *object;
	VALUE rb_repo, hex;

	rb_scan_args(argc, argv, "11", &rb_repo, &hex);
	Data_Get_Struct(rb_repo, git_repository, repo);

	rb_iv_set(self, "repo", rb_repo);

	if (NIL_P(hex)) {
		object = git_object_new(repo, type);

		if (object == NULL)
			rb_raise(rb_eRuntimeError, "Failed to instantiate new in-memory object");

	} else {
		git_oid oid;

		git_oid_mkstr(&oid, RSTRING_PTR(hex));
		object = git_repository_lookup(repo, &oid, type);

		if (object == NULL)
			rb_raise(rb_eRuntimeError, "Object not found");
	}

	DATA_PTR(self) = object;
	return Qnil;
}

static VALUE rb_git_object_read_raw(VALUE self)
{
	return rb_git_repo_read(rb_iv_get(self, "repo"), rb_iv_get(self, "@sha"));
}

static VALUE rb_git_object_write(VALUE self)
{
	git_object *object;
	char new_hex[40];
	VALUE sha;

	Data_Get_Struct(self, git_object, object);

	if (git_object_write(object) < 0)
		rb_raise(rb_eRuntimeError, "failed to write object to repository");

	git_oid_fmt(new_hex, git_object_id(object));
	sha = rb_str_new(new_hex, 40);

	return sha;
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
	return rb_git_object_init(GIT_OBJ_COMMIT, argc, argv, self);
}

static VALUE rb_git_commit_message_GET(VALUE self)
{
	git_commit *commit;
	Data_Get_Struct(self, git_commit, commit);

	return rb_str_new2(git_commit_message(commit));
}

static VALUE rb_git_commit_message_SET(VALUE self, VALUE val)
{
	git_commit *commit;
	Data_Get_Struct(self, git_commit, commit);

	Check_Type(val, T_STRING);
	git_commit_set_message(commit, RSTRING_PTR(val));
	return Qnil;
}

static VALUE rb_git_commit_message_short_GET(VALUE self)
{
	git_commit *commit;
	Data_Get_Struct(self, git_commit, commit);

	return rb_str_new2(git_commit_message_short(commit));
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

static VALUE rb_git_args2person(git_person *person, VALUE rb_name, VALUE rb_email, VALUE rb_time)
{
	Check_Type(rb_name, T_STRING);
	Check_Type(rb_email, T_STRING);
	Check_Type(rb_time, T_FIXNUM);

	memset(person, 0x0, sizeof(git_person));

	strncpy(person->name, RSTRING_PTR(rb_name), 63);
	strncpy(person->email, RSTRING_PTR(rb_email), 63);
	person->time = FIX2INT(rb_time);

	return Qnil;
}

static VALUE rb_git_commit_committer_GET(VALUE self)
{
	git_commit *commit;
	Data_Get_Struct(self, git_commit, commit);

	return rb_git_person2hash(git_commit_committer(commit));
}

static VALUE rb_git_commit_committer_SET(VALUE self, VALUE rb_name, VALUE rb_email, VALUE rb_time)
{
	git_commit *commit;
	git_person new_person;
	Data_Get_Struct(self, git_commit, commit);

	rb_git_args2person(&new_person, rb_name, rb_email, rb_time);
	git_commit_set_committer(commit, &new_person);
	return Qnil;
}

static VALUE rb_git_commit_author_GET(VALUE self)
{
	git_commit *commit;
	Data_Get_Struct(self, git_commit, commit);

	return rb_git_person2hash(git_commit_author(commit));
}

static VALUE rb_git_commit_author_SET(VALUE self, VALUE rb_name, VALUE rb_email, VALUE rb_time)
{
	git_commit *commit;
	git_person new_person;
	Data_Get_Struct(self, git_commit, commit);

	rb_git_args2person(&new_person, rb_name, rb_email, rb_time);
	git_commit_set_author(commit, &new_person);
	return Qnil;
}


static VALUE rb_git_commit_time_GET(VALUE self)
{
	git_commit *commit;
	Data_Get_Struct(self, git_commit, commit);

	return ULONG2NUM(git_commit_time(commit));
}

static VALUE rb_git_commit_tree_GET(VALUE self)
{
	git_commit *commit;
	const git_tree *tree;
	Data_Get_Struct(self, git_commit, commit);

	tree = git_commit_tree(commit);
	return tree ? rb_git_createobject((git_object *)tree) : Qnil;
}

static VALUE rb_git_commit_tree_SET(VALUE self, VALUE val)
{
	git_commit *commit;
	git_tree *tree;
	Data_Get_Struct(self, git_commit, commit);

	tree = (git_tree *)rb_git_get_C_object(self, val, GIT_OBJ_TREE);
	git_commit_set_tree(commit, tree);
	return Qnil;
}


/*
 * Ribbit Tag
 */

static VALUE rb_git_tag_allocate(VALUE klass)
{
	git_tag *tag = NULL;
	return Data_Wrap_Struct(klass, NULL, NULL, tag);
}

static VALUE rb_git_tag_init(int argc, VALUE *argv, VALUE self)
{
	return rb_git_object_init(GIT_OBJ_TAG, argc, argv, self);
}

static VALUE rb_git_tag_target_GET(VALUE self)
{
	git_tag *tag;
	Data_Get_Struct(self, git_tag, tag);

	return rb_git_createobject((git_object*)git_tag_target(tag));
}

static VALUE rb_git_tag_target_SET(VALUE self, VALUE val)
{
	git_tag *tag;
	git_object *target;
	Data_Get_Struct(self, git_tag, tag);

	target = rb_git_get_C_object(self, val, GIT_OBJ_ANY);
	git_tag_set_target(tag, target);
	return Qnil;
}

static VALUE rb_git_tag_target_type_GET(VALUE self)
{
	git_tag *tag;
	Data_Get_Struct(self, git_tag, tag);

	return rb_str_new2(git_obj_type_to_string(git_tag_type(tag)));
}

static VALUE rb_git_tag_name_GET(VALUE self)
{
	git_tag *tag;
	Data_Get_Struct(self, git_tag, tag);

	return rb_str_new2(git_tag_name(tag));
}

static VALUE rb_git_tag_name_SET(VALUE self, VALUE val)
{
	git_tag *tag;
	Data_Get_Struct(self, git_tag, tag);

	Check_Type(val, T_STRING);
	git_tag_set_name(tag, RSTRING_PTR(val));
	return Qnil;
}

static VALUE rb_git_tag_tagger_GET(VALUE self)
{
	git_tag *tag;
	const git_person *person;
	Data_Get_Struct(self, git_tag, tag);

	person = git_tag_tagger(tag);
	return rb_git_person2hash(person);
}

static VALUE rb_git_tag_tagger_SET(VALUE self, VALUE rb_name, VALUE rb_email, VALUE rb_time)
{
	git_tag *tag;
	git_person new_person;
	Data_Get_Struct(self, git_tag, tag);

	rb_git_args2person(&new_person, rb_name, rb_email, rb_time);
	git_tag_set_tagger(tag, &new_person);
	return Qnil;
}

static VALUE rb_git_tag_message_GET(VALUE self)
{
	git_tag *tag;
	Data_Get_Struct(self, git_tag, tag);

	return rb_str_new2(git_tag_message(tag));
}

static VALUE rb_git_tag_message_SET(VALUE self, VALUE val)
{
	git_tag *tag;
	Data_Get_Struct(self, git_tag, tag);

	Check_Type(val, T_STRING);
	git_tag_set_message(tag, RSTRING_PTR(val));
	return Qnil;
}


/*
 * Ribbit Tree
 */
static VALUE rb_git_treeentry_allocate(VALUE klass)
{
	git_tree_entry *tree_entry = NULL;
	return Data_Wrap_Struct(klass, NULL, NULL, tree_entry);
}

static VALUE rb_git_tree_entry_init(int argc, VALUE *argv, VALUE self)
{
	return Qnil;
}

static VALUE rb_git_createentry(git_tree_entry *entry)
{
	VALUE obj;

	if (entry == NULL)
		return Qnil;

	obj = Data_Wrap_Struct(rb_cRibbitTreeEntry, NULL, NULL, entry);
	/* TODO: attributes? */

	return obj;
}

static VALUE rb_git_tree_entry_attributes_GET(VALUE self)
{
	git_tree_entry *tree_entry;
	Data_Get_Struct(self, git_tree_entry, tree_entry);

	return INT2FIX(git_tree_entry_attributes(tree_entry));
}

static VALUE rb_git_tree_entry_attributes_SET(VALUE self, VALUE val)
{
	git_tree_entry *tree_entry;
	Data_Get_Struct(self, git_tree_entry, tree_entry);

	Check_Type(val, T_FIXNUM);
	git_tree_entry_set_attributes(tree_entry, FIX2INT(val));
	return Qnil;
}

static VALUE rb_git_tree_entry_name_GET(VALUE self)
{
	git_tree_entry *tree_entry;
	Data_Get_Struct(self, git_tree_entry, tree_entry);

	return rb_str_new2(git_tree_entry_name(tree_entry));
}

static VALUE rb_git_tree_entry_name_SET(VALUE self, VALUE val)
{
	git_tree_entry *tree_entry;
	Data_Get_Struct(self, git_tree_entry, tree_entry);

	Check_Type(val, T_STRING);
	git_tree_entry_set_name(tree_entry, RSTRING_PTR(val));
	return Qnil;
}

static VALUE rb_git_tree_entry_sha_GET(VALUE self)
{
	git_tree_entry *tree_entry;
	char out[40];
	Data_Get_Struct(self, git_tree_entry, tree_entry);

	git_oid_fmt(out, git_tree_entry_id(tree_entry));
	return rb_str_new(out, 40);
}

static VALUE rb_git_tree_entry_sha_SET(VALUE self, VALUE val)
{
	git_tree_entry *tree_entry;
	git_oid id;
	Data_Get_Struct(self, git_tree_entry, tree_entry);

	Check_Type(val, T_STRING);
	if (git_oid_mkstr(&id, RSTRING_PTR(val)) < 0)
		rb_raise(rb_eTypeError, "Invalid SHA1 value");
	git_tree_entry_set_id(tree_entry, &id);
	return Qnil;
}

static VALUE rb_git_tree_entry_2object(VALUE self)
{
	git_tree_entry *tree_entry;
	Data_Get_Struct(self, git_tree_entry, tree_entry);

	return rb_git_createobject(git_tree_entry_2object(tree_entry));
}



/*
 * Ribbit Tree
 */

static VALUE rb_git_tree_allocate(VALUE klass)
{
	git_tree *tree = NULL;
	return Data_Wrap_Struct(klass, NULL, NULL, tree);
}

static VALUE rb_git_tree_init(int argc, VALUE *argv, VALUE self)
{
	return rb_git_object_init(GIT_OBJ_TREE, argc, argv, self);
}

static VALUE rb_git_tree_entrycount(VALUE self)
{
	git_tree *tree;
	Data_Get_Struct(self, git_tree, tree);

	return INT2FIX(git_tree_entrycount(tree));
}

static VALUE rb_git_tree_get_entry(VALUE self, VALUE entry_id)
{
	git_tree *tree;
	Data_Get_Struct(self, git_tree, tree);

	if (TYPE(entry_id) == T_FIXNUM)
		return rb_git_createentry((git_tree_entry *)git_tree_entry_byindex(tree, FIX2INT(entry_id)));

	else if (TYPE(entry_id) == T_STRING)
		return rb_git_createentry((git_tree_entry *)git_tree_entry_byname(tree, RSTRING_PTR(entry_id)));

	else
		rb_raise(rb_eTypeError, "entry_id must be either an index or a filename");
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


static VALUE rb_git_walker_next(VALUE self)
{
	git_revwalk *walk;
	git_commit *commit;

	Data_Get_Struct(self, git_revwalk, walk);
	commit = git_revwalk_next(walk);

	return commit ? rb_git_createobject((git_object*)commit) : Qfalse;
}

static VALUE rb_git_walker_push(VALUE self, VALUE rb_commit)
{
	git_revwalk *walk;
	git_commit *commit;

	Data_Get_Struct(self, git_revwalk, walk);
	commit = (git_commit *)rb_git_get_C_object(self, rb_commit, GIT_OBJ_COMMIT);
	git_revwalk_push(walk, commit);

	return Qnil;
}

static VALUE rb_git_walker_hide(VALUE self, VALUE rb_commit)
{
	git_revwalk *walk;
	git_commit *commit;

	Data_Get_Struct(self, git_revwalk, walk);
	commit = (git_commit *)rb_git_get_C_object(self, rb_commit, GIT_OBJ_COMMIT);
	git_revwalk_hide(walk, commit);

	return Qnil;
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

	/*
	 * Repository
	 */
	rb_cRibbitRepo = rb_define_class_under(rb_cRibbit, "Repository", rb_cObject);
	rb_define_alloc_func(rb_cRibbitRepo, rb_git_repo_allocate);
	rb_define_method(rb_cRibbitRepo, "initialize", rb_git_repo_init, 1);
	rb_define_method(rb_cRibbitRepo, "exists", rb_git_repo_exists, 1);
	rb_define_method(rb_cRibbitRepo, "hash",   rb_git_repo_obj_hash,  2);
	rb_define_method(rb_cRibbitRepo, "read",   rb_git_repo_read,   1);
	rb_define_method(rb_cRibbitRepo, "write",  rb_git_repo_write,  2);
	rb_define_method(rb_cRibbitRepo, "lookup",  rb_git_repo_lookup,  -1);


	/*
	 * Object
	 */
	rb_cRibbitObject = rb_define_class_under(rb_cRibbit, "Object", rb_cObject);
	rb_define_alloc_func(rb_cRibbitObject, rb_git_object_allocate);
	rb_define_method(rb_cRibbitObject, "read_raw", rb_git_object_read_raw, 0);
	rb_define_method(rb_cRibbitObject, "write", rb_git_object_write, 0);
	rb_define_method(rb_cRibbitObject, "==", rb_git_object_equal, 1);
	rb_define_method(rb_cRibbitObject, "sha", rb_git_object_sha_GET, 0);
	rb_define_method(rb_cRibbitObject, "type", rb_git_object_type_GET, 0);


	/*
	 * Commit
	 */
	rb_cRibbitCommit = rb_define_class_under(rb_cRibbit, "Commit", rb_cRibbitObject);
	rb_define_alloc_func(rb_cRibbitCommit, rb_git_commit_allocate);
	rb_define_method(rb_cRibbitCommit, "initialize", rb_git_commit_init, -1);

	rb_define_method(rb_cRibbitCommit, "message", rb_git_commit_message_GET, 0);
	rb_define_method(rb_cRibbitCommit, "message=", rb_git_commit_message_SET, 1);

	/* Read only */
	rb_define_method(rb_cRibbitCommit, "message_short", rb_git_commit_message_short_GET, 0); 

	/* Read only */
	rb_define_method(rb_cRibbitCommit, "time", rb_git_commit_time_GET, 0); /* READ ONLY */

	rb_define_method(rb_cRibbitCommit, "committer", rb_git_commit_committer_GET, 0);
	rb_define_method(rb_cRibbitCommit, "committer=", rb_git_commit_committer_SET, 3);

	rb_define_method(rb_cRibbitCommit, "author", rb_git_commit_author_GET, 0);
	rb_define_method(rb_cRibbitCommit, "author=", rb_git_commit_author_SET, 3);

	rb_define_method(rb_cRibbitCommit, "tree", rb_git_commit_tree_GET, 0);
	rb_define_method(rb_cRibbitCommit, "tree=", rb_git_commit_tree_SET, 1);


	/*
	 * Tag
	 */
	rb_cRibbitTag = rb_define_class_under(rb_cRibbit, "Tag", rb_cRibbitObject);
	rb_define_alloc_func(rb_cRibbitTag, rb_git_tag_allocate);
	rb_define_method(rb_cRibbitTag, "initialize", rb_git_tag_init, -1);

	rb_define_method(rb_cRibbitTag, "message", rb_git_tag_message_GET, 0);
	rb_define_method(rb_cRibbitTag, "message=", rb_git_tag_message_SET, 1);

	rb_define_method(rb_cRibbitTag, "name", rb_git_tag_name_GET, 0);
	rb_define_method(rb_cRibbitTag, "name=", rb_git_tag_name_SET, 1);

	rb_define_method(rb_cRibbitTag, "target", rb_git_tag_target_GET, 0);
	rb_define_method(rb_cRibbitTag, "target=", rb_git_tag_target_SET, 1);

	/* Read only */
	rb_define_method(rb_cRibbitTag, "target_type", rb_git_tag_target_type_GET, 0);

	rb_define_method(rb_cRibbitTag, "tagger", rb_git_tag_tagger_GET, 0);
	rb_define_method(rb_cRibbitTag, "tagge=", rb_git_tag_tagger_SET, 3);


	/*
	 * Tree entry 
	 */
	rb_cRibbitTreeEntry = rb_define_class_under(rb_cRibbit, "TreeEntry", rb_cObject);
	rb_define_alloc_func(rb_cRibbitTreeEntry, rb_git_treeentry_allocate);
	// rb_define_method(rb_cRibbitTreeEntry, "initialize", rb_git_treeentry_allocate, -1);
	rb_define_method(rb_cRibbitTreeEntry, "to_object", rb_git_tree_entry_2object, 0);

	rb_define_method(rb_cRibbitTreeEntry, "name", rb_git_tree_entry_name_GET, 0);
	rb_define_method(rb_cRibbitTreeEntry, "name=", rb_git_tree_entry_name_SET, 1);

	rb_define_method(rb_cRibbitTreeEntry, "attributes", rb_git_tree_entry_attributes_GET, 0);
	rb_define_method(rb_cRibbitTreeEntry, "attributes=", rb_git_tree_entry_attributes_SET, 1);

	rb_define_method(rb_cRibbitTreeEntry, "sha", rb_git_tree_entry_sha_GET, 0);
	rb_define_method(rb_cRibbitTreeEntry, "sha=", rb_git_tree_entry_sha_SET, 1);


	/*
	 * Tree
	 */
	rb_cRibbitTree = rb_define_class_under(rb_cRibbit, "Tree", rb_cRibbitObject);
	rb_define_alloc_func(rb_cRibbitTree, rb_git_tree_allocate);
	rb_define_method(rb_cRibbitTree, "initialize", rb_git_tree_init, -1);
	rb_define_method(rb_cRibbitTree, "entry_count", rb_git_tree_entrycount, 0);
	rb_define_method(rb_cRibbitTree, "get_entry", rb_git_tree_get_entry, 1);
	rb_define_method(rb_cRibbitTree, "[]", rb_git_tree_get_entry, 1);


	/*
	 * Walker
	 */
	rb_cRibbitWalker = rb_define_class_under(rb_cRibbit, "Walker", rb_cObject);
	rb_define_alloc_func(rb_cRibbitWalker, rb_git_walker_allocate);
	rb_define_method(rb_cRibbitWalker, "initialize", rb_git_walker_init, 1);
	rb_define_method(rb_cRibbitWalker, "push", rb_git_walker_push, 1);
	rb_define_method(rb_cRibbitWalker, "next", rb_git_walker_next, 0);
	rb_define_method(rb_cRibbitWalker, "hide", rb_git_walker_hide, 1);
	rb_define_method(rb_cRibbitWalker, "reset", rb_git_walker_reset, 0);
	rb_define_method(rb_cRibbitWalker, "sorting", rb_git_walker_sorting, 1);


	/*
	 * Constants
	 */
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

