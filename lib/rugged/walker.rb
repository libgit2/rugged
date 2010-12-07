module Rugged
  class Walker
    include Enumerable

    def each
      while commit = self.next
        yield commit
      end
    end

  end
end
