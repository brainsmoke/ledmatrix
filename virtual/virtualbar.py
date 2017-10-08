
import sys, json, math
import virtualdisplay

if __name__ == '__main__':
    xmin, dx = 1/240., 1/120.
    ymax, dy = 15/16., 1/8.
    
    pixels = [ (xmin+x*dx,ymax-y*dy) for y in range(8) for x in range(120) ]

    w, h = 1200, 80
    v = virtualdisplay.VirtualDisplay(pixels, (w,h), dotsize=7, inverse_gamma=1)
    s = ''
    while True:
        s = s+sys.stdin.read(len(pixels)+1)
        if s == '':
            break
        last=0
        for i,c in enumerate(s):
            if ord(c) & 0x80:
                v.write(''.join(chr((ord(c)&0x7f)*2)+"\0\0" for c in s[last:i+1][:len(pixels)]))
                last = i+1

        s = s[last:]

