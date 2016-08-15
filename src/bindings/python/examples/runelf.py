import opensocdebug 
import sys

if len(sys.argv) < 2:
    print "Usage: runelf.py <filename> [-verify-memload]"
    exit(1)

elffile = sys.argv[1]

verify = (len(sys.argv) = 3) and (sys.argv[2] == "-verify-memload")

osd = opensocdebug.Session()

osd.reset(halt=True)

for m in osd.get_modules("STM"):
    m.log("stm{:03x}.log".format(m.get_id()))

for m in osd.get_modules("CTM"):
    m.log("ctm{:03x}.log".format(m.get_id()), elffile, verify)

for m in osd.get_modules("MAM"):
    m.loadelf(elffile)

osd.start()
osd.wait(10)
