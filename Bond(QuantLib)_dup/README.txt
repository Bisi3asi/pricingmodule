* 프로젝트 파일 구성 및 변경 가능 기본 설정 설명 *

[CmakeLists.txt] : Ln 1 ~ 6
	- 프로젝트명 설정
	- 출력 dll 파일명 설정
	- dll 연동 및 데이터 테스트용 실행 파일명 설정 (.cpp, exe)

[CmakePresets.json] : Ln 34
	- 리눅스 빌드 시 so 파일이 출력될 위치 지정 (절대 경로)

[/src]
	- src 예하에 cpp 및 h 파일 탑재해야 함
	- .h : include 정의, 외부 노출 인터페이스 함수 정의 (extern "C", EXPORT 필수), 구조체 정의, namespace 정의 (필요시)
	- .cpp : 외부 노출 인터페이스 함수 구현