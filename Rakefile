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

task :embedded_clean do
  lib_path = File.expand_path '../ext/rugged/libgit2_embed.a', __FILE__
  system "rm #{lib_path}"
end
Rake::Task[:clean].prerequisites << :embedded_clean

desc "Open an irb session preloaded with Rugged"
task :console do
  sh "irb -rubygems -I lib -r ./lib/rugged"
end

#
# Tests
#
task :default => [:compile, :test]

task :pack_dist do
  dir = File.dirname(File.expand_path(__FILE__))
  output = File.join(dir, 'ext', 'rugged', 'vendor', 'libgit2-dist.tar.gz')
  Dir.chdir(ENV['LIBGIT2_PATH']) do
    `git archive --format=tar --prefix=libgit2-dist/ HEAD | gzip > #{output}`
  end
end

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

