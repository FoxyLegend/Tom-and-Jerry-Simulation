# -*- coding: utf-8 -*-
"""
@author: Sumin Han
"""

from bs4 import BeautifulSoup            # HTML parsing library
from lxml import html

nf= open('MapData.txt','w')              # open text file to write output

with open('kaistmap.osm.xml','r') as f:      # open html file to read data
    page = f.read()


#soup = BeautifulSoup(page)
soup = BeautifulSoup(page, "lxml")
nodes = soup('node', {})      # save the name of tracks only from whole html file data
ways = soup('way', {})

print "nodes", len(nodes)
print "ways", len(ways)

nf.write("# nodes\n")
node_list = []
for n in nodes:
   node_list += [(n['id'], n['lat'], n['lon'])]
   tmpstr = "v\t" + n['id'] + "\t" + n['lat'] + "\t" + n['lon'] + "\n"
   nf.write(tmpstr)

nf.write("# polygons\n")
bnum = 0
for w in ways:
   isBuild = False
   bname = '_'
   for t in w.select("tag"):
      k = t['k']
      if k == 'building':
         isBuild = True
         bnum += 1
      if k == 'name':
         bname = t['v']

   if isBuild:
      tmpstr = "p " + bname.encode('utf8')
      for nd in w.select("nd"):
         kk = nd['ref']
         tmpstr = tmpstr + "\t" + kk;
         
      tmpstr = tmpstr + "\n"
      nf.write(tmpstr)

print "There are ", bnum, "buildings"

nf.close()
