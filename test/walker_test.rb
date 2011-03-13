require File.expand_path "../test_helper", __FILE__
require 'base64'

context "Rugged::Walker stuff" do
  setup do
    @path = File.dirname(__FILE__) + '/fixtures/testrepo.git/'
    @repo = Rugged::Repository.new(@path)
    @walker = Rugged::Walker.new(@repo)
  end

  test "can walk a simple revlist" do
    @walker.push("9fd738e8f7967c078dceed8190330fc8648ee56a")
    data = []
    4.times do
      data << @walker.next.sha
    end
    shas = data.sort.map {|a| a[0,5]}.join('.')
    assert_equal "4a202.5b5b0.84960.9fd73", shas
    assert_equal false, @walker.next
  end

  test "can walk a part of a revlist" do
    sha = "8496071c1b46c854b31185ea97743be6a8774479"
    @walker.push(sha)
    assert_equal sha, @walker.next.sha
    assert_equal false, @walker.next
  end

  test "can hide part of a list" do
    @walker.push("9fd738e8f7967c078dceed8190330fc8648ee56a")
    @walker.hide("5b5b025afb0b4c913b4c338a42934a3863bf3644")
    2.times { @walker.next }
    assert_equal false, @walker.next
  end

  # resetting a walker emtpies the walking queue
  test "can reset a walker" do
    sha = "8496071c1b46c854b31185ea97743be6a8774479"
    @walker.push(sha)
    assert_equal sha, @walker.next.sha
    assert_equal false, @walker.next
    @walker.reset
    assert_equal sha, @walker.next.sha
  end

  test "can enumerable" do
    @walker.push("9fd738e8f7967c078dceed8190330fc8648ee56a")
    enum = @walker.sort { |a, b| a.sha <=> b.sha }.map { |a| a.sha[0, 4] }.join('.')
    assert_equal "4a20.5b5b.8496.9fd7", enum
  end

  def do_sort(sorting)
    sha = "a4a7dce85cf63874e984719f4fdd239f5145052f"
    @walker.sorting(sorting)
    @walker.push(sha)
    data = []
    6.times do
      data << @walker.next
    end
    data
  end

  def revlist_with_sorting(sorting)
    data = do_sort sorting
    shas = data.map {|a| a.sha[0,5] if a }.join('.')
  end

  def is_toposorted(list)
    list.all? do |commit|
      commit.parents.all? { |parent| list.index(commit) < list.index(parent) }
    end
  end

  test "can sort order by date" do
    time = revlist_with_sorting(Rugged::SORT_DATE)
    assert_equal "a4a7d.c4780.9fd73.4a202.5b5b0.84960", time
  end

  test "can sort order by topo" do
    sort_list = do_sort(Rugged::SORT_TOPO)
    assert_equal is_toposorted(sort_list), true
  end

  test "can sort order by date reversed" do
    time = revlist_with_sorting(Rugged::SORT_DATE | Rugged::SORT_REVERSE)
    assert_equal "84960.5b5b0.4a202.9fd73.c4780.a4a7d", time
  end

  test "can sort order by topo reversed" do
    sort_list = do_sort(Rugged::SORT_TOPO | Rugged::SORT_REVERSE).reverse
    assert_equal is_toposorted(sort_list), true
  end

end
