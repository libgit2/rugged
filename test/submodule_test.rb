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
end
