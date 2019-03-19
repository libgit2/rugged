require 'test_helper'
require 'base64'

module RepositoryApplyTestHelpers
  def assert_workdir_contents(path, test_value)
    assert_equal File.read(File.join(@repo.workdir, path)), test_value
  end

  def assert_index_content(path, test_value)
    assert_equal @repo.lookup(@repo.index[path][:oid]).content, test_value
  end

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
    @original_commit = @repo.head.target.oid
  end

  def test_apply_index
    new_commit, new_content, original_content = update_file(@repo, 'README')

    assert_index_content 'README', new_content

    diff = @repo.diff(new_commit, @original_commit)

    assert_equal true, @repo.apply(diff, {:location => :index})
    assert_index_content 'README', original_content
  end

  def test_apply_workdir
    new_commit, new_content, original_content = update_file(@repo, 'README')

    @repo.checkout_head(:strategy => :force)
    assert_workdir_contents 'README', new_content

    diff = @repo.diff(new_commit, @original_commit)

    assert_equal true, @repo.apply(diff)
    assert_workdir_contents 'README', original_content
  end

  def test_apply_both
    new_commit, new_content, original_content = update_file(@repo, 'README')

    @repo.checkout_head(:strategy => :force)
    assert_index_content 'README', new_content
    assert_workdir_contents 'README', new_content

    diff = @repo.diff(new_commit, @original_commit)

    assert_equal true, @repo.apply(diff, :location => :both)
    assert_index_content 'README', original_content
    assert_workdir_contents 'README', original_content
  end

  def test_location_option
    new_commit, new_content, original_content = update_file(@repo, 'README')

    diff = @repo.diff(new_commit, @original_commit)

    assert_raises TypeError do
      @repo.apply(diff, :location => :invalid)
    end
  end

  def test_callbacks
    new_commit, new_content, original_content = update_file(@repo, 'README')

    assert_equal @repo.lookup(@repo.index["README"][:oid]).content, new_content

    diff = @repo.diff(new_commit, @original_commit)

    hunk_cb = Proc.new { |hunk|
      assert hunk.is_a?(Rugged::Diff::Hunk)
    }

    delta_cb = Proc.new { |delta|
      assert delta.is_a?(Rugged::Diff::Delta)
    }

    assert_equal true, @repo.apply(diff, {:location => :index, :hunk_callback => hunk_cb, :delta_callback => delta_cb})
    assert_index_content 'README', original_content

    hunk_skip_all = Proc.new {
      false
    }

    new_commit, new_content, original_content = update_file(@repo, 'README')
    diff = @repo.diff(new_commit, @original_commit)
    assert_equal true, @repo.apply(diff, {:location => :index, :hunk_callback => hunk_skip_all})
    assert_index_content 'README', new_content

    delta_fail = Proc.new {
      nil
    }

    assert_raises RuntimeError do
      @repo.apply(diff, {:location => :index, :delta_callback => delta_fail})
    end

    assert_raises ArgumentError do
      @repo.apply(diff, {:location => :index, :delta_callback => 'this is not a callable object'})
    end

    assert_raises ArgumentError do
      @repo.apply(diff, {:location => :index, :hunk_callback => 'this is not a callable object'})
    end
  end
end