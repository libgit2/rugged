dir = File.dirname(File.expand_path(__FILE__))
$LOAD_PATH.unshift dir + '/../lib'
$TESTING = true
require 'test/unit'
require 'rubygems'
require 'rugged'
require 'pp'

##
# test/spec/mini 3
# http://gist.github.com/25455
# chris@ozmm.org
#
def context(*args, &block)
  return super unless (name = args.first) && block
  require 'test/unit'
  klass = Class.new(defined?(ActiveSupport::TestCase) ? ActiveSupport::TestCase : Test::Unit::TestCase) do
    def self.test(name, &block)
      define_method("test_#{name.gsub(/\W/,'_')}", &block) if block
    end
    def self.xtest(*args) end
    def self.setup(&block) define_method(:setup, &block) end
    def self.teardown(&block) define_method(:teardown, &block) end
  end
  (class << klass; self end).send(:define_method, :name) { name.gsub(/\W/,'_') }
  klass.class_eval &block
  ($contexts ||= []) << klass # make sure klass doesn't get GC'd
  klass
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
