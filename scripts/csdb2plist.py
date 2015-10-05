#!/usr/bin/python

import os
import sys
from xml.dom.minidom import Document, parseString
	

def getText(node, tag) :
	nextNode = None
	tags = tag.split('/')
	for t in tags :
		#print t, node
		for c in node.childNodes :
			# print ">> ", c.nodeType
			if c.nodeType == c.ELEMENT_NODE :
				# print ">> ", c.tagName
				if c.tagName == t :
					nextNode = c
					break
		if nextNode == None :
			return ''
		node = nextNode
		nextNode = None
						
	rc = ''
	# print "Found node ", node
	for t in node.childNodes :		
		if t.nodeType == c.TEXT_NODE :
			rc += t.data
	return rc



def main(argv) :
	try :
		xmlData = open(argv[0]).read()
	except :
		xmlData = None
		
	#outf = open(argv[1], 'w')
		
	groupList = {}
	eventList = {}
	
	code = 'iso8859_1'
		
	if xmlData :
		doc = parseString(xmlData)
		xmlreleases = doc.getElementsByTagName('Release')
		#print "Found %d releases" % (len(releases))
 		#outf.write("[Releases]\n")
		#try :

		events = {}
		releases = {}

		for r in xmlreleases :
			rel = {}
		
			name = getText(r, 'Name').strip()
			id = getText(r, 'ID')
			if id :
				id = int(id)
			else :
				id = -1
			rating = getText(r, 'CSDbRating')
			rt = 0
			if rating :
				rt = int(float(rating) * 100)
				rating = str(rt)
		
			#if rt < 900 :
			#	continue
		
			group = getText(r, 'ReleasedBy/Group/Group').strip()
			event = getText(r, 'Achievement/Event/Name').strip()
			compo = getText(r, 'Achievement/Compo').strip()
			place = getText(r, 'Achievement/Place').strip()
			if place :
				place = int(place)
			else :
				place = 0

			type = getText(r, 'ReleaseType')

			if type == 'C64 Music' and event :
				if event in events :
					events[event].append((place, id))
				else :
					events[event] = [ (place, id) ]

			sids = r.getElementsByTagName('HVSCPath')
			fnames = []
			for s in sids :			
				fnames.append(s.firstChild.data)

			releases[id] = (name, group, fnames)
		
			if (type.endswith('Music Collection') or type.endswith('Diskmag') or type.endswith('Demo')) and rt > 0 :
				print(type, name)
				sids = r.getElementsByTagName('HVSCPath')
				fnames = []
				for s in sids :			
					fnames.append(s.firstChild.data)
				outf = open('r%d.plist' % (id,), 'w')
				outf.write((u';%s\t%s\n' % (name, group)).encode(code))
				for fn in fnames :
					if fn[0] == '/' :
						fn = fn[1:]
					outf.write('csdb::%s\n' % (fn,))
				outf.close()
		i = 0				
		for ename in events :
			print(ename)
			e = events[ename]
			e.sort()
			outf = open('p%d.plist' % (i,), 'w')
			i+=1
			outf.write((u';%s Music Compo\n' % (ename,)).encode(code))
			for x in e :
				rel = releases[x[1]]
				for fn in rel[2] :
					if fn[0] == '/' :
						fn = fn[1:]
					outf.write('csdb::%s\n' % (fn,))
			outf.close()

if __name__ == "__main__":
	main(sys.argv[1:])
