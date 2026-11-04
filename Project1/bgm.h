#ifndef BGM_H
#define BGM_H

// 배경 음악 (BGM)
//  - 외부 라이브러리 없이 Windows 에 기본으로 들어 있는 winmm 의 PlaySoundW 로 재생한다
//  - SND_ASYNC : 음악이 나오는 동안에도 게임이 멈추지 않는다 (따로 재생)
//  - SND_LOOP  : 곡이 끝나면 처음부터 계속 반복한다
//  - 음악 파일은 map.txt 와 같은 방식으로 찾는다 : 작업 폴더 → exe 폴더 → 그 위 폴더들

#include <windows.h>
#include <mmsystem.h>
#include <wchar.h>
#include <stdio.h>

#pragma comment(lib, "winmm.lib") // PlaySoundW 가 들어 있는 라이브러리 (Windows 기본 포함)

#define BGM_FILE L"flowerbed_fields.wav" // 배경 음악 파일 이름 (Project1 폴더, map.txt 옆)

// 파일이 있으면 1
static int BgmExists(const wchar_t* path) {
	DWORD attrs = GetFileAttributesW(path);
	return attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
}

// 배경 음악 재생 시작. 성공하면 1, 음악 파일을 못 찾으면 0 (소리 없이 게임은 그대로 진행)
static int BgmStart(void) {
	wchar_t file[MAX_PATH];
	wchar_t dir[MAX_PATH];

	// 1) 작업 폴더 (VS 에서 실행하면 Project1 폴더)
	GetCurrentDirectoryW(MAX_PATH, dir);
	swprintf(file, MAX_PATH, L"%ls\\%ls", dir, BGM_FILE);

	// 2) 없으면 exe 폴더부터 한 단계씩 위로 올라가며 찾는다 (exe 더블클릭 실행 대비)
	if (!BgmExists(file)) {
		DWORD len = GetModuleFileNameW(NULL, dir, MAX_PATH);
		int found = 0;
		for (int up = 0; !found && len > 0 && up < 4; up++) {
			wchar_t* slash = wcsrchr(dir, L'\\');
			if (slash == NULL) break;
			*slash = L'\0';                                                       // 한 단계 위 폴더로
			swprintf(file, MAX_PATH, L"%ls\\%ls", dir, BGM_FILE);
			if (BgmExists(file)) { found = 1; break; }
			swprintf(file, MAX_PATH, L"%ls\\Project1\\%ls", dir, BGM_FILE);         // 바깥 솔루션으로 빌드한 경우
			if (BgmExists(file)) { found = 1; break; }
		}
		if (!found) return 0;
	}

	return PlaySoundW(file, NULL, SND_FILENAME | SND_ASYNC | SND_LOOP | SND_NODEFAULT) ? 1 : 0;
}

// 배경 음악 정지
static void BgmStop(void) {
	PlaySoundW(NULL, NULL, 0);
}

#endif
