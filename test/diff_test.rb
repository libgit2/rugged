require "test_helper"

class RepoDiffTest < Rugged::SandboxedTestCase
  def test_with_oid_string
    repo = sandbox_init("attr")
    diff = repo.diff("605812a", "370fe9ec22", :context_lines => 1, :interhunk_lines => 1)

    deltas = diff.deltas
    patches = diff.patches
    hunks = patches.map(&:hunks).flatten
    lines = hunks.map(&:lines).flatten

    assert_equal 5, diff.size
    assert_equal 5, deltas.size
    assert_equal 5, patches.size

    assert_equal 2, deltas.select(&:added?).size
    assert_equal 1, deltas.select(&:deleted?).size
    assert_equal 2, deltas.select(&:modified?).size

    assert_equal 5, hunks.size

    assert_equal((7 + 24 + 1 + 6 + 6), lines.size)
    assert_equal((1), lines.select(&:context?).size)
    assert_equal((24 + 1 + 5 + 5), lines.select(&:addition?).size)
    assert_equal((7 + 1), lines.select(&:deletion?).size)
  end

  def test_with_nil_on_left_side
    repo = sandbox_init("attr")
    diff = repo.diff("605812a", nil, :context_lines => 1, :interhunk_lines => 1)

    deltas = diff.deltas
    patches = diff.patches
    hunks = patches.map(&:hunks).flatten
    lines = hunks.map(&:lines).flatten

    assert_equal 16, diff.size
    assert_equal 16, deltas.size
    assert_equal 16, patches.size

    assert_equal 0, deltas.select(&:added?).size
    assert_equal 16, deltas.select(&:deleted?).size
    assert_equal 0, deltas.select(&:modified?).size

    assert_equal 15, hunks.size

    assert_equal 115, lines.size
    assert_equal 0, lines.select(&:context?).size
    assert_equal 0, lines.select(&:addition?).size
    assert_equal 113, lines.select(&:deletion?).size
  end

  def test_with_nil_on_right_side
    repo = sandbox_init("attr")
    diff = repo.diff(nil, "605812a", :context_lines => 1, :interhunk_lines => 1)

    deltas = diff.deltas
    patches = diff.patches
    hunks = patches.map(&:hunks).flatten
    lines = hunks.map(&:lines).flatten

    assert_equal 16, diff.size
    assert_equal 16, deltas.size
    assert_equal 16, patches.size

    assert_equal 16, deltas.select(&:added?).size
    assert_equal 0, deltas.select(&:deleted?).size
    assert_equal 0, deltas.select(&:modified?).size

    assert_equal 15, hunks.size

    assert_equal 115, lines.size
    assert_equal 0, lines.select(&:context?).size
    assert_equal 113, lines.select(&:addition?).size
    assert_equal 0, lines.select(&:deletion?).size
  end
end

class RepoWorkdirDiffTest < Rugged::SandboxedTestCase
  def test_basic_diff
    repo = sandbox_init("status")
    diff = repo.diff_workdir("26a125ee1bf",
      :context_lines => 3,
      :interhunk_lines => 1,
      :include_ignored => true,
      :include_untracked => true
    )

    deltas = diff.deltas
    patches = diff.patches
    hunks = patches.map(&:hunks).flatten
    lines = hunks.map(&:lines).flatten

    assert_equal 14, deltas.size
    assert_equal 14, patches.size

    assert_equal 0, deltas.select(&:added?).size
    assert_equal 4, deltas.select(&:deleted?).size
    assert_equal 4, deltas.select(&:modified?).size
    assert_equal 1, deltas.select(&:ignored?).size
    assert_equal 5, deltas.select(&:untracked?).size

    assert_equal 8, hunks.size

    assert_equal 13, lines.size
    assert_equal 4, lines.select(&:context?).size
    assert_equal 5, lines.select(&:addition?).size
    assert_equal 4, lines.select(&:deletion?).size
  end
end

class CommitDiffTest < Rugged::SandboxedTestCase
  def test_diff_with_parent
    repo = sandbox_init("attr")
    commit = Rugged::Commit.lookup(repo, "605812a")

    diff = commit.diff(:context_lines => 1, :interhunk_lines => 1)

    deltas = diff.deltas
    patches = diff.patches
    hunks = patches.map(&:hunks).flatten
    lines = hunks.map(&:lines).flatten

    assert_equal 5, diff.size
    assert_equal 5, deltas.size
    assert_equal 5, patches.size

    assert_equal 0, deltas.select(&:added?).size
    assert_equal 3, deltas.select(&:deleted?).size
    assert_equal 2, deltas.select(&:modified?).size

    assert_equal 4, hunks.size

    assert_equal(51, lines.size)
    assert_equal(2, lines.select(&:context?).size)
    assert_equal(3, lines.select(&:addition?).size)
    assert_equal(46, lines.select(&:deletion?).size)
  end

  def test_diff_with_parent_for_initial_commit
    repo = sandbox_init("attr")
    commit = Rugged::Commit.lookup(repo, "6bab5c79cd5140d0f800917f550eb2a3dc32b0da")

    diff = commit.diff(:context_lines => 1, :interhunk_lines => 1, :reverse => true)

    deltas = diff.deltas
    patches = diff.patches
    hunks = patches.map(&:hunks).flatten
    lines = hunks.map(&:lines).flatten

    assert_equal 13, diff.size
    assert_equal 13, deltas.size
    assert_equal 13, patches.size

    assert_equal 13, deltas.select(&:added?).size
    assert_equal 0, deltas.select(&:deleted?).size
    assert_equal 0, deltas.select(&:modified?).size

    assert_equal 13, hunks.size

    assert_equal(72, lines.size)
    assert_equal(0, lines.select(&:context?).size)
    assert_equal(70, lines.select(&:addition?).size)
    assert_equal(0, lines.select(&:deletion?).size)
  end
end

class CommitToWorkdirDiffTest < Rugged::SandboxedTestCase
  def test_basic_diff
    repo = sandbox_init("status")
    a = Rugged::Commit.lookup(repo, "26a125ee1bf")

    diff = a.diff_workdir(
      :context_lines => 3,
      :interhunk_lines => 1,
      :include_ignored => true,
      :include_untracked => true
    )

    deltas = diff.deltas
    patches = diff.patches
    hunks = patches.map(&:hunks).flatten
    lines = hunks.map(&:lines).flatten

    assert_equal 14, deltas.size
    assert_equal 14, patches.size

    assert_equal 0, deltas.select(&:added?).size
    assert_equal 4, deltas.select(&:deleted?).size
    assert_equal 4, deltas.select(&:modified?).size
    assert_equal 1, deltas.select(&:ignored?).size
    assert_equal 5, deltas.select(&:untracked?).size

    assert_equal 8, hunks.size

    assert_equal 13, lines.size
    assert_equal 4, lines.select(&:context?).size
    assert_equal 5, lines.select(&:addition?).size
    assert_equal 4, lines.select(&:deletion?).size
  end
end

class TreeToWorkdirDiffTest < Rugged::SandboxedTestCase
  def test_basic_diff
    repo = sandbox_init("status")
    a = Rugged::Tree.lookup(repo, "26a125ee1bf").tree

    diff = a.diff_workdir(
      :context_lines => 3,
      :interhunk_lines => 1,
      :include_ignored => true,
      :include_untracked => true
    )

    deltas = diff.deltas
    patches = diff.patches
    hunks = patches.map(&:hunks).flatten
    lines = hunks.map(&:lines).flatten

    assert_equal 14, deltas.size
    assert_equal 14, patches.size

    assert_equal 0, deltas.select(&:added?).size
    assert_equal 4, deltas.select(&:deleted?).size
    assert_equal 4, deltas.select(&:modified?).size
    assert_equal 1, deltas.select(&:ignored?).size
    assert_equal 5, deltas.select(&:untracked?).size

    assert_equal 8, hunks.size

    assert_equal 13, lines.size
    assert_equal 4, lines.select(&:context?).size
    assert_equal 5, lines.select(&:addition?).size
    assert_equal 4, lines.select(&:deletion?).size
  end
end

class TreeToTreeDiffTest < Rugged::SandboxedTestCase
  def test_basic_diff
    repo = sandbox_init("attr")
    a = Rugged::Tree.lookup(repo, "605812a").tree
    b = Rugged::Tree.lookup(repo, "370fe9ec22").tree
    c = Rugged::Tree.lookup(repo, "f5b0af1fb4f5c").tree

    diff = a.diff(b, :context_lines => 1, :interhunk_lines => 1)

    deltas = diff.deltas
    patches = diff.patches
    hunks = patches.map(&:hunks).flatten
    lines = hunks.map(&:lines).flatten

    assert_equal 5, diff.size
    assert_equal 5, deltas.size
    assert_equal 5, patches.size

    assert_equal 2, deltas.select(&:added?).size
    assert_equal 1, deltas.select(&:deleted?).size
    assert_equal 2, deltas.select(&:modified?).size

    assert_equal 5, hunks.size

    assert_equal((7 + 24 + 1 + 6 + 6), lines.size)
    assert_equal((1), lines.select(&:context?).size)
    assert_equal((24 + 1 + 5 + 5), lines.select(&:addition?).size)
    assert_equal((7 + 1), lines.select(&:deletion?).size)


    diff = c.diff(b, :context_lines => 1, :interhunk_lines => 1)
    deltas = diff.deltas
    patches = diff.patches
    hunks = patches.map(&:hunks).flatten
    lines = hunks.map(&:lines).flatten

    assert_equal 2, deltas.size
    assert_equal 2, patches.size

    assert_equal 0, deltas.select(&:added?).size
    assert_equal 0, deltas.select(&:deleted?).size
    assert_equal 2, deltas.select(&:modified?).size

    assert_equal 2, hunks.size

    assert_equal((8 + 15), lines.size)
    assert_equal((1), lines.select(&:context?).size)
    assert_equal((1), lines.select(&:addition?).size)
    assert_equal((7 + 14), lines.select(&:deletion?).size)
  end

  def test_diff_with_empty_tree
    repo = sandbox_init("attr")
    a = Rugged::Tree.lookup(repo, "605812a").tree

    diff = a.diff(nil, :context_lines => 1, :interhunk_lines => 1)

    deltas = diff.deltas
    patches = diff.patches
    hunks = patches.map(&:hunks).flatten
    lines = hunks.map(&:lines).flatten

    assert_equal 16, diff.size
    assert_equal 16, deltas.size
    assert_equal 16, patches.size

    assert_equal 0, deltas.select(&:added?).size
    assert_equal 16, deltas.select(&:deleted?).size
    assert_equal 0, deltas.select(&:modified?).size

    assert_equal 15, hunks.size

    assert_equal 115, lines.size
    assert_equal 0, lines.select(&:context?).size
    assert_equal 0, lines.select(&:addition?).size
    assert_equal 113, lines.select(&:deletion?).size
  end

  def test_diff_with_rev_string
    repo = sandbox_init("attr")
    a = Rugged::Tree.lookup(repo, "605812a").tree

    diff = a.diff("370fe9ec22", :context_lines => 1, :interhunk_lines => 1)

    deltas = diff.deltas
    patches = diff.patches
    hunks = patches.map(&:hunks).flatten
    lines = hunks.map(&:lines).flatten

    assert_equal 5, diff.size
    assert_equal 5, deltas.size
    assert_equal 5, patches.size

    assert_equal 2, deltas.select(&:added?).size
    assert_equal 1, deltas.select(&:deleted?).size
    assert_equal 2, deltas.select(&:modified?).size

    assert_equal 5, hunks.size

    assert_equal((7 + 24 + 1 + 6 + 6), lines.size)
    assert_equal((1), lines.select(&:context?).size)
    assert_equal((24 + 1 + 5 + 5), lines.select(&:addition?).size)
    assert_equal((7 + 1), lines.select(&:deletion?).size)
  end

  def test_diff_merge
    repo = sandbox_init("attr")
    a = Rugged::Tree.lookup(repo, "605812a").tree
    b = Rugged::Tree.lookup(repo, "370fe9ec22").tree
    c = Rugged::Tree.lookup(repo, "f5b0af1fb4f5c").tree

    diff = a.diff(b).merge!(c.diff(b))
    
    deltas = diff.deltas
    patches = diff.patches
    hunks = patches.map(&:hunks).flatten
    lines = hunks.map(&:lines).flatten

    assert_equal 6, diff.size
    assert_equal 6, patches.size
    assert_equal 6, deltas.size

    assert_equal 2, deltas.select(&:added?).size
    assert_equal 1, deltas.select(&:deleted?).size
    assert_equal 3, deltas.select(&:modified?).size

    assert_equal 6, hunks.size

    assert_equal 59, lines.size
    assert_equal 1, lines.select(&:context?).size
    assert_equal 36, lines.select(&:addition?).size
    assert_equal 22, lines.select(&:deletion?).size
  end

  def test_symlink_blob_mode_changed_to_regular_file
    repo = sandbox_init("unsymlinked.git")

    a = Rugged::Tree.lookup(repo, "7fccd7").tree
    b = Rugged::Tree.lookup(repo, "806999").tree

    diff = a.diff(b)

    deltas = diff.deltas
    patches = diff.patches
 
    assert_equal 3, diff.size
    assert_equal 3, patches.size
    assert_equal 3, deltas.size

    assert_equal 1, deltas.select(&:added?).size
    assert_equal 2, deltas.select(&:deleted?).size
    assert_equal 0, deltas.select(&:modified?).size
    assert_equal 0, deltas.select(&:typechange?).size
  end

  def test_symlink_blob_mode_changed_to_regular_file_as_typechange
    repo = sandbox_init("unsymlinked.git")

    a = Rugged::Tree.lookup(repo, "7fccd7").tree
    b = Rugged::Tree.lookup(repo, "806999").tree

    diff = a.diff(b, :include_typechange => true)

    deltas = diff.deltas
    patches = diff.patches
 
    assert_equal 2, diff.size
    assert_equal 2, patches.size
    assert_equal 2, deltas.size

    assert_equal 0, deltas.select(&:added?).size
    assert_equal 1, deltas.select(&:deleted?).size
    assert_equal 0, deltas.select(&:modified?).size
    assert_equal 1, deltas.select(&:typechange?).size
  end

  def test_regular_blob_mode_changed_to_executable_file
    repo = sandbox_init("unsymlinked.git")

    a = Rugged::Tree.lookup(repo, "806999").tree
    b = Rugged::Tree.lookup(repo, "a8595c").tree

    diff = a.diff(b)

    deltas = diff.deltas
    patches = diff.patches
 
    assert_equal 1, diff.size
    assert_equal 1, patches.size
    assert_equal 1, deltas.size

    assert_equal 0, deltas.select(&:added?).size
    assert_equal 0, deltas.select(&:deleted?).size
    assert_equal 1, deltas.select(&:modified?).size
    assert_equal 0, deltas.select(&:typechange?).size
  end

  def test_size
    repo = sandbox_init("diff")

    a = repo.lookup("d70d245ed97ed2aa596dd1af6536e4bfdb047b69")
    b = repo.lookup("7a9e0b02e63179929fed24f0a3e0f19168114d10")

    diff = a.tree.diff(b.tree)
    assert_equal 2, diff.size
  end

  def test_each_delta
    repo = sandbox_init("diff")

    a = repo.lookup("d70d245ed97ed2aa596dd1af6536e4bfdb047b69")
    b = repo.lookup("7a9e0b02e63179929fed24f0a3e0f19168114d10")

    diff = a.tree.diff(b.tree)

    deltas = []
    diff.each_delta do |delta|
      assert_instance_of Rugged::Diff::Delta, delta
      deltas << delta
    end

    assert_equal 2, deltas.size

    assert_equal "another.txt", deltas[0].old_file[:path]
    assert_equal "another.txt", deltas[0].new_file[:path]

    refute deltas[0].binary?

    assert_equal "readme.txt", deltas[1].old_file[:path]
    assert_equal "readme.txt", deltas[1].new_file[:path]

    refute deltas[1].binary?
  end

  def test_diff_treats_files_bigger_as_max_size_as_binary
    repo = sandbox_init("diff")

    a = repo.lookup("d70d245ed97ed2aa596dd1af6536e4bfdb047b69")
    b = repo.lookup("7a9e0b02e63179929fed24f0a3e0f19168114d10")

    diff = a.tree.diff(b.tree, :max_size => 10)
    assert_equal 2, diff.patches.size
    assert_equal <<-EOS, diff.patch
diff --git a/another.txt b/another.txt
index 3e5bcba..546c735 100644
Binary files a/another.txt and b/another.txt differ
diff --git a/readme.txt b/readme.txt
index 7b808f7..29ab705 100644
Binary files a/readme.txt and b/readme.txt differ
EOS
  end

  def test_contraining_paths
    repo = sandbox_init("diff")

    a = repo.lookup("d70d245ed97ed2aa596dd1af6536e4bfdb047b69")
    b = repo.lookup("7a9e0b02e63179929fed24f0a3e0f19168114d10")

    diff = a.tree.diff(b.tree, :paths => ["readme.txt"])
    assert_equal "M\treadme.txt\n", diff.patch(:compact => true)

    diff = a.tree.diff(b.tree, :paths => ["r*.txt"])
    assert_equal "M\treadme.txt\n", diff.patch(:compact => true)

    diff = a.tree.diff(b.tree, :paths => ["*.txt"])
    assert_equal "M\tanother.txt\nM\treadme.txt\n", diff.patch(:compact => true)

    diff = a.tree.diff(b.tree, :paths => ["*.txt"], :disable_pathspec_match => true)
    assert_equal "", diff.patch(:compact => true)

    diff = a.tree.diff(b.tree, :paths => ["readme.txt"], :disable_pathspec_match => true)
    assert_equal "M\treadme.txt\n", diff.patch(:compact => true)
  end

  def test_options
    repo = sandbox_init("diff")

    a = repo.lookup("d70d245ed97ed2aa596dd1af6536e4bfdb047b69")
    b = repo.lookup("7a9e0b02e63179929fed24f0a3e0f19168114d10")

    diff = a.tree.diff(b.tree, :context_lines => 0)

    assert_equal <<-EOS, diff.patch
diff --git a/another.txt b/another.txt
index 3e5bcba..546c735 100644
--- a/another.txt
+++ b/another.txt
@@ -2 +2 @@ Git is fast. With Git, nearly all operations are performed locally, giving
-it a huge speed advantage on centralized systems that constantly have to
+it an huge speed advantage on centralized systems that constantly have to
@@ -11,4 +10,0 @@ from the start.
-Let's see how common operations stack up against Subversion, a common
-centralized version control system that is similar to CVS or
-Perforce. Smaller is faster.
-
@@ -34,0 +31,4 @@ SVN.
+Let's see how common operations stack up against Subversion, a common
+centralized version control system that is similar to CVS or
+Perforce. Smaller is faster.
+
diff --git a/readme.txt b/readme.txt
index 7b808f7..29ab705 100644
--- a/readme.txt
+++ b/readme.txt
@@ -1 +1 @@
-The Git feature that really makes it stand apart from nearly every other SCM
+The Git feature that r3ally mak3s it stand apart from n3arly 3v3ry other SCM
@@ -10,4 +9,0 @@ This means that you can do things like:
-Frictionless Context Switching. Create a branch to try out an idea, commit a
-few times, switch back to where you branched from, apply a patch, switch
-back to where you are experimenting, and merge it in.
-
@@ -27,3 +22,0 @@ Notably, when you push to a remote repository, you do not have to push all
-of your branches. You can choose to share just one of your branches, a few
-of them, or all of them. This tends to free people to try new ideas without
-worrying about having to plan how and when they are going to merge it in or
@@ -35 +28 @@ incredibly easy and it changes the way most developers work when they learn
-it.
+it.!
\\ No newline at end of file
EOS
  end

  def test_iteration
    repo = sandbox_init("diff")

    a = repo.lookup("d70d245ed97ed2aa596dd1af6536e4bfdb047b69")
    b = repo.lookup("7a9e0b02e63179929fed24f0a3e0f19168114d10")

    diff = a.tree.diff(b.tree)

    patches = []

    diff.each_patch do |patch|
      assert_instance_of Rugged::Diff::Patch, patch
      patches << patch
    end

    assert_equal 2, patches.size

    assert_equal "another.txt", patches[0].delta.old_file[:path]
    assert_equal "another.txt", patches[0].delta.new_file[:path]

    refute patches[0].delta.binary?

    hunks = []
    patches[0].each_hunk do |hunk|
      assert_instance_of Rugged::Diff::Hunk, hunk
      hunks << hunk
    end
    assert_equal 3, hunks.size

    assert hunks[0].header.start_with? "@@ -1,5 +1,5 @@"
    assert hunks[1].header.start_with? "@@ -8,10 +8,6 @@" 
    assert hunks[2].header.start_with? "@@ -32,6 +28,10 @@"

    assert_equal "readme.txt", patches[1].delta.old_file[:path]
    assert_equal "readme.txt", patches[1].delta.new_file[:path]

    refute patches[1].delta.binary?

    hunks = []
    patches[1].each_hunk do |hunk|
      assert_instance_of Rugged::Diff::Hunk, hunk
      hunks << hunk
    end
    assert_equal 3, hunks.size

    assert hunks[0].header.start_with? "@@ -1,4 +1,4 @@"
    assert hunks[1].header.start_with? "@@ -7,10 +7,6 @@"
    assert hunks[2].header.start_with? "@@ -24,12 +20,9 @@"

    lines = []
    hunks[0].each_line do |line|
      lines << line
    end
    assert_equal 5, lines.size

    assert_equal :deletion, lines[0].line_origin
    assert_equal "The Git feature that really makes it stand apart from nearly every other SCM\n", lines[0].content

    assert_equal :addition, lines[1].line_origin
    assert_equal "The Git feature that r3ally mak3s it stand apart from n3arly 3v3ry other SCM\n", lines[1].content

    assert_equal :context, lines[2].line_origin
    assert_equal "out there is its branching model.\n", lines[2].content

    assert_equal :context, lines[3].line_origin
    assert_equal "\n", lines[3].content

    assert_equal :context, lines[4].line_origin
    assert_equal "Git allows and encourages you to have multiple local branches that can be\n", lines[4].content
  end

  def test_each_patch_returns_enumerator
    repo = sandbox_init("diff")

    a = repo.lookup("d70d245ed97ed2aa596dd1af6536e4bfdb047b69")
    b = repo.lookup("7a9e0b02e63179929fed24f0a3e0f19168114d10")

    diff = a.tree.diff(b.tree)

    assert_instance_of Enumerator, diff.each_patch

    patches = []
    diff.each_patch.each do |patch|
      assert_instance_of Rugged::Diff::Patch, patch
      patches << patch
    end
    assert_equal 2, patches.size
  end

  def test_each_hunk_returns_enumerator
    repo = sandbox_init("diff")

    a = repo.lookup("d70d245ed97ed2aa596dd1af6536e4bfdb047b69")
    b = repo.lookup("7a9e0b02e63179929fed24f0a3e0f19168114d10")

    diff = a.tree.diff(b.tree)

    diff.each_patch do |patch|
      assert_instance_of Enumerator, patch.each_hunk
    end
  end

  def test_each_line_returns_enumerator
    repo = sandbox_init("diff")

    a = repo.lookup("d70d245ed97ed2aa596dd1af6536e4bfdb047b69")
    b = repo.lookup("7a9e0b02e63179929fed24f0a3e0f19168114d10")

    diff = a.tree.diff(b.tree)

    diff.each_patch do |patch|
      patch.each_hunk do |hunk|
        assert_instance_of Enumerator, hunk.each_line
      end
    end
  end

  def test_patch
    repo = sandbox_init("diff")

    a = repo.lookup("d70d245ed97ed2aa596dd1af6536e4bfdb047b69")
    b = repo.lookup("7a9e0b02e63179929fed24f0a3e0f19168114d10")

    diff = a.tree.diff(b.tree)

    expected = <<-EOS
diff --git a/another.txt b/another.txt
index 3e5bcba..546c735 100644
--- a/another.txt
+++ b/another.txt
@@ -1,5 +1,5 @@
 Git is fast. With Git, nearly all operations are performed locally, giving
-it a huge speed advantage on centralized systems that constantly have to
+it an huge speed advantage on centralized systems that constantly have to
 communicate with a server somewhere.
 
 Git was built to work on the Linux kernel, meaning that it has had to
@@ -8,10 +8,6 @@ reducing the overhead of runtimes associated with higher-level
 languages. Speed and performance has been a primary design goal of the Git
 from the start.
 
-Let's see how common operations stack up against Subversion, a common
-centralized version control system that is similar to CVS or
-Perforce. Smaller is faster.
-
 For testing, large AWS instances were set up in the same availability
 zone. Git and SVN were installed on both machines, the Ruby repository was
 copied to both Git and SVN servers, and common operations were performed on
@@ -32,6 +28,10 @@ Clearly, in many of these common version control operations, Git is one or
 two orders of magnitude faster than SVN, even under ideal conditions for
 SVN.
 
+Let's see how common operations stack up against Subversion, a common
+centralized version control system that is similar to CVS or
+Perforce. Smaller is faster.
+
 One place where Git is slower is in the initial clone operation. Here, Git
 is downloading the entire history rather than only the latest version. As
 seen in the above charts, it's not considerably slower for an operation that
diff --git a/readme.txt b/readme.txt
index 7b808f7..29ab705 100644
--- a/readme.txt
+++ b/readme.txt
@@ -1,4 +1,4 @@
-The Git feature that really makes it stand apart from nearly every other SCM
+The Git feature that r3ally mak3s it stand apart from n3arly 3v3ry other SCM
 out there is its branching model.
 
 Git allows and encourages you to have multiple local branches that can be
@@ -7,10 +7,6 @@ those lines of development takes seconds.
 
 This means that you can do things like:
 
-Frictionless Context Switching. Create a branch to try out an idea, commit a
-few times, switch back to where you branched from, apply a patch, switch
-back to where you are experimenting, and merge it in.
-
 Role-Based Codelines. Have a branch that always contains only what goes to
 production, another that you merge work into for testing, and several
 smaller ones for day to day work.
@@ -24,12 +20,9 @@ not going to work, and just delete it - abandoning the work\xE2\x80\x94with nobody else
 ever seeing it (even if you've pushed other branches in the meantime).
 
 Notably, when you push to a remote repository, you do not have to push all
-of your branches. You can choose to share just one of your branches, a few
-of them, or all of them. This tends to free people to try new ideas without
-worrying about having to plan how and when they are going to merge it in or
 share it with others.
 
 There are ways to accomplish some of this with other systems, but the work
 involved is much more difficult and error-prone. Git makes this process
 incredibly easy and it changes the way most developers work when they learn
-it.
+it.!
\\ No newline at end of file
EOS

    expected.force_encoding('binary') if expected.respond_to?(:force_encoding)

    assert_equal expected, diff.patch
  end

  def test_patch_compact
    repo = sandbox_init("diff")

    a = repo.lookup("d70d245ed97ed2aa596dd1af6536e4bfdb047b69")
    b = repo.lookup("7a9e0b02e63179929fed24f0a3e0f19168114d10")

    diff = a.tree.diff(b.tree)

    assert_equal <<-EOS, diff.patch(:compact => true)
M\tanother.txt
M\treadme.txt
EOS
  end
end
