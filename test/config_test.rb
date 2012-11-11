require "test_helper"

describe Rugged::Config do
  before do
    @repo = Rugged::Repository.new(temp_repo('testrepo.git'))
  end

  it "can read the config file from repo" do
    config = @repo.config
    assert_equal 'false', config['core.bare']
    assert_nil config['not.exist']
  end

  it "can read the config file from path" do
    config = Rugged::Config.new(File.join(@repo.path, 'config'))
    assert_equal 'false', config['core.bare']
  end

  it "can read the global config file" do
    config = Rugged::Config.global
    assert config['user.name'] != nil
    assert_nil config['core.bare']
  end

  it "can write config values" do
    config = @repo.config
    config['custom.value'] = 'my value'

    config2 = @repo.config
    assert_equal 'my value', config2['custom.value']

    content = File.read(File.join(@repo.path, 'config'))
    assert_match(/value = my value/, content)
  end

  it "can delete config values" do
    config = @repo.config
    config.delete('core.bare')

    config2 = @repo.config
    assert_nil config2.get('core.bare')
  end

end
