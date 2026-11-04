"""Prepare reviewed public repository, card and matching-source archives. No upload or card writes."""
import hashlib
import json
import subprocess
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
RELEASE = json.loads((ROOT/'localization/release.json').read_text())
VERSION = RELEASE['version']
OUT = ROOT/'dist'/('github-'+VERSION)
PREFIX = 'CamDesk-v'+VERSION+'-50D109'

def digest(data):
    return hashlib.sha256(data).hexdigest()

def encode(value):
    return (json.dumps(value, ensure_ascii=False, indent=2)+'\n').encode('utf-8')

def put_archive(name, files):
    with zipfile.ZipFile(OUT/name, 'w', zipfile.ZIP_DEFLATED) as archive:
        for path, data in sorted(files.items()):
            if path.startswith(('/', '\\')) or '..' in Path(path).parts:
                raise ValueError('Unsafe archive path: '+path)
            info = zipfile.ZipInfo(path)
            info.create_system = 3
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = (0o100755 if data.startswith(b'#!') else 0o100644) << 16
            archive.writestr(info, data)
    # Independent reopen, CRC validation, exact member inventory and content hashes.
    with zipfile.ZipFile(OUT/name) as archive:
        assert archive.testzip() is None
        assert set(archive.namelist()) == set(files)
        for path, data in files.items():
            assert digest(archive.read(path)) == digest(data), path

def main():
    OUT.mkdir(parents=True, exist_ok=True)
    repository = {}
    def add(path):
        repository[path] = (ROOT/path).read_bytes()
    for path in ['README.md', 'README.en.md', 'LICENSE', 'THIRD_PARTY_NOTICES.md', '.gitattributes', 'upstream.lock.json']:
        add(path)
    repository['.gitignore'] = b'upstream/\nbuild/\ndist/\nbackups/\ndownloads/\n__pycache__/\n*.zip\n'
    for folder in ['ui', 'modules/camdesk', 'tests', 'localization']:
        for path in (ROOT/folder).rglob('*'):
            if not path.is_file() or any(p in {'build', '__pycache__', '.git'} for p in path.parts):
                continue
            relative = path.relative_to(ROOT).as_posix()
            if relative == 'localization/README.md':
                continue  # old workstation-specific development history
            if relative == 'localization/fonts/source-font.ttc':
                continue  # unused local copy; regeneration uses the documented system font package
            add(relative)
    scripts = ['prepare-zh.py', 'prepare_ui.py', 'fetch-localization.py', 'build-core.sh',
               'build-module.sh', 'build-cjk-fonts.py', 'build-ui-font.py', 'build-ui-face.py',
               'test-ui.sh', 'test-zh.sh', 'test.sh', 'render-ui-build.py', 'stage-zh.py', 'package-github.py']
    for name in scripts:
        add('scripts/'+name)
    for path in (ROOT/'docs/release').glob('*.md'):
        if path.name != 'README.zh.md':
            add(path.relative_to(ROOT).as_posix())
    for name, source in [('status', 'home'), ('liveview', 'monitor'), ('manual-aperture', 'manual-aperture')]:
        repository['docs/images/'+name+'.png'] = (ROOT/f'dist/camdesk-{VERSION}/actual-{source}-bitmap.png').read_bytes()
    # Public staging expects the general instructions rather than a private card backup path.
    repository['docs/中文使用说明.txt'] = (ROOT/'docs/release/INSTALL.zh.md').read_bytes()
    for path, data in repository.items():
        if path.endswith(('.md', '.txt', '.py', '.sh', '.json')):
            for private in [b'X:\\CodexProjects', b'C:\\Users\\Peace', b'wxid_', b'root@Peace']:
                # The packager's exclusion patterns are legitimate source text, not leaked paths.
                if path != 'scripts/package-github.py' and private in data:
                    raise ValueError('Workstation content in public file: '+path)
    github_name = PREFIX+'-github.zip'
    put_archive(github_name, repository)
    # Prepared source subset: tracked files actually present in this pinned partial checkout,
    # plus all generated source and the exact module source used for the binary.
    upstream = ROOT/'upstream/ml'
    lock = json.loads((ROOT/'upstream.lock.json').read_text())
    head = subprocess.check_output(['git', '-C', str(upstream), 'rev-parse', 'HEAD']).decode().strip()
    assert head == lock['commit']
    tracked = subprocess.check_output(['git', '-C', str(upstream), 'ls-files', '-z']).decode().split('\0')
    snapshot = {}
    for path in tracked:
        if path and not path.lower().endswith('.fir') and (upstream/path).is_file():
            snapshot['upstream/ml/'+path] = (upstream/path).read_bytes()
    for name in ['camdesk_ui.c', 'cjk_font.c', 'cjk_font.h', 'face.c', 'face.h', 'nordic.h',
                 'nordic_draw.c', 'zh_translation.c', 'zh_translation.h']:
        snapshot['upstream/ml/src/'+name] = (upstream/'src'/name).read_bytes()
    for path in (ROOT/'modules/camdesk').iterdir():
        if path.is_file():
            built_source = upstream/'modules/camdesk'/path.name
            assert built_source.read_bytes() == path.read_bytes(), path.name
            snapshot['upstream/ml/modules/camdesk/'+path.name] = path.read_bytes()
    source_manifest = {'version': VERSION, 'upstream_commit': head, 'target': '50D.109',
        'kind': 'prepared source subset used for this target; no Git history',
        'files': {name: digest(data) for name, data in sorted(snapshot.items())}}
    source = dict(repository)
    source.update(snapshot)
    source['SOURCE-MANIFEST.json'] = encode(source_manifest)
    source_name = PREFIX+'-source.zip'
    put_archive(source_name, source)
    # Never rebuild or edit distributed binaries during release packaging.
    private_manifest = json.loads((ROOT/f'dist/camdesk-{VERSION}/manifest.json').read_text())
    card = {}
    for path, expected in private_manifest['files'].items():
        data = (ROOT/f'dist/camdesk-{VERSION}/card'/path).read_bytes()
        assert digest(data) == expected, path
        if path != '使用说明.txt':
            card[path] = data
    for name, path in [('README.zh.md', 'README.md'), ('README.en.md', 'README.en.md'),
                       ('INSTALL.zh.md', 'docs/release/INSTALL.zh.md'), ('INSTALL.en.md', 'docs/release/INSTALL.en.md'),
                       ('THIRD_PARTY_NOTICES.md', 'THIRD_PARTY_NOTICES.md'), ('LICENSE', 'LICENSE')]:
        card[name] = repository[path]
    # Root guides must also work when extracted onto the card, without repository-only images/links.
    card['README.zh.md'] = ('# CamDesk '+VERSION+'\n\n成品平台：EOS 50D／佳能固件1.0.9（50D.109）。需已安装魔灯。\n\n项目面向Magic Lantern相机，其他机型尚未验证，需适配重新编译。请先阅读INSTALL.zh.md；同时提供的源码包为'+source_name+'。\n\n最新实机回归待完成。\n').encode('utf-8')
    card['README.en.md'] = ('# CamDesk '+VERSION+'\n\nBinary target: EOS 50D / Canon firmware 1.0.9 (50D.109). A working Magic Lantern installation is required.\n\nThe project targets Magic Lantern cameras; other models require porting and rebuilding. Read INSTALL.en.md first. Matching source: '+source_name+'.\n\nLatest camera regression is pending.\n').encode()
    public_manifest = {key: private_manifest[key] for key in ['version', 'target', 'qemu', 'upstream', 'translation_source', 'translations', 'glyphs_per_size', 'font_sizes', 'required_symbols', 'camera_validation', 'instrument_font']}
    public_manifest.update({'project_scope': 'Magic Lantern cameras; only EOS 50D has development/camera-test history',
        'binary_compatibility': 'Canon EOS 50D with Canon firmware 1.0.9 only',
        'matching_source_archive': source_name, 'files': {name: digest(data) for name, data in sorted(card.items())}})
    card['manifest.json'] = encode(public_manifest)
    card_name = PREFIX+'-card.zip'
    put_archive(card_name, card)
    names = [github_name, source_name, card_name]
    (OUT/'SHA256SUMS.txt').write_text(''.join(digest((OUT/name).read_bytes())+'  '+name+'\n' for name in names), encoding='utf-8')
    (OUT/'RELEASE.md').write_bytes(repository['docs/release/RELEASE.md'])
    (OUT/'UPLOAD.md').write_bytes(repository['docs/release/UPLOAD.md'])
    report = {'version': VERSION, 'archives': {name: {'bytes': (OUT/name).stat().st_size,
        'sha256': digest((OUT/name).read_bytes())} for name in names},
        'repository_files': len(repository), 'prepared_upstream_files': len(snapshot),
        'card_files': len(card), 'binary_hashes_unchanged': True,
        'archive_crc_inventory_and_content_hashes': 'passed', 'github_published': False}
    (OUT/'PACKAGE-REPORT.json').write_bytes(encode(report))
    print(json.dumps(report, ensure_ascii=False, indent=2))

if __name__ == '__main__':
    main()
