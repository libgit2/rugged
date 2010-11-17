#
# Some example proposed usages of Rugged
#  so we can make sure the API design is OK
#

#
# low level reading and writing
#

repo = Rugged::Repository.new(path_to_repo)
if repo.exists(blob_sha)
  # reading
  blob_data, type = repo.read(blob_sha)
  blob = repo.lookup(blob_sha)
  puts blob.data
  assert blob.data == blob_data && type == 'blob'

  # writing
  blob_data += "FU, SPIDERMAN 3"

  new_sha = repo.write(blob_data, blob)

  blob.data(blob_data)
  assert !blob.written?
  new_sha2 = blob.write
  assert new_sha == new_sha2
  assert blob.written?
end

# 
# getting the contents of a blob with commit and path
#

commit = repo.lookup(commit_sha)
commit.tree.each do |entry|
  if (entry.name == path) 
    && (entry.type == 'blob')
    return entry.gobect.data
  end
end

# 
# getting commit data from sha
#

commit = repo.lookup(commit_sha, 'commit')
puts "SHA: " + commit.sha
puts commit.author.name
puts commit.author.email
puts commit.author.date.strftime("%y-%m-%d")
puts
puts commit.message


# 
# listing a single level tree
#

commit = repo.lookup(commit_sha)
commit.tree.each do |entry|
  puts entry.name
end


# 
# git ls-tree -rt
#

def ls_tree(tree_sha)
  tree = repo.lookup(tree_sha, 'tree')
  tree.each do |entry|
    puts [entry.mode, entry.sha, entry.name].join(' ')
    if entry.type == 'tree'
      ls_tree(entry.sha)
    end
  end
end

commit = repo.lookup(commit_sha)
ls_tree(tree_sha)

# 
# updating a file and committing
#

commit = repo.lookup(head_sha)
tree = commit.tree
entry = tree.path("file.txt")
blob = entry.gobject
blob.data(blob.data + "EXTRA")
bsha = blob.write
tree.add(entry.sha(bsha))
tsha = tree.write

new_commit = Rugged::Commit.new
person = Rugged::Person.new("Scott", "scott@github.com", Time.now)
commit.author(person)
commit.message("updated file.txt")
commit.parents([head_sha])
commit.tree(tsha)
commit_sha = commit.write

repo.update_ref('refs/heads/master', commit_sha)

#
# git log
#

walker = repo.walker
walker.push(head_sha)
while sha = walker.next
  commit = repo.lookup(sha)
  puts [commit.sha[0, 8], commit.short_message].join(' ')
end

#
# git log --since=1.week.ago A ^B
#

since_time = Time.parse("1 week ago")

walker = repo.walker
walker.push(a_sha)
walker.hide(b_sha)
while sha = walker.next
  commit = repo.lookup(sha)
  if commit.author.date > since_time
    walker.hide(sha)
  else
    puts [commit.sha[0, 8], commit.short_message].join(' ')
  end
end

#
# git log --all
#

walker = repo.walker
repo.branches(:heads).each do |branch|
  walker.push(branch.sha)
end
while sha = walker.next
  ...
end


#
# finding the last commit that touched a file
#

walker = repo.walker
walker.push(head_sha)
while sha = walker.next
  commit = repo.lookup(sha)
  diff = commit.diff_tree(commit.parents[0])
  if diff.include?(path)
    return commit.sha
  end
end





