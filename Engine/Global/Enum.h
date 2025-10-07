#pragma once

enum class EKeyInput : uint8
{
	// 이동 키
	W,
	A,
	S,
	D,
	Q,
	E,
	C,

	// 화살표 키
	Up,
	Down,
	Left,
	Right,

	// 액션 키
	Space,
	Enter,
	Esc,
	Tab,
	Shift,
	Ctrl,
	Alt,

	// 숫자 키
	Num0,
	Num1,
	Num2,
	Num3,
	Num4,
	Num5,
	Num6,
	Num7,
	Num8,
	Num9,

	// 마우스
	MouseLeft,
	MouseRight,
	MouseMiddle,

	// 기타
	F1,
	F2,
	F3,
	F4,
	Backspace,
	Delete,

	End
};

enum class EKeyStatus : uint8
{
	Up,        // 눌려있지 않은 상태 (현재 false)
	Pressed,   // 이번 프레임에 눌림 (이전 false, 현재 true)
	Down,      // 눌려있는 상태 (현재 true)
	Released,  // 이번 프레임에 떼어짐 (이전 true, 현재 false)
	Unknown,    // 알 수 없음

	End
};

/**
 * @brief 로그 타입 열거형
 * 콘솔 윈도우에서 로그의 종류와 색상을 결정하는데 사용
 */
enum class ELogType : uint8
{
	// 기본 로그 타입
	Info,        // 일반 정보 (흰색)
	Warning,     // 경고 (노란색)
	Error,       // 에러 (빨간색)
	Success,     // 성공 (초록색)
	
	// 시스템 로그 타입
	System,      // 시스템 메시지 (회색)
	Debug,       // 디버그 메시지 (파란색)
	
	// 사용자 정의 로그 타입
	UELog,       // UE_LOG 명령어 (초록색)
	Terminal,    // 터미널 명령어 (하늘색)
	TerminalError, // 터미널 에러 (빨간색)
	Command,     // 사용자 입력 명령어 (주황색)
	
	End
};


enum class EViewMode : uint8
{
	Lit,
	Unlit,
	WireFrame
};

// View types
// Viewport Type 이라는 명칭은 언리얼 찾아보니까 다른 기능이 있어서 안씀
enum class EViewType : uint8
{
	Perspective,
	OrthoTop,
	OrthoBottom,
	OrthoLeft,
	OrthoRight,
	OrthoFront,
	OrthoBack
};


// Viewport layout modes
enum class EViewportLayout : uint8
{
	Single = 0,
	Quad = 1
};

enum class EViewportType : uint8
{
	Perspective,
	Top,
	Right,
	Front
};
