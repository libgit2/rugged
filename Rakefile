# stolen largely from defunkt/mustache
require 'rake/testtask'
require 'rake/rdoctask'

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

#
# Tests
#

task :default => :test

desc "Run tests"
task :turn do
  suffix = "-n #{ENV['TEST']}" if ENV['TEST']
  sh "turn test/*_test.rb #{suffix}"
end

Rake::TestTask.new do |t|
  t.libs << 'lib'
  t.pattern = 'test/**/*_test.rb'
  t.verbose = false
end

#
# Gems
#
begin
  require 'mg'
  MG.new("rugged.gemspec")

  desc "Push a new version to Gemcutter and publish docs."
  task :publish => "gem:publish" do
    require File.dirname(__FILE__) + '/lib/mustache/version'

    system "git tag v#{Rugged::Version}"
    sh "git push origin master --tags"
    sh "git clean -fd"
    exec "rake pages"
  end
rescue LoadError
end

begin
  require 'rcov/rcovtask'
  Rcov::RcovTask.new do |t|
    t.test_files = FileList['test/test_*.rb']
    # t.verbose = true     # uncomment to see the executed command
  end
rescue LoadError
end

#
# Documentation
#

desc "Publish to GitHub Pages"
task :pages => [ "man:build" ] do
  Dir['man/*.html'].each do |f|
    cp f, File.basename(f).sub('.html', '.newhtml')
  end

  `git commit -am 'generated manual'`
  `git checkout site`

  Dir['*.newhtml'].each do |f|
    mv f, f.sub('.newhtml', '.html')
  end

  `git add .`
  `git commit -m updated`
  `git push site site:master`
  `git checkout master`
  puts :done
end
