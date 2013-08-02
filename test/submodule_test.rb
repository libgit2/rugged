require 'test_helper'

class SubmoduleTest < Rugged::SubmoduleTestCase
  def setup
    super
    @repo = setup_submodule
  end

  def test_submodule_simple_lookup
    # lookup pending change in .gitmodules that is not in HEAD
    assert Rugged::Submodule.lookup(@repo, 'sm_added_and_uncommited')

    # lookup pending change in .gitmodules that is neither in HEAD nor index
    assert Rugged::Submodule.lookup(@repo, 'sm_gitmodules_only')

    # lookup git repo subdir that is not added as submodule */
    assert_raises Rugged::SubmoduleError do
      Rugged::Submodule.lookup(@repo, 'not-submodule')
    end

    # lookup existing directory that is not a submodule
    assert_nil Rugged::Submodule.lookup(@repo, 'just_a_dir')

    # lookup existing file that is not a submodule
    assert_nil Rugged::Submodule.lookup(@repo, 'just_a_file')

    # lookup non-existent item
    assert_nil Rugged::Submodule.lookup(@repo, 'no_such_file')
  end

  def test_submodule_attribute_getters
    submodule = Rugged::Submodule.lookup(@repo, 'sm_unchanged')

    assert submodule.path.end_with?('sm_unchanged')
    assert submodule.url.end_with?('submod2_target')
    assert_equal 'sm_unchanged', submodule.name
  end

  def test_submodule_status_unchanged
    submodule = Rugged::Submodule.lookup(@repo, 'sm_unchanged')
    expected = [:in_head, :in_index, :in_config, :in_workdir]
    assert_equal expected, submodule.status
    assert submodule.in_head?
    assert submodule.in_index?
    assert submodule.in_config?
    assert submodule.in_workdir?
  end

  def test_submodule_status_ignore_none
    submodule = Rugged::Submodule.lookup(@repo, 'sm_changed_index')
    assert_includes submodule.status, :dirty_workdir_index
    assert submodule.dirty_workdir_index?

    submodule = Rugged::Submodule.lookup(@repo, 'sm_changed_head')
    assert_includes submodule.status, :modified_in_workdir
    assert submodule.modified_in_workdir?

    submodule = Rugged::Submodule.lookup(@repo, 'sm_changed_file')
    assert_includes submodule.status, :modified_files_in_workdir
    assert submodule.modified_files_in_workdir?

    submodule = Rugged::Submodule.lookup(@repo, 'sm_changed_untracked_file')
    assert_includes submodule.status, :untracked_files_in_workdir
    assert submodule.untracked_files_in_workdir?

    submodule = Rugged::Submodule.lookup(@repo, 'sm_missing_commits')
    assert_includes submodule.status, :modified_in_workdir
    assert submodule.modified_in_workdir?

    submodule = Rugged::Submodule.lookup(@repo, 'sm_added_and_uncommited')
    assert_includes submodule.status, :added_to_index
    assert submodule.added_to_index?

    sm_unchanged_path = File.join(@repo.workdir, 'sm_unchanged')

    # removed sm_unchanged for deleted workdir
    FileUtils.remove_entry_secure(sm_unchanged_path)
    submodule = Rugged::Submodule.lookup(@repo, 'sm_unchanged')
    assert_includes submodule.status, :deleted_from_workdir
    assert submodule.deleted_from_workdir?

    # now mkdir sm_unchanged to test uninitialized
    FileUtils.mkdir(sm_unchanged_path, :mode => 0755)
    submodule = Rugged::Submodule.lookup(@repo, 'sm_unchanged')
    submodule.reload
    assert_includes submodule.status, :uninitialized
    assert submodule.uninitialized?

    # update sm_changed_head in index
    submodule = Rugged::Submodule.lookup(@repo, 'sm_changed_head')
    submodule.add_to_index
    assert_includes submodule.status, :modified_in_index
    assert submodule.modified_in_index?

    # remove sm_changed_head from index */
    index = @repo.index
    index.remove('sm_changed_head')
    index.write

    submodule = Rugged::Submodule.lookup(@repo, 'sm_changed_head')
    submodule.reload
    assert_includes submodule.status, :deleted_from_index
    assert submodule.deleted_from_index?
  end

end
