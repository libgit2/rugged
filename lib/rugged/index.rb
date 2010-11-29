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
    def assume_valid
      flags & 0x01 == 0x01
    end

    def update_needed
      flags & 0x02 == 0x02
    end

    def stage
      (flags & 0x0c) >> 2
    end
  end
end
