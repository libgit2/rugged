require 'rake/testtask'

begin
  require 'rake/extensiontask'
rescue LoadError
  abort <<-error
  rake-compile is missing; Rugged depends on rake-compiler to build the C wrapping code.

  Install it by running `gem i rake-compiler`
error
end

Rake::ExtensionTask.new('rugged') do |r|
  r.lib_dir = 'lib/rugged'
end

desc "checkout libgit2 source"
task :checkout do
  if !ENV['LIBGIT2_DEV']
    sh "git submodule update --init"
  end
end
Rake::Task[:compile].prerequisites.insert(0, :checkout)

task :embedded_clean do
  lib_path = File.expand_path '../ext/rugged/libgit2_embed.a', __FILE__
  system "rm #{lib_path}"
end
Rake::Task[:clean].prerequisites << :embedded_clean

desc "Open an irb session preloaded with Rugged"
task :console do
  exec "script/console"
end

#
# Tests
#
task :default => [:compile, :test]

task :cover do
  ruby 'test/coverage/cover.rb'
end

Rake::TestTask.new do |t|
  t.libs << 'lib:test'
  t.pattern = 'test/**/*_test.rb'
  t.verbose = false
  t.warning = true
end

begin
  require 'rdoc/task'
  Rake::RDocTask.new do |rdoc|
    rdoc.rdoc_dir = 'rdoc'
    rdoc.rdoc_files.include('ext/**/*.c')
    rdoc.rdoc_files.include('lib/**/*.rb')
  end
rescue LoadError
end

