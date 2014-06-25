module Rugged
  class Remote
    def self.each(repo, *args)
      warn "DEPRECATION WARNING: Rugged::Remote.each is deprecated and will be removed."
      repo.remotes.each(*args)
    end

    def self.new(repo, *args)
      warn "DEPRECATION WARNING: Rugged::Remote.new is deprecated and will be removed."
      repo.remotes.create_anonymous(*args)
    end

    def self.add(repo, *args)
      warn "DEPRECATION WARNING: Rugged::Remote.add is deprecated and will be removed."
      repo.remotes.create(*args)
    end

    def self.lookup(repo, name)
      warn "DEPRECATION WARNING: Rugged::Remote.lookup is deprecated and will be removed."
      repo.remotes[name]
    end

    def self.names(repo)
      warn "DEPRECATION WARNING: Rugged::Remote.names is deprecated and will be removed."
      repo.remotes.each_name.to_a
    end
  end
end
