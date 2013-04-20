require "test_helper"

class TreeToTreeDiffTest < Rugged::SandboxedTestCase
  def setup
    super

    @repo = sandbox_init("diff")

    @a = @repo.lookup("d70d245ed97ed2aa596dd1af6536e4bfdb047b69")
    @b = @repo.lookup("7a9e0b02e63179929fed24f0a3e0f19168114d10")
  end

  def test_size
    diff = @a.tree.diff(@b.tree)
    assert_equal 2, diff.size
  end

  def test_each_delta
    diff = @a.tree.diff(@b.tree)

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
    diff = @a.tree.diff(@b.tree, :max_size => 10)
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

  def test_diff_prefiltering
    diff = @a.tree.diff(@b.tree) do |diff_so_far, delta_to_be_added, matched_pathspec|
      delta_to_be_added.old_file[:path] == "readme.txt"
    end

    assert_equal "M\treadme.txt\n", diff.patch(:compact => true)
  end

  def test_contraining_paths
    diff = @a.tree.diff(@b.tree, :paths => ["readme.txt"])
    assert_equal "M\treadme.txt\n", diff.patch(:compact => true)

    diff = @a.tree.diff(@b.tree, :paths => ["r*.txt"])
    assert_equal "M\treadme.txt\n", diff.patch(:compact => true)

    diff = @a.tree.diff(@b.tree, :paths => ["*.txt"])
    assert_equal "M\tanother.txt\nM\treadme.txt\n", diff.patch(:compact => true)

    diff = @a.tree.diff(@b.tree, :paths => ["*.txt"], :disable_pathspec_match => true)
    assert_equal "", diff.patch(:compact => true)

    diff = @a.tree.diff(@b.tree, :paths => ["readme.txt"], :disable_pathspec_match => true)
    assert_equal "M\treadme.txt\n", diff.patch(:compact => true)
  end

  def test_options
    diff = @a.tree.diff(@b.tree, :context_lines => 0)

    assert_equal <<-EOS, diff.patch
diff --git a/another.txt b/another.txt
index 3e5bcba..546c735 100644
--- a/another.txt
+++ b/another.txt
@@ -2 +2 @@
-it a huge speed advantage on centralized systems that constantly have to
+it an huge speed advantage on centralized systems that constantly have to
@@ -11,4 +10,0 @@
-Let's see how common operations stack up against Subversion, a common
-centralized version control system that is similar to CVS or
-Perforce. Smaller is faster.
-
@@ -34,0 +31,4 @@
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
@@ -10,4 +9,0 @@
-Frictionless Context Switching. Create a branch to try out an idea, commit a
-few times, switch back to where you branched from, apply a patch, switch
-back to where you are experimenting, and merge it in.
-
@@ -27,3 +22,0 @@
-of your branches. You can choose to share just one of your branches, a few
-of them, or all of them. This tends to free people to try new ideas without
-worrying about having to plan how and when they are going to merge it in or
@@ -35 +28 @@
-it.
+it.!
\\ No newline at end of file
EOS
  end

  def test_iteration
    diff = @a.tree.diff(@b.tree)

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

    assert_equal "@@ -1,5 +1,5 @@\n", hunks[0].header
    assert_equal "@@ -8,10 +8,6 @@\n", hunks[1].header
    assert_equal "@@ -32,6 +28,10 @@\n", hunks[2].header

    assert_equal "readme.txt", patches[1].delta.old_file[:path]
    assert_equal "readme.txt", patches[1].delta.new_file[:path]

    refute patches[1].delta.binary?

    hunks = []
    patches[1].each_hunk do |hunk|
      assert_instance_of Rugged::Diff::Hunk, hunk
      hunks << hunk
    end
    assert_equal 3, hunks.size

    assert_equal "@@ -1,4 +1,4 @@\n", hunks[0].header
    assert_equal "@@ -7,10 +7,6 @@\n", hunks[1].header
    assert_equal "@@ -24,12 +20,9 @@\n", hunks[2].header

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
    diff = @a.tree.diff(@b.tree)

    assert_instance_of Enumerator, diff.each_patch

    patches = []
    diff.each_patch.each do |patch|
      assert_instance_of Rugged::Diff::Patch, patch
      patches << patch
    end
    assert_equal 2, patches.size
  end

  def test_each_hunk_returns_enumerator
    diff = @a.tree.diff(@b.tree)

    diff.each_patch do |patch|
      assert_instance_of Enumerator, patch.each_hunk
    end
  end

  def test_each_line_returns_enumerator
    diff = @a.tree.diff(@b.tree)

    diff.each_patch do |patch|
      patch.each_hunk do |hunk|
        assert_instance_of Enumerator, hunk.each_line
      end
    end
  end

  def test_patch
    diff = @a.tree.diff(@b.tree)

    assert_equal <<-EOS, diff.patch
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
@@ -8,10 +8,6 @@
 languages. Speed and performance has been a primary design goal of the Git
 from the start.
 
-Let's see how common operations stack up against Subversion, a common
-centralized version control system that is similar to CVS or
-Perforce. Smaller is faster.
-
 For testing, large AWS instances were set up in the same availability
 zone. Git and SVN were installed on both machines, the Ruby repository was
 copied to both Git and SVN servers, and common operations were performed on
@@ -32,6 +28,10 @@
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
@@ -7,10 +7,6 @@
 
 This means that you can do things like:
 
-Frictionless Context Switching. Create a branch to try out an idea, commit a
-few times, switch back to where you branched from, apply a patch, switch
-back to where you are experimenting, and merge it in.
-
 Role-Based Codelines. Have a branch that always contains only what goes to
 production, another that you merge work into for testing, and several
 smaller ones for day to day work.
@@ -24,12 +20,9 @@
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
  end

  def test_patch_compact
    diff = @a.tree.diff(@b.tree)

    assert_equal <<-EOS, diff.patch(:compact => true)
M\tanother.txt
M\treadme.txt
EOS
  end
end
