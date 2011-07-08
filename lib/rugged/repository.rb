module Rugged

  class Repository
    def walk(from, sorting=Rugged::SORT_DATE, &block)
      walker = Rugged::Walker.new(self)
      walker.sorting(sorting)
      walker.push(from)
      walker.each(&block)
    end

    def head
      ref = Reference.lookup(self, "HEAD")
      ref.resolve
    end

    def lookup(oid)
      Rugged::Object.lookup(self, oid)
    end

    def refs
      Rugged::Reference.each(self)
    end

    def tags(pattern="")
      Rugged::Tag.each(self, pattern)
    end
  end

end
