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
extern VALUE rb_cRuggedObject;
extern VALUE rb_cRuggedRepo;

VALUE rb_cRuggedTree;
VALUE rb_cRuggedTreeBuilder;

static VALUE rb_git_treeentry_fromC(const git_tree_entry *entry)
{
	VALUE rb_entry;

	if (!entry)
		return Qnil;

	rb_entry = rb_hash_new();

	rb_hash_aset(rb_entry, CSTR2SYM("name"), rugged_str_new2(git_tree_entry_name(entry), NULL));
	rb_hash_aset(rb_entry, CSTR2SYM("oid"), rugged_create_oid(git_tree_entry_id(entry)));

	rb_hash_aset(rb_entry, CSTR2SYM("attributes"), INT2FIX(git_tree_entry_attributes(entry)));
	rb_hash_aset(rb_entry, CSTR2SYM("type"), INT2FIX(git_tree_entry_type(entry)));

	return rb_entry;
}

/*
 * Rugged Tree
 */
static VALUE rb_git_tree_entrycount(VALUE self)
{
	git_tree *tree;
	RUGGED_OBJ_UNWRAP(self, git_tree, tree);

	return INT2FIX(git_tree_entrycount(tree));
}

static VALUE rb_git_tree_get_entry(VALUE self, VALUE entry_id)
{
	git_tree *tree;
	RUGGED_OBJ_UNWRAP(self, git_tree, tree);

	if (TYPE(entry_id) == T_FIXNUM)
		return rb_git_treeentry_fromC(git_tree_entry_byindex(tree, FIX2INT(entry_id)));

	else if (TYPE(entry_id) == T_STRING)
		return rb_git_treeentry_fromC(git_tree_entry_byname(tree, StringValueCStr(entry_id)));

	else
		rb_raise(rb_eTypeError, "entry_id must be either an index or a filename");
}

static VALUE rb_git_tree_each(VALUE self)
{
	git_tree *tree;
	unsigned int i, count;
	RUGGED_OBJ_UNWRAP(self, git_tree, tree);

	if (!rb_block_given_p())
		return rb_funcall(self, rb_intern("to_enum"), 0);

	count = git_tree_entrycount(tree);

	for (i = 0; i < count; ++i) {
		const git_tree_entry *entry = git_tree_entry_byindex(tree, i);
		rb_yield(rb_git_treeentry_fromC(entry));
	}

	return Qnil;
}

static int rugged__treewalk_cb(const char *root, git_tree_entry *entry, void *proc)
{
	rb_funcall((VALUE)proc, rb_intern("call"), 2,
		rugged_str_new2(root, NULL),
		rb_git_treeentry_fromC(entry));

	return GIT_SUCCESS;
}

static VALUE rb_git_tree_walk(VALUE self, VALUE rb_mode)
{
	git_tree *tree;
	int error, mode = 0;
	ID id_mode;

	RUGGED_OBJ_UNWRAP(self, git_tree, tree);

	if (!rb_block_given_p())
		return rb_funcall(self, rb_intern("to_enum"), 2, CSTR2SYM("walk"), rb_mode);

	Check_Type(rb_mode, T_SYMBOL);
	id_mode = SYM2ID(rb_mode);

	if (id_mode == rb_intern("preorder"))
		mode = GIT_TREEWALK_PRE;
	else if (id_mode == rb_intern("postorder"))
		mode = GIT_TREEWALK_POST;
	else
		rb_raise(rb_eTypeError,
			"Invalid iteration mode. Expected `:preorder` or `:postorder`");

	error = git_tree_walk(tree, &rugged__treewalk_cb, mode, (void *)rb_block_proc());
	rugged_exception_check(error);

	return Qnil;
}

static VALUE rb_git_tree_subtree(VALUE self, VALUE rb_path)
{
	int error;
	git_tree *tree, *subtree;
	VALUE owner;

	RUGGED_OBJ_UNWRAP(self, git_tree, tree);
	RUGGED_OBJ_OWNER(self, owner);

	Check_Type(rb_path, T_STRING);

	error = git_tree_get_subtree(&subtree, tree, StringValueCStr(rb_path));
	rugged_exception_check(error);

	return rugged_object_new(owner, (git_object *)subtree);
}

static void rb_git_treebuilder_free(git_treebuilder *bld)
{
	git_treebuilder_free(bld);
}

static VALUE rb_git_treebuilder_allocate(VALUE klass)
{
	return Data_Wrap_Struct(klass, NULL, &rb_git_treebuilder_free, NULL);
}

static VALUE rb_git_treebuilder_init(int argc, VALUE *argv, VALUE self)
{
	git_treebuilder *builder;
	git_tree *tree = NULL;
	VALUE rb_object;
	int error;

	if (rb_scan_args(argc, argv, "01", &rb_object) == 1) {
		if (!rb_obj_is_kind_of(rb_object, rb_cRuggedTree))
			rb_raise(rb_eTypeError, "A Rugged::Tree instance is required");

		RUGGED_OBJ_UNWRAP(rb_object, git_tree, tree);
	}

	error = git_treebuilder_create(&builder, tree);
	rugged_exception_check(error);

	DATA_PTR(self) = builder;
	return Qnil;
}

static VALUE rb_git_treebuilder_clear(VALUE self)
{
	git_treebuilder *builder;
	Data_Get_Struct(self, git_treebuilder, builder);
	git_treebuilder_clear(builder);
	return Qnil;
}

static VALUE rb_git_treebuilder_get(VALUE self, VALUE path)
{
	git_treebuilder *builder;
	Data_Get_Struct(self, git_treebuilder, builder);

	Check_Type(path, T_STRING);

	return rb_git_treeentry_fromC(git_treebuilder_get(builder, StringValueCStr(path)));
}

static VALUE rb_git_treebuilder_insert(VALUE self, VALUE rb_entry)
{
	git_treebuilder *builder;
	VALUE rb_path, rb_oid, rb_attr;
	git_oid oid;
	int error;

	Data_Get_Struct(self, git_treebuilder, builder);
	Check_Type(rb_entry, T_HASH);

	rb_path = rb_hash_aref(rb_entry, CSTR2SYM("path"));
	Check_Type(rb_path, T_STRING);

	rb_oid = rb_hash_aref(rb_entry, CSTR2SYM("oid"));
	Check_Type(rb_oid, T_STRING);
	rugged_exception_check(git_oid_fromstr(&oid, StringValueCStr(rb_oid)));

	rb_attr = rb_hash_aref(rb_entry, CSTR2SYM("attributes"));
	Check_Type(rb_attr, T_FIXNUM);

	error = git_treebuilder_insert(NULL,
		builder,
		StringValueCStr(rb_path),
		&oid,
		FIX2INT(rb_attr));

	rugged_exception_check(error);
	return Qnil;
}

static VALUE rb_git_treebuilder_remove(VALUE self, VALUE path)
{
	git_treebuilder *builder;
	int error;

	Data_Get_Struct(self, git_treebuilder, builder);
	Check_Type(path, T_STRING);

	error = git_treebuilder_remove(builder, StringValueCStr(path));
	if (error == GIT_ENOTFOUND)
		return Qfalse;

	rugged_exception_check(error);
	return Qtrue;
}

static VALUE rb_git_treebuilder_write(VALUE self, VALUE rb_repo)
{
	git_treebuilder *builder;
	rugged_repository *repo;
	git_oid written_id;
	int error;

	if (!rb_obj_is_kind_of(rb_repo, rb_cRuggedRepo))
		rb_raise(rb_eTypeError, "Expecting a Rugged::Repository instance");

	Data_Get_Struct(self, git_treebuilder, builder);
	Data_Get_Struct(rb_repo, rugged_repository, repo);

	error = git_treebuilder_write(&written_id, repo->repo, builder);
	rugged_exception_check(error);

	return rugged_create_oid(&written_id);
}

static int treebuilder_cb(const git_tree_entry *entry, void *opaque)
{
	VALUE proc = (VALUE)opaque;
	VALUE ret = rb_funcall(proc, rb_intern("call"), 1, rb_git_treeentry_fromC(entry));
	return rugged_parse_bool(ret);
}

static VALUE rb_git_treebuilder_filter(VALUE self)
{
	git_treebuilder *builder;

	rb_need_block();
	Data_Get_Struct(self, git_treebuilder, builder);

	git_treebuilder_filter(builder, &treebuilder_cb, (void *)rb_block_proc());
	return Qnil;
}

void Init_rugged_tree()
{
	/*
	 * Tree
	 */
	rb_cRuggedTree = rb_define_class_under(rb_mRugged, "Tree", rb_cRuggedObject);
	rb_define_method(rb_cRuggedTree, "count", rb_git_tree_entrycount, 0);
	rb_define_method(rb_cRuggedTree, "length", rb_git_tree_entrycount, 0);
	rb_define_method(rb_cRuggedTree, "get_entry", rb_git_tree_get_entry, 1);
	rb_define_method(rb_cRuggedTree, "get_subtree", rb_git_tree_subtree, 1);
	rb_define_method(rb_cRuggedTree, "[]", rb_git_tree_get_entry, 1);
	rb_define_method(rb_cRuggedTree, "each", rb_git_tree_each, 0);
	rb_define_method(rb_cRuggedTree, "walk", rb_git_tree_walk, 1);

	rb_cRuggedTreeBuilder = rb_define_class_under(rb_cRuggedTree, "Builder", rb_cObject);
	rb_define_alloc_func(rb_cRuggedTreeBuilder, rb_git_treebuilder_allocate);
	rb_define_method(rb_cRuggedTreeBuilder, "initialize", rb_git_treebuilder_init, -1);
	rb_define_method(rb_cRuggedTreeBuilder, "clear", rb_git_treebuilder_clear, 0);
	rb_define_method(rb_cRuggedTreeBuilder, "[]", rb_git_treebuilder_get, 1);
	rb_define_method(rb_cRuggedTreeBuilder, "insert", rb_git_treebuilder_insert, 1);
	rb_define_method(rb_cRuggedTreeBuilder, "<<", rb_git_treebuilder_insert, 1);
	rb_define_method(rb_cRuggedTreeBuilder, "remove", rb_git_treebuilder_remove, 1);
	rb_define_method(rb_cRuggedTreeBuilder, "write", rb_git_treebuilder_write, 1);
	rb_define_method(rb_cRuggedTreeBuilder, "reject!", rb_git_treebuilder_filter, 0);
}
