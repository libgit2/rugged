require 'tempfile'
require 'tmpdir'
require 'rubygems'
require 'minitest/autorun'
require 'rugged'
require 'pp'

# backwards compat with test/spec/mini 3
alias :context :describe

module Rugged
  class TestCase < MiniTest::Spec
    TEST_DIR = File.dirname(File.expand_path(__FILE__))

    class << self
      # backwards compat with test/spec/mini 3
      alias :setup :before
      alias :teardown :after
      alias :test :it
    end

    # backwards compat with test/unit
    alias :assert_not_nil :refute_nil
    alias :assert_raise :assert_raises

    private

    def temp_repo(repo)
      dir = Dir.mktmpdir 'dir'
      repo_dir = File.join(TEST_DIR, (File.join('fixtures', repo, '.')))
      `git clone #{repo_dir} #{dir}`
      dir
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
