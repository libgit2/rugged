#
# Some example proposed usages of Rugged
#  so we can make sure the API design is OK
#

#
# low level reading and writing
#

repo = Rugged::Repository.new(path_to_repo)
if repo.exists(blob_oid)
  # reading
  blob_data, type = repo.read(blob_oid)
  blob = repo.lookup(blob_oid)
  puts blob.data
  assert blob.data == blob_data && type == 'blob'

  # writing
  blob_data += "FU, SPIDERMAN 3"

  new_oid = repo.write(blob_data, blob)

  blob.data(blob_data)
  assert !blob.written?
  new_oid2 = blob.write
  assert new_oid == new_oid2
  assert blob.written?
end

# 
# getting the contents of a blob with commit and path
#

commit = repo.lookup(commit_oid)
commit.tree.each do |entry|
  if (entry.name == path) 
    && (entry.type == 'blob')
    return entry.gobect.data
  end
end

# 
# getting commit data from oid
#

commit = repo.lookup(commit_oid, 'commit')
puts "oid: " + commit.oid
puts commit.author.name
puts commit.author.email
puts commit.author.date.strftime("%y-%m-%d")
puts
puts commit.message


# 
# listing a single level tree
#

commit = repo.lookup(commit_oid)
commit.tree.each do |entry|
  puts entry.name
end


# 
# git ls-tree -rt
#

def ls_tree(tree_oid)
  tree = repo.lookup(tree_oid, 'tree')
  tree.each do |entry|
    puts [entry.mode, entry.oid, entry.name].join(' ')
    if entry.type == 'tree'
      ls_tree(entry.oid)
    end
  end
end

commit = repo.lookup(commit_oid)
ls_tree(tree_oid)

# 
# updating a file and committing
#

commit = repo.lookup(head_oid)
tree = commit.tree
entry = tree.path("file.txt")
blob = entry.gobject
blob.data(blob.data + "EXTRA")
boid = blob.write
tree.add(entry.oid(boid))
toid = tree.write

new_commit = Rugged::Commit.new
person = Rugged::Person.new("Scott", "scott@github.com", Time.now)
commit.author(person)
commit.message("updated file.txt")
commit.parents([head_oid])
commit.tree(toid)
commit_oid = commit.write

repo.update_ref('refs/heads/master', commit_oid)

#
# git log
#

walker = repo.walker
walker.push(head_oid)
while oid = walker.next
  commit = repo.lookup(oid)
  puts [commit.oid[0, 8], commit.short_message].join(' ')
end

#
# git log --since=1.week.ago A ^B
#

since_time = Time.parse("1 week ago")

walker = repo.walker
walker.push(a_oid)
walker.hide(b_oid)
while oid = walker.next
  commit = repo.lookup(oid)
  if commit.author.date > since_time
    walker.hide(oid)
  else
    puts [commit.oid[0, 8], commit.short_message].join(' ')
  end
end

#
# git log --all
#

walker = repo.walker
repo.branches(:heads).each do |branch|
  walker.push(branch.oid)
end
while oid = walker.next
  ...
end


#
# finding the last commit that touched a file
#

walker = repo.walker
walker.push(head_oid)
while oid = walker.next
  commit = repo.lookup(oid)
  diff = commit.diff_tree(commit.parents[0])
  if diff.include?(path)
    return commit.oid
  end
end





