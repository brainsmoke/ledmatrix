
import sys

import pygame
pygame.init()

black = (0, 0, 0)

class VirtualDisplay(object):
    def __init__(self, pixels, windowsize=(720, 480), dotsize=12, inverse_gamma=2.2, timed_io=None):
        self.timed_io = timed_io
        self.w, self.h = windowsize
        self.dotsize = float(dotsize)/self.h
        self.pixels = pixels
        self.screen = pygame.display.set_mode(windowsize, pygame.RESIZABLE|pygame.DOUBLEBUF)
        pygame.key.set_repeat(500,25)
        self.screen.fill(black)
        self.gamma_map = tuple( int( (x/255.)**(1/inverse_gamma) * 255. ) for x in xrange(256) )

    def draw_led(self, surf, x, y, color, r):
         pygame.draw.circle(surf, color, (x, y), r)

    def render(self, data):
        surf = pygame.display.get_surface()
        r = int(self.dotsize*self.h/2.)

        for i in xrange(min(len(self.pixels), len(data)/3)):
            color = tuple( self.gamma_map[ord(x)] for x in data[i*3:i*3+3] )
            x, y = self.pixels[i]
            self.draw_led(surf, int(x*self.w), int(y*self.h), color, r)

    def check_events(self):
        for event in pygame.event.get():
            if event.type == pygame.KEYDOWN:
                if self.timed_io:
                    self.timed_io.add_input(event.unicode)
            if event.type == pygame.QUIT:
                if self.timed_io:
                    self.timed_io.add_input('q')
            if event.type == pygame.VIDEORESIZE:
                self.screen = pygame.display.set_mode((event.w, event.h), pygame.RESIZABLE|pygame.DOUBLEBUF)
                self.w = event.w
                self.h = event.h

    def write(self, data):
        self.check_events()
        self.render(data)
        pygame.display.flip()
        self.screen.fill(black)
