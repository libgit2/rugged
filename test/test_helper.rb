require 'rubygems'

require 'tempfile'
require 'tmpdir'
require 'minitest/autorun'
require 'rugged'
require 'pp'

module Rugged
  class TestCase < MiniTest::Spec
    TEST_DIR = File.dirname(File.expand_path(__FILE__))

    private

    def temp_repo(repo)
      dir = Dir.mktmpdir 'dir'
      repo_dir = File.join(TEST_DIR, (File.join('fixtures', repo, '.')))
      `git clone #{repo_dir} #{dir}`
      dir
    end

    def destroy_temp_repo(path)
      FileUtils.remove_entry_secure(path)
    end

    def rm_loose(oid)
      base_path = File.join(@path, "objects", oid[0, 2])

      file = File.join(base_path, oid[2, 38])
      dir_contents = File.join(base_path, "*")

      File.delete(file)

      if Dir[dir_contents].empty?
        Dir.delete(base_path)
      end
    end

    def with_default_encoding(encoding, &block)
      old_encoding = Encoding.default_internal

      new_encoding = Encoding.find(encoding)
      Encoding.default_internal = new_encoding

      yield new_encoding

      Encoding.default_internal = old_encoding
    end
  end
end

MiniTest::Spec.register_spec_type(/./, Rugged::TestCase)
