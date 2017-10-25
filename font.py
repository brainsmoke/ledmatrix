
import sys



class Font(object):

	def extract(self, x, y, w, h, dx, dy, img_w, img_h, data):
		char = [] 
		max_ = 0
		for py in range(y*h,(y+1)*h,dy):
			row = []
			for px in range(x*w,(x+1)*w,dx):
				n = ord(data[px + py*img_w])
				row.append(n)
				if n > 0 and max_ < len(row):
					max_ = len(row)
			char.append(row)
		for i in range(len(char)):
			char[i] = [0]+char[i][:max_]+[0]
	
		return char, max_

	def __init__(self):

		def readline_skip_comments(f):
			while True:
				l = f.readline()
				if l[0] != '#':
					return l

		f = open("font/nyqfont.pgm", 'r')

		if readline_skip_comments(f) != "P5\n":
			print "wrong format"
			sys.exit(1)

		w,h = [ int(x) for x in readline_skip_comments(f).split(' ') ]

		if readline_skip_comments(f) != "255\n":
			sys.exit(1)

		d = bytes(f.read())
		f.close()

		if len(d) != w*h:
			print "bad size"

		self.chars = {}
		self.width = {}
		for x in range(32, 127):
			self.chars[chr(x)], self.width[chr(x)] = self.extract(x-32, 0, 16, 16, 1, 2, w, h, d)
		self.width[' '] = 9

	def text_size(self, text):
		return sum(self.get_glyph(c)[1] for c in text)

	def get_glyph(self, c):
		return (self.chars[c], self.width[c]+1.2)

	def print_glyphs(self):
		print self.chars
		for i in self.chars.keys():
			print i
			for l in self.chars[i]:
				print ''.join("%2x " % (x,) for x in l)

