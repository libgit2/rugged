module Rugged
  class Index
    include Enumerable

    def to_s
      s = "#<Rugged::Index:0x1024bc2f8>\n"
      self.each do |entry|
        s << "  [#{entry[:stage]}] '#{entry[:path]}'\n"
      end
      s
    end
  end
end
