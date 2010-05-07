require 'lib/ribbit/version'

Gem::Specification.new do |s|
  s.name              = "ribbit"
  s.version           = Ribbit::Version
  s.date              = Time.now.strftime('%Y-%m-%d')
  s.summary           = "Ribbit is a Ruby binding to the libgit2 linkable library"
  s.homepage          = "http://github.com/schacon/ribbit"
  s.email             = "schacon@gmail.com"
  s.authors           = [ "Scott Chacon" ]
  s.files             = %w( README API Rakefile LICENSE )
  s.files            += Dir.glob("lib/**/*")
  s.files            += Dir.glob("man/**/*")
  s.files            += Dir.glob("test/**/*")
  s.extensions        = ['ext/ribbit/extconf.rb']
  s.description       = <<desc
Ribbit is a Ruby bindings to the libgit2 linkable C Git library. This is
for testing and using the libgit2 library in a language that is awesome.
desc
end
