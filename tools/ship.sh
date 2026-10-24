#!/bin/sh
# 展示の PC へ送る(fumiya の PC の Git Bash で、リポジトリの一番上から sh tools/ship.sh)
#   ビルドした x64/Release/NozExhibition.exe を一番上(data と同じ所)に写して、commit して push する。
#   動かす PC では git pull origin main して ./NozExhibition.exe を起動するだけ。
#   (x64/ は .gitignore で外しているので、一番上に写した exe だけを上げる)
set -e
cd "$(dirname "$0")/.."
EXE=x64/Release/NozExhibition.exe
if [ ! -f "$EXE" ]; then
	echo "$EXE が無い。Release / x64 でビルドしてから。"
	exit 1
fi
cp "$EXE" ./NozExhibition.exe
git add NozExhibition.exe
if git diff --cached --quiet; then
	echo "exe は前と同じ。上げる物なし。"
else
	git commit -m "展示の PC 用に、ビルドした exe を置く($(date '+%Y-%m-%d %H:%M'))"
fi
git push origin main
echo "push した。動かす PC で:git pull origin main → ./NozExhibition.exe"
