module Rugged
  class Person
    attr_accessor :name, :email

	def initialize(name, email, time)
		@name = name
		@email = email
		@time = time.to_i
	end

	def time
		Time.at(@time)
	end

	def time=(t)
		@time = t.to_i
	end

  end
end
