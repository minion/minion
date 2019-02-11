#!/usr/bin/env python

from __future__ import print_function, absolute_import, division, generators, nested_scopes, with_statement

import os, sys, functools, argparse, re, copy
from os.path import join, getsize
import json
import subprocess
import platform

try:
    from subprocess import DEVNULL # py3k
except ImportError:
    import os
    DEVNULL = open(os.devnull, 'wb')


## Output functions
verbose=1

# Strip directory, .cpp and add .o
def objname(arg):
    return objdir + os.path.basename(arg)[:-4] + ".o"

def fatal_error(*args):
    print(*args, file=sys.stderr)
    sys.exit(1)

def verbose_print(level, *args):
    if level <= verbose:
        print(*args)

## Capture Git options
def progout(prog):
    process = subprocess.Popen(prog, stdout=subprocess.PIPE,
                                     stderr=DEVNULL)
    (output, err) = process.communicate()
    exit_code = process.wait()
    return (output, err, exit_code)

#Check if prog can be run
def progexists(prog):
    try:
        subprocess.call([prog], stdout=DEVNULL,
                                stderr=DEVNULL)
    except OSError as e:
        return False
    return True

def getGitVersion():
    try:
        (out, err, code) = progout(["git", "log", "-1", '--pretty=format:"%h (%ai)"'])
        if code != 0 and err != "":
            return "\"<unknown>\""
        else:
            return out.decode("utf-8")
    except Exception:
        return "\"<unknown>\""

# Reads JSON from string 'instr', with a more helpful error message
# than json.loads. 'outname' should give filename the JSON came from
def readjson(instr, outname):
    try:
        data = json.loads(instr)
        return data
    except ValueError as err:
        print(err)
        m = re.search("line (\d+) column (\d+)", str(err))
        print("Invalid JSON in %s:\nError: '%s'\n"%(outname, err), file=sys.stderr)
        if (m == None) or (m.group(2) == None) or (m.group(1) == None):
            print(instr, file=sys.stderr)
        else:
            line = 0
            for s in str.splitlines(instr):
                print(s, file=sys.stderr)
                if line == int(m.group(1)) - 1:
                    print("-"*(int(m.group(2)) - 1) + "^", file=sys.stderr)
                line = line + 1

        sys.exit(1)

# Get list of all files in a directory with .h or .hpp extensions
def getfilelist(dir):
    filelist=[]
    for root, dirs, files in os.walk(dir):
        f=list(filter(lambda a:(a[-4:]==".hpp" or a[-2:]==".h"), files))
        for i in range(len(f)):
            f[i]=os.path.join(root, f[i])
        filelist=filelist+f
    return filelist

# Grab all JSON in a file which are wrapped in '/* JSON ... */'
def grabjsonfromfile(fname):
    allfile=open(fname,"r").read()
    jsonblocks = []
    idx=allfile.find("/* JSON")
    while idx>-1:
        idxend=allfile[idx:].find("*/")+idx
        if idxend==-1:
            fatal_error("WARNING: found beginning of JSON comment but not end in "+fname)
        gsp=str.expandtabs(allfile[idx+7:idxend].strip(), 4)

        jsonblocks.append(readjson(gsp, fname))

        allfile=allfile[idxend:]
        idx=allfile.find("/* JSON")
    return jsonblocks

##########################################################
##########################################################
##########################################################

parser = argparse.ArgumentParser(description="Minion Builder")
parser.add_argument('--domains64', action='store_const', const=["-DDOMAINS64"],
                    help='Enable 64-bit domains')
parser.add_argument('--wdeg', help='Enable wdeg heuristics (yes/no, default = yes)')

parser.add_argument('--quick', action='store_const', const=['-DQUICK_COMPILE'],
                    help='Quick build')
parser.add_argument('--static', action='store_const', const=["-static"],
                    help='Enable static build')

parser.add_argument('--compiler', help="Set compiler")

parser.add_argument('--constraints', help="Comma seperated list of constraints to use")

parser.add_argument('--debug', action='store_const',
                    const=['-D_GLIBCXX_DEBUG', '-DMINION_DEBUG', '-DMORE_SEARCH_INFO', '-g'],
                    help='Enable debugging')
parser.add_argument('--print', action='store_const', const=['-DMINION_DEBUG_PRINT'],
                    help='Enable verbose debug printing')
parser.add_argument('--info', action='store_const', const=['-DMORE_SEARCH_INFO'],
                    help="Print info..")

parser.add_argument('--unoptimised', action='store_true',
                    help="Disable optimisation")

parser.add_argument('--basicopt', action='store_true',
                    help="Enable basic optimisation")

parser.add_argument('--profile', action='store_const', const=['-fno-inline', '-fno-inline-functions','-g'],
                    help='Enable compiler flags for profiling')

parser.add_argument('--extraflags', help="Add extra compiler options")

parser.add_argument('--setflags', help="Override all other compiler flags (experts only!)")

parser.add_argument('--buildsystem', help="Set build system. Current options: make (default), sh, tup", default="make")

arg = parser.parse_args()


scriptdir = os.path.dirname(os.path.realpath(__file__))
currentdir = os.getcwd()

outsrcdir = os.getcwd() + "/src/"
objdir = os.getcwd() + "/bin/"

if scriptdir == currentdir:
    fatal_error("ERROR: Can't build Minion in it's source directory\n",
                "Run this script from another directory, for example:\n",
                "mkdir bin; cd bin; ../configure.py")

if not os.path.exists(outsrcdir):
    os.mkdir(outsrcdir)

if not os.path.exists(outsrcdir):
    fatal_error("ERROR: Can't create dir ", outsrcdir)

if not os.path.exists(objdir):
    os.mkdir(objdir)

if not os.path.exists(objdir):
    fatal_error("ERROR: Can't create dir ", objdir)

verbose_print(1, "Current dir: " + currentdir)
verbose_print(1, "Minion base dir: " + scriptdir)

commandargs = ["-Wall", "-std=gnu++11", "-Wextra", "-Wno-unused-parameter", "-Wno-sign-compare",
               "-I", scriptdir + "/minion", "-I", outsrcdir]

for c in ['domains64', 'quick', 'debug', 'print', 'info', 'profile', 'static']:
    if getattr(arg, c) != None:
        commandargs = commandargs + getattr(arg, c)

if getattr(arg, 'wdeg'):
    if arg.wdeg == 'yes':
        commandargs = commandargs + ["-DWDEG"]
    elif arg.wdeg <> 'no':
        fatal_error("Invalid argument for --wdeg:" + arg.wdeg)
else:
    commandargs = commandargs + ["-DWDEG"]

if arg.extraflags:
    commandargs = commandargs + arg.extraflags.split()

if arg.setflags:
    commandargs = arg.setflags.split()

if not arg.unoptimised:
    if arg.profile:
        commandargs = commandargs + ["-O"]
    elif arg.basicopt:
        commandargs = commandargs + ["-O2"]
    else:
        if platform.system() == 'Darwin' and ("-static" not in commandargs):
            commandargs = commandargs + ["-O3", "-mdynamic-no-pic", "-fomit-frame-pointer"]
        else:
            commandargs = commandargs + ["-O3", "-fomit-frame-pointer"]

if arg.compiler:
    compiler = arg.compiler
else:
    compiler = ""
    if progexists('ccache'):
        compiler = "ccache " + compiler

    if progexists('c++'):
        compiler = compiler + ' c++'
    elif progexists('g++'):
        compiler = compiler + ' g++'
    elif progexists('clang++'):
        compiler = compiler + ' clang++'
    else:
        fatal_error("Unable to find working C++ compiler.. please use --compiler")

verbose_print(1, "Compiler flags" + str(commandargs))

constraints=[]

filelist = getfilelist(scriptdir+"/minion")

verbose_print(1, "Checking " + str(len(filelist)) + " files")

for fname in getfilelist(scriptdir+"/minion"):
    verbose_print(2, fname)
    data = grabjsonfromfile(fname)
    verbose_print(2, data)
    for d in data:
        d["filename"] = fname
        if d["type"] == "constraint":
            constraints.append(d)
        else:
            fatal_error("Bad 'type' in JSON: "+str(d)+" in "+fname)

## Check for validity of JSON, collisions of internal_name
def validate_names(constraints):
    internal_names = set()
    names = set()
    for c in constraints:
        if c["internal_name"] in internal_names:
            fatal_error("Duplicate internal_name: " + c["internal_name"])
        internal_names.add(c["internal_name"])
        if c["name"] in names:
            fatal_error("Duplicate name: " + c["name"])
        names.add(c["name"])

validate_names(constraints)

if verbose >= 1:
    print("Found the following constraints:")
    print([c["name"] for c in constraints])

## Filter list, if variable exists
if arg.constraints:
    newcon = []
    for c in list(set(arg.constraints.split(",") + ['true','false','pow','product'])):
        found = False
        for con in constraints:
            if con["name"] == c or con["internal_name"] == c:
                found = True
                newcon = newcon + [con]
        if not found:
            fatal_error("Did not find constraint '"+c+"'")
    constraints = newcon

constraints.sort()

constraintsrclist = []
constraintsrclistshort = []

constraint_chunk = 0
# Force first loop to create a file
constraint_current_chunk = 4

for c in constraints:
    constraint_current_chunk += 1
    if constraint_current_chunk >= 5:
        constraint_chunk += 1
        constraint_current_chunk = 0
        srcname = "build_constraint_" + str(constraint_chunk) + ".cpp"
        constraintsrclistshort.append("src/"+srcname)
        srcname = outsrcdir+srcname
        constraintsrclist.append(srcname)
        with open(srcname, "w") as conout:
            conout.write("// Minion constraint file\n")
            
    varcount = sum([ m in ["read_list", "read_var", "read_2_vars"] for m in c["args"]])
    with open(srcname, "a") as conout:
        conout.write('#include "minion.h"\n')
        conout.write('#include "' + c["filename"] + '"\n\n')
        conout.write("BUILD_CT("+c["internal_name"] + ", "+str(varcount)+")\n")


# Output buildstart
with open(outsrcdir+"BuildStaticStart.cpp", "w") as bss:
    constraintsrclist.append(outsrcdir+"BuildStaticStart.cpp")
    bss.write('#include "minion.h"\n')
    for c in constraints:
        bss.write("AbstractConstraint* build_constraint_" + c["internal_name"] + "(ConstraintBlob&);\n")

    bss.write("AbstractConstraint* build_constraint(ConstraintBlob& b) {\n")
    bss.write("  switch(b.constraint->type) {\n")
    for c in constraints:
        bss.write("  case " + c["internal_name"]+ ":\n")
        bss.write("    return build_constraint_" + c["internal_name"] + "(b);\n")
    bss.write("  default: abort();\n")
    bss.write("  }\n}\n")

with open(outsrcdir+"constraint_defs.h", "w") as defs:
    defs.write("ConstraintDef constraint_list[] = {\n")
    for c in constraints:
        args = "{" + ",".join(c["args"]) + "}"
        defs.write('{{ "{name}", {inname}, {count}, {{ {args} }} }},\n'
                    .format(name=c["name"], inname=c["internal_name"],
                            count=len(c["args"]), args=args))
    defs.write("};\n")

with open(outsrcdir+"ConstraintEnum.h", "w") as enum:
    enum.write("#ifndef CONSTRAINT_ENUM_QWE\n")
    enum.write("#define CONSTRAINT_ENUM_QWE\n")
    enum.write("enum ConstraintType {\n")
    for c in constraints:
        enum.write(c["internal_name"]+",\n")
    enum.write("};\n")
    enum.write("#endif\n")


with open(outsrcdir+"BuildDefines.h", "w") as defs:
    defs.write('#define GIT_VER ' + getGitVersion() + '\n')

minionsrclist = ['minion/BuildVariables.cpp',
'minion/BuildCSP.cpp',
'minion/commandline_parse.cpp',
'minion/lock.cpp',
'minion/debug_functions.cpp',
'minion/get_info.cpp',
'minion/minion.cpp',
'minion/globals.cpp',
'minion/preprocess.cpp',
'minion/system/trigger_timer.cpp',
'minion/system/sha1.cpp',
'minion/help/help.cpp',
'minion/inputfile_parse/inputfile_parse.cpp',
'minion/dump_state.cpp',
'minion/parallel.cpp',
'minion/search_dump.cpp',
'minion/search_dump_sql.cpp',
]



if arg.buildsystem == "make":
    outname = currentdir + "/Makefile"
    qw = ''
    def varsub(x):
        return ' $(' + x + ') '
elif arg.buildsystem == "sh":
    outname = currentdir + "/build.sh"
    qw = '"'
    def varsub(x):
        return ' ${' + x + '} '
elif arg.buildsystem == "tup":
    outname = currentdir + "/Tupfile"
    qw = ''
    def varsub(x):
        return ' $(' + x + ') '
else:
    print("Build system not supported")
    sys.exit(1)

with open(outname, "w") as out:
    constraintobjlist = [objname(x) for x in constraintsrclist]
    minionobjlist = [objname(x) for x in minionsrclist]
    print(minionsrclist)
    print(minionobjlist)
    if arg.buildsystem == "sh":
        out.write("#!/usr/bin/env bash\n")

    if arg.buildsystem == "make":
        commandargs = commandargs + ["-MD", "-MP"]

    out.write('FLAGS=' + qw + ' '.join(commandargs)+ qw +'\n')
    
    if arg.buildsystem != "tup":
        out.write('CONSRCS=' + qw + ' '.join(constraintsrclist)+ qw +'\n')
        out.write('CONOBJS=' + qw + ' '.join(constraintobjlist)+ qw +'\n')
        out.write('MINOBJS=' + qw + ' '.join(minionobjlist)+ qw +'\n')
    
    if arg.buildsystem == "tup":
        out.write(": foreach ")
        out.write(" ".join(constraintsrclistshort + ["src/BuildStaticStart.cpp"] + [ "../"+i for i in minionsrclist]))
        out.write(" |> ")
        out.write(compiler + ' ' + varsub('FLAGS') + ' -c %f -o %o |> bin/%B.o\n')
        out.write(': bin/*.o |> ' + compiler + ' ' + varsub('FLAGS') + '%f -o %o |> minion\n')
        sys.exit(0)
        
    
    if arg.buildsystem == "make":
        out.write('all : minion\n')

    if arg.buildsystem == "make":
        out.write("-include $(CONOBJS:.o=.d)\n")
        out.write("-include $(MINOBJS:.o=.d)\n")
    for i in constraintsrclist:
        if arg.buildsystem == "make":
            out.write(objname(i) + ":\n")
#            out.write(objname(i) + ": "+i+'\n')
        out.write('\t'+compiler+' '+varsub('FLAGS') + ' -c -o ' +
                   objname(i) + " " + i +'\n')

    for i in minionsrclist:
        if arg.buildsystem == "make":
            out.write(objname(i)+ " :\n")
            # out.write(objname(i)+ " : " + scriptdir + "/" + i +'\n')
        out.write('\t'+compiler+' ' + varsub('FLAGS') + ' -c -o ' +
                   objname(i) + " " + scriptdir + "/" + i +'\n')
    if arg.buildsystem == "make":
        out.write('minion: $(CONOBJS) $(MINOBJS)\n')
    out.write('\t' + compiler + ' ' + varsub('FLAGS') + varsub('CONOBJS') +
               varsub('MINOBJS') + ' -pthread -o minion\n')
