require "test_helper"

class TestRebase < Rugged::TestCase
  def setup
    @repo = FixtureRepo.from_libgit2("rebase")
    @sig = {
      name: "Rebaser",
      email: "rebaser@rebaser.rb",
      time: Time.at(1405694510),
      time_offset: 0,
    }
  end

  def test_rebase_inmemory_with_commits
    branch = @repo.branches["beef"].target
    upstream = @repo.branches["master"].target

    rebase = Rugged::Rebase.new(@repo, branch, upstream, inmemory: true)

    assert_equal({
      type: :pick,
      id: "da9c51a23d02d931a486f45ad18cda05cf5d2b94"
    }, rebase.next)

    rebase.abort
  end

  def test_rebase_inmemory_with_refs
    branch = @repo.branches["beef"]
    upstream = @repo.branches["master"]

    rebase = Rugged::Rebase.new(@repo, branch, upstream, inmemory: true)

    assert_equal({
      type: :pick,
      id: "da9c51a23d02d931a486f45ad18cda05cf5d2b94"
    }, rebase.next)

    rebase.abort
  end

  def test_rebase_inmemory_with_revparse
    branch = @repo.branches["beef"].target.oid[0..8]
    upstream = @repo.branches["master"].target.oid[0..8]

    rebase = Rugged::Rebase.new(@repo, branch, upstream, inmemory: true)

    assert_equal({
      type: :pick,
      id: "da9c51a23d02d931a486f45ad18cda05cf5d2b94"
    }, rebase.next)

    rebase.abort
  end

  def test_merge_next
    rebase = Rugged::Rebase.new(@repo, "refs/heads/beef", "refs/heads/master")

    op = rebase.next()
    assert_equal :pick, op[:type]
    assert_equal "da9c51a23d02d931a486f45ad18cda05cf5d2b94", op[:id]

    statuses = []
    @repo.status do |file, status|
      statuses << [file, status]
    end
    assert_equal 1, statuses.length
    assert_equal "beef.txt", statuses[0][0]

    rebase.abort()
  end

  def test_merge_finish
    rebase = Rugged::Rebase.new(@repo, "refs/heads/gravy", "refs/heads/veal")
    op = rebase.next()
    rebase.commit(committer: @sig)

    op = rebase.next()
    assert_nil op

    rebase.finish(@sig)
  end

  def test_rebase_onto_empty_commit
    base_id = Rugged::Commit.create(@repo, {
      message: "Add Readme",
      committer: @sig,
      author: @sig,
      parents: [],
      tree: Rugged::Tree.empty(@repo).update([
        { action: :upsert,
          oid: Rugged::Blob.from_buffer(@repo, "# README"),
          filemode: 0100644,
          path: "README.md"
        }
      ])
    })
    assert_equal "a8dc4354f25fb9187158daea19b1c3a9a55fff1d", base_id
    base = @repo.lookup(base_id)

    empty_id = Rugged::Commit.create(@repo, {
      message: "Add Readme",
      committer: @sig,
      author: @sig,
      parents: [ base ],
      tree: base.tree
    })
    assert_equal "912fb4d9110912bac7ebbdb6668bbd6bf9e049ac", empty_id
    empty = @repo.lookup(empty_id)
    assert_equal [ base_id ], empty.parent_ids

    changes_id = Rugged::Commit.create(@repo, {
      message: "Add License",
      committer: @sig,
      author: @sig,
      parents: [ base ],
      tree: base.tree.update([
        { action: :upsert,
          oid: Rugged::Blob.from_buffer(@repo, "# LICENSE"),
          filemode: 0100644,
          path: "LICENSE.md"
        }
      ])
    })
    assert_equal "eb4d5d00a6fb39c7270b8cc852ef3f245d82f2ff", changes_id
    changes = @repo.lookup(changes_id)
    assert_equal [ base_id ], changes.parent_ids

    rebase = Rugged::Rebase.new(@repo, changes, empty, inmemory: true)

    assert rebase.next

    rebased_commit_id = rebase.commit(committer: @sig)
    assert_equal "15ad95ba0acfa47f9e0694712c56bbfe3dbdddb8", rebased_commit_id

    rebased_commit = @repo.lookup(rebased_commit_id)
    assert_equal [ empty_id ], rebased_commit.parent_ids

    refute rebase.next

    rebase.finish(@sig)
  end

  def test_rebase_does_not_create_new_commits
    rebase = Rugged::Rebase.new(@repo, "refs/heads/gravy", "refs/heads/veal")

    op = rebase.next()
    first_commit_id = rebase.commit(committer: @sig)

    op = rebase.next()
    assert_nil op

    rebase.finish(@sig)

    rebase = Rugged::Rebase.new(@repo, "refs/heads/gravy", "refs/heads/veal")

    op = rebase.next()
    assert_equal first_commit_id, rebase.commit(committer: @sig)

    op = rebase.next()
    assert_nil op

    rebase.finish(@sig)
  end

  def test_merge_commit_fails_without_options
    rebase = Rugged::Rebase.new(@repo, "refs/heads/gravy", "refs/heads/veal")

    rebase.next()

    assert_raises TypeError do
      rebase.commit()
    end
  end

  def test_merge_commit_fails_with_nil_committer
    rebase = Rugged::Rebase.new(@repo, "refs/heads/gravy", "refs/heads/veal")

    rebase.next()

    assert_raises ArgumentError do
      rebase.commit(committer: nil)
    end
  end

  def test_merge_options
    rebase = Rugged::Rebase.new(@repo, "refs/heads/asparagus", "refs/heads/master",
                                fail_on_conflict: true, skip_reuc: true)
    assert_raises(Rugged::MergeError) { rebase.next() }
    rebase.abort()
  end

  def test_inmemory_resolve_conflicts
    rebase = Rugged::Rebase.new(@repo, "refs/heads/asparagus", "refs/heads/master", inmemory: true)

    op = rebase.next()
    idx = rebase.inmemory_index

    assert idx
    assert_equal :pick, op[:type]
    assert_equal "33f915f9e4dbd9f4b24430e48731a59b45b15500", op[:id]
    assert !@repo.index.conflicts?
    assert idx.conflicts?

    # We can't commit with conflicts pending
    assert_raises(Rugged::RebaseError) { rebase.commit(committer: @sig) }

    # But we can fix those conflicts in the in-memory index
    idx.conflict_remove("asparagus.txt")
    idx.add(path: "asparagus.txt", oid: "414dfc71ead79c07acd4ea47fecf91f289afc4b9", mode: 0100644)
    assert !idx.conflicts?

    commit_id = rebase.commit(committer: @sig)
    assert_equal "db7af47222181e548810da2ab5fec0e9357c5637", commit_id

    # Make sure the next operation is what we expect
    op = rebase.next()
    assert_equal :pick, op[:type]
    assert_equal "0f5f6d3353be1a9966fa5767b7d604b051798224", op[:id]
  end

  def test_inmemory_already_applied_patch
    rebase = Rugged::Rebase.new(@repo, "refs/heads/asparagus", "refs/heads/master", inmemory: true)

    rebase.next()

    idx = rebase.inmemory_index
    idx.read_tree(@repo.branches["master"].target.tree)

    assert_nil rebase.commit(committer: @sig)

    # Make sure the next operation is what we expect
    op = rebase.next()
    assert_equal :pick, op[:type]
    assert_equal "0f5f6d3353be1a9966fa5767b7d604b051798224", op[:id]
  end

  def test_rebase_does_not_lose_files
    Rugged::Commit.create(@repo, {
      :author => { :email => "rebaser@rebaser.com", :time => Time.now, :name => "Rebaser" },
      :committer => { :email => "rebaser@rebaser.com", :time => Time.now, :name => "Rebaser" },
      :message => "Add some files",
      :parents => [ @repo.branches["gravy"].target_id ],
      :update_ref => "refs/heads/gravy",
      :tree => @repo.branches["gravy"].target.tree.update([
        { :action => :upsert,
          :path => "some/nested/file",
          :oid => Rugged::Blob.from_buffer(@repo, "a new blob content"),
          :filemode => 0100644
        }
      ])
    })

    rebase = Rugged::Rebase.new(@repo, "refs/heads/gravy", "refs/heads/veal")

    assert rebase.next
    assert rebase.commit({ committer: { :email => "rebaser@rebaser.com", :name => "Rebaser" } })

    assert rebase.next
    assert rebase.commit({ committer: { :email => "rebaser@rebaser.com", :name => "Rebaser" } })
  end

  def test_inmemory_rebase_does_not_lose_files
    Rugged::Commit.create(@repo, {
      :author => { :email => "rebaser@rebaser.com", :time => Time.now, :name => "Rebaser" },
      :committer => { :email => "rebaser@rebaser.com", :time => Time.now, :name => "Rebaser" },
      :message => "Add some files",
      :parents => [ @repo.branches["gravy"].target_id ],
      :update_ref => "refs/heads/gravy",
      :tree => @repo.branches["gravy"].target.tree.update([
        { :action => :upsert,
          :path => "some/nested/file",
          :oid => Rugged::Blob.from_buffer(@repo, "a new blob content"),
          :filemode => 0100644
        }
      ])
    })

    rebase = Rugged::Rebase.new(@repo, "refs/heads/gravy", "refs/heads/veal", inmemory: true)

    assert rebase.next
    assert rebase.commit({ committer: { :email => "rebaser@rebaser.com", :name => "Rebaser" } })

    assert rebase.next
    assert rebase.commit({ committer: { :email => "rebaser@rebaser.com", :name => "Rebaser" } })
  end
end
