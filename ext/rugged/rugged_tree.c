/*
 * The MIT License
 *
 * Copyright (c) 2010 Scott Chacon
 * Copyright (c) 2010 Vicent Marti
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

extern VALUE rb_cRugged;
extern VALUE rb_cRuggedObject;
VALUE rb_cRuggedTree;
VALUE rb_cRuggedTreeEntry;

/*
 * Tree entry
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

	obj = Data_Wrap_Struct(rb_cRuggedTreeEntry, NULL, NULL, entry);
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
	git_object *object;
	int error;

	Data_Get_Struct(self, git_tree_entry, tree_entry);

	error = git_tree_entry_2object(&object, tree_entry);
	rugged_exception_check(error);

	return rugged_object2rb(object);
}



/*
 * Rugged Tree
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


void Init_rugged_tree()
{
	/*
	 * Tree entry 
	 */
	rb_cRuggedTreeEntry = rb_define_class_under(rb_cRugged, "TreeEntry", rb_cObject);
	rb_define_alloc_func(rb_cRuggedTreeEntry, rb_git_treeentry_allocate);
	// rb_define_method(rb_cRuggedTreeEntry, "initialize", rb_git_treeentry_allocate, -1);
	rb_define_method(rb_cRuggedTreeEntry, "to_object", rb_git_tree_entry_2object, 0);

	rb_define_method(rb_cRuggedTreeEntry, "name", rb_git_tree_entry_name_GET, 0);
	rb_define_method(rb_cRuggedTreeEntry, "name=", rb_git_tree_entry_name_SET, 1);

	rb_define_method(rb_cRuggedTreeEntry, "attributes", rb_git_tree_entry_attributes_GET, 0);
	rb_define_method(rb_cRuggedTreeEntry, "attributes=", rb_git_tree_entry_attributes_SET, 1);

	rb_define_method(rb_cRuggedTreeEntry, "sha", rb_git_tree_entry_sha_GET, 0);
	rb_define_method(rb_cRuggedTreeEntry, "sha=", rb_git_tree_entry_sha_SET, 1);


	/*
	 * Tree
	 */
	rb_cRuggedTree = rb_define_class_under(rb_cRugged, "Tree", rb_cRuggedObject);
	rb_define_alloc_func(rb_cRuggedTree, rb_git_tree_allocate);
	rb_define_method(rb_cRuggedTree, "initialize", rb_git_tree_init, -1);
	rb_define_method(rb_cRuggedTree, "entry_count", rb_git_tree_entrycount, 0);
	rb_define_method(rb_cRuggedTree, "get_entry", rb_git_tree_get_entry, 1);
	rb_define_method(rb_cRuggedTree, "[]", rb_git_tree_get_entry, 1);
}
