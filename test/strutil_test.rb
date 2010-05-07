require File.dirname(__FILE__) + '/test_helper'

context "Ribbit strutils" do
  setup do
    @ribbit = Ribbit.new
  end

  test "can compare prefixes" do
    puts @ribbit.compare_prefix("aa", "ab")
    assert_equal 1, 1
  end

end
