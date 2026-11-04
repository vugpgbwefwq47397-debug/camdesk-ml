"""Generate a 40px Latin RBF; use the same open font as the Chinese glyph set."""
import math,struct
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
ROOT=Path(__file__).resolve().parent.parent

def main():
    sample=(ROOT/'upstream/ml/data/fonts/argnor32.rbf').read_bytes()
    magic=struct.unpack_from('<II',sample)
    height,stride,first,last=40,5,32,126
    charsize=height*stride
    font=ImageFont.truetype('/usr/share/fonts/truetype/wqy/wqy-microhei.ttc',38)
    widths=bytearray();bitmap=bytearray()
    for code in range(first,last+1):
        c=chr(code);advance=math.ceil(font.getlength(c))+1
        if advance>40:raise ValueError('Glyph wider than RBF cell: '+c)
        widths.append(advance)
        image=Image.new('1',(40,height));draw=ImageDraw.Draw(image)
        # Stable shared baseline; punctuation must not jump to the middle of a number.
        draw.text((0,-1),c,font=font,fill=1)
        for y in range(height):
            row=bytearray(stride)
            for x in range(40):
                if image.getpixel((x,y)):row[x//8]|=1<<(x%8)
            bitmap+=row
    wmap=116;cmap=wmap+len(widths)
    header=struct.pack('<II64s11I',*magic,b'CamDesk Nordic 40',charsize,30,height,40,first,last,0,wmap,cmap,0,0)
    assert len(header)==116
    output=ROOT/'localization/fonts/cdnord40.rbf'
    output.write_bytes(header+widths+bitmap)
    print('Nordic font:',len(widths),'glyphs,',output.stat().st_size,'bytes')
if __name__=='__main__':main()
