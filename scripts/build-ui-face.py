"""Build the UI's neutral bold face from pinned Inter 4.1, with native 4-bit AA."""
import hashlib,json,math,struct
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
ROOT=Path(__file__).resolve().parent.parent
HEIGHTS=(12,20,23,28,32,40,56,64,96,128,160)

def main():
    source=ROOT/'ui/fonts/InterDisplay-Bold.ttf'
    manifest=json.loads((ROOT/'ui/fonts/source.json').read_text())
    assert hashlib.sha256(source.read_bytes()).hexdigest()==manifest['files'][source.name]
    records=bytearray()
    for height in HEIGHTS:
        font=ImageFont.truetype(str(source),height-1)
        for code in range(32,127):
            ch=chr(code);bbox=font.getbbox(ch)
            base_width=max(math.ceil(font.getlength(ch)),bbox[2]-min(0,bbox[0]))+1
            image=Image.new('L',(base_width,height));draw=ImageDraw.Draw(image)
            draw.text((-min(0,bbox[0]),-1),ch,font=font,fill=255)
            # The physical LCD compresses 720 bitmap columns to a 640-wide 4:3 view.
            width=math.ceil(base_width*9/8)
            image=image.resize((width,height),Image.Resampling.LANCZOS)
            samples=[(v*15+127)//255 for v in image.tobytes()]
            data=bytearray();i=0
            while i<len(samples):
                n=1
                while n<16 and i+n<len(samples) and samples[i+n]==samples[i]:n+=1
                data.append(samples[i]*16+n-1);i+=n
            assert width<=256
            records+=struct.pack('<4HI',height,code,width,width,len(data))+data
    output=ROOT/'localization/fonts/cdneutral.cdf'
    result=b'CDF2'+struct.pack('<3I',1,len(HEIGHTS)*95,len(records)+16)+records
    output.write_bytes(result)
    (ROOT/'localization/fonts/NEUTRAL-FONT-LICENSE.txt').write_bytes((ROOT/'ui/fonts/LICENSE.txt').read_bytes())
    (ROOT/'ui/fonts/bitmap-manifest.json').write_text(json.dumps({'name':'CamDesk Neutral Bold','derived_from':'Inter Display Bold 4.1','heights':HEIGHTS,'horizontal_pixel_aspect_compensation':'9/8','coverage_bits':4,'encoding':'RLE16','bytes':len(result),'sha256':hashlib.sha256(result).hexdigest()},indent=2),encoding='utf-8')
    print(f'Neutral bold face: {len(HEIGHTS)} sizes, {len(result)} bytes, 4-bit coverage')
if __name__=='__main__':main()
