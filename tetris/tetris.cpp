#include <windows.h>
#include <stdio.h>

HINSTANCE hInstance;
HWND hMainWindow;

HDC hMemDC, hBlockDC;
HBITMAP hMemPrev, hBlockPrev;

int board[12][25];

typedef struct _TAG_POSITION {
    int x;
    int y;
} POSITION;

typedef struct _TAG_BLOCK {
    int rotate;
    POSITION p[3];
} BLOCK;

BLOCK block[8] = {
    {1, {{0,  0},{0, 0}, {0 ,0}}},  // null
    {2, {{0, -1},{0, 1}, {0 ,2}}},  // tetris
    {4, {{0, -1},{0, 1}, {1 ,1}}},  // L1
    {4, {{0, -1},{0, 1}, {-1,1}}},  // L2
    {2, {{0, -1},{1, 0}, {1 ,1}}},  // key1
    {2, {{0, -1},{-1,0}, {-1,1}}},  // key2
    {1, {{0,  1},{1, 0}, {1 ,1}}},  // square
    {4, {{0, -1},{1, 0}, {-1 ,0}}},  // T
};

typedef struct _TAG_STATUS {
    int x;
    int y;
    int type;
    int rotate;
	int score; // 점수
	int level; // 레벨
	int speed; // 속도
} STATUS;

STATUS current;

int random(int max) {
    return (int)(rand() / (RAND_MAX + 1.0) * max);
}

bool putBlock(STATUS s, bool action = false) {
    if(board[s.x][s.y] != 0) {
        return false;
    }


    if(action) {
        board[s.x][s.y] = s.type;
    }


    for(int i = 0; i < 3; i++) {
        int dx = block[s.type].p[i].x;
        int dy = block[s.type].p[i].y;
        int r = s.rotate % block[s.type].rotate;
        for(int j = 0; j < r; j++) {
            int nx = dx, ny = dy;
            dx = ny; dy = -nx;
        }
        if(board[s.x + dx][s.y + dy] != 0) {
            return false;
        }
        if(action) {
            board[s.x + dx][s.y + dy] = s.type;
        }
    }
    if(!action) {
        putBlock(s, true);
    }
    return true;
}

bool deleteBlock(STATUS s) {
    board[s.x][s.y] = 0;

    for(int i = 0; i < 3; i++) {
        int dx = block[s.type].p[i].x;
        int dy = block[s.type].p[i].y;
        int r = s.rotate % block[s.type].rotate;
        for(int j = 0; j < r; j++) {
            int nx = dx, ny = dy;
            dx = ny; dy = -nx;
        }
        board[s.x + dx][s.y + dy] = 0;
    }

    return true;
}

void showBoard() {
    for(int x = 1; x <= 10; x++) {
        for(int y = 1; y <= 20; y++) {
            BitBlt(hMemDC, (x - 1) * 24, (20 -y) * 24, 24, 24, hBlockDC, 0, board[x][y] * 24, SRCCOPY);
        }
    }
}

int downBlock();

bool processInput() {
    bool ret = false;
    STATUS n = current;
    if(GetAsyncKeyState(VK_LEFT)) {
        n.x--;
    } else if(GetAsyncKeyState(VK_RIGHT)) {
        n.x++;
    } else if(GetAsyncKeyState(VK_UP)) {
        n.rotate++;
    } else if(GetAsyncKeyState(VK_DOWN)) {
        n.y--;
        ret = true;
    } else if(GetAsyncKeyState(VK_SPACE)) { // SPACE키를 누르면 블럭이 바로 떨어짐
		while(downBlock());
		return ret;
	} else if(GetAsyncKeyState(VK_ESCAPE)) { // ESC키를 누르면 종료
		PostQuitMessage(0);
	}

    if(n.x != current.x || n.y != current.y || n.rotate != current.rotate) {
        deleteBlock(current);
        if(putBlock(n)) {
            current = n;
        } else {
            putBlock(current);
        }
    }
    
    return ret;
}

int GG = 0;
void gameOver() {
	GG = 1;
    KillTimer(hMainWindow, 1);
    for(int x = 1; x <= 10;x++) {
        for(int y = 1; y <= 20; y++) {
            if(board[x][y] != 0) {
                board[x][y] = 1;
            }
        }
	}
    InvalidateRect(hMainWindow, NULL, false);
}

void deleteLine() {
    for(int y = 1; y < 23; y++) {
        bool flag = true;
        for(int x = 1;x <= 10; x++) {
            if(board[x][y] == 0) {
                flag = false;
            }
        }
        
        if(flag) {
            for(int j = y; j < 23; j++) {
                for(int i = 1; i <= 10; i++) {
                    board[i][j] = board[i][j + 1];
                }
            }
            y--;
			current.score++; // 라인이 사라지면 점수 ++
			if (current.score%5 == 0) // 점수 5점당 레벨 1이 오름
			{
				if (current.level != 10) // 레벨은 10이상 오르지 않음
				{
					current.level++; // 레벨 1 증가
					current.speed--; // 속도 증가
				}
			}
        }
    }
}

void blockDown() {
    deleteBlock(current);
    current.y--;
    if(!putBlock(current)) {
        current.y++;
        putBlock(current);
        
        deleteLine();
        
        current.x = 5;
        current.y = 21;
        current.type = random(7) + 1;
        current.rotate = random(4);
        if(!putBlock(current)) {
            gameOver();
        }
    }
}

int downBlock() { // 블록이 떨어지는 함수
	deleteBlock(current);
    current.y--;

	if(!putBlock(current)) {
        current.y++;
        putBlock(current);
        
        deleteLine();
        
        current.x = 5;
        current.y = 21;
        current.type = random(7) + 1;
        current.rotate = random(4);
		return 0;
    }

	return 1;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {
        case WM_CREATE: {
            for(int x = 0; x < 12; x++) {
                for(int y = 0; y < 25; y++) {
                    if(x == 0 || x == 11 || y == 0) {
                        board[x][y] = 1;
                    } else {
                        board[x][y] = 0;
                    }
                }
            }
            
            current.x = 5;
            current.y = 21;
            current.type = random(7) + 1;
            current.rotate = random(4);
			current.score = 0; // 점수 초기화
			current.level = 1; // 레벨 초기화
			current.speed = 10; // 속도 초기화

            putBlock(current);

            HDC hdc = GetDC(hWnd);
            
            hMemDC = CreateCompatibleDC(hdc);
            HBITMAP hBitmap = CreateCompatibleBitmap(hdc, 24 * 10, 24 * 20);
            hMemPrev = (HBITMAP)SelectObject(hMemDC, hBitmap);
            
            hBlockDC = CreateCompatibleDC(hdc);
            hBitmap = LoadBitmap(hInstance, "BLOCKS");
            hBlockPrev = (HBITMAP)SelectObject(hBlockDC, hBitmap);
            
            // debug
            BitBlt(hMemDC, 0, 0, 24, 24, hBlockDC, 0, 0, SRCCOPY);
            
            ReleaseDC(hWnd, hdc);
            break;
        }
        case WM_TIMER: {
            static int w = 0;
            if(w % 2 == 0) {
                if(processInput()) {
                    w = 0;
                }
            }
            if(w % current.speed == 0) {
                blockDown();
            }
            w++;
            
            InvalidateRect(hWnd, NULL, false);
            break;
        }

        case WM_PAINT: {
            showBoard();

			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);

			char str1[80];
			char str2[80];
			SetTextColor(hdc, RGB(255,255,255)); // 글자색을 흰색
			SetBkColor(hdc, RGB(0,0,0)); // 배경색을 검은색
			sprintf(str1, "레벨 : %d", current.level);
			sprintf(str2, "점수 : %d", current.score);

            BitBlt(hdc, 0, 0, 24 * 10, 24 * 20, hMemDC, 0, 0, SRCCOPY);
			TextOut(hdc, 180, 0, str1, strlen(str1)); // 레벨 출력
			TextOut(hdc, 180, 15, str2, strlen(str2)); // 점수 출력
			if(GG==1) // 게임오버 출력
			{
				TextOut(hdc, 90, 233, "Game Over ", 10);
			}
            EndPaint(hWnd, &ps);
            break;
        }
        case WM_DESTROY: {
            HBITMAP hBitmap = (HBITMAP)SelectObject(hMemDC, hMemPrev);
            DeleteObject(hBitmap);
            DeleteObject(hMemDC);
            
            hBitmap = (HBITMAP)SelectObject(hBlockDC, hBlockPrev);
            DeleteObject(hBitmap);
            DeleteObject(hBlockDC);
            
            PostQuitMessage(0);
            break;
        }
        default:
            return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

int WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR cmdLine, int cmdShow) {
    hInstance = hInst;
    WNDCLASSEX wc;
    static LPCTSTR pClassName = "NicoNicoProgramming2";  // 긏깋긚뼹

    wc.cbSize        = sizeof(WNDCLASSEX);               // ?몾뫬긖귽긛
    wc.style         = CS_HREDRAW | CS_VREDRAW;          // 긏깋긚긚?귽깑
    wc.lpfnWndProc   = (WNDPROC)WndProc;
    wc.cbClsExtra    = 0;                                // 뺚뫉긽긾깏긳깓긞긏
    wc.cbWndExtra    = 0;                                // ?궻긖귽긛
    wc.hInstance     = hInst;                            // 귽깛긚?깛긚
    wc.hIcon         = NULL;                             // 귺귽긓깛
    wc.hCursor       = LoadCursor(NULL,IDC_ARROW);       // 긇??깑
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);         // 봶똧륡
    wc.lpszMenuName  = NULL;                             // 긽긦깄?
    wc.lpszClassName = pClassName;                       // 긏깋긚뼹
    wc.hIconSm       = NULL;                             // 룷궠궋귺귽긓깛

    if (!RegisterClassEx(&wc)) return FALSE;             // 뱋?

    RECT r;
    r.left = r.top = 0;
    r.right = 24 * 10;
    r.bottom = 24 * 20;
    AdjustWindowRectEx(&r, WS_OVERLAPPED | WS_MINIMIZEBOX | WS_SYSMENU | WS_CAPTION, false, 0);

    hMainWindow = CreateWindow(pClassName, "Tetris", WS_OVERLAPPED | WS_MINIMIZEBOX | WS_SYSMENU | WS_CAPTION,
        CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top,
        NULL, NULL, hInst, NULL);
    
    ShowWindow(hMainWindow, SW_SHOW);
	
    MSG msg;
	SetTimer(hMainWindow, 1, 1000 /30, NULL);
    while(GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
	KillTimer(hMainWindow, 1);
    
    return 0;
}
