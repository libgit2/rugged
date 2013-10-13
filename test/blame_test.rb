require "test_helper"
require "time"

class BlameTest < Rugged::SandboxedTestCase
  def setup
    super
    @repo = sandbox_init("testrepo")
  end

  def teardown
    @repo.close
    super
  end

  def test_blame_simple
    blame = Rugged::Blame.new(@repo, "branch_file.txt")
    assert_equal 2, blame.count

    assert_equal({
      :lines_in_hunk => 1,
      :final_commit_id => "c47800c7266a2be04c571c04d5a6614691ea99bd",
      :final_start_line_number => 1,
      :final_signature => {
        :name => "Scott Chacon",
        :email => "schacon@gmail.com",
        :time => Time.parse("2010-05-25 11:58:14 -0700")
      },
      :orig_commit_id => "0000000000000000000000000000000000000000",
      :orig_path => "branch_file.txt",
      :orig_start_line_number => 1,
      :orig_signature => nil,
      :boundary => false
    }, blame[0])

    assert_equal({
      :lines_in_hunk => 1,
      :final_commit_id => "a65fedf39aefe402d3bb6e24df4d4f5fe4547750",
      :final_start_line_number => 2,
      :final_signature => {
        :name => "Scott Chacon",
        :email => "schacon@gmail.com",
        :time => Time.parse("2011-08-09 19:33:46 -0700")
      },
      :orig_commit_id => "0000000000000000000000000000000000000000",
      :orig_path => "branch_file.txt",
      :orig_start_line_number => 2,
      :orig_signature => nil,
      :boundary => false
    }, blame[1])
  end
end
