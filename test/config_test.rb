require File.expand_path "../test_helper", __FILE__

context "Rugged::Config tests" do
  setup do
    @path = File.dirname(__FILE__) + '/fixtures/testrepo.git/'
    @repo = Rugged::Repository.new(@path)
  end

  xtest "can read the commit data" do
    #pp config = Rugged::Config.new(File.join(@repo.path, 'config'))
  end
end
