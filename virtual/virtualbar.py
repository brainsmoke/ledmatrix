
import sys, json, math
import virtualdisplay


if __name__ == '__main__':
    width, height = 120, 8
    if len(sys.argv) > 2:
        width = int(sys.argv[1])
        height = int(sys.argv[2])

    w = 1200
    h = 600

    if width*h > height*w:
        h = int(w*height/width)
    else:
        w = int(h*width/height)

    dotsize = int(7.5*w/width/10)

    xmin, dx = .5/width, 1./width
    ymin, dy = .5/height, 1./height
    
    pixels = [ (xmin+x*dx,ymin+y*dy) for y in range(height) for x in range(width) ]

    v = virtualdisplay.VirtualDisplay(pixels, (w,h), dotsize=dotsize, inverse_gamma=1)
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

