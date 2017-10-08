import sys,time


def next_frame(i, t):
	return [ 255-((x+i)%120)*2 for y in range(8) for x in range(120) ]


i=0
t0 = time.time()

while True:
	i+=1
	i%=120
	sys.stdout.write(''.join(chr((x/2)&0x7f) for x in next_frame(i, time.time()-t0))+"\x80")
	sys.stdout.flush()
	time.sleep(.005)
