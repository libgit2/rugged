class Rugged
  class TreeEntry

    def <=>(a)
      self.name <=> a.name
    end

  end
end
