"""Generate six CRBF sizes using the open WenQuanYi Micro Hei font."""
import json, math, struct
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

ROOT=Path(__file__).resolve().parent.parent
FONT='/usr/share/fonts/truetype/wqy/wqy-microhei.ttc'

def main():
    translations=json.loads((ROOT/'localization/zh_CN.json').read_text(encoding='utf-8'))
    chars=sorted({c for v in list(translations.values())+list(translations) for c in v if ord(c)>=128})
    output=ROOT/'localization/fonts';output.mkdir(exist_ok=True)
    for size in (12,20,23,28,32,40):
        font=ImageFont.truetype(FONT,size-2)
        result=bytearray(b'CRBF'+struct.pack('<IHI',1,size,len(chars)))
        for ch in chars:
            bbox=font.getbbox(ch);width=max(1,min(64,math.ceil(font.getlength(ch))+2))
            image=Image.new('1',(width,size));draw=ImageDraw.Draw(image)
            draw.text((1-bbox[0],(size-(bbox[3]-bbox[1]))//2-bbox[1]),ch,font=font,fill=1)
            result+=struct.pack('<IHH',ord(ch),width,width+1)
            for y in range(size):
                row=bytearray((width+7)//8)
                for x in range(width):
                    if image.getpixel((x,y)):row[x//8]|=1<<(x%8)
                result+=row
        (output/f'cjk{size}.rbf').write_bytes(result)
        print(f'CJK {size}px: {len(chars)} glyphs, {len(result)} bytes')
    license_file=Path('/usr/share/doc/fonts-wqy-microhei/copyright')
    (output/'FONT-LICENSE.txt').write_bytes(license_file.read_bytes())

if __name__=='__main__':main()
