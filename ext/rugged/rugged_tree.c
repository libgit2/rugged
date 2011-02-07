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

extern VALUE rb_mRugged;
extern VALUE rb_cRuggedObject;
VALUE rb_cRuggedTree;
VALUE rb_cRuggedTreeEntry;

typedef struct {
	git_tree_entry *entry;
	VALUE tree;
} rugged_tree_entry;

/*
 * Tree entry
 */

void rb_git_tree_entry__free(rugged_tree_entry *entry)
{
	free(entry);
}

void rb_git_tree_entry__mark(rugged_tree_entry *entry)
{
	rb_gc_mark(entry->tree);
}

static VALUE rb_git_createentry(VALUE tree, git_tree_entry *c_entry)
{
	rugged_tree_entry *entry;
	VALUE obj;

	if (c_entry == NULL)
		return Qnil;

	entry = malloc(sizeof(rugged_tree_entry));
	if (entry == NULL)
		rb_raise(rb_eNoMemError, "out of memory");

	entry->entry = c_entry;
	entry->tree = tree;

	return Data_Wrap_Struct(rb_cRuggedTreeEntry, 
			rb_git_tree_entry__mark, rb_git_tree_entry__free, entry);
}

static VALUE rb_git_tree_entry_attributes_GET(VALUE self)
{
	rugged_tree_entry *tree_entry;
	Data_Get_Struct(self, rugged_tree_entry, tree_entry);

	return INT2FIX(git_tree_entry_attributes(tree_entry->entry));
}

static VALUE rb_git_tree_entry_attributes_SET(VALUE self, VALUE val)
{
	rugged_tree_entry *tree_entry;
	Data_Get_Struct(self, rugged_tree_entry, tree_entry);

	Check_Type(val, T_FIXNUM);
	git_tree_entry_set_attributes(tree_entry->entry, FIX2INT(val));
	return Qnil;
}

static VALUE rb_git_tree_entry_name_GET(VALUE self)
{
	rugged_tree_entry *tree_entry;
	Data_Get_Struct(self, rugged_tree_entry, tree_entry);

	return rugged_str_new2(git_tree_entry_name(tree_entry->entry), NULL);
}

static VALUE rb_git_tree_entry_name_SET(VALUE self, VALUE val)
{
	rugged_tree_entry *tree_entry;
	Data_Get_Struct(self, rugged_tree_entry, tree_entry);

	Check_Type(val, T_STRING);
	git_tree_entry_set_name(tree_entry->entry, RSTRING_PTR(val));
	return Qnil;
}

static VALUE rb_git_tree_entry_sha_GET(VALUE self)
{
	rugged_tree_entry *tree_entry;
	char out[40];
	Data_Get_Struct(self, rugged_tree_entry, tree_entry);

	git_oid_fmt(out, git_tree_entry_id(tree_entry->entry));
	return rugged_str_new(out, 40, NULL);
}

static VALUE rb_git_tree_entry_sha_SET(VALUE self, VALUE val)
{
	rugged_tree_entry *tree_entry;
	git_oid id;
	Data_Get_Struct(self, rugged_tree_entry, tree_entry);

	Check_Type(val, T_STRING);
	if (git_oid_mkstr(&id, RSTRING_PTR(val)) < 0)
		rb_raise(rb_eTypeError, "Invalid SHA1 value");
	git_tree_entry_set_id(tree_entry->entry, &id);
	return Qnil;
}

static VALUE rb_git_tree_entry_2object(VALUE self)
{
	rugged_tree_entry *tree_entry;
	rugged_object *owner_tree;
	git_object *object;
	int error;

	Data_Get_Struct(self, rugged_tree_entry, tree_entry);
	Data_Get_Struct(tree_entry->tree, rugged_object, owner_tree);

	error = git_tree_entry_2object(&object, tree_entry->entry);
	rugged_exception_check(error);

	return rugged_object_new(owner_tree->owner, object);
}



/*
 * Rugged Tree
 */
static VALUE rb_git_tree_init(int argc, VALUE *argv, VALUE self)
{
	return rb_git_object_init(GIT_OBJ_TREE, argc, argv, self);
}

static VALUE rb_git_tree_entrycount(VALUE self)
{
	git_tree *tree;
	RUGGED_OBJ_UNWRAP(self, git_tree, tree);

	return INT2FIX(git_tree_entrycount(tree));
}

static VALUE rb_git_tree_clear(VALUE self)
{
	git_tree *tree;
	RUGGED_OBJ_UNWRAP(self, git_tree, tree);

	git_tree_clear_entries(tree);
	return Qnil;
}

static VALUE rb_git_tree_get_entry(VALUE self, VALUE entry_id)
{
	git_tree *tree;
	RUGGED_OBJ_UNWRAP(self, git_tree, tree);

	if (TYPE(entry_id) == T_FIXNUM)
		return rb_git_createentry(self, git_tree_entry_byindex(tree, FIX2INT(entry_id)));

	else if (TYPE(entry_id) == T_STRING)
		return rb_git_createentry(self, git_tree_entry_byname(tree, RSTRING_PTR(entry_id)));

	else
		rb_raise(rb_eTypeError, "entry_id must be either an index or a filename");
}

static VALUE rb_git_tree_add_entry(VALUE self, VALUE rb_oid, VALUE rb_filename, VALUE rb_mode)
{
	git_tree_entry *new_entry;
	git_oid oid;
	git_tree *tree;
	int error;

	RUGGED_OBJ_UNWRAP(self, git_tree, tree);

	Check_Type(rb_oid, T_STRING);
	Check_Type(rb_filename, T_STRING);
	Check_Type(rb_mode, T_FIXNUM);

	error = git_oid_mkstr(&oid, RSTRING_PTR(rb_oid));
	rugged_exception_check(error);

	error = git_tree_add_entry(&new_entry, tree, &oid, RSTRING_PTR(rb_filename), FIX2INT(rb_mode));
	rugged_exception_check(error);

	return rb_git_createentry(self, new_entry);
}


void Init_rugged_tree()
{
	/*
	 * Tree entry 
	 */
	rb_cRuggedTreeEntry = rb_define_class_under(rb_mRugged, "TreeEntry", rb_cObject);
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
	rb_cRuggedTree = rb_define_class_under(rb_mRugged, "Tree", rb_cRuggedObject);
	rb_define_method(rb_cRuggedTree, "initialize", rb_git_tree_init, -1);
	rb_define_method(rb_cRuggedTree, "entry_count", rb_git_tree_entrycount, 0);
	rb_define_method(rb_cRuggedTree, "clear", rb_git_tree_clear, 0);
	rb_define_method(rb_cRuggedTree, "add_entry", rb_git_tree_add_entry, 3);
	rb_define_method(rb_cRuggedTree, "get_entry", rb_git_tree_get_entry, 1);
	rb_define_method(rb_cRuggedTree, "[]", rb_git_tree_get_entry, 1);
}
