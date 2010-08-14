require File.dirname(__FILE__) + '/test_helper'
require 'base64'

context "Ribbit::Odb packed stuff" do
  setup do
    path = File.dirname(__FILE__) + '/fixtures/testrepo.git/objects'
    @odb = Ribbit::Odb.new(path)
  end

  test "can tell if a packed object exists" do
    assert @odb.exists("41bc8c69075bbdb46c5c6f0566cc8cc5b46e8bd9")
    assert @odb.exists("f82a8eb4cb20e88d1030fd10d89286215a715396")
  end

  test "can read a packed object from the db" do
    data, len, type = @odb.read("41bc8c69075bbdb46c5c6f0566cc8cc5b46e8bd9")
    assert_match 'tree f82a8eb4cb20e88d1030fd10d89286215a715396', data
    assert_equal 230, len
    assert_equal "commit", type
  end

end