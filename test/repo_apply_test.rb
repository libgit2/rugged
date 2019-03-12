require 'test_helper'
require 'base64'

module RepositoryApplyTestHelpers
  def blob_contents(repo, path)
    repo.lookup(repo.head.target.tree[path][:oid]).content
  end

  def update_file(repo, path)
    original = blob_contents(repo, path)
    content = new_content(original)
    blob = repo.write(content, :blob)

    index = repo.index
    index.read_tree(repo.head.target.tree)
    index.add(:path => path, :oid => blob, :mode => 0100644)
    new_tree = index.write_tree(repo)

    oid = do_commit(repo, new_tree, "Update #{path}")

    return repo.lookup(oid), content, original
  end

  def do_commit(repo, tree, message)
    Rugged::Commit.create(repo,
      :tree => tree,
      :author => person,
      :committer => person,
      :message => message,
      :parents => repo.empty? ? [] : [ repo.head.target ].compact,
      :update_ref => 'HEAD',
    )
  end

  def new_content(original)
    "#{original}Some new line\n"
  end

  def person
    {:name => 'Scott', :email => 'schacon@gmail.com', :time => Time.now }
  end
end

class RepositoryApplyTest < Rugged::TestCase
  include RepositoryApplyTestHelpers

  def setup
    @repo = FixtureRepo.from_libgit2("testrepo")
  end

  def test_apply_index
    original_commit = @repo.head.target.oid
    new_commit, new_content, original_content = update_file(@repo, 'README')

    assert_equal @repo.lookup(@repo.index["README"][:oid]).content, new_content

    diff = @repo.diff(new_commit, original_commit)

    assert_equal true, @repo.apply(diff, {:location => :index})
    assert_equal @repo.lookup(@repo.index["README"][:oid]).content, original_content
  end

  def test_apply_workdir
    original_commit = @repo.head.target.oid
    new_commit, new_content, original_content = update_file(@repo, 'README')

    @repo.checkout_head(:strategy => :force)
    assert_equal File.read(File.join(@repo.workdir, 'README')), new_content

    diff = @repo.diff(new_commit, original_commit)

    assert_equal true, @repo.apply(diff, {:location => :workdir})
    assert_equal File.read(File.join(@repo.workdir, 'README')), original_content
  end

  def test_apply_both
    original_commit = @repo.head.target.oid
    new_commit, new_content, original_content = update_file(@repo, 'README')

    @repo.checkout_head(:strategy => :force)
    assert_equal @repo.lookup(@repo.index["README"][:oid]).content, new_content
    assert_equal File.read(File.join(@repo.workdir, 'README')), new_content

    diff = @repo.diff(new_commit, original_commit)

    assert_equal true, @repo.apply(diff)
    assert_equal @repo.lookup(@repo.index["README"][:oid]).content, original_content
    assert_equal File.read(File.join(@repo.workdir, 'README')), original_content
  end

  def test_location_option
    original_commit = @repo.head.target.oid
    new_commit, new_content, original_content = update_file(@repo, 'README')

    diff = @repo.diff(new_commit, original_commit)

    assert_raises TypeError do
      @repo.apply(diff, :location => :invalid)
    end
  end
end