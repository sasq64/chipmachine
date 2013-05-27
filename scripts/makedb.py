#!/usr/bin/python
import sqlite3
import sys
import os


def scanDir(path, depth) :
	global dbc
	global db
	try:
		content = [os.path.join(path, x) for x in os.listdir(path)]
	except OSError:
		print >>sys.stderr, "# problem with {0}".format(path)
		return

	dirs = sorted([x for x in content if os.path.isdir(x)])
	files = sorted([x for x in content if os.path.isfile(x)])

	for d in dirs:
		if os.path.islink(d):
			continue
		scanDir(d, depth + 1)

	for f in files :
		if f.lower().endswith('.sid') :
			data = open(f, 'rb').read()
			title = data[22:22+32].strip('\x00') #  "ISO-8859-1"
			composer = data[54:54+32].strip('\x00')
			copyright = data[86:86+32].strip('\x00')
			print f, title, composer, copyright
			dbc.execute('insert into songs values (null, ?,?,?,?,?)', (str(f), title, composer, 'SID', 'COPYRIGHT='+copyright))

	db.commit()
	 


def main(argv) :

	global dbc
	global db

	db = sqlite3.connect('hvsc.db')
	db.text_factory = str

	dbc = db.cursor()
	try :
		dbc.execute('create table songs (_id INTEGER PRIMARY KEY, path TEXT, title TEXT, composer TEXT, format TEXT, metadata TEXT)')
#		dbc.execute('create index nameidx on songs (name)')
#		dbc.execute('create table songs (_id INTEGER PRIMARY KEY, name TEXT, author INTEGER, game INTEGER, type INTEGER)')
#		dbc.execute('create table types (_id INTEGER PRIMARY KEY, tname TEXT)')
#		dbc.execute('create table authors (_id INTEGER PRIMARY KEY, aname TEXT)')
#		dbc.execute('create table games (_id INTEGER PRIMARY KEY, gname TEXT)')
		db.commit()
	except :
		print "DB FAILED"
		return 0
	

	scanDir(argv[0], 0)

	return 0;

	types = {}
	authors = {}
	games = {}
	
	for l in open('allmods.txt') :
		name = l.split('\t')[1].strip()
		parts = name.split('\\')
		if parts[0] == 'Ad Lib' or parts[0] == 'Video Game Music':
			parts = [parts[0] + '/' + parts[1]] + parts[2:]
		if parts[0] == 'YM' and parts[1] == 'Synth Pack':
			parts = [parts[0] + '/' + parts[1]] + parts[2:]

		if parts[2].startswith('coop-') :
			parts = [parts[0]] + [parts[1] + '/' + parts[2]] + parts[3:]
			
		if len(parts) == 5 and (parts[3].startswith('instr') or parts[3].startswith('songs')) :
			parts = parts[:2] + [parts[3] + '/' + parts[4]]
			
		if len(parts) > 4 :
			parts = parts[:2] + ['/'.join(parts[3:])]

		type = parts[0]
		author = parts[1]
		game = 'NONE'
		
		
		if len(parts) == 3 :
			name = parts[2]
		elif len(parts) == 4 :
			game = parts[2]
			name = parts[3]
		#elif len(parts) == 5 :
		#	author = author + ' ' + parts[2]
		#	game = parts[3]
		#	name = parts[4]
		else :
			print "Strange line ", parts
			raise Exception('Unknown format')
		
		try:

#			if types.has_key(type) :
#				tid = types[type]
#			else :
#				dbc.execute('insert into types values (null, ?)', (type,))
#				tid = types[type] = dbc.lastrowid
#				print "Found new type", type, tid
#			
#			if games.has_key(game) :
#				gid = games[game]
#			else :
#				dbc.execute('insert into games values (null, ?)', (game,))
#				gid = games[game] = dbc.lastrowid
#				print "Found new game", game, gid
#			
#			if authors.has_key(author) :
#				aid = authors[author]
#			else :
#				dbc.execute('insert into authors values (null, ?)', (author,))
#				aid = authors[author] = dbc.lastrowid
#				print "Found new author", author, aid
#			
#			dbc.execute('insert into songs values (null, ?,?,?,?)', (name, aid, gid, tid))
			dbc.execute('insert into songs values (null, ?,?,?,?)', (name, author, game, type))
		except :
			print "Could not insert ",name, type, game, author
			raise
	
	db.commit();
		
	
if __name__ == "__main__":
	main(sys.argv[1:])
