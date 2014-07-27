#!/usr/bin/python
import sqlite3
import sys

def main(argv) :
	print "MAIN"
	db = sqlite3.connect('music.db')
	db.text_factory = str

	dbc = db.cursor()
	dbc.execute('select title,game,composer,format,path from song where collection=?', '2');
	print "Getting rows"
	f = open('hvsc.txt', 'w')
	for row in dbc :
		f.write('%s\t%s\t%s\t%s\t%s\n' % row)
	f.close()

	dbc = db.cursor()
	dbc.execute('select title,game,composer,format,path from song where collection=?', '3');
	print "Getting rows"
	f = open('rsn.txt', 'w')
	for row in dbc :
		f.write('%s\t%s\t%s\t%s\t%s\n' % row)
	f.close()

if __name__ == "__main__":
	main(sys.argv[1:])
