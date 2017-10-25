import sys,time

WIDTH=(144*3)

def calc_b(x):
	return int( ( x % 144 ) * 255 ) // 144

def calc_q(x):
	return int( int( x%144 == 0 ) * 255 )

def next_frame(i, t):
	return [ calc_b(y+i) for y in range(144) for x in range(24) ]
	#return [ calc_q(x+i) for y in range(8) for x in range(WIDTH) ]
#	return [ 255*int(x>i) for y in range(8) for x in range(432) ]
#	return [ 16*y for y in range(8) for x in range(144*3) ]
#	return [ 255 for y in range(8) for x in range(144*3) ]


i=0
t0 = time.time()

while True:
	i+=1
	i%=144*3
	sys.stdout.write(''.join(chr((x/2)&0x7f) for x in next_frame(i, time.time()-t0))+"\x80")
	sys.stdout.flush()
	time.sleep(.005)
