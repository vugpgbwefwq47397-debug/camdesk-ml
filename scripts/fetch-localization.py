"""Fetch pinned GPL Chinese translation and font sources, checking each Git blob."""
import concurrent.futures
import hashlib
import json
from pathlib import Path
import urllib.request

ROOT = Path(__file__).resolve().parent.parent
MANIFEST = json.loads((ROOT / 'localization/vendor-manifest.json').read_text())
DEST = ROOT / 'localization/vendor'

def fetch(entry):
    target = DEST / entry['path']
    def matches(data):
        return hashlib.sha1(b'blob ' + str(len(data)).encode() + b'\0' + data).hexdigest() == entry['sha']
    if target.exists() and matches(target.read_bytes()):
        return
    url = f"https://raw.githubusercontent.com/{MANIFEST['repository']}/{MANIFEST['commit']}/{entry['path']}"
    opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))
    with opener.open(url, timeout=45) as response:
        data = response.read()
    if not matches(data):
        raise ValueError('Source checksum mismatch: ' + entry['path'])
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_bytes(data)

if __name__ == '__main__':
    with concurrent.futures.ThreadPoolExecutor(max_workers=6) as pool:
        list(pool.map(fetch, MANIFEST['files']))
    print('Pinned Chinese sources verified:', len(MANIFEST['files']))
