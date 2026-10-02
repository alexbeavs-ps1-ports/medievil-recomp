"""Bind the render-only capture-list relocation to the owned USA disc.

Writes guarded declarative instruction patches and an exact AOT mod producer.
No disc assets or generated C are written to the source tree.
"""
import copy
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'psxrecomp/tools'))
from aot_overlay_pipeline import Disc

# All constructors/readers/writers of the 100-entry marked-cell list, including
# the branch delay-slot LUI in its cleanup routine. New table: 1024 + sentinel.
RELOCATIONS = {
    0x80051104: (0x3C03800F, 0x3C038030),
    0x80051108: (0xAC60EA24, 0xAC600000),
    0x80051560: (0x3C02800F, 0x3C028030),
    0x8005156C: (0x3C02800F, 0x3C028030),
    0x80051570: (0x8C43EA24, 0x8C430000),
    0x8005157C: (0x2444EA24, 0x24440000),
    0x80051F54: (0x3C02800F, 0x3C028030),
    0x80051F58: (0x2442EA24, 0x24420000),
    0x80052094: (0x3C03800F, 0x3C038030),
    0x80052098: (0x2463EA24, 0x24630000),
}

def main():
    disc = Disc(Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / 'disc/MediEvil (USA).cue')
    data = disc.read('MEDIEVIL.EXE')
    base = struct.unpack_from('<I', data, 0x18)[0]
    profile_path = ROOT / 'aot/overlays.json'
    profile = json.loads(profile_path.read_text())
    sha = hashlib.sha256(Path(disc.binary).read_bytes()).hexdigest()
    if sha != profile['disc_hashes']['sha256']:
        raise ValueError('Unsupported disc revision')
    # This overlay's packed horizontal/vertical outcode test uses the same
    # predicate as the main renderer. Its instruction guard is unique among
    # the disc's overlays, so an unrelated resident image cannot opt in.
    tl = disc.read('OVERLAYS/TL.BIN')
    tl_offset, tl_word = 0x9E4, 0x144001A5
    if struct.unpack_from('<I', tl, tl_offset)[0] != tl_word:
        raise ValueError('TL polygon-rejection guard changed')
    collisions = [name for name in disc.files if name.startswith('OVERLAYS/')
                  and len(disc.read(name)) >= tl_offset + 4
                  and struct.unpack_from('<I', disc.read(name), tl_offset)[0] == tl_word]
    if collisions != ['OVERLAYS/TL.BIN']:
        raise ValueError(f'TL culling instruction is not unique: {collisions}')
    manifest = ROOT / 'mods/preloaded/packages/medievil.enhancement.widescreen/1.0.0/manifest.toml'
    text = '''format_version = 7
id = "medievil.enhancement.widescreen"
version = "1.0.0"
name = "MediEvil Adaptive View"
author = "MediEvil Recompiled"
license = "GPL-3.0-only"
description = "Native-wide rendering with expanded terrain capture and polygon buffers."
resolver = "declarative"
save_compatibility = "shared"

[[target]]
game_id = "SCUS-94227"
disc_sha256 = "''' + sha + '''"

[[feature]]
id = "widescreen"
name = "Adaptive View"
description = "Render additional world geometry to fit the window. Menus and movies retain their original proportions."
group = "Display"
default_enabled = true

[[option]]
feature = "widescreen"
id = "aspect"
label = "View"
type = "choice"
default = "Fit"

[[option.choice]]
value = "Fit"
label = "Fit to Window"

[[option.choice]]
value = "4:3"
label = "4:3"

[[option.choice]]
value = "16:9"
label = "16:9"

[[option.choice]]
value = "21:9"
label = "21:9"

[[option.choice]]
value = "32:9"
label = "32:9"

[[plugin]]
feature = "widescreen"
id = "medievil.widescreen"
'''
    lba, _ = disc.files['MEDIEVIL.EXE']
    for address, (expected, replacement) in RELOCATIONS.items():
        file_offset = 0x800 + address - base
        actual = struct.unpack_from('<I', data, file_offset)[0]
        if actual != expected:
            raise ValueError(f'Capture relocation guard failed at {address:#x}: {actual:#x}')
        text += f'''
# Capture-list reference at {address:#010x}.
[[patch]]
feature = "widescreen"
target = "disc_user"
offset = {lba * 2048 + file_offset}
expected = "{struct.pack('<I',expected).hex(' ')}"
replace = "{struct.pack('<I',replacement).hex(' ')}"
'''
    manifest.parent.mkdir(parents=True, exist_ok=True)
    manifest.write_text(text, encoding='utf-8', newline='\n')
    original = next(image for image in profile['images'] if image['files'] == ['MEDIEVIL.EXE'] and 'mod_package' not in image)
    patched = bytearray(data[0x800:])
    for address, (_, replacement) in RELOCATIONS.items():
        struct.pack_into('<I', patched, address - base, replacement)
    # The extractor sees the original disc. Declare the patched EXE body as
    # a verified extent so it receives an independent native producer.
    modified = {
        'method': 'fixed_address_extents', 'mod_package': 'medievil-wide',
        'allow_missing': True,
        'excluded_ranges': copy.deepcopy(original['excluded_ranges']),
        'extents': [{
            'file': 'MEDIEVIL.EXE', 'file_offset': '0x800',
            'base': hex(base), 'address': hex(base), 'load_addr': hex(base),
            'size': hex(len(patched)), 'sha256': hashlib.sha256(patched).hexdigest(),
            'entries': [hex(struct.unpack_from('<I', data, 0x10)[0])],
        }],
    }
    profile['images'] = [image for image in profile['images'] if image.get('mod_package') != 'medievil-wide'] + [modified]
    profile['mod_packages'] = [{
        'name': 'medievil-wide', 'id': 'medievil.enhancement.widescreen', 'version': '1.0.0',
        'manifest': manifest.relative_to(ROOT).as_posix(),
        'manifest_sha256': hashlib.sha256(text.encode()).hexdigest(),
        'features': {'widescreen': {}}, 'plugins': ['medievil.widescreen'],
    }]
    profile['expected_records'] = 28
    profile_path.write_text(json.dumps(profile, indent=2)+'\n', newline='\r\n')
    print('Verified 10 capture-list guards and unique TL cull instruction; added patched native AOT producer')

if __name__ == '__main__':
    main()
