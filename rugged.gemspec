require './lib/rugged/version'

Gem::Specification.new do |s|
  s.name              = "rugged"
  s.version           = Rugged::Version
  s.date              = Time.now.strftime('%Y-%m-%d')
  s.summary           = "Rugged is a Ruby binding to the libgit2 linkable library"
  s.homepage          = "http://github.com/libgit2/rugged"
  s.email             = "schacon@gmail.com"
  s.authors           = [ "Scott Chacon", "Vicent Marti" ]
  s.files             = %w( README.md Rakefile LICENSE )
  s.files            += Dir.glob("lib/**/*.rb")
  s.files            += Dir.glob("man/**/*")
  s.files            += Dir.glob("test/**/*")
  s.files            += Dir.glob("ext/**/*.c") + Dir.glob("ext/**/*.h")
  s.extensions        = ['ext/rugged/extconf.rb']
  s.description       = <<desc
Rugged is a Ruby bindings to the libgit2 linkable C Git library. This is
for testing and using the libgit2 library in a language that is awesome.
desc
end
