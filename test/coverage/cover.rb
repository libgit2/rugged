require 'json'
require 'set'

CWD = File.expand_path(File.dirname(__FILE__))

IGNORE_METHOD = %w(
  git_blob_free
  git_blob_lookup
  git_blob_lookup_prefix
  git_commit_create_v
  git_commit_free
  git_commit_id
  git_commit_lookup_prefix
  git_commit_parent_oid
  git_commit_time_offset
  git_commit_tree_oid
  git_config_file__ondisk
  git_config_find_global
  git_config_find_system
  git_index_entry_stage
  imaxdiv
  git_object__size
  git_odb_add_alternate
  git_odb_add_backend
  git_odb_backend_loose
  git_odb_backend_pack
  git_odb_new
  git_odb_open_rstream
  git_odb_read_header
  git_odb_read_prefix
  git_odb_write
  git_oid_allocfmt
  git_oid_cpy
  git_oid_ncmp
  git_oid_pathfmt
  git_oid_streq
  git_oid_to_string
  git_reference_owner
  git_repository_odb
  git_repository_set_odb
  git_signature_dup
  git_signature_now
  git_tag_free
  git_tag_id
  git_tag_lookup
  git_tag_lookup_prefix
  git_tag_target_oid
  git_tree_free
  git_tree_id
  git_tree_lookup
  git_tree_lookup_prefix
  git_reference_listall
  git_reflog_delete
  git_reflog_rename
)

source_files = Dir.glob("#{CWD}/../../ext/rugged/*.{c,h}")
method_list = nil
look_for = []
found = Set.new

File.open("#{CWD}/HEAD.json") do |f|
  json_data = JSON.parse(f.read())
  method_list = json_data['groups']
end

method_list.each do |_, methods|
  methods.reject! { |m| IGNORE_METHOD.include? m }
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
