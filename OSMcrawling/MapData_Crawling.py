# -*- coding: utf-8 -*-
"""
@author: Sumin Han
"""

from bs4 import BeautifulSoup            # HTML parsing library
from lxml import html

file= open('MapData.txt','w')              # open text file to write output

with open('kaistmap.osm.xml','r') as f:      # open html file to read data
    page = f.read()


#soup = BeautifulSoup(page)
soup = BeautifulSoup(page, "lxml")
nodes = soup('node', {})      # save the name of tracks only from whole html file data
ways = soup('way', {})

print "nodes", len(nodes)
print "ways", len(ways)

node_list = []
for n in nodes:
   node_list += [(n['id'], n['lat'], n['lon'])]

def position(key):
    for k, lat, lon in node_list:
        if key == k:
            return lat, lon
    return (-1, -1)

bnum = 0
for w in ways:
    isBuild = False
    bname = '*NONAME*'
    for t in w.select("tag"):
        k = t['k']
        if k == 'building':
            isBuild = True
            bnum += 1
        if k == 'name':
            bname = t['v']

    if isBuild:
        print bname
        file.write(bname.encode('utf8') + '\n')
        for nd in w.select("nd"):
            kk = nd['ref']
            print kk, position(kk)
            file.write(kk + " " + position(kk)[0] + " " + position(kk)[1] + "\n")

print "There are ", bnum, "buildings"

file.close()
