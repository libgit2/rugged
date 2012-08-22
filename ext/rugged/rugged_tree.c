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
	VALUE type;

	if (!entry)
		return Qnil;

	rb_entry = rb_hash_new();

	rb_hash_aset(rb_entry, CSTR2SYM("name"), rugged_str_new2(git_tree_entry_name(entry), NULL));
	rb_hash_aset(rb_entry, CSTR2SYM("oid"), rugged_create_oid(git_tree_entry_id(entry)));

	rb_hash_aset(rb_entry, CSTR2SYM("filemode"), INT2FIX(git_tree_entry_filemode(entry)));

	switch(git_tree_entry_type(entry)) {
		case GIT_OBJ_TREE:
			type = CSTR2SYM("tree");
			break;

		case GIT_OBJ_BLOB:
			type = CSTR2SYM("blob");
			break;

		default:
			type = Qnil;
			break;
	}
	rb_hash_aset(rb_entry, CSTR2SYM("type"), type);

	return rb_entry;
}

/*
 * Rugged Tree
 */
/*
 *	call-seq:
 *		tree.count -> count
 *		tree.length -> count
 *
 *	Return the number of entries contained in the tree.
 *
 *	Note that this only applies to entries in the root of the tree,
 *	not any other entries contained in sub-folders.
 */
static VALUE rb_git_tree_entrycount(VALUE self)
{
	git_tree *tree;
	Data_Get_Struct(self, git_tree, tree);

	return INT2FIX(git_tree_entrycount(tree));
}

/*
 *	call-seq:
 *		tree[e] -> entry
 *		tree.get_entry(e) -> entry
 *
 *	Return one of the entries from a tree as a +Hash+. If +e+ is a number, the +e+nth entry
 *	from the tree will be returned. If +e+ is a string, the entry with that name
 *	will be returned.
 *
 *	If the entry doesn't exist, +nil+ will be returned.
 *
 *		tree[3] #=> {:name => "foo.txt", :type => :blob, :oid => "d8786bfc97485e8d7b19b21fb88c8ef1f199fc3f", :filemode => 0}
 *
 *		tree['bar.txt'] #=> {:name => "bar.txt", :type => :blob, :oid => "de5ba987198bcf2518885f0fc1350e5172cded78", :filemode => 0}
 *
 *		tree['baz.txt'] #=> nil
 */
static VALUE rb_git_tree_get_entry(VALUE self, VALUE entry_id)
{
	git_tree *tree;
	Data_Get_Struct(self, git_tree, tree);

	if (TYPE(entry_id) == T_FIXNUM)
		return rb_git_treeentry_fromC(git_tree_entry_byindex(tree, FIX2INT(entry_id)));

	else if (TYPE(entry_id) == T_STRING)
		return rb_git_treeentry_fromC(git_tree_entry_byname(tree, StringValueCStr(entry_id)));

	else
		rb_raise(rb_eTypeError, "entry_id must be either an index or a filename");
}

/*
 *	call-seq:
 *		tree.each { |entry| block }
 *		tree.each -> Iterator
 *
 *	Call +block+ with each of the entries of the subtree as a +Hash+. If no +block+
 *	is given, an +Iterator+ is returned instead.
 *
 *	Note that only the entries in the root of the tree are yielded; if you need to
 *	list also entries in subfolders, use +tree.walk+ instead.
 *
 *		tree.each { |entry| puts entry.inspect }
 *
 *	generates:
 *
 *		{:name => "foo.txt", :type => :blob, :oid => "d8786bfc97485e8d7b19b21fb88c8ef1f199fc3f", :filemode => 0}
 *		{:name => "bar.txt", :type => :blob, :oid => "de5ba987198bcf2518885f0fc1350e5172cded78", :filemode => 0}
 *		...
 */
static VALUE rb_git_tree_each(VALUE self)
{
	git_tree *tree;
	unsigned int i, count;
	Data_Get_Struct(self, git_tree, tree);

	if (!rb_block_given_p())
		return rb_funcall(self, rb_intern("to_enum"), 0);

	count = git_tree_entrycount(tree);

	for (i = 0; i < count; ++i) {
		const git_tree_entry *entry = git_tree_entry_byindex(tree, i);
		rb_yield(rb_git_treeentry_fromC(entry));
	}

	return Qnil;
}

static int rugged__treewalk_cb(const char *root, const git_tree_entry *entry, void *proc)
{
	rb_funcall((VALUE)proc, rb_intern("call"), 2,
		rugged_str_new2(root, NULL),
		rb_git_treeentry_fromC(entry));

	return GIT_OK;
}

/*
 *	call-seq:
 *		tree.walk(mode) { |root, entry| block }
 *		tree.walk(mode) -> Iterator
 *
 *	Walk +tree+ with the given mode (either +:preorder+ or +:postorder+) and yield
 *	to +block+ every entry in +tree+ and all its subtrees, as a +Hash+. The +block+
 *	also takes a +root+, the relative path in the traversal, starting from the root
 *	of the original tree.
 *
 *	If no +block+ is given, an +Iterator+ is returned instead.
 *
 *		tree.walk(:postorder) { |root, entry| puts "#{root}#{entry[:name]} [#{entry[:oid]}]" }
 *
 *	generates:
 *
 *		USAGE.rb [02bae86c91f96b5fdb6b1cf06f5aa3612139e318]
 *		ext [23f135b3c576b6ac4785821888991d7089f35db1]
 *		ext/rugged [25c88faa9302e34e16664eb9c990deb2bcf77849]
 *		ext/rugged/extconf.rb [40c1aa8a8cec8ca444ed5758e3f00ecff093070a]
 *		...
 */
static VALUE rb_git_tree_walk(VALUE self, VALUE rb_mode)
{
	git_tree *tree;
	int error, mode = 0;
	ID id_mode;

	Data_Get_Struct(self, git_tree, tree);

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

static VALUE rb_git_tree_path(VALUE self, VALUE rb_path)
{
  int error;
  git_tree *tree;
  git_tree_entry *entry;
  VALUE rb_entry;
  Data_Get_Struct(self, git_tree, tree);
  Check_Type(rb_path, T_STRING);

  error = git_tree_entry_bypath(&entry, tree, StringValueCStr(rb_path));
  rugged_exception_check(error);

  rb_entry = rb_git_treeentry_fromC(entry);
  git_tree_entry_free(entry);

  return rb_entry;
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

		Data_Get_Struct(rb_object, git_tree, tree);
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

	rb_path = rb_hash_aref(rb_entry, CSTR2SYM("name"));
	Check_Type(rb_path, T_STRING);

	rb_oid = rb_hash_aref(rb_entry, CSTR2SYM("oid"));
	Check_Type(rb_oid, T_STRING);
	rugged_exception_check(git_oid_fromstr(&oid, StringValueCStr(rb_oid)));

	rb_attr = rb_hash_aref(rb_entry, CSTR2SYM("filemode"));
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
	git_repository *repo;
	git_oid written_id;
	int error;

	if (!rb_obj_is_kind_of(rb_repo, rb_cRuggedRepo))
		rb_raise(rb_eTypeError, "Expecting a Rugged::Repository instance");

	Data_Get_Struct(self, git_treebuilder, builder);
	Data_Get_Struct(rb_repo, git_repository, repo);

	error = git_treebuilder_write(&written_id, repo, builder);
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
	rb_define_method(rb_cRuggedTree, "path", rb_git_tree_path, 1);
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
