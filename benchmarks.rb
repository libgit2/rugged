require 'benchmark'
require 'rubygems'
require 'grit'
require 'pp'

dir = File.dirname(File.expand_path(__FILE__))
$LOAD_PATH.unshift dir + '/lib'
require 'ribbit'

n = 5000

sha = "8496071c1b46c854b31185ea97743be6a8774479"
psha = "41bc8c69075bbdb46c5c6f0566cc8cc5b46e8bd9"
raw = Ribbit::Lib.hex_to_raw(sha)

repo = File.dirname(File.expand_path(__FILE__)) + "/test/fixtures/testrepo.git"

psha = "f7c7e7614af3d26a05aeb324528b6d42076c4e6d"
sha = "78af21ad7104d6f64b46cbc87f6ca944b8f412da"
repo = "/Users/schacon/projects/github/.git"

Benchmark.bm do |x|

  @grit = Grit::Repo.new(repo).git
  @ribbit = Ribbit::Odb.new(repo + '/objects')

  x.report("Grit   object_exists? loose ") do
    @grit = Grit::Repo.new(repo).git
    1.upto(n) { @grit.object_exists?(sha) }
  end
  x.report("Grit   object_exists? packed") do
    @grit = Grit::Repo.new(repo).git
    1.upto(n) { @grit.object_exists?(psha) }
  end
  x.report("Ribbit object_exists? loose ") do
    @ribbit = Ribbit::Odb.new(repo + '/objects')
    1.upto(n) { @ribbit.exists(sha) }
  end
  x.report("Ribbit object_exists? packed") do
    @ribbit = Ribbit::Odb.new(repo + '/objects')
    1.upto(n) { @ribbit.exists(psha) }
  end

end

exit

n = 50000

Benchmark.bm do |x|

  x.report("Ribbit Hex2Raw:") do
    1.upto(n) { Ribbit::Lib.hex_to_raw(sha) }
  end
  x.report("Ruby Hex2Raw  :") do
    1.upto(n) { [sha].pack("H*") }
  end
  x.report("Ribbit Raw2Hex:") do
    1.upto(n) { Ribbit::Lib.raw_to_hex(raw) }
  end
  x.report("Ruby Raw2Hex  :") do
    1.upto(n) { raw.unpack("H*")[0] }
  end
end