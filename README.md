# noz_exhbition

大学祭のブース入口の大型モニター用、デモ巡回プログラム(DXライブラリ)。起動すると全画面で、`data/demos.txt` の順にデモを流し続ける。Esc で終了。

| デモ | 中身 |
|---|---|
| `noz_guide` | ノズキン:タイトルの景色とロゴ + ブースの案内(`data/guide.txt`) |
| `noz_cards` | ノズキン:ゲーム紹介カード(プレイ中のクリップ・ジャンル・説明・人数) |
| `qma` | クイズ OS×UNIX:「突然ですが、ここで問題です」→ 1 問 → 時間切れ →「正解は 静岡理工科大学の 授業の中で！」 |
| `circle` | NOZ GAMES のサークル紹介(`data/circle.txt`。写真は `circle/photos/`) |
| `pv` | PV(DXLib の描画でその場で作る。約 38 秒。曲はノズキンのオープニング。文は `data/pv.txt`) |

ビルド:`NozExhibition.sln`(VS2022、x64)。DXLib の場所は `LocalPaths.props`。実行の作業フォルダはリポジトリの直下。
