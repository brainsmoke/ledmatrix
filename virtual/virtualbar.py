
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
        h = int(w*height//width)
    else:
        w = int(h*width//height)

    dotsize = int(7.5*w/width/10)

    xmin, dx = .5/width, 1./width
    ymin, dy = .5/height, 1./height
    
    pixels = [ (xmin+x*dx,ymin+y*dy) for y in range(height) for x in range(width) ]

    v = virtualdisplay.VirtualDisplay(pixels, (w,h), dotsize=dotsize, inverse_gamma=1)
    s = b''
    while True:
        s = s+sys.stdin.buffer.read(len(pixels)+1)
        if s == b'':
            break
        last=0
        for i,c in enumerate(s):
            if c & 0x80:
                v.write(bytes( (c&0x7f)*2*q for c in s[last:i+1][:len(pixels)] for q in (1,0,0) ))
                last = i+1

        s = s[last:]

