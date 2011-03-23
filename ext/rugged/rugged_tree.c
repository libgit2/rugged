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
extern VALUE rb_cRuggedRepo;

VALUE rb_cRuggedTree;
VALUE rb_cRuggedTreeEntry;

/*
 * Tree entry
 */

static VALUE rb_git_createentry(VALUE tree, git_tree_entry *c_entry)
{
	if (c_entry == NULL)
		return Qnil;

	return Data_Wrap_Struct(rb_cRuggedTreeEntry, NULL, NULL, c_entry);
}

static VALUE rb_git_tree_entry_attributes_GET(VALUE self)
{
	git_tree_entry *tree_entry;
	Data_Get_Struct(self, git_tree_entry, tree_entry);

	return INT2FIX(git_tree_entry_attributes(tree_entry));
}

static VALUE rb_git_tree_entry_name_GET(VALUE self)
{
	git_tree_entry *tree_entry;
	Data_Get_Struct(self, git_tree_entry, tree_entry);

	return rugged_str_new2(git_tree_entry_name(tree_entry), NULL);
}

static VALUE rb_git_tree_entry_sha_GET(VALUE self)
{
	git_tree_entry *tree_entry;
	Data_Get_Struct(self, git_tree_entry, tree_entry);
	return rugged_create_oid(git_tree_entry_id(tree_entry));
}

static VALUE rb_git_tree_entry_2object(VALUE self, VALUE rb_repo)
{
	git_tree_entry *tree_entry;
	rugged_repository *repo;

	git_object *object;
	int error;

	if (!rb_obj_is_instance_of(rb_repo, rb_cRuggedRepo))
		rb_raise(rb_eTypeError, "Expecting a Rugged Repository");

	Data_Get_Struct(self, git_tree_entry, tree_entry);
	Data_Get_Struct(rb_repo, rugged_repository, repo);

	error = git_tree_entry_2object(&object, repo->repo, tree_entry);
	rugged_exception_check(error);

	return rugged_object_new(rb_repo, object);
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
		return rb_git_createentry(self, git_tree_entry_byindex(tree, FIX2INT(entry_id)));

	else if (TYPE(entry_id) == T_STRING)
		return rb_git_createentry(self, git_tree_entry_byname(tree, RSTRING_PTR(entry_id)));

	else
		rb_raise(rb_eTypeError, "entry_id must be either an index or a filename");
}

void Init_rugged_tree()
{
	/*
	 * Tree entry 
	 */
	rb_cRuggedTreeEntry = rb_define_class_under(rb_mRugged, "TreeEntry", rb_cObject);
	rb_define_method(rb_cRuggedTreeEntry, "to_object", rb_git_tree_entry_2object, 1);
	rb_define_method(rb_cRuggedTreeEntry, "name", rb_git_tree_entry_name_GET, 0);
	rb_define_method(rb_cRuggedTreeEntry, "attributes", rb_git_tree_entry_attributes_GET, 0);
	rb_define_method(rb_cRuggedTreeEntry, "sha", rb_git_tree_entry_sha_GET, 0);


	/*
	 * Tree
	 */
	rb_cRuggedTree = rb_define_class_under(rb_mRugged, "Tree", rb_cRuggedObject);
	rb_define_method(rb_cRuggedTree, "entry_count", rb_git_tree_entrycount, 0);
	rb_define_method(rb_cRuggedTree, "get_entry", rb_git_tree_get_entry, 1);
	rb_define_method(rb_cRuggedTree, "[]", rb_git_tree_get_entry, 1);
}
