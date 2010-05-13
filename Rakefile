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
  obj_list = obj_list.sort
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
  list = list.sort
  return list
end

def all_targets_in_module mod
  depend = {mod => []}
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
        depend[bin_name].concat(depended_obj path)

        action[bin_name] = [] if action[bin_name] == nil
        action[bin_name] << "$(CC) $(CFLAGS) $(LDFLAGS) #{obj_name} #{(depended_obj path).join " "} -o #{bin_name}"
      end

      depend[mod] << obj_name

      depend[obj_name] = [] if depend[obj_name] == nil
      depend[obj_name] << path
      depend[obj_name].concat(depended_header path)

      action[obj_name] = [] if action[obj_name] == nil
      action[obj_name] << "$(CC) $(CFLAGS) -c #{path} -o #{obj_name}"
    end
  end

  if defined? LIB_MODULES and LIB_MODULES.include? mod
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

  if defined? TEST_MODULES and TEST_MODULES.include? mod
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
    all_bin_list = all_bin_list.sort
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
  depend.keys.sort.each do |target|
    depend[target] = depend[target].sort
    $g_depend[target] = depend[target]
    if BUILD_MODULES.include? target
      content += <<CONTENT
#{target}: bin obj #{depend[target].join " "}
#{action[target].collect {|act| "\t#{act}\n"}}
CONTENT
    else
      content += <<CONTENT
#{target}: #{depend[target].join " "}
#{action[target].collect {|act| "\t#{act}\n"}}
CONTENT
    end
  end

  return content
end

def gen_make_targets
  content = ""
  BUILD_MODULES.sort.each do |mod|
    content += all_targets_in_module mod
    content += "\n"
  end

  content += "all: bin obj #{LIB_MODULES.collect {|mod| "bin/lib#{mod}.a "} if defined? LIB_MODULES}"
  $g_depend.keys.sort.each do |target|
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
  BUILD_MODULES.sort.each do |mod|
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
  BUILD_MODULES.sort.each do |mod|
    Find.find(mod) do |path|
      if FileTest.directory? path
        list << path
      end
    end
  end
  list = list.sort
  min_list = []
  if list.length < 2
    min_list = list
  else
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

def warn_bad_style
  flist = all_src_files

  flist.each do |path|
    if defined? NO_STYLE_CHECK
      should_skip = false
      NO_STYLE_CHECK.each do |mod|
        if path.start_with? mod
          should_skip = true
          break
        end
      end
      next if should_skip
    end

    # check if used tabs in spacing
    File.open(path) do |f|
      row = 1
      f.each_line do |line|
        if line =~ /^[ ]*\t/
          puts "style warning: #{path}, #{row}: please don't use tab for spacing"
        end
        row += 1
      end
    end

    # check if has #ifndef guard
    if path.end_with? ".h"
      File.open(path) do |f|
        has_guard = false
        guard_name = nil
        f.each_line do |line|
          if guard_name == nil
            if line =~ /^\#ifndef.*_H_$/
              guard_name = line.split[-1]
            end
          else
            # last line is #ifndef, so this line must be #define
            if line =~ /^\#define/ and line.include? guard_name
              has_guard = true
              break
            else
              guard_name = nil
            end
          end

        end

        if has_guard == false
          puts "style warning: #{path}, no #ifndef guard found!"
        end
      end
    end

    # check if #endif are followed by comments
    File.open(path) do |f|
      row = 1
      f.each_line do |line|
        if line =~ /^\#endif/
          unless line.include? '//' or line.include? '/*'
            puts "style warning: #{path}:#{row}, #endif should be followed by comments!"
          end
        end
        row += 1
      end
    end

    # check if .h files have got @author, @file tags
    if path.end_with? ".h"
      has_author_tag = false
      has_file_tag = false
      File.open(path) do |f|
        f.each_line do |line|
          if line =~ /@author/
            has_author_tag = true
          end
          if line =~ /@file/
            has_file_tag = true
          end
        end
      end
      if has_author_tag == false
        puts "style warning: #{path}, does not have @author tag!"
      end
      if has_file_tag == false
        puts "style warning: #{path}, does not have @file tag"
      end
    end

    # check if there is multiple return in one function, when there is memory allocation
    File.open(path) do |f|
      inside_fun = nil
      return_rows = []
      row = 1
      has_mem_alloc = false
      f.each_line do |line|
        if line =~ /^[a-z].*\{[ \t]*$/
          inside_fun = line.strip[0..-2].strip
          return_rows = []
          has_mem_alloc = false
        elsif line =~ /^\}.*/
          inside_fun = nil
          if return_rows.size > 1 and has_mem_alloc == true
            puts "style warning: #{path}:[#{return_rows.join ", "}], multiple return statement in one function, when there is memory allocation!"
          end
          return_rows = []
          has_mem_alloc = false
        elsif inside_fun != nil
          if line =~ /^[ \t]*return .*;.*$/
            return_rows << row
          elsif line =~ /[a-z]*_new\(/ or line =~ /malloc[a-z_]*\(/
            has_mem_alloc = true
          end
        end
        row += 1
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
  warn_bad_style
  assert_no_duplicate_fname
  puts "No duplicate file name"
  assert_src_header_pair
  puts "All .c files either has got a .h file, or they have main() entry"
  puts "Finished checking"
end

desc "Generate Makefile"
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

#{
if File.exists? "Doxyfile"
  # only enable "make api" when there is already an Doxyfile
"api: #{all_src_files.collect {|f| f + " "}}
	doxygen
"
end
}

clean:
	rm -rf bin
	rm -rf obj
	rm -rf api
	rm -f *.log
	find . -iname *~ -delete

#{gen_make_targets}

MF_EOF
    mf.write mf_content
  end

  if defined? GEN_MINGW32_MAKEFILE and GEN_MINGW32_MAKEFILE == true
    File.open("Makefile.mingw32", "w") do |mf_mingw32|

      # well, for mingw32, we don't generate doxygen file      
      mf_mingw32_content = <<MF_EOF
# WARNING! This file is automatically generated by "rake gen".
# WARNING! Any modification to it would be lost after another "rake gen" operation!

# Automatically generated at #{Time.now}

CC=gcc
CFLAGS=-ggdb -Wall #{BUILD_MODULES.collect {|mod| "-I#{mod} "}} -D_FILE_OFFSET_BITS=64
LDFLAGS=-lpthread -lm

default: bin obj #{DEFAULT_BUILD_MODULES.collect {|mod| mod + " "}}

bin:
	md bin

obj:
#{mk_obj_dirs_list.collect {|dir| "\tmd obj/#{dir}\n"}}

clean:
	rd /S /Q bin
	rd /S /Q obj
	rd /S /Q api
	for /r %f in (*.log) do del "%f"
	for /r %f in (*~) do del "%f"

#{gen_make_targets}

MF_EOF
      mf_mingw32.write mf_mingw32_content
    end
  end
end

desc "Run all test cases"
task :test do
  TEST_MODULES.each do |mod|
    system "make #{mod}"
  end
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

desc "Show all notes like TODO, ENHANCE, etc"
task :notes do
  ["TODO", "NOTE", "ENHANCE", "XXX", "FIXME"].each do |note|
    system "grep #{note} * -r -n --color"
    puts
  end
end

task :default do
  puts "Run 'rake -T' to get list of functions"
  puts "Run 'rake gen' to generate Makefile"
end

