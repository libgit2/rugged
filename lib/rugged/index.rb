module Rugged
  class Index
    include Enumerable

    def each
      entries = self.entry_count
      0.upto(entries - 1) do |i|
        yield self[i]
      end
    end
  end
    
  class IndexEntry
    def valid?
      (flags & FLAGS_VALID) != 0
    end
  end

end
