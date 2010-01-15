# Helper Rakefile for liquid project
# Author: Santa Zhang

require 'find'

load "rake.gen.conf"

$g_depend = {}

def has_main_entry? file
  File.open(file).read =~ /^int main\(/
end

# finds all depended obj file, including inherit dependency
def depended_obj file
  visited = []
  not_visited = depended_header file
  
  loop do
    break if not_visited.length == 0
    header = not_visited[0]
    c_src = header[0..-3] + ".c"
    depended_header(c_src).each do |h|
      (not_visited << h) if not (not_visited.include? h or visited.include? h)
    end
    not_visited.delete header
    visited << header
  end

  obj_list = visited.map do |header|
    "obj/#{header[0..-3]}.o"
  end

  return obj_list
end

# finds all directly depended header files, without inherit dependency
def depended_header file
  list = []
  File.open(file[0..-3] + ".c") do |src_f|
    src_f.each_line do |line|
      line = line.strip
      if line =~ /#include \"/
        header = line[10..-2]
        list.concat(all_src_files.select {|f| f.end_with? header})
      end
    end
  end
  if File.exist? file[0..-3] + ".h"
    File.open(file[0..-3] + ".h") do |src_f|
      src_f.each_line do |line|
        line = line.strip
        if line =~ /#include \"/
          header = line[10..-2]
          list.concat(all_src_files.select {|f| f.end_with? header})
        end
      end
    end
  end
  list = list.uniq
  return list
end

def all_targets_in_module mod
  depend = {mod => ["bin", "obj"]}
  action = {mod => []}

  all_obj_list = []
  all_bin_list = [] # all the executable file list

  Find.find(mod) do |path|
    next if FileTest.directory? path
    if path =~ /\.c$/
      obj_name = "obj/#{path[0..-3] + ".o"}"
      all_obj_list << obj_name
      if has_main_entry? path
        bin_name = "bin/#{(File.basename path)[0..-3]}"
        all_bin_list << bin_name
        depend[mod] << bin_name

        depend[bin_name] = [] if depend[bin_name] == nil
        depend[bin_name] << obj_name
        depend[bin_name] << (depended_obj path)

        action[bin_name] = [] if action[bin_name] == nil
        action[bin_name] << "$(CC) $(CFLAGS) $(LDFLAGS) #{obj_name} #{(depended_obj path).join " "} -o #{bin_name}"
      end

      depend[mod] << obj_name

      depend[obj_name] = [] if depend[obj_name] == nil
      depend[obj_name] << path
      depend[obj_name] << (depended_header path)

      action[obj_name] = [] if action[obj_name] == nil
      action[obj_name] << "$(CC) $(CFLAGS) -c #{path} -o #{obj_name}"
    end
  end

  if LIB_MODULES.include? mod
    # make .a file
    lib_name = "bin/lib#{mod}.a"
    depend[mod] << lib_name

    depend[lib_name] = [] if depend[lib_name] == nil
    depend[lib_name] << all_obj_list

    action[lib_name] = [] if action[lib_name] == nil
    action[lib_name] << "ar cq #{lib_name} $^"
    action[lib_name] << "ranlib #{lib_name}"
    action[lib_name] << ""
  end

  if TEST_MODULES.include? mod
    # add a run#{mod} target
    run_mod = "run#{mod}"
    depend[run_mod] = [] if depend[run_mod] == nil
    depend[run_mod] << mod

    action[run_mod] = [] if action[run_mod] == nil
    action[run_mod] << "@clear"

    echo_info = "Running test module \\'#{mod}\\'"
    action[run_mod] << "@echo #{echo_info}"
    action[run_mod] << "@echo #{"=" * (echo_info.length - 2)}"

    test_case_counter = 1
    all_bin_list.each do |e|
      echo_info =  "#{test_case_counter} of #{all_bin_list.length}: Running test case \\'#{File.basename e}\\'"
      action[run_mod] << "@echo #{echo_info}"
      action[run_mod] << "@echo #{"-" * (echo_info.length - 2)}"
      action[run_mod] << "@#{e}"
      action[run_mod] << "@echo #{"-" * (echo_info.length - 2)}"
      action[run_mod] << "@echo"
      test_case_counter += 1
    end
  end

  content = ""
  depend.keys.each do |target|
    $g_depend[target] = depend[target]
    content += <<CONTENT
#{target}: #{depend[target].join " "}
#{action[target].collect {|act| "\t#{act}\n"}}
CONTENT
  end

  return content
end

def gen_make_targets
  content = ""
  BUILD_MODULES.each do |mod|
    content += all_targets_in_module mod
    content += "\n"
  end

  content += "all: bin obj #{LIB_MODULES.collect {|mod| "bin/lib#{mod}.a "}}"
  $g_depend.keys.each do |target|
    if target =~ /\.o$/ or target =~ /^bin\//
      content += target + " "
    end
  end
  content += "\n\n"
  return content
end

# return a list of all source files (.h, .c)
# like find . -iname *.h && find . -iname *.c
$g_all_src_files = nil
def all_src_files
  return $g_all_src_files if $g_all_src_files != nil
  $g_all_src_files = []
  BUILD_MODULES.each do |mod|
    Find.find(mod) do |path|
      if FileTest.directory? path
        # do nothing
      elsif path =~ /\.(c|h)$/
        $g_all_src_files << path
      end
    end
  end
  return $g_all_src_files
end

#return a list of all directories in source folders
def mk_obj_dirs_list
  list = []
  BUILD_MODULES.each do |mod|
    Find.find(mod) do |path|
      if FileTest.directory? path
        list << path
      end
    end
  end
  list = list.sort
  min_list = []
  (0..(list.length - 2)).each do |idx|
    a = list[idx]
    b = list[idx + 1]
    if (b.start_with? a) == false
      min_list << a
    end
    if idx == list.length - 2
      min_list << b
    end
  end
  return min_list
end

# assert that there is no duplicate file name in any folder
# note that folder1/a.c and folder2/a.c are duplicated!
def assert_no_duplicate_fname
  inverse_map = {}
  all_src_files.each do |path|
    base_name = File.basename path
    if inverse_map.has_key? base_name
      raise "Duplicate file name: '#{path}' & '#{inverse_map[base_name]}'"
    end
    inverse_map[base_name] = path
  end
end

# ensure that every .c file either has an associated .h file, or they have main() entry
def assert_src_header_pair
  flist = all_src_files
  flist.each do |path|
    if path =~ /\.c$/
      header_path = path[0..-3] + ".h"
      if has_main_entry? path
        # has main() entry, so there should not be a .h file
        raise "'#{path}' already have main() entry, so '#{header_path}' is not needed!" if flist.include? header_path
      else
        # does not have main() entry, so it must have a .h file
        raise "'#{path}' found, but '#{header_path}' does not exist!" unless flist.include? header_path
      end
    end
  end
end

desc "Do style checking"
task :check do
  puts "Checking project..."
  puts "List of files to be checked:"
  all_src_files.each do |f|
    puts "  #{f}"
  end
  puts "--"
  assert_no_duplicate_fname
  puts "No duplicate file name"
  assert_src_header_pair
  puts "All .c files either has got a .h file, or they have main() entry"
# TODO style checking
  puts "Finished checking"
end

desc "Generate Makefile & include files"
task :gen => :check do
  puts "Generating Makefile"
  File.open("Makefile", "w") do |mf|
    
    mf_content = <<MF_EOF
# WARNING! This file is automatically generated by "rake gen".
# WARNING! Any modification to it would be lost after another "rake gen" operation!

# Automatically generated at #{Time.now}

CC=gcc
CFLAGS=-ggdb -Wall #{BUILD_MODULES.collect {|mod| "-I#{mod} "}} -D_FILE_OFFSET_BITS=64
LDFLAGS=-lpthread -lm

default: bin obj #{DEFAULT_BUILD_MODULES.collect {|mod| mod + " "}}

bin:
	mkdir -p bin

obj:
#{mk_obj_dirs_list.collect {|dir| "\tmkdir -p obj/#{dir}\n"}}

api: #{all_src_files.collect {|f| f + " "}}
	doxygen

clean:
	rm -rf bin
	rm -rf obj
	rm -rf api
	find . -iname *~ -delete

#{gen_make_targets}

MF_EOF
    mf.write mf_content
  end
end

desc "Run all test cases"
task :test do
  system "make test"
  result = {}
  TEST_MODULES.each do |mod|
    Find.find(mod) do |f|
      next if FileTest.directory? f
      if f =~ /\.c$/ and has_main_entry? f
        e = (File.basename f)[0..-3]
        puts "== running test 'bin/#{e}'"
        ret = system "cd bin && ./#{e}"
        result[e] = ret
        if ret
          puts "-- success --\n\n"
        else
          puts "-- failure --\n\n"
        end
      end
    end
  end

  puts "\n\n=== final report ===\n"
  puts "-- success:\n"
  result.each do |k, v|
    puts "  #{k}" if v == true
  end
  puts "\n-- failure:\n"
  result.each do |k, v|
    puts "* #{k}" if v == false
  end
end

task :default do
  puts "Run 'rake -T' to get list of functions"
  puts "Run 'rake gen' to generate Makefile"
end

