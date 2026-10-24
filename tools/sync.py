#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
noz_exhbition のソース(.cpp / .h)は、CP932(BOM なし)で置いてある。改行は CRLF(.gitattributes の eol=crlf で、git が変換する)。
Linux 側でそのまま編集すると壊しやすいので、UTF-8 の作業用の写しを作って、そこを編集してから戻す。
(quiz_academy の tools/sync.py を、このリポジトリ用に直したもの。違いは、ソースが src/ の下にあることと、vcxproj が NozExhibition.vcxproj なこと)
data/ の中の .txt(デモの順番・案内の文・サークル紹介)も CP932 + CRLF なので、同じように扱う。

  python3 tools/sync.py export <作業用のフォルダ>   … リポジトリ → 作業用(UTF-8 + LF)
  python3 tools/sync.py import <作業用のフォルダ>   … 作業用 → リポジトリ(CP932 + CRLF)。NozExhibition.vcxproj のファイルの一覧も、今ある .cpp / .h に合わせ直す
  python3 tools/sync.py check                       … リポジトリの中のソースが、全部 CP932 + CRLF で読めるかだけ調べる

import は、書いた後に必ず検証する(CP932 で読める・改行が全部 CRLF・BOM が無い・NUL 文字が無い・作業用の文面と一致する)。
検証に通らなければ、そこで止まる(コミットしないこと)。
"""
import os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
VCXPROJ = 'NozExhibition.vcxproj'
SRC_DIRS = ['src']   # .cpp / .h を写すフォルダ(中のフォルダも全部)
VCXPROJ2 = VCXPROJ  # ファイルの一覧を src/ のソースに合わせ直すプロジェクト

def src_dir_files(base):
    # SRC_DIRS の中の .cpp / .h(base からの相対のパス)
    files = []
    for top in SRC_DIRS:
        for d, ds, fs in os.walk(os.path.join(base, top)):
            ds.sort()
            for f in sorted(fs):
                if f.endswith('.cpp') or f.endswith('.h'): files.append(os.path.relpath(os.path.join(d, f), base))
    return files

def repo_files():
    # ソース(直下の .cpp / .h)と、data/ の中の .txt(UI の文言など)。vcxproj の一覧に入れるのはソースだけ
    files = [f for f in sorted(os.listdir(ROOT)) if f.endswith('.cpp') or f.endswith('.h')]
    files += src_dir_files(ROOT)
    for d, _, fs in os.walk(os.path.join(ROOT, 'data')):
        for f in sorted(fs):
            if f.endswith('.txt'): files.append(os.path.relpath(os.path.join(d, f), ROOT))
    return files

def work_files(work):
    files = [f for f in sorted(os.listdir(work)) if f.endswith('.cpp') or f.endswith('.h')]
    files += src_dir_files(work)
    for d, _, fs in os.walk(os.path.join(work, 'data')):
        for f in sorted(fs):
            if f.endswith('.txt'): files.append(os.path.relpath(os.path.join(d, f), work))
    return files

def verify(path, text=None):
    b = open(path, 'rb').read()
    t = b.decode('cp932')
    assert not b.startswith(b'\xef\xbb\xbf'), f'BOM: {path}'
    assert b'\x00' not in b, f'NUL: {path}'
    assert b.count(b'\n') == b.count(b'\r\n') and b.count(b'\r') == b.count(b'\r\n'), f'改行: {path}'
    if text is not None: assert t.replace('\r\n', '\n') == text, f'文面が違う: {path}'

def cmd_export(work):
    for f in repo_files():
        t = open(os.path.join(ROOT, f), 'rb').read().decode('cp932').replace('\r\n', '\n')
        p = os.path.join(work, f)
        os.makedirs(os.path.dirname(p) or '.', exist_ok=True)
        open(p, 'w', encoding='utf-8', newline='').write(t)
    print('export:', len(repo_files()), 'files ->', work)

def update_vcxproj():
    # (quiz_academy の当時の器の物。このリポジトリでは呼ばない)
    p = os.path.join(ROOT, VCXPROJ)
    raw = open(p, 'rb').read()
    bom = raw.startswith(b'\xef\xbb\xbf')
    v = raw.decode('utf-8-sig')
    nl = '\r\n' if '\r\n' in v else '\n'
    v = v.replace('\r\n', '\n')
    cpps = sorted([f for f in os.listdir(ROOT) if f.endswith('.cpp')], key=str.lower)
    hs = sorted([f for f in os.listdir(ROOT) if f.endswith('.h')], key=str.lower)
    items = '  <ItemGroup>\n' + ''.join(f'    <ClCompile Include="{f}" />\n' for f in cpps) + '  </ItemGroup>\n'
    items += '  <ItemGroup>\n' + ''.join(f'    <ClInclude Include="{f}" />\n' for f in hs) + '  </ItemGroup>\n'
    # ClCompile / ClInclude の ItemGroup を、まとめて差し替える(他の設定は触らない)
    pat = re.compile(r'(  <ItemGroup>\n(?:    <Cl(?:Compile|Include) Include="[^"]+" />\n)+  </ItemGroup>\n)+')
    assert len(pat.findall(v)) == 1, 'vcxproj の、ファイルの一覧の場所が見つからない'
    v = pat.sub(lambda m: items, v, count=1)
    out = v.replace('\n', nl).encode('utf-8')
    open(p, 'wb').write((b'\xef\xbb\xbf' if bom else b'') + out)

def update_vcxproj2():
    # NozExhibition.vcxproj のファイルの一覧を、src/ のソースに合わせ直す。無ければ何もしない
    p = os.path.join(ROOT, VCXPROJ2)
    if not os.path.exists(p): return
    raw = open(p, 'rb').read()
    bom = raw.startswith(b'\xef\xbb\xbf')
    v = raw.decode('utf-8-sig')
    nl = '\r\n' if '\r\n' in v else '\n'
    v = v.replace('\r\n', '\n')
    files = src_dir_files(ROOT)
    cpps = sorted([f for f in files if f.endswith('.cpp')], key=str.lower)
    hs = sorted([f for f in files if f.endswith('.h')], key=str.lower)
    win = lambda f: f.replace('/', '\\')
    items = '  <ItemGroup>\n' + ''.join(f'    <ClCompile Include="{win(f)}" />\n' for f in cpps) + '  </ItemGroup>\n'
    items += '  <ItemGroup>\n' + ''.join(f'    <ClInclude Include="{win(f)}" />\n' for f in hs) + '  </ItemGroup>\n'
    pat = re.compile(r'(  <ItemGroup>\n(?:    <Cl(?:Compile|Include) Include="[^"]+" />\n)+  </ItemGroup>\n)+')
    assert len(pat.findall(v)) == 1, VCXPROJ2 + ' の、ファイルの一覧の場所が見つからない'
    v = pat.sub(lambda m: items, v, count=1)
    out = v.replace('\n', nl).encode('utf-8')
    open(p, 'wb').write((b'\xef\xbb\xbf' if bom else b'') + out)

def cmd_import(work):
    # 先に、全部のファイルを CP932 に直してみる。1つでも直せない文字(CP932 に無い記号など)があれば、何も書かずに止まる
    todo = []
    bad = []
    for f in work_files(work):
        t = open(os.path.join(work, f), encoding='utf-8').read().replace('\r\n', '\n')
        crlf = t.replace('\n', '\r\n')
        try:
            b = crlf.encode('cp932')
        except UnicodeEncodeError as e:
            line = crlf.count('\n', 0, e.start) + 1   # (e.start は、改行を CRLF に直した後の文字列での位置)
            bad.append(f'{f}:{line}: CP932 に無い文字 {crlf[e.start]!r}')
            continue
        todo.append((f, t, b))
    if bad:
        print('import: 止めた(何も書いていない)')
        for m in bad: print('  ' + m)
        sys.exit(1)

    n = 0
    for rel, t, b in todo:
        p = os.path.join(ROOT, rel)
        os.makedirs(os.path.dirname(p) or '.', exist_ok=True)
        open(p, 'wb').write(b)
        verify(p, t)
        n += 1
    update_vcxproj2()
    cmd_check()
    print('import:', n, 'files <-', work)

def cmd_check():
    for f in repo_files():
        verify(os.path.join(ROOT, f))
        assert os.path.getsize(os.path.join(ROOT, f)) > 0, f + ' が空'
    print('check: ok', len(repo_files()), 'files')

if __name__ == '__main__':
    if len(sys.argv) >= 3 and sys.argv[1] == 'export': cmd_export(sys.argv[2])
    elif len(sys.argv) >= 3 and sys.argv[1] == 'import': cmd_import(sys.argv[2])
    elif len(sys.argv) >= 2 and sys.argv[1] == 'check': cmd_check()
    else: print(__doc__)
