#!/bin/sh
# 展示の PC へ送る(fumiya の PC の Git Bash で、リポジトリの一番上から sh tools/ship.sh)
#   ビルドした x64/Release/NozExhibition.exe を一番上(data と同じ所)に写して、commit して push する。
#   動かす PC では git pull origin main して ./NozExhibition.exe を起動するだけ。
#   (x64/ は .gitignore で外しているので、一番上に写した exe だけを上げる)
set -e
cd "$(dirname "$0")/.."
# exe を探す(2026-10-24 fumiya:あるのに無いと言われた。決め打ちをやめて、x64 の下の exe を全部見て、一番新しい物を使う)
#EXE=x64/Release/NozExhibition.exe
EXE=$(ls -t x64/Release/*.exe x64/*/*.exe x64/*/*/*.exe 2>/dev/null | head -1 || true)
if [ -z "$EXE" ]; then
	echo "x64 の下に exe が見つからへん。今いる所:$(pwd)"
	echo "x64 の中身:"; ls -R x64 2>/dev/null | head -40
	exit 1
fi
echo "使う exe:$EXE"
cp "$EXE" ./NozExhibition.exe
git add NozExhibition.exe
if git diff --cached --quiet; then
	echo "exe は前と同じ。上げる物なし。"
else
	git commit -m "展示の PC 用に、ビルドした exe を置く($(date '+%Y-%m-%d %H:%M'))"
fi
git push origin main
echo "push した。動かす PC で:git pull origin main → ./NozExhibition.exe"
