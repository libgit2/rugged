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

VALUE rb_cRuggedIndex;
extern VALUE rb_mRugged;
extern VALUE rb_cRuggedCommit;
extern VALUE rb_cRuggedDiff;
extern VALUE rb_cRuggedTree;

static void rb_git_indexentry_toC(git_index_entry *entry, VALUE rb_entry);
static VALUE rb_git_indexentry_fromC(const git_index_entry *entry);

/*
 * Index
 */

static void rb_git_index__free(git_index *index)
{
	git_index_free(index);
}

VALUE rugged_index_new(VALUE klass, VALUE owner, git_index *index)
{
	VALUE rb_index = Data_Wrap_Struct(klass, NULL, &rb_git_index__free, index);
	rugged_set_owner(rb_index, owner);
	return rb_index;
}

/*
 *  call-seq:
 *    Index.new([path])
 *
 *  Create a bare index object based on the index file at +path+.
 *
 *  Any index methods that rely on the ODB or a working directory (e.g. #add)
 *  will raise a Rugged::IndexError.
 */
static VALUE rb_git_index_new(int argc, VALUE *argv, VALUE klass)
{
	git_index *index;
	int error;

	VALUE rb_path;
	const char *path = NULL;

	if (rb_scan_args(argc, argv, "01", &rb_path) == 1) {
		Check_Type(rb_path, T_STRING);
		path = StringValueCStr(rb_path);
	}

	error = git_index_open(&index, path);
	rugged_exception_check(error);

	return rugged_index_new(klass, Qnil, index);
}

/*
 *  call-seq:
 *    index.clear -> nil
 *
 *  Clear the contents (remove all entries) of the index object. Changes are in-memory only
 *  and can be saved by calling #write.
 */
static VALUE rb_git_index_clear(VALUE self)
{
	git_index *index;
	Data_Get_Struct(self, git_index, index);
	git_index_clear(index);
	return Qnil;
}

/*
 *  call-seq:
 *    index.reload -> nil
 *
 *  Reloads the index contents from the disk, discarding any changes that
 *  have not been saved through #write.
 */
static VALUE rb_git_index_read(VALUE self)
{
	git_index *index;
	int error;

	Data_Get_Struct(self, git_index, index);

	error = git_index_read(index);
	rugged_exception_check(error);

	return Qnil;
}

/*
 *  call-seq:
 *    index.write -> nil
 *
 *  Writes the index object from memory back to the disk, persisting all changes.
 */
static VALUE rb_git_index_write(VALUE self)
{
	git_index *index;
	int error;

	Data_Get_Struct(self, git_index, index);

	error = git_index_write(index);
	rugged_exception_check(error);

	return Qnil;
}

/*
 *  call-seq:
 *    index.count -> int
 *
 *  Returns the number of entries currently in the index.
 */
static VALUE rb_git_index_count(VALUE self)
{
	git_index *index;
	Data_Get_Struct(self, git_index, index);
	return INT2FIX(git_index_entrycount(index));
}

/*
 *  call-seq:
 *    index[path[, stage = 0]]     -> entry or nil
 *    index[position]              -> entry or nil
 *    index.get(path[, stage = 0]) -> entry or nil
 *    index.get(position)          -> entry or nil
 *
 *  Return a specific entry in the index.
 *
 *  The first two forms returns entries based on their +path+ in the index and an optional +stage+,
 *  while the last two forms return entries based on their position in the index.
 */
static VALUE rb_git_index_get(int argc, VALUE *argv, VALUE self)
{
	git_index *index;
	const git_index_entry *entry = NULL;

	VALUE rb_entry, rb_stage;

	Data_Get_Struct(self, git_index, index);

	rb_scan_args(argc, argv, "11", &rb_entry, &rb_stage);

	if (TYPE(rb_entry) == T_STRING) {
		int stage = 0;

		if (!NIL_P(rb_stage)) {
			Check_Type(rb_stage, T_FIXNUM);
			stage = FIX2INT(rb_stage);
		}

		entry = git_index_get_bypath(index, StringValueCStr(rb_entry), stage);
	}

	else if (TYPE(rb_entry) == T_FIXNUM) {
		if (argc > 1) {
			rb_raise(rb_eArgError,
				"Too many arguments when trying to lookup entry by index");
		}

		entry = git_index_get_byindex(index, FIX2INT(rb_entry));
	} else {
		rb_raise(rb_eArgError,
			"Invalid type for `entry`: expected String or Fixnum");
	}

	return entry ? rb_git_indexentry_fromC(entry) : Qnil;
}

/*
 *  call-seq:
 *    index.each { |entry| } -> nil
 *    index.each -> Enumerator
 *
 *  Passes each entry of the index to the given block.
 *
 *  If no block is given, an enumerator is returned instead.
 */
static VALUE rb_git_index_each(VALUE self)
{
	git_index *index;
	unsigned int i, count;

	Data_Get_Struct(self, git_index, index);

	if (!rb_block_given_p())
		return rb_funcall(self, rb_intern("to_enum"), 0);

	count = (unsigned int)git_index_entrycount(index);
	for (i = 0; i < count; ++i) {
		const git_index_entry *entry = git_index_get_byindex(index, i);
		if (entry)
			rb_yield(rb_git_indexentry_fromC(entry));
	}

	return Qnil;
}

/*
 *  call-seq:
 *    index.remove(path[, stage = 0]) -> nil
 *
 *  Removes the entry at the given +path+ with the given +stage+
 *  from the index.
 */
static VALUE rb_git_index_remove(int argc, VALUE *argv, VALUE self)
{
	git_index *index;
	int error, stage = 0;

	VALUE rb_entry, rb_stage;

	Data_Get_Struct(self, git_index, index);

	if (rb_scan_args(argc, argv, "11", &rb_entry, &rb_stage) > 1) {
		Check_Type(rb_stage, T_FIXNUM);
		stage = FIX2INT(rb_stage);
	}

	Check_Type(rb_entry, T_STRING);

	error = git_index_remove(index, StringValueCStr(rb_entry), stage);
	rugged_exception_check(error);

	return Qnil;
}

/*
 *  call-seq:
 *    index.remove_dir(dir[, stage = 0]) -> nil
 *
 *  Removes all entries under the given +dir+ with the given +stage+
 *  from the index.
 */
static VALUE rb_git_index_remove_directory(int argc, VALUE *argv, VALUE self)
{
	git_index *index;
	int error, stage = 0;

	VALUE rb_dir, rb_stage;

	Data_Get_Struct(self, git_index, index);

	if (rb_scan_args(argc, argv, "11", &rb_dir, &rb_stage) > 1) {
		Check_Type(rb_stage, T_FIXNUM);
		stage = FIX2INT(rb_stage);
	}

	Check_Type(rb_dir, T_STRING);

	error = git_index_remove_directory(index, StringValueCStr(rb_dir), stage);
	rugged_exception_check(error);

	return Qnil;
}

/*
 *  call-seq:
 *    index << entry -> nil
 *    index << path -> nil
 *    index.add(entry) -> nil
 *    index.add(path) -> nil
 *    index.update(entry) -> nil
 *    index.update(path) -> nil
 *
 *  Add a new entry to the index or update an existing entry in the index.
 *
 *  If passed a +path+ to an existing, readable file relative to the workdir,
 *  creates a new index entry based on this file.
 *
 *  Alternatively, a new index entry can be created by passing a Hash containing
 *  all key/value pairs of an index entry.
 *
 *  Any gitignore rules that might match +path+ (or the +:path+ value of the
 *  entry hash) are ignored.
 *
 *  If the index entry at +path+ (or +:path+) currently contains a merge conflict,
 *  it will no longer be marked as conflicting and the data about the conflict
 *  will be moved into the "resolve undo" (REUC) section of the index.
 */
static VALUE rb_git_index_add(VALUE self, VALUE rb_entry)
{
	git_index *index;
	int error = 0;

	Data_Get_Struct(self, git_index, index);

	if (TYPE(rb_entry) == T_HASH) {
		git_index_entry entry;

		rb_git_indexentry_toC(&entry, rb_entry);
		error = git_index_add(index, &entry);
	}

	else if (TYPE(rb_entry) == T_STRING) {
		error = git_index_add_bypath(index, StringValueCStr(rb_entry));
	}

	else {
		rb_raise(rb_eTypeError,
			"Expecting a hash defining an Index Entry or a path to a file in the repository");
	}

	rugged_exception_check(error);
	return Qnil;
}

int rugged__index_matched_path_cb(const char *path, const char *matched_pathspec, void *payload)
{
	int *exception = (int *)payload;

	VALUE rb_result, rb_args = rb_ary_new2(2);
	rb_ary_push(rb_args, rb_str_new2(path));
	rb_ary_push(rb_args, matched_pathspec == NULL ? Qnil : rb_str_new2(matched_pathspec));

	rb_result = rb_protect(rb_yield_splat, rb_args, exception);

	if (*exception)
		return GIT_ERROR;

	return RTEST(rb_result) ? 0 : 1;
}

/*
 *  call-seq:
 *    index.add_all(pathspec = [][, options])                            -> nil
 *    index.add_all(pathspec = [][, options]) { |path, pathspec| block } -> nil
 *
 *  Add or update index entries matching files in the working directory.
 *
 *  Searches the working directory for files that +pathspec+ and adds them
 *  to +index+ (by updating an existing entry or adding a new entry).
 *
 *  +pathspec+ can either be a String, or an Array of Strings.
 *  If +pathspec+ is empty, all entries in the index will be matched.
 *
 *  Files that are ignored due to +.gitignore+ rules will be skipped,
 *  unless they're already have an entry in +index+.
 *
 *  Files that are marked as the result of a merge request, will have this
 *  marking removed and the merge conflict information will be moved into the
 *  "resolve undo" (REUC) section of +index+.
 *
 *  If a block is given, each matched +path+ and the +pathspec+ that matched
 *  it will be passed to the block. If the return value of +block+ is
 *  falsy, the matching item will not be added to the index.
 *
 *  This method will fail in bare index instances.
 *
 *  The following options can be passed in the +options+ Hash:
 *
 *  :force ::
 *    If +true+, any +.gitignore+ rules will be ignored.
 *
 *  :disable_pathspec_match ::
 *    If +true+, glob expansion will be disabled and exact matching will be forced.
 *
 *  :check_pathspec ::
 *    If +true+, and the +:force+ options is +false+ or not given, exact matches
 *    of ignored files or files that are not already in +index+ will raise a
 *    Rugged::InvalidError. This emulates <code>git add -A</code>.
 */
static VALUE rb_git_index_add_all(int argc, VALUE *argv, VALUE self)
{
	VALUE rb_pathspecs, rb_options;

	git_index *index;
	git_strarray pathspecs;
	int error, exception = 0;
	unsigned int flags = GIT_INDEX_ADD_DEFAULT;

	Data_Get_Struct(self, git_index, index);

	if (rb_scan_args(argc, argv, "02", &rb_pathspecs, &rb_options) > 1) {
		Check_Type(rb_options, T_HASH);

		if (RTEST(rb_hash_aref(rb_options, CSTR2SYM("force"))))
			flags |= GIT_INDEX_ADD_FORCE;

		if (RTEST(rb_hash_aref(rb_options, CSTR2SYM("disable_pathspec_match"))))
			flags |= GIT_INDEX_ADD_DISABLE_PATHSPEC_MATCH;

		if (RTEST(rb_hash_aref(rb_options, CSTR2SYM("check_pathspec"))))
			flags |= GIT_INDEX_ADD_CHECK_PATHSPEC;
	}

	rugged_rb_ary_to_strarray(rb_pathspecs, &pathspecs);

	error = git_index_add_all(index, &pathspecs, flags,
		rb_block_given_p() ? rugged__index_matched_path_cb : NULL, &exception);

	xfree(pathspecs.strings);

	if (exception)
		rb_jump_tag(exception);

	rugged_exception_check(error);
	return Qnil;
}

/*
 *  call-seq:
 *    index.update_all(pathspec = [])                            -> nil
 *    index.update_all(pathspec = []) { |path, pathspec| block } -> nil
 *
 *  Update all index entries to match the working directory.
 *
 *  Searches +index+ for entries that match +pathspec+ and synchronizes
 *  them with the content of the working directory.
 *
 *  +pathspec+ can either be a String, or an Array of Strings.
 *  If +pathspec+ is empty, all entries in the index will be matched.
 *
 *  Entries where the corresponding working directory file no longer exists
 *  get deleted, all other matched entries will get updated to reflect their
 *  working directory state (the latest version of the a file's content will
 *  automatically be added to the ODB).
 *
 *  If a block is given, each matched +path+ and the +pathspec+ that matched
 *  it will be passed to the block. If the return value of +block+ is
 *  falsy, the matching item will not be updated in the index.
 *
 *  This method will fail in bare index instances.
 */
static VALUE rb_git_index_update_all(int argc, VALUE *argv, VALUE self)
{
	VALUE rb_pathspecs = rb_ary_new();;

	git_index *index;
	git_strarray pathspecs;
	int error, exception = 0;

	Data_Get_Struct(self, git_index, index);

	rb_scan_args(argc, argv, "01", &rb_pathspecs);

	rugged_rb_ary_to_strarray(rb_pathspecs, &pathspecs);

	error = git_index_update_all(index, &pathspecs,
		rb_block_given_p() ? rugged__index_matched_path_cb : NULL, &exception);

	xfree(pathspecs.strings);

	if (exception)
		rb_jump_tag(exception);
	rugged_exception_check(error);

	return Qnil;
}

/*
 *  call-seq:
 *    index.remove_all(pathspec = []) -> nil
 *    index.remove_all(pathspec = []) { |path, pathspec| block } -> nil
 *
 *  Remove all matching index entries.
 *
 *  Searches +index+ for entries that match +pathspec+ and removes them
 *  from the index.
 *
 *  +pathspec+ can either be a String, or an Array of Strings.
 *  If +pathspec+ is empty, all entries in the index will be matched.
 *
 *  If a block is given, each matched +path+ and the +pathspec+ that matched
 *  it will be passed to the block. If the return value of +block+ is
 *  falsy, the matching item will not be removed from the index.
 */
static VALUE rb_git_index_remove_all(int argc, VALUE *argv, VALUE self)
{
	VALUE rb_pathspecs = rb_ary_new();

	git_index *index;
	git_strarray pathspecs;
	int error, exception = 0;

	Data_Get_Struct(self, git_index, index);

	rb_scan_args(argc, argv, "01", &rb_pathspecs);

	if (NIL_P(rb_pathspecs))
		rb_pathspecs = rb_ary_new();

	rugged_rb_ary_to_strarray(rb_ary_to_ary(rb_pathspecs), &pathspecs);

	error = git_index_remove_all(index, &pathspecs,
		rb_block_given_p() ? rugged__index_matched_path_cb : NULL, &exception);

	xfree(pathspecs.strings);

	if (exception)
		rb_jump_tag(exception);
	rugged_exception_check(error);

	return Qnil;
}

static VALUE rb_git_indexentry_fromC(const git_index_entry *entry)
{
	VALUE rb_entry, rb_mtime, rb_ctime;
	unsigned int valid, stage;

	if (!entry)
		return Qnil;

	rb_entry = rb_hash_new();

	rb_hash_aset(rb_entry, CSTR2SYM("path"), rb_str_new_utf8(entry->path));
	rb_hash_aset(rb_entry, CSTR2SYM("oid"), rugged_create_oid(&entry->oid));

	rb_hash_aset(rb_entry, CSTR2SYM("dev"), INT2FIX(entry->dev));
	rb_hash_aset(rb_entry, CSTR2SYM("ino"), INT2FIX(entry->ino));
	rb_hash_aset(rb_entry, CSTR2SYM("mode"), INT2FIX(entry->mode));
	rb_hash_aset(rb_entry, CSTR2SYM("gid"), INT2FIX(entry->gid));
	rb_hash_aset(rb_entry, CSTR2SYM("uid"), INT2FIX(entry->uid));
	rb_hash_aset(rb_entry, CSTR2SYM("file_size"), INT2FIX(entry->file_size));

	valid = (entry->flags & GIT_IDXENTRY_VALID);
	rb_hash_aset(rb_entry, CSTR2SYM("valid"), valid ? Qtrue : Qfalse);

	stage = (entry->flags & GIT_IDXENTRY_STAGEMASK) >> GIT_IDXENTRY_STAGESHIFT;
	rb_hash_aset(rb_entry, CSTR2SYM("stage"), INT2FIX(stage));

	rb_mtime = rb_time_new(entry->mtime.seconds, entry->mtime.nanoseconds / 1000);
	rb_ctime = rb_time_new(entry->ctime.seconds, entry->ctime.nanoseconds / 1000);

	rb_hash_aset(rb_entry, CSTR2SYM("ctime"), rb_ctime);
	rb_hash_aset(rb_entry, CSTR2SYM("mtime"), rb_mtime);

	return rb_entry;
}

static inline unsigned int
default_entry_value(VALUE rb_entry, const char *key)
{
	VALUE val = rb_hash_aref(rb_entry, CSTR2SYM(key));
	if (NIL_P(val))
		return 0;

	Check_Type(val, T_FIXNUM);
	return FIX2INT(val);
}

static void rb_git_indexentry_toC(git_index_entry *entry, VALUE rb_entry)
{
	VALUE val;

	Check_Type(rb_entry, T_HASH);

	val = rb_hash_aref(rb_entry, CSTR2SYM("path"));
	Check_Type(val, T_STRING);
	entry->path = StringValueCStr(val);

	val = rb_hash_aref(rb_entry, CSTR2SYM("oid"));
	Check_Type(val, T_STRING);
	rugged_exception_check(
		git_oid_fromstr(&entry->oid, StringValueCStr(val))
	);

	entry->dev = default_entry_value(rb_entry, "dev");
	entry->ino = default_entry_value(rb_entry, "ino");
	entry->mode = default_entry_value(rb_entry, "mode");
	entry->gid = default_entry_value(rb_entry, "gid");
	entry->uid = default_entry_value(rb_entry, "uid");
	entry->file_size = (git_off_t)default_entry_value(rb_entry, "file_size");

	if ((val = rb_hash_aref(rb_entry, CSTR2SYM("mtime"))) != Qnil) {
		if (!rb_obj_is_kind_of(val, rb_cTime))
			rb_raise(rb_eTypeError, ":mtime must be a Time instance");

		entry->mtime.seconds = NUM2INT(rb_funcall(val, rb_intern("to_i"), 0));
		entry->mtime.nanoseconds = NUM2INT(rb_funcall(val, rb_intern("usec"), 0)) * 1000;
	} else {
		entry->mtime.seconds = entry->mtime.nanoseconds = 0;
	}

	if ((val = rb_hash_aref(rb_entry, CSTR2SYM("ctime"))) != Qnil) {
		if (!rb_obj_is_kind_of(val, rb_cTime))
			rb_raise(rb_eTypeError, ":ctime must be a Time instance");

		entry->ctime.seconds = NUM2INT(rb_funcall(val, rb_intern("to_i"), 0));
		entry->ctime.nanoseconds = NUM2INT(rb_funcall(val, rb_intern("usec"), 0)) * 1000;
	} else {
		entry->ctime.seconds = entry->ctime.nanoseconds = 0;
	}

	entry->flags = 0x0;
	entry->flags_extended = 0x0;

	val = rb_hash_aref(rb_entry, CSTR2SYM("stage"));
	if (!NIL_P(val)) {
		unsigned int stage = NUM2INT(val);
		entry->flags &= ~GIT_IDXENTRY_STAGEMASK;
		entry->flags |= (stage << GIT_IDXENTRY_STAGESHIFT) & GIT_IDXENTRY_STAGEMASK;
	}

	val = rb_hash_aref(rb_entry, CSTR2SYM("valid"));
	if (!NIL_P(val)) {
		entry->flags &= ~GIT_IDXENTRY_VALID;
		if (rugged_parse_bool(val))
			entry->flags |= GIT_IDXENTRY_VALID;
	} else {
		entry->flags |= GIT_IDXENTRY_VALID;
	}
}

/*
 *  call-seq:
 *    index.write_tree([repo]) -> oid
 *
 *  Write the index to a tree, either in the index's repository, or in
 *  the given +repo+.
 *
 *  If the index contains any files in conflict, writing the tree will fail.
 *
 *  Returns the OID string of the written tree object.
 */
static VALUE rb_git_index_writetree(int argc, VALUE *argv, VALUE self)
{
	git_index *index;
	git_oid tree_oid;
	int error;
	VALUE rb_repo;

	Data_Get_Struct(self, git_index, index);

	if (rb_scan_args(argc, argv, "01", &rb_repo) == 1) {
		git_repository *repo = NULL;
		rugged_check_repo(rb_repo);
		Data_Get_Struct(rb_repo, git_repository, repo);
		error = git_index_write_tree_to(&tree_oid, index, repo);
	}
	else {
		error = git_index_write_tree(&tree_oid, index);
	}

	rugged_exception_check(error);
	return rugged_create_oid(&tree_oid);
}

/*
 *  call-seq:
 *    index.read_tree(tree)
 *
 *  Clear the current index and start the index again on top of +tree+
 *
 *  Further index operations (+add+, +update+, +remove+, etc) will
 *  be considered changes on top of +tree+.
 */
static VALUE rb_git_index_readtree(VALUE self, VALUE rb_tree)
{
	git_index *index;
	git_tree *tree;
	int error;

	Data_Get_Struct(self, git_index, index);
	Data_Get_Struct(rb_tree, git_tree, tree);

	error = git_index_read_tree(index, tree);
	rugged_exception_check(error);

	return Qnil;
}

/*
 *  call-seq:
 *    index.diff([options]) -> diff
 *    index.diff(diffable[, options]) -> diff
 *
 *  The first form returns a diff between the index and the current working
 *  directory.
 *
 *  The second form returns a diff between the index and the given diffable object.
 *  +diffable+ can either be a +Rugged::Commit+ or a +Rugged::Tree+.
 *
 *  The index will be used as the "old file" side of the diff, while the working
 *  directory or the +diffable+ will be used for the "new file" side.
 *
 *  The following options can be passed in the +options+ Hash:
 *
 *  :paths ::
 *    An array of paths / fnmatch patterns to constrain the diff to a specific
 *    set of files. Also see +:disable_pathspec_match+.
 *
 *  :max_size ::
 *    An integer specifying the maximum byte size of a file before a it will
 *    be treated as binary. The default value is 512MB.
 *
 *  :context_lines ::
 *    The number of unchanged lines that define the boundary of a hunk (and
 *    to display before and after the actual changes). The default is 3.
 *
 *  :interhunk_lines ::
 *    The maximum number of unchanged lines between hunk boundaries before the hunks
 *    will be merged into a one. The default is 0.
 *
 *  :reverse ::
 *    If true, the sides of the diff will be reversed.
 *
 *  :force_text ::
 *    If true, all files will be treated as text, disabling binary attributes & detection.
 *
 *  :ignore_whitespace ::
 *    If true, all whitespace will be ignored.
 *
 *  :ignore_whitespace_change ::
 *    If true, changes in amount of whitespace will be ignored.
 *
 *  :ignore_whitespace_eol ::
 *    If true, whitespace at end of line will be ignored.
 *
 *  :ignore_submodules ::
 *    if true, submodules will be excluded from the diff completely.
 *
 *  :patience ::
 *    If true, the "patience diff" algorithm will be used (currenlty unimplemented).
 *
 *  :include_ignored ::
 *    If true, ignored files will be included in the diff.
 *
 *  :include_untracked ::
 *   If true, untracked files will be included in the diff.
 *
 *  :include_unmodified ::
 *    If true, unmodified files will be included in the diff.
 *
 *  :recurse_untracked_dirs ::
 *    Even if +:include_untracked+ is true, untracked directories will only be
 *    marked with a single entry in the diff. If this flag is set to true,
 *    all files under ignored directories will be included in the di ff, too.
 *
 *  :disable_pathspec_match ::
 *    If true, the given +:paths+ will be applied as exact matches, instead of
 *    as fnmatch patterns.
 *
 *  :deltas_are_icase ::
 *    If true, filename comparisons will be made with case-insensitivity.
 *
 *  :include_untracked_content ::
 *    if true, untracked content will be contained in the the diff patch text.
 *
 *  :skip_binary_check ::
 *    If true, diff deltas will be generated without spending time on binary
 *    detection. This is useful to improve performance in cases where the actual
 *    file content difference is not needed.
 *
 *  :include_typechange ::
 *    If true, type changes for files will not be interpreted as deletion of
 *    the "old file" and addition of the "new file", but will generate
 *    typechange records.
 *
 *  :include_typechange_trees ::
 *    Even if +:include_typechange+ is true, blob -> tree changes will still
 *    usually be handled as a deletion of the blob. If this flag is set to true,
 *    blob -> tree changes will be marked as typechanges.
 *
 *  :ignore_filemode ::
 *    If true, file mode changes will be ignored.
 *
 *  :recurse_ignored_dirs ::
 *    Even if +:include_ignored+ is true, ignored directories will only be
 *    marked with a single entry in the diff. If this flag is set to true,
 *    all files under ignored directories will be included in the diff, too.
 */
static VALUE rb_git_index_diff(int argc, VALUE *argv, VALUE self)
{
	git_index *index;
	git_diff_options opts = GIT_DIFF_OPTIONS_INIT;
	git_repository *repo;
	git_diff_list *diff = NULL;
	VALUE owner, rb_other, rb_options;
	int error;

	rb_scan_args(argc, argv, "01:", &rb_other, &rb_options);
	rugged_parse_diff_options(&opts, rb_options);

	Data_Get_Struct(self, git_index, index);
	owner = rugged_owner(self);
	Data_Get_Struct(owner, git_repository, repo);

	if (NIL_P(rb_other)) {
		error = git_diff_index_to_workdir(&diff, repo, index, &opts);
	} else {
		// Need to flip the reverse option, so that the index is by default
		// the "old file" side of the diff.
		opts.flags ^= GIT_DIFF_REVERSE;

		if (rb_obj_is_kind_of(rb_other, rb_cRuggedCommit)) {
			git_tree *other_tree;
			git_commit *commit;
			Data_Get_Struct(rb_other, git_commit, commit);
			error = git_commit_tree(&other_tree, commit);

			if (!error)
				error = git_diff_tree_to_index(&diff, repo, other_tree, index, &opts);
		} else if (rb_obj_is_kind_of(rb_other, rb_cRuggedTree)) {
			git_tree *other_tree;
			Data_Get_Struct(rb_other, git_tree, other_tree);
			error = git_diff_tree_to_index(&diff, repo, other_tree, index, &opts);
		} else {
			xfree(opts.pathspec.strings);
			rb_raise(rb_eTypeError, "A Rugged::Commit or Rugged::Tree instance is required");
		}
	}

	xfree(opts.pathspec.strings);
	rugged_exception_check(error);

	return rugged_diff_new(rb_cRuggedDiff, self, diff);
}

/*
 *  call-seq:
 *    index.conflicts? -> true or false
 *
 *  Determines if the index contains entries representing conflicts.
 */
static VALUE rb_git_index_conflicts_p(VALUE self)
{
	git_index *index;
	Data_Get_Struct(self, git_index, index);
	return git_index_has_conflicts(index) ? Qtrue : Qfalse;
}


/*
 *  Document-class: Rugged::Index
 *
 *  == Index Entries
 *
 *  Index entries are represented as Hash instances with the following key/value pairs:
 *
 *  path: ::
 *    The entry's path in the index.
 *
 *  oid: ::
 *    The oid of the entry's git object (blob / tree).
 *
 *  dev: ::
 *    The device for the index entry.
 *
 *  ino: ::
 *    The inode for the index entry.
 *
 *  mode: ::
 *    The current permissions of the index entry.
 *
 *  gid: ::
 *    Group ID of the index entry's owner.
 *
 *  uid: ::
 *    User ID of the index entry's owner.
 *
 *  file_size: ::
 *    The index entry's size, in bytes.
 *
 *  valid: ::
 *    +true+ if the index entry is valid, +false+ otherwise.
 *
 *  stage: ::
 *    The current stage of the index entry.
 *
 *  mtime: ::
 *    A Time instance representing the index entry's time of last modification.
 *
 *  mtime: ::
 *    A Time instance representing the index entry's time of last status change
 *    (ie. change of owner, group, mode, etc.).
 */
void Init_rugged_index(void)
{
	/*
	 * Index
	 */
	rb_cRuggedIndex = rb_define_class_under(rb_mRugged, "Index", rb_cObject);
	rb_define_singleton_method(rb_cRuggedIndex, "new", rb_git_index_new, -1);

	rb_define_method(rb_cRuggedIndex, "count", rb_git_index_count, 0);
	rb_define_method(rb_cRuggedIndex, "reload", rb_git_index_read, 0);
	rb_define_method(rb_cRuggedIndex, "clear", rb_git_index_clear, 0);
	rb_define_method(rb_cRuggedIndex, "write", rb_git_index_write, 0);
	rb_define_method(rb_cRuggedIndex, "get", rb_git_index_get, -1);
	rb_define_method(rb_cRuggedIndex, "[]", rb_git_index_get, -1);
	rb_define_method(rb_cRuggedIndex, "each", rb_git_index_each, 0);
	rb_define_method(rb_cRuggedIndex, "diff", rb_git_index_diff, -1);

	rb_define_method(rb_cRuggedIndex, "conflicts?", rb_git_index_conflicts_p, 0);

	rb_define_method(rb_cRuggedIndex, "add", rb_git_index_add, 1);
	rb_define_method(rb_cRuggedIndex, "update", rb_git_index_add, 1);
	rb_define_method(rb_cRuggedIndex, "<<", rb_git_index_add, 1);

	rb_define_method(rb_cRuggedIndex, "remove", rb_git_index_remove, -1);
	rb_define_method(rb_cRuggedIndex, "remove_dir", rb_git_index_remove_directory, -1);

	rb_define_method(rb_cRuggedIndex, "add_all", rb_git_index_add_all, -1);
	rb_define_method(rb_cRuggedIndex, "update_all", rb_git_index_update_all, -1);
	rb_define_method(rb_cRuggedIndex, "remove_all", rb_git_index_remove_all, -1);

	rb_define_method(rb_cRuggedIndex, "write_tree", rb_git_index_writetree, -1);
	rb_define_method(rb_cRuggedIndex, "read_tree", rb_git_index_readtree, 1);

	rb_const_set(rb_cRuggedIndex, rb_intern("ENTRY_FLAGS_STAGE"), INT2FIX(GIT_IDXENTRY_STAGEMASK));
	rb_const_set(rb_cRuggedIndex, rb_intern("ENTRY_FLAGS_STAGE_SHIFT"), INT2FIX(GIT_IDXENTRY_STAGESHIFT));
	rb_const_set(rb_cRuggedIndex, rb_intern("ENTRY_FLAGS_VALID"), INT2FIX(GIT_IDXENTRY_VALID));
}
