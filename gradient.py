import sys,time

i=0
while True:
	i+=1
	i%=120
	sys.stdout.write("".join(chr(127-(x+i)%120) for x in range(120))*8+"\x80")
	sys.stdout.flush()
	time.sleep(.005)
