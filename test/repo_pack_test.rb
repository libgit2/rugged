require "test_helper"
require 'base64'

context "Rugged::Repository packed stuff" do
  setup do
    path = File.dirname(__FILE__) + '/fixtures/testrepo.git/'
    @repo = Rugged::Repository.new(path)
  end

  test "can tell if a packed object exists" do
    assert @repo.exists?("41bc8c69075bbdb46c5c6f0566cc8cc5b46e8bd9")
    assert @repo.exists?("f82a8eb4cb20e88d1030fd10d89286215a715396")
  end

  test "can read a packed object from the db" do
    rawobj = @repo.read("41bc8c69075bbdb46c5c6f0566cc8cc5b46e8bd9")
    assert_match 'tree f82a8eb4cb20e88d1030fd10d89286215a715396', rawobj.data
    assert_equal 230, rawobj.len
    assert_equal :commit, rawobj.type
  end

end
