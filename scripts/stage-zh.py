"""Package the matching Chinese core, module and complete font set."""
import hashlib,json,shutil,struct,zipfile
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent
SOURCE=ROOT/'upstream/ml'
CORE=SOURCE/'platform/50D.109/build'
MODULE=SOURCE/'modules/camdesk/build'
RELEASE=json.loads((ROOT/'localization/release.json').read_text())
OUTPUT=ROOT/('dist/camdesk-'+RELEASE['version'])
STAGE=OUTPUT/'card'

def main():
    face=(ROOT/'localization/fonts/cdneutral.cdf').read_bytes()
    face_manifest=json.loads((ROOT/'ui/fonts/bitmap-manifest.json').read_text())
    if hashlib.sha256(face).hexdigest()!=face_manifest['sha256'] or face[:4]!=b'CDF2' or struct.unpack_from('<I',face,12)[0]!=len(face):
        raise SystemExit('Instrument face does not match tested bitmap atlas')
    symbols={line.split()[-1] for line in (CORE/'50D_109.sym').read_text().splitlines() if line.split()}
    required=set((MODULE/'camdesk.dep').read_text().split())
    if required-symbols:raise SystemExit('Unresolved interfaces: '+str(required-symbols))
    dictionary=json.loads((ROOT/'localization/zh_CN.json').read_text(encoding='utf-8'))
    chars={ord(c) for text in list(dictionary)+list(dictionary.values()) for c in text if ord(c)>=128}
    for size in (12,20,23,28,32,40):
        data=(ROOT/f'localization/fonts/cjk{size}.rbf').read_bytes();count=struct.unpack_from('<I',data,10)[0];offset=14;found=set()
        for _ in range(count):
            cp,w,_=struct.unpack_from('<IHH',data,offset);found.add(cp);offset+=8+((w+7)//8)*size
        if offset!=len(data) or chars-found:raise SystemExit('Incomplete or malformed Chinese font')
    files={'autoexec.bin':CORE/'autoexec.bin','ML/MODULES/50D_109.sym':CORE/'50D_109.sym','ML/MODULES/camdesk.mo':MODULE/'camdesk.mo','使用说明.txt':ROOT/'docs/中文使用说明.txt','ML/FONTS/FONT-LICENSE.txt':ROOT/'localization/fonts/FONT-LICENSE.txt','ML/GPL-LICENSE.txt':SOURCE/'COPYING'}
    for directory,pattern,destination in [('fonts','*.rbf','ML/FONTS'),('cropmks','*.bmp','ML/CROPMKS'),('vram','*.lut','ML/DATA')]:
        for file in (SOURCE/'data'/directory).glob(pattern):files[destination+'/'+file.name]=file
    for file in (ROOT/'localization/fonts').glob('cjk*.rbf'):files['ML/FONTS/'+file.name]=file
    files['ML/FONTS/cdnord40.rbf']=ROOT/'localization/fonts/cdnord40.rbf'
    files['ML/FONTS/cdneutral.cdf']=ROOT/'localization/fonts/cdneutral.cdf'
    files['ML/FONTS/NEUTRAL-FONT-LICENSE.txt']=ROOT/'localization/fonts/NEUTRAL-FONT-LICENSE.txt'
    for name,file in files.items():
        target=STAGE/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(file,target)
    manifest={'version':RELEASE['version'],'target':'50D.109','qemu':False,'scope':RELEASE['scope'],'upstream':json.loads((ROOT/'upstream.lock.json').read_text())['commit'],'translation_source':json.loads((ROOT/'localization/vendor-manifest.json').read_text())['commit'],'translations':len(dictionary),'glyphs_per_size':len(chars),'font_sizes':[12,20,23,28,32,40],'required_symbols':sorted(required),'camera_validation':'pending','files':{name:hashlib.sha256((STAGE/name).read_bytes()).hexdigest() for name in files}}
    manifest['instrument_font']=json.loads((ROOT/'ui/fonts/bitmap-manifest.json').read_text())
    (OUTPUT/'manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')
    archive=OUTPUT/('50D-1.0.9-中文工作台-'+RELEASE['tag']+'.zip')
    with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
        for name in sorted(files):z.write(STAGE/name,name)
        z.writestr('汉化校验记录.json',json.dumps(manifest,ensure_ascii=False,indent=2))
    (OUTPUT/'SHA256SUMS.txt').write_text(hashlib.sha256(archive.read_bytes()).hexdigest()+'  '+archive.name+'\n',encoding='utf-8')
    print(f'Chinese package ready: {len(files)} files, {len(required)} interfaces, {len(dictionary)} translations, {len(chars)} glyphs per size')

if __name__=='__main__':main()
