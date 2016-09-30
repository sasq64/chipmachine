#!/usr/bin/python

import os
import sys
import struct
from bisect import bisect

def parse(fname) :
    f = open(fname)
    
    hashMap = {}
    lastName = ""
    
    for line in f :
        a = line.split('=')
        if len(a) == 2 :
            hash = int(a[0][:16], 16)
            
            #if lastName.find('Savage.sid') > -1 :
             #   print "%s %s => %08x" % (lastName, a[0], hash)
            
            if hashMap.has_key(hash) :
                print "Found duplicate ", lastName
            
            lengths = []
            lens = a[1].split(' ')
            for l in lens :
                tt = l.split(':')
                if len(tt) == 2 :
                    #print tt
                    sec = int(tt[0]) * 60 + int(tt[1][:2])
                    lengths.append(sec)
                    
                    
            hashMap[hash] = lengths

        else :
            lastName = line
    return hashMap
                

    
def main(argv) :
    hashmap = parse(argv[0])
    
    f = open('out.dat', 'w')
    
    
    a = sorted(hashmap)
    f.write(struct.pack('>BB', 0xff, 1))
    f.write(struct.pack('>I', len(a)))
    offset = 0
    offsets = []

    for x in a :
        lens = hashmap[x]
        
        if len(lens) == 1 :        
            f.write(struct.pack('>QH', x, lens[0]))
        else :
            f.write(struct.pack('>QH', x, offset | 0x8000))
            for i in range(len(lens)-1) :
                l = lens[i]
                offsets.append(struct.pack('>H', l))            
            offsets.append(struct.pack('>H', lens[-1] | 0x8000))
            offset += len(lens)
    
    print "Offset is ", offset
    f.write(''.join(offsets))
    
    
import hashlib

table = None
tablen = -1
tabdata = None


def lookup(f):
    global table, tablen, tabdata
    m = hashlib.md5()
    data = open(f).read()
    
    offset = 120
    a = struct.unpack('>4sHHHHHHHI32s32s32s', data[0:118])
    if a[1] == 2 :
        b = struct.unpack('>HBBH', data[118:124])
        offset = 126
        
    # a[6] = songs, a[8] = speed
    
    print "%d songs, %x speed, %x flags" % (a[6], a[8], b[0])
    
    m.update(data[offset:])
    
    print "LEN ", len(data[offset:])
    
    m.update(data[0xb:0xc])
    m.update(data[0xa:0xb])
    m.update(data[0xd:0xe])
    m.update(data[0xc:0xd])
    m.update(data[0xf:0x10])
    m.update(data[0xe:0xf])
    
    temp = '\x3c'
    
    for i in range(a[6]) :
        print ord(temp[0])
        m.update(temp)
   # m.update(temp)

    #m.update(data)
    md5 = m.hexdigest()    
    hash = int(md5[:8], 16)
    
    print "%s => %s => %08x" % (f, md5, hash)
    
    if not table :
        tabdata = open('out.dat').read()
        tablen = struct.unpack('>L', tabdata[0:4])[0]
        table = []
        for i in range(tablen) :
            j = i*6 + 4
            x = struct.unpack('>LH', tabdata[j:j+6])
            table.append(x[0])
        
    where = bisect(table, hash)    
    y = struct.unpack('>LH', tabdata[where*6+4:where*6+10])
    print "%d -> Hash %02x -> %02x (%02d)" % (where, hash, y[0], y[1])
    where -= 1
    x = struct.unpack('>LH', tabdata[where*6+4:where*6+10])        
    print "%d -> Hash %02x -> %02x (%02d)" % (where, hash, x[0], x[1] & 0x7fff)
    
    if x[1] & 0x8000 :
        offset = 4 + tablen * 6 + (x[1] & 0x7fff) * 2
        print struct.unpack('>HHHH', tabdata[offset:offset+8])
    
    
    
        


def test(f) :
    lookup(f, False, 0)
    lookup(f, True, 0)
    lookup(f, False, 2)
    lookup(f, True, 2)

if __name__ == "__main__":
    main(sys.argv[1:])
