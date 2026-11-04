"""Render the C renderer's trace with the exact packaged Latin and CJK bitmap fonts."""
import json,struct
from pathlib import Path
from PIL import Image,ImageDraw
ROOT=Path(__file__).resolve().parent.parent

def fonts():
    result={}
    for size,name in [(12,'term12'),(20,'term20'),(23,'argnor23'),(28,'argnor28'),(32,'argnor32'),(40,'cdnord40')]:
        path=ROOT/('localization/fonts' if size==40 else 'upstream/ml/data/fonts')/(name+'.rbf')
        data=path.read_bytes();header=struct.unpack_from('<II64s11I',data)
        charsize,_,height,_,first,last,_,wmap,cmap,_,_=header[3:]
        stride=charsize//height;glyphs={}
        for cp in range(first,last+1):
            width=data[wmap+cp-first];glyph=Image.new('1',(max(width,1),height))
            for y in range(height):
                for x in range(width):
                    if data[cmap+(cp-first)*charsize+y*stride+x//8]&(1<<(x%8)):glyph.putpixel((x,y),1)
            glyphs[cp]=(glyph,width)
        data=(ROOT/f'localization/fonts/cjk{size}.rbf').read_bytes();count=struct.unpack_from('<I',data,10)[0];pos=14
        for _ in range(count):
            cp,w,advance=struct.unpack_from('<IHH',data,pos);pos+=8;stride=(w+7)//8
            glyph=Image.new('1',(w,size))
            for y in range(size):
                for x in range(w):
                    if data[pos+y*stride+x//8]&(1<<(x%8)):glyph.putpixel((x,y),1)
            pos+=size*stride;glyphs[cp]=(glyph,advance)
        result[size]=glyphs
    return result

def color(index):
    if 38<=index<=79:
        v=round((index-38)*255/41);return (v,v,v)
    return {0:(85,90,94),1:(255,255,255),2:(0,0,0),8:(235,70,65),19:(230,162,72)}.get(index,(120,120,120))

def main():
    release=json.loads((ROOT/'localization/release.json').read_text())
    output=ROOT/('dist/camdesk-'+release['version']);output.mkdir(parents=True,exist_ok=True)
    palette=[]
    for index in range(256):palette.extend(color(index))
    for name in ['monitor','home','manual-aperture']:
        data=(ROOT/f'build/ui-{name}.idx').read_bytes()
        assert len(data)==720*480
        image=Image.frombytes('P',(720,480),data);image.putpalette(palette)
        image=image.convert('RGB').resize((640,480),Image.Resampling.LANCZOS)
        image.save(output/f'actual-{name}-bitmap.png')
        print(output/f'actual-{name}-bitmap.png')
    (output/'preview-note.txt').write_text('由实际C字体解码和绘制输出直接生成，使用相机打包字库；黑白参数屏不含固定示例图片。监视预览的灰色中央区域代表实机取景画面。实机颜色和刷新待核对。\n',encoding='utf-8')
if __name__=='__main__':main()
