require File.dirname(__FILE__) + '/test_helper'
require 'base64'

context "Ribbit::Repository stuff" do
  setup do
    @path = File.dirname(__FILE__) + '/fixtures/testrepo.git/objects'
    @repo = Ribbit::Repository.new(@path)
  end

  def rm_loose(sha)
    dir = sha[0, 2]
    rest = sha[2, 38]
    file = File.join(@path, dir, rest)
    `rm -f #{file}`
  end

  test "can tell if an object exists or not" do
    assert @repo.exists("8496071c1b46c854b31185ea97743be6a8774479")
    assert @repo.exists("1385f264afb75a56a5bec74243be9b367ba4ca08")
    assert !@repo.exists("ce08fe4884650f067bd5703b6a59a8b3b3c99a09")
    assert !@repo.exists("8496071c1c46c854b31185ea97743be6a8774479")
  end

  test "can read an object from the db" do
    data, len, type = @repo.read("8496071c1b46c854b31185ea97743be6a8774479")
    assert_match 'tree 181037049a54a1eb5fab404658a3a250b44335d7', data
    assert_equal 172, len
    assert_equal "commit", type
  end

  test "returns false if an object is not there" do
    assert !@repo.read("a496071c1b46c854b31185ea97743be6a8774471")
  end

  test "can close the db" do
    @repo.exists("8496071c1b46c854b31185ea97743be6a8774479")
    @repo.close
    assert !@repo.exists("8496071c1b46c854b31185ea97743be6a8774479")
  end

  test "can hash data" do
    content = "my test data\n"
    sha = @repo.hash(content, "blob")
    assert_equal "76b1b55ab653581d6f2c7230d34098e837197674", sha
    assert !@repo.exists("76b1b55ab653581d6f2c7230d34098e837197674")
  end

  test "can write to the db" do
    content = "my test data\n"
    sha = @repo.write(content, "blob")
    assert_equal "76b1b55ab653581d6f2c7230d34098e837197674", sha
    assert @repo.exists("76b1b55ab653581d6f2c7230d34098e837197674")
    rm_loose("76b1b55ab653581d6f2c7230d34098e837197674")
  end

end