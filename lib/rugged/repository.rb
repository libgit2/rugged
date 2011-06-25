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

    def lookup(sha1)
      Rugged::Object.lookup(self, sha1)
    end
  end

end
