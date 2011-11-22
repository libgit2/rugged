require 'json'
require 'set'

CWD = File.expand_path(File.dirname(__FILE__))

source_files = Dir.glob("#{CWD}/../../ext/rugged/*.{c,h}")
method_list = nil
look_for = []
found = Set.new

File.open("#{CWD}/HEAD.json") do |f|
  json_data = JSON.parse(f.read())
  method_list = json_data['groups']
end

method_list.each do |_, methods|
  look_for += methods
end

source_files.each do |file|
  File.open(file) do |f|
    contents = f.read()
    look_for.each do |method|
      if contents.index(method) != nil
        found.add(method)
      end
    end
  end
end

total_missing = 0
total_methods = 0

method_list.each do |group, gr_methods|
  gr_miss = gr_methods.reject {|m| found.include? m}
  print "#{group} [#{gr_methods.size - gr_miss.size}/#{gr_methods.size}]: "

  total_missing += gr_miss.size
  total_methods += gr_methods.size

  gr_methods.each do |m|
    print found.include?(m) ? "." : "M"
  end

  print "\n"

  if not gr_miss.empty?
    print "    Missing: #{gr_miss.join(', ')}\n"
  end

  print "\n"
end

puts "TOTAL: [#{total_methods - total_missing}/#{total_methods}] wrapped. (#{100.0 * (total_methods - total_missing)/total_methods}%)"
