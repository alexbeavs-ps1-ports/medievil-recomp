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
# the branch delay-slot LUI in its cleanup routine. New table: 2048 + sentinel.
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
SUBDIVISIONS = {
    0x80022108: (0x290A1000, 0x290A0000),
    0x8002279C: (0x290A1000, 0x290A0000),
}

# Function boundaries and the optional subdivision thresholds are verified
# against the owned executable, independently of any generated C output.
ENGINE_GUARDS = {
    0x800514FC: 0x3C02800F, 0x80051500: 0x944217BE,
    0x8005176C: 0x27BDFF08, 0x80051770: 0xAFB500E4,
    0x80021CEC: 0x27BDFF50,
    0x8007A02C: 0x27BDFFE0, 0x8007A030: 0xAFB00010,
    0x8007A0C0: 0x27BDFFE8, 0x8007A0C4: 0xAFB00010,
    0x80022108: 0x290A1000, 0x8002279C: 0x290A1000,
    0x80021EAC: 0x1900FFCF, 0x8002249C: 0x1D000006, 0x800224B0: 0x0501FE4E,
}

def main():
    disc = Disc(Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / 'disc/MediEvil (USA).cue')
    data = disc.read('MEDIEVIL.EXE')
    base = struct.unpack_from('<I', data, 0x18)[0]
    for address, expected in ENGINE_GUARDS.items():
        if struct.unpack_from('<I', data, 0x800 + address - base)[0] != expected:
            raise ValueError(f'Terrain engine guard failed at {address:#x}')
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
description = "Adaptive rendering, extended terrain distance and configurable subdivision."
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

[[option]]
feature = "widescreen"
id = "draw_distance"
label = "Draw distance"
type = "choice"
default = "3x"

[[option.choice]]
value = "1x"
label = "Original"

[[option.choice]]
value = "2x"
label = "Extended (2x)"

[[option.choice]]
value = "3x"
label = "Extended (3x)"

[[option]]
feature = "widescreen"
id = "subdivision_bypass"
label = "Bypass terrain subdivision"
type = "boolean"
default = "true"
'''
    lba, _ = disc.files['MEDIEVIL.EXE']
    for address, (expected, replacement) in {**RELOCATIONS, **SUBDIVISIONS}.items():
        file_offset = 0x800 + address - base
        actual = struct.unpack_from('<I', data, file_offset)[0]
        if actual != expected:
            raise ValueError(f'Capture relocation guard failed at {address:#x}: {actual:#x}')
        condition = 'when = { subdivision_bypass = "true" }\n' if address in SUBDIVISIONS else ''
        text += f'''
# Capture-list reference at {address:#010x}.
[[patch]]
feature = "widescreen"
target = "disc_user"
offset = {lba * 2048 + file_offset}
expected = "{struct.pack('<I',expected).hex(' ')}"
replace = "{struct.pack('<I',replacement).hex(' ')}"
{condition}'''
    manifest.parent.mkdir(parents=True, exist_ok=True)
    manifest.write_text(text, encoding='utf-8', newline='\n')
    original = next(image for image in profile['images'] if image['files'] == ['MEDIEVIL.EXE'] and 'mod_package' not in image)
    names = ('medievil-wide', 'medievil-wide-subdivision-bypass')
    profile['images'] = [image for image in profile['images'] if image.get('mod_package') not in names]
    profile['mod_packages'] = []
    # Compile both selections. Changing a visual option must not dirty the
    # engine and drop its entire terrain funnel into interpreted execution.
    for bypass, name in enumerate(names):
        patched = bytearray(data[0x800:])
        patches = {**RELOCATIONS, **(SUBDIVISIONS if bypass else {})}
        for address, (_, replacement) in patches.items():
            struct.pack_into('<I', patched, address - base, replacement)
        profile['images'].append({
            'method': 'fixed_address_extents', 'mod_package': name,
            'allow_missing': True,
            'excluded_ranges': copy.deepcopy(original['excluded_ranges']),
            'extents': [{
                'file': 'MEDIEVIL.EXE', 'file_offset': '0x800',
                'base': hex(base), 'address': hex(base), 'load_addr': hex(base),
                'size': hex(len(patched)), 'sha256': hashlib.sha256(patched).hexdigest(),
                'entries': [hex(struct.unpack_from('<I', data, 0x10)[0])],
            }],
        })
        profile['mod_packages'].append({
            'name': name, 'id': 'medievil.enhancement.widescreen', 'version': '1.0.0',
            'manifest': manifest.relative_to(ROOT).as_posix(),
            'manifest_sha256': hashlib.sha256(text.encode()).hexdigest(),
            'features': {'widescreen': {'subdivision_bypass': bool(bypass)}},
            'plugins': ['medievil.widescreen'],
        })
    profile['expected_records'] = 29
    profile_path.write_text(json.dumps(profile, indent=2)+'\n', newline='\r\n')
    print('Verified engine guards; added native AOT producers for both subdivision selections')

if __name__ == '__main__':
    main()
