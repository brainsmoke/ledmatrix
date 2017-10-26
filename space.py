import sys,time,random,math,select

import font

SHIPS = [
([
[ 0x00, 0x0a, 0x74, 0x88, 0xb5, 0xc8, 0xc3, 0xcd, 0xe7, 0xf7, 0xfd, 0xff, 0xff, 0xff, 0xff, 0xcb, 0x34, 0x00, 0x00, 0x00, 0x00, 0x19, 0x60, 0x82, 0x7c, 0x7c, 0x82, 0x60, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00 ],
[ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x39, 0x7a, 0xd2, 0xff, 0xff, 0xcc, 0x34, 0x00, 0x00, 0x00, 0x00, 0x26, 0x98, 0xde, 0xf8, 0xff, 0xff, 0xff, 0xff, 0xf8, 0xde, 0x98, 0x26, 0x00, 0x00, 0x00 ],
[ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x39, 0x7a, 0xd2, 0xff, 0xff, 0xc9, 0x2c, 0x04, 0x51, 0x9d, 0xe8, 0xff, 0xf7, 0xf0, 0xf1, 0xf1, 0xf0, 0xf7, 0xff, 0xe8, 0x9d, 0x57, 0x15, 0x00 ],
[ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x7e, 0xde, 0xff, 0xff, 0xff, 0xff, 0xfc, 0xf3, 0xf3, 0xfc, 0xff, 0xff, 0xff, 0xff, 0xfd, 0xf7, 0xeb, 0xdb, 0x99, 0x27, 0x00 ],
[ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x7e, 0xde, 0xff, 0xff, 0xff, 0xff, 0xfc, 0xf3, 0xf3, 0xfc, 0xff, 0xff, 0xff, 0xff, 0xfd, 0xf7, 0xeb, 0xdb, 0x99, 0x27, 0x00 ],
[ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x39, 0x7a, 0xd2, 0xff, 0xff, 0xc9, 0x2c, 0x04, 0x51, 0x9d, 0xe8, 0xff, 0xf7, 0xf0, 0xf1, 0xf1, 0xf0, 0xf7, 0xff, 0xe8, 0x9d, 0x57, 0x15, 0x00 ],
[ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x39, 0x7a, 0xd2, 0xff, 0xff, 0xcc, 0x34, 0x00, 0x00, 0x00, 0x00, 0x26, 0x98, 0xde, 0xf8, 0xff, 0xff, 0xff, 0xff, 0xf8, 0xde, 0x98, 0x26, 0x00, 0x00, 0x00 ],
[ 0x00, 0x00, 0x74, 0x88, 0xb5, 0xc8, 0xc3, 0xcd, 0xe7, 0xf7, 0xfd, 0xff, 0xff, 0xff, 0xff, 0xcb, 0x34, 0x00, 0x00, 0x00, 0x00, 0x19, 0x60, 0x82, 0x7c, 0x7c, 0x82, 0x60, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00 ],
], 255),

([
[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
[0, 0, 192, 255, 255, 255, 255, 255, 127, 127, 127, 0, 0, 0, 0, 0],
[0, 192, 255, 255, 255, 255, 255, 192, 0, 0, 0, 0, 0, 0, 0, 0],
[255, 255, 255, 255, 255, 255, 127, 127, 0, 0, 0, 0, 127, 192, 192, 127],
[0, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255],
[255, 255, 255, 255, 255, 255, 127, 127, 0, 0, 0, 0, 127, 192, 192, 127],
[0, 192, 255, 255, 255, 255, 255, 192, 0, 0, 0, 0, 0, 0, 0, 0],
[0, 0, 192, 255, 255, 255, 255, 255, 127, 127, 127, 0, 0, 0, 0, 0]
], 64)

]


class SpaceAni(object):

	def move_ship(self):
		self.ship_x += self.dx/16
		if self.ship_x > self.w + 400:
			self.ship_x = -random.randint(30, 80)
			self.new_dx = 1.
			self.ship = SHIPS[int(random.randrange(8)==0)]

	def new_text(self):
		self.text_x = self.w+10
		self.text = ''.join(self.text_source.get_line())
		self.text_size = self.font.text_size(self.text)/2

	def move_text(self):
		self.text_x -= self.dx/16
		if self.text_x < -self.text_size:
			self.new_text()

	def paint_wide_bitmap(self, x, y, bitmap, intensity=255):
		ship_x = x - .75
		grid_x = int(math.floor(ship_x))+1
		diff = grid_x-ship_x
		off = min(1.999999, max(0, 2*diff)) # on 2x grid
		ratio = off-int(math.floor(off))    # on 2x grid
		off = int(math.floor(off))          # on 2x grid
		for dy in range(len(bitmap)):
			base = self.w*(y+dy)
			for dx in range((len(bitmap[dy])-1)//2):
				x = grid_x+dx
				if 0 <= x < self.w:
					arr_x = off+2*dx
					a0 = bitmap[dy][arr_x]
					a1 = bitmap[dy][arr_x+1]
					c0 = a0*intensity + self.frame[base+x]*(255-a0)
					c1 = a1*intensity + self.frame[base+x]*(255-a1)
					r0, r1 = 1-ratio, ratio
					self.frame[base+x] = int(r0*c0 + r1*c1)/255

	def paint_text(self, x):
		off = 0
		for i,c in enumerate(self.text):
			g, keming = self.font.get_glyph(c)
			self.paint_wide_bitmap(x+off/2.,0,g, 192)
			off += keming

	def paint_ship(self, x):
		bitmap, intensity = self.ship
		self.paint_wide_bitmap(x,0,bitmap,intensity)

	def add_star(self):
		self.stars.append( [ self.w, random.randint(0,self.h-1), random.randint(15, 80)/10. ] )
		self.next_star = random.randint(3, 64)

	def move_stars(self):
		for i in range(len(self.stars)-1, -1, -1):
			x, y, v = self.stars[i]
			v *= self.dx
			x -= v*self.dt
			if x + v < 0:
				self.stars[i:i+1] = []
			else:
				self.stars[i][0] = x

		self.next_star -= self.dx
		if self.next_star <= 0:
			self.add_star()

	def paint_stars(self):
		for x, y, v in self.stars:
			v += self.dx
			x = int(x)
			v = int(v)
			base = self.w*y
			l = int(v*v/8)+1
			for i in range(l):
				ix = x+i
				if 0 <= ix < self.w:
					self.frame[base+ix] += int(255-255*i/l)


	def change_dx(self):
		self.next_dx_change -= 1
		self.dx = self.dx*.97 + self.new_dx*.03
		if self.next_dx_change <= 0:
			if -8 < self.ship_x < self.w:
				self.new_dx += random.randint(4, 8)/3.
			self.next_dx_change = random.randint(300, 1000)

	def clear(self):
		for i in xrange(len(self.frame)):
			self.frame[i]=0

	def __init__(self, w, h, text_source):
		self.w, self.h = w, h
		self.frame = [0]*(w*h)
		self.stars = []
		self.add_star()
		self.ship = SHIPS[0]
		self.ship_x = -30
		self.dx = 0.
		self.new_dx = 1.
		self.next_dx_change = 1
		self.dt = 1/16.
		self.font = font.Font()
		self.text_source = text_source
		self.new_text()

	def next_frame(self, i, t):
		self.clear()
		self.change_dx()
		self.move_stars()
		self.paint_stars()
		self.move_text()
		self.paint_text(self.text_x)
		self.move_ship()
		self.paint_ship(self.ship_x)
		return [ min(255, x) for x in self.frame ]


class TextSource(object):
	def __init__(self, default_lines, infile):
		self.default_lines = default_lines
		self.default_i = 0
		self.fifo = infile
		self.fd = self.fifo.fileno()
		self.fifo_data = ''

	def fifo_get_line(self):
		while True:
			s = self.fifo_data.split('\n', 1)
			if len(s) == 2:
				self.fifo_data = s[1]
				return s[0]
			d = ''
			if select.select([sys.stdin], [], [], 0) == ([sys.stdin], [], []):
				d = sys.stdin.read(1024)
			self.fifo_data += d
			if d == '':
				return ''

	def get_line(self):
		l = self.fifo_get_line()
		if l == "":
			l = self.default_lines[self.default_i]
			self.default_i += 1
			self.default_i %= len(self.default_lines)
		return l


text_source = TextSource( ["https://github.com/techinc/ledmatrix", "Welcome to Techinc! :-D"], sys.stdin )
ani = SpaceAni(120, 8, text_source)

f = font.Font()
#f.print_glyphs()

i=0
t0 = time.time()

while True:
	i+=1
	i%=120
	sys.stdout.write(''.join(chr((x/2)&0x7f) for x in ani.next_frame(i, time.time()-t0))+"\x80")
	sys.stdout.flush()
	time.sleep(.005)
