require 'rake/testtask'

begin
  require 'rake/extensiontask'
rescue LoadError
  abort <<-error
  rake-compile is missing; Rugged depends on rake-compiler to build the C wrapping code.

  Install it by running `gem i rake-compiler`
error
end

gemspec = Gem::Specification::load(File.expand_path('../rugged.gemspec', __FILE__))

Gem::PackageTask.new(gemspec) do |pkg|
end

Rake::ExtensionTask.new('rugged', gemspec) do |r|
  r.lib_dir = 'lib/rugged'

  r.cross_platform = ["i386-mingw32", "x64-mingw32"]
  r.cross_compile = true

  if ruby_vers = ENV['RUBY_CC_VERSION']
    ruby_vers = ENV['RUBY_CC_VERSION'].split(':')
  else
    ruby_vers = [RUBY_VERSION]
  end

  Array(r.cross_platform).each do |platf|
    # Use rake-compilers config.yml to determine the toolchain that was used
    # to build Ruby for this platform.
    host_platform = begin
      config_file = YAML.load_file(File.expand_path("~/.rake-compiler/config.yml"))
      _, rbfile = config_file.find{ |key, fname| key.start_with?("rbconfig-#{platf}-") }
      IO.read(rbfile).match(/CONFIG\["CC"\] = "(.*)"/)[1].sub(/\-gcc/, '')
    rescue
      nil
    end

    ruby_vers.each do |ruby_ver|
      file "#{r.tmp_dir}/#{platf}/rugged/#{ruby_ver}/#{r.binary(platf)}" => "compile:libgit2:#{platf}"
    end

    r.cross_config_options << {
      platf => [
        "--with-git2-lib=#{File.expand_path(r.tmp_dir)}/#{platf}/libgit2",
        "--with-git2-include=#{File.expand_path(".")}/vendor/libgit2/include"
      ]
    }

    task "compile:libgit2:#{platf}" do
      Dir.chdir("vendor/libgit2") do
        old_value, ENV["CROSS_COMPILE"] = ENV["CROSS_COMPILE"], host_platform
        begin
          sh "make -f Makefile.embed clean"
          sh "make -f Makefile.embed"
        ensure
          ENV["CROSS_COMPILE"] = old_value
        end
      end

      FileUtils.mkdir_p "#{r.tmp_dir}/#{platf}/libgit2"
      FileUtils.cp 'vendor/libgit2/libgit2.a', "#{r.tmp_dir}/#{platf}/libgit2/libgit2.a"
    end
  end
end

desc "checkout libgit2 source"
task :checkout do
  if !ENV['CI_BUILD']
    sh "git submodule update --init"
  end
end
Rake::Task[:compile].prerequisites.insert(0, :checkout)

namespace :clean do
  task :libgit2 do
    FileUtils.rm_rf("vendor/libgit2/build")
  end
end
Rake::Task[:clean].prerequisites << "clean:libgit2"

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

