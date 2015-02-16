#!/usr/bin/python
import sqlite3
import sys

# db, collection
def main(argv) :
	print "MAIN"
	dbname = argv[0]
	collname = argv[1]

	db = sqlite3.connect(dbname)
	db.text_factory = str


	print "NAME:" + collname
	dbc = db.cursor()
	dbc.execute('select ROWID from collection where id=?', (collname,))
	res = dbc.fetchone()
	if not res :
		return

	collid = int(res[0])
	print "ID:" + str(collid)
	dbc = db.cursor()
	dbc.execute('select title,game,composer,format,path from song where collection=?', (collid,));
	print "Getting rows"
	f = open(collname + '.txt', 'w')
	for row in dbc :
		f.write('%s\t%s\t%s\t%s\t%s\n' % row)
	f.close()

if __name__ == "__main__":
	main(sys.argv[1:])
