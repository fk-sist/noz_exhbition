# noz_exhbition 作業ルール

大学祭(2026-10-24・25。306講義室)のブースの入口の大型モニターに流す、展示用のデモ巡回プログラム。C/C++ + DXライブラリ(DXLib)、Windows。
オーナーは fumiya。ノズキン(`fk-sist/noz_kingdom`)・クイズ OS×UNIX(`fk-sist/quiz_academy` のブランチ `rebuild`)から、部品と素材を写して作っている。

## やりとり

- 常に日本語。fumiya と呼ぶ。相棒として、気安いタメ口(関西弁寄り)で話す。返事は短く、結論から。
- 原案・既存コードに寄り添う。写してきた部品(下の表)は、元のリポジトリの書き方のまま。新しく書く所も、元のリポジトリの書き方に合わせる。
- QMA の画面は、quiz_academy の画面の構成・座標・動きをそのまま使う(勝手に組み替えない)。

## この環境の制約(最重要)

- クラウド側は Linux。**Visual Studio + DXLib のビルドも実行もできない。** 動作確認は、fumiya が手元で pull してビルドして行う。
- 存在を確かめていない DXLib の関数を想像で書かない。構文チェックは `tools/stub/DxLib.h`(宣言だけ):
  `g++ -std=c++14 -fsyntax-only -I tools/stub -I <作業用>/src -I <作業用>/src/qz <作業用>/src/xxx.cpp`
- 報告には、「確かめた所」と「見ていない所(画面・動き)」を分けて書く。

## 文字コードと改行

- `.cpp` / `.h` と `data/*.txt`、`noz/data/gamelist.txt`、`qma/quiz/packs/*.qz` は **CP932、BOM なし、CRLF**。
- ソースは直接編集しない。`tools/sync.py` で UTF-8 の作業用の写しを作って編集し、戻す(quiz_academy と同じ道具):
  ```
  python3 tools/sync.py export <作業用のフォルダ>
  python3 tools/sync.py import <作業用のフォルダ>    # 検証つき。NozExhibition.vcxproj のファイルの一覧も合わせ直す
  ```
- `.md` と `tools/` の中は UTF-8。`NozExhibition.vcxproj` / `.sln` は UTF-8(BOM つき)。

## 作り

- `src/main.cpp`:WinMain・フォント・60fps のループ。Esc で終了。マウスを消し、モニターが消えないように(SetThreadExecutionState)。
- `src/director.cpp`(`demo.h`):デモの巡回。`data/demos.txt` の順に流し、間は暗転。下の帯(案内の文)と BGM。デモを足す時は `DemoDef` を作って `demo_table[]` に足す。
- デモ:`demo_noz.cpp`(ブース案内・ゲーム紹介カード)、`demo_qma.cpp`(突然ですが、ここで問題です)、`demo_circle.cpp`(サークル紹介。写真)、`demo_pv.cpp`(movie/ の動画)。
- 写してきた部品:`src/qz/`(quiz_academy の core/。問題パックの読み込みと出題の準備)、`ui_ease`・`effect_open`(quiz_academy の game/)、`gameGenre`・`resource_manager.h`(noz_kingdom)、
  ノズキンのタイトルの景色とロゴの描き方(`demo_noz.cpp` の中。noz_kingdom の BootManager.cpp から)。元を直したら、こっちにも写し直すか考える。
- 素材:`qma/`(quiz_academy から)、`noz/`(noz_kingdom から。クリップ `noz/img/thumb/`・ゲームの一覧 `noz/data/gamelist.txt`)、`circle/photos/`(サークルの写真。長辺 1920 に縮めた物)、`movie/`(PV。fumiya が置く)。
- 文言:`data/guide.txt`(案内・下の帯)、`data/circle.txt`(サークル紹介の場面)。調整値は各ファイルの冒頭の `#define`。

## コーディング規約

- `fopen` / `sprintf` などを使うソースは、先頭に `#define _CRT_SECURE_NO_WARNINGS`。乱数は `GetRand`。
- コメント(コメントアウトした行を含む)は消さない。処理を外す時はコメントアウトか `#if 0`。
- 大きい字(60px 以上)のフォントは、`SetFontCacheCharNum` を少なく(48 以下。150px 級は 16)してから作る。
- push は `main` へ。
