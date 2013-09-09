$:.push File.expand_path("../lib", __FILE__)
require 'rugged/version'

Gem::Specification.new do |s|
  s.name                  = "rugged"
  s.version               = Rugged::Version
  s.date                  = Time.now.strftime('%Y-%m-%d')
  s.summary               = "Rugged is a Ruby binding to the libgit2 linkable library"
  s.homepage              = "http://github.com/libgit2/rugged"
  s.email                 = "schacon@gmail.com"
  s.authors               = [ "Scott Chacon", "Vicent Marti" ]
  s.license               = "MIT"
  s.files                 = %w( README.md LICENSE )
  s.files                 += Dir.glob("lib/**/*.rb")
  s.files                 += Dir.glob("ext/**/*.[ch]")
  s.files                 += Dir.glob("vendor/libgit2/{include,src,deps}/**/*.[ch]")
  s.files                 += Dir.glob("vendor/libgit2/{Makefile.embed,AUTHORS,COPYING}")
  s.extensions            = ['ext/rugged/extconf.rb']
  s.required_ruby_version = '>= 1.9.3'
  s.description           = <<desc
Rugged is a Ruby bindings to the libgit2 linkable C Git library. This is
for testing and using the libgit2 library in a language that is awesome.
desc
  s.add_development_dependency "rake-compiler", ">= 0.9.0"
  s.add_development_dependency "pry"
  s.add_development_dependency "minitest", "~> 3.0.0"
end
