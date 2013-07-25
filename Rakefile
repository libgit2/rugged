require 'rake/testtask'

begin
  require 'rake/extensiontask'
rescue LoadError
  abort <<-error
  rake-compile is missing; Rugged depends on rake-compiler to build the C wrapping code.

  Install it by running `gem i rake-compiler`
error
end

begin
  mingw_available = true
  require 'rake/extensioncompiler'
  Rake::ExtensionCompiler.mingw_host
rescue
  mingw_available = false
end

gemspec = Gem::Specification::load(File.expand_path('../rugged.gemspec', __FILE__))
Rake::ExtensionTask.new('rugged', gemspec) do |r|
  r.lib_dir = 'lib/rugged'
  if mingw_available
    lg2_dir = File.expand_path("../tmp/#{Rake::ExtensionCompiler.mingw_host}", __FILE__)

    r.cross_compile = true
    r.cross_config_options << "--with-git2-lib=#{lg2_dir}"
  end
end

desc "checkout libgit2 source"
task :checkout do
  if !ENV['LIBGIT2_DEV']
    sh "git submodule update --init"
  end
end
Rake::Task[:compile].prerequisites.insert(0, :checkout)


if mingw_available
  namespace :cross do
    task :libgit2 => :embedded_clean do
      Dir.chdir("vendor/libgit2") do
        sh "CROSS_COMPILE=#{Rake::ExtensionCompiler.mingw_host} make -f Makefile.embed"
      end

      FileUtils.mkdir_p "tmp/#{Rake::ExtensionCompiler.mingw_host}"
      FileUtils.cp 'vendor/libgit2/libgit2.a', File.expand_path("../tmp/#{Rake::ExtensionCompiler.mingw_host}", __FILE__)
    end
  end

  Rake::Task[:cross].prerequisites.insert(0, "cross:libgit2")
else
  task :cross do
    abort "No MinGW tools or unknown setup platform?"
  end
end

task :embedded_clean do
  Dir.chdir("vendor/libgit2") do
    sh "make -f Makefile.embed clean"
  end
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
  t.libs << 'lib' << 'test'
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

