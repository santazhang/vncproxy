APPNAME="vncproxy"
VERSION="0.2"

def options(opt):
    opt.load("compiler_cxx")

def configure(conf):
    conf.load("compiler_cxx")
    conf.check(lib="sqlite3")
    conf.env.append_value("CXXFLAGS", ["-O3", "-Wall"])

def build(bld):
    bld.program(source=bld.path.ant_glob(["*.c", "*.cc"]), target="vncproxy", lib=["pthread", "sqlite3"])

