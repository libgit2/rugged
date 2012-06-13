require "test_helper"

context "Rugged::Config tests" do
  setup do
    @repo = Rugged::Repository.new(temp_repo('testrepo.git'))
  end

  test "can read the config file from repo" do
    config = @repo.config
    assert_equal 'false', config['core.bare']
    assert_nil config['not.exist']
  end

  test "can read the config file from path" do
    config = Rugged::Config.new(File.join(@repo.path, 'config'))
    assert_equal 'false', config['core.bare']
  end

  test "can read the global config file" do
    config = Rugged::Config.open_global
    assert_not_nil config['user.name']
    assert_nil config['core.bare']
  end

  test "can write config values" do
    config = @repo.config
    config['custom.value'] = 'my value'

    config2 = @repo.config
    assert_equal 'my value', config2['custom.value']

    content = File.read(File.join(@repo.path, 'config'))
    assert_match(/value = my value/, content)
  end

  test "can delete config values" do
    config = @repo.config
    config.delete('core.bare')

    config2 = @repo.config
    assert_nil config2.get('core.bare')
  end

end
