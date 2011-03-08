module Rugged

  class Repository
    def walk(from, sorting=Rugged::SORT_DATE)
      walker = Rugged::Walker.new(self)
      walker.sorting(sorting)
      walker.push(from)

      while commit = walker.next
        yield commit
      end
    end

    def head
      ref = Reference.lookup(self, "HEAD")
      ref.resolve
    end
  end

end
