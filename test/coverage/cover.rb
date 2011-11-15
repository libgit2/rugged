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

method_list.each do |group, gr_methods|
  gr_found = gr_methods.select {|m| found.include? m}
  puts "#{group} [#{gr_found.size}/#{gr_methods.size}]"

  gr_methods.each do |m|
    puts "  #{m} #{found.include?(m) ? "" : "** MISSING"}"
  end
end

