#pragma once
#include "DxLib.h"
#include <vector>



// グループID定数
// ミニゲーム側は 1, 2, 3... を自分で定義して使う
#define RESOURCE_GROUP_COMMON  0
#define RESOURCE_GROUP_STEP    1
#define RESOURCE_GROUP_HI      2
#define RESOURCE_GROUP_NOZ     3
#define RESOURCE_GROUP_SHUN    4
#define RESOURCE_GROUP_U       5
#define RESOURCE_GROUP_FLUSHQUIZ 6
#define RESOURCE_GROUP_GINOTAI 7
// 使っている番号（重ならないように、新しく決める時はここに書き足す）
//   30～40 : ミニゲーム（timing_main_kou 30 / HI2 31 / ineq 32 / bet 33 / irabou 34 / potato 35 / relay 36 / mogura 37 / sandwich 38 / cut 39 / dash 40）
//   90～99 : システムの画面（MainTutorial のチュートリアル画像 90 / playerRecord の成績画面の写真 91）



extern int gl_SEs[100];



struct Resource {
	int  handle;
	bool is_sound; // true=音声, false=画像
};


// -------------------------------------------------------
// ResourceGroup : 1グループ分のリソースを管理するクラス
// -------------------------------------------------------
class ResourceGroup {
public:
	int group_id;

	ResourceGroup(int id) : group_id(id) {}

	// 画像ロード → ハンドルを返す（失敗時は -1）
	int LoadImg(const char* path) {
		int h = LoadGraph(path);
		if (h == -1) return -1;
		resources.push_back({ h, false });
		return h;
	}

	int LoadDivImg(const char* path, int allNum, int xNum, int yNum, int xSize, int ySize, int* handleBuf) {
		int result = LoadDivGraph(path, allNum, xNum, yNum, xSize, ySize, handleBuf);
		if (result == -1) return -1;
		for (int i = 0; i < allNum; i++) {
			resources.push_back({ handleBuf[i], false });
		}
		return result;
	}

	// 音声ロード → ハンドルを返す（失敗時は -1）
	int LoadSound(const char* path) {
		int h = LoadSoundMem(path);
		if (h == -1) return -1;
		resources.push_back({ h, true });
		return h;
	}

	// グループ内のリソースを全解放
	void UnloadAll(void) {
		for (auto& r : resources) {
			if (r.is_sound) DeleteSoundMem(r.handle);
			else            DeleteGraph(r.handle);
		}
		resources.clear();
	}

	int GetCount(void) const { return (int)resources.size(); }

	// 鳴っている音を止めて、止めたハンドルを out に足す(ゲーム中の PAUSE 用。PauseMenu.cpp)
	void StopPlayingSounds(std::vector<int>& out) {
		for (auto& r : resources) {
			if (!r.is_sound) continue;
			if (CheckSoundMem(r.handle) != 1) continue;
			StopSoundMem(r.handle);
			out.push_back(r.handle);
		}
	}

private:
	std::vector<Resource> resources;
};





// -------------------------------------------------------
// ResourceManager : 素材グループ(主にゲーム単位)をまとめて管理するクラス
// -------------------------------------------------------
class ResourceManager {
public:
	ResourceManager() {}

	~ResourceManager() {
		UnloadAll();
	}

	// グループを作成して返す（既に存在する場合はそのまま返す）
	ResourceGroup* CreateGroup(int group_id) {
		ResourceGroup* g = FindGroup(group_id);
		if (g != nullptr) return g;
		groups.push_back(ResourceGroup(group_id));
		return &groups.back();
	}

	// グループIDからグループを取得（存在しない場合は NULL を返す)
	ResourceGroup* GetGroup(int group_id) {
		return FindGroup(group_id);
	}

	// 指定グループのリソースを解放・削除
	void UnloadGroup(int group_id) {
		for (int i = 0; i < (int)groups.size(); i++) {
			if (groups[i].group_id == group_id) {
				groups[i].UnloadAll();
				groups.erase(groups.begin() + i);
				return;
			}
		}
	}

	// 共用グループ以外を全解放（ミニゲーム切り替え時に呼ぶ）
	void UnloadGameGroups(void) {
		for (auto& g : groups) {
			if (g.group_id != RESOURCE_GROUP_COMMON) {
				g.UnloadAll();
			}
		}
		// COMMON以外を削除
		for (int i = (int)groups.size() - 1; i >= 0; i--) {
			if (groups[i].group_id != RESOURCE_GROUP_COMMON) {
				groups.erase(groups.begin() + i);
			}
		}
	}

	// 全グループの、鳴っている音を止める(ゲーム中の PAUSE 用。止めたハンドルを out に足す)
	void StopPlayingSounds(std::vector<int>& out) {
		for (auto& g : groups) g.StopPlayingSounds(out);
	}

	// 全グループを解放（アプリ終了時に呼ぶ）
	void UnloadAll(void) {
		for (auto& g : groups) g.UnloadAll();
		groups.clear();
	}

private:
	std::vector<ResourceGroup> groups;

	ResourceGroup* FindGroup(int group_id) {
		for (auto& g : groups) {
			if (g.group_id == group_id) return &g;
		}
		return nullptr;
	}
};



#ifndef ENV_RESOURCE_H
#define ENV_RESOURCE_H
#endif