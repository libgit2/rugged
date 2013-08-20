require "test_helper"

class TagTest < Rugged::TestCase
  include Rugged::RepositoryAccess

  def test_lookup_raises_error_if_object_type_does_not_match
    assert_raises Rugged::InvalidError do
      # blob
      Rugged::Tag.lookup(@repo, "fa49b077972391ad58037050f2a75f74e3671e92")
    end

    assert_raises Rugged::InvalidError do
      # commit
      Rugged::Tag.lookup(@repo, "8496071c1b46c854b31185ea97743be6a8774479")
    end

    assert_raises Rugged::InvalidError do
      # tree
      Rugged::Tag.lookup(@repo, "c4dc1555e4d4fa0e0c9c3fc46734c7c35b3ce90b")
    end

    subclass = Class.new(Rugged::Tag)

    assert_raises Rugged::InvalidError do
      # blob
      subclass.lookup(@repo, "fa49b077972391ad58037050f2a75f74e3671e92")
    end

    assert_raises Rugged::InvalidError do
      # commit
      subclass.lookup(@repo, "8496071c1b46c854b31185ea97743be6a8774479")
    end

    assert_raises Rugged::InvalidError do
      # tree
      subclass.lookup(@repo, "c4dc1555e4d4fa0e0c9c3fc46734c7c35b3ce90b")
    end
  end

  def test_reading_a_tag
    oid = "0c37a5391bbff43c37f0d0371823a5509eed5b1d"
    obj = @repo.lookup(oid)

    assert_equal oid, obj.oid
    assert_equal :tag, obj.type
    assert_equal "test tag message\n", obj.message
    assert_equal "v1.0", obj.name
    assert_equal "5b5b025afb0b4c913b4c338a42934a3863bf3644", obj.target.oid
    assert_equal :commit, obj.target_type
    c = obj.tagger
    assert_equal "Scott Chacon", c[:name]
    assert_equal 1288114383, c[:time].to_i
    assert_equal "schacon@gmail.com", c[:email]
  end

  def test_reading_the_oid_of_a_tag
    oid = "0c37a5391bbff43c37f0d0371823a5509eed5b1d"
    obj = @repo.lookup(oid)

    assert_equal "5b5b025afb0b4c913b4c338a42934a3863bf3644", obj.target_oid
  end
end

class TagWriteTest < Rugged::TestCase
  include Rugged::TempRepositoryAccess

  def test_writing_a_tag
    tag_oid = Rugged::Tag.create(@repo, 'tag', "5b5b025afb0b4c913b4c338a42934a3863bf3644", {
      :message => "test tag message\n",
      :tagger  => { :name => 'Scott', :email => 'schacon@gmail.com', :time => Time.now }
    })

    tag = @repo.lookup(tag_oid)
    assert_equal :tag, tag.type
    assert_equal "5b5b025afb0b4c913b4c338a42934a3863bf3644", tag.target.oid
    assert_equal "test tag message\n", tag.message
    assert_equal "Scott", tag.tagger[:name]
    assert_equal "schacon@gmail.com", tag.tagger[:email]
  end

  def test_writing_a_tag_without_signature
    name = 'Rugged User'
    email = 'rugged@example.com'
    @repo.config['user.name'] = name
    @repo.config['user.email'] = email

    tag_oid = Rugged::Tag.create(@repo,
      :name    => 'tag',
      :message => "test tag message\n",
      :target  => "5b5b025afb0b4c913b4c338a42934a3863bf3644")

    tag = @repo.lookup(tag_oid)
    assert_equal name, tag.tagger[:name]
    assert_equal email, tag.tagger[:email]
  end

  def test_tag_invalid_message_type
    assert_raises TypeError do
      Rugged::Tag.create(@repo, 'tag', "5b5b025afb0b4c913b4c338a42934a3863bf3644", {
        :message => :invalid_message,
        :tagger  => {:name => 'Scott', :email => 'schacon@gmail.com', :time => Time.now }
      })
    end
  end

  def test_writing_light_tags
    Rugged::Tag.create(@repo, 'tag', "5b5b025afb0b4c913b4c338a42934a3863bf3644")

    tag = Rugged::Reference.lookup(@repo, "refs/tags/tag")
    assert_equal "5b5b025afb0b4c913b4c338a42934a3863bf3644", tag.target
  end
end
