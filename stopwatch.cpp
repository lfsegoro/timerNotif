#include <windows.h>
#include <sstream>
#include <string>
//#include <cstdlib>
#include <stack>
#include <cctype>

HWND hThresholdDisplay, hButton, hDisplay, hInput;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void UpdateTimerDisplay(HWND);
void PlayAlarmSound(HWND hWnd);
double EvaluateExpression(const std::wstring& expression);

bool running = false;
DWORD startTime;
double threshold = 0;
bool beepEnabled = true;
bool isFlashing = false;
COLORREF displayBgColor = RGB(255, 255, 255); // Default white
HBRUSH hDisplayBrush = CreateSolidBrush(displayBgColor);
bool paused = false;




int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"StopwatchApp";
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        WS_EX_TOPMOST,
        wc.lpszClassName, nullptr,
        WS_POPUP | WS_VISIBLE,
        0, 0, 60, 60,
        nullptr, nullptr, hInstance, nullptr);

   hDisplay = CreateWindow(L"STATIC", L"00:00:00", WS_VISIBLE | WS_CHILD | SS_CENTER,
        0, 0, 60, 20, hwnd, nullptr, hInstance, nullptr);

   hInput = CreateWindow(L"EDIT", L"0", WS_VISIBLE | WS_CHILD | WS_BORDER,
       0, 20, 60, 20, hwnd, (HMENU)2, hInstance, nullptr);

    hThresholdDisplay = CreateWindow(L"STATIC", L"Threshold: 0", WS_VISIBLE | WS_CHILD | SS_CENTER,
       0, 60, 60, 20, hwnd, nullptr, hInstance, nullptr);

    hButton = CreateWindow(L"BUTTON", L"Go/Restart", WS_VISIBLE | WS_CHILD,
        0, 40, 60, 20, hwnd, (HMENU)1, hInstance, nullptr);


    HFONT hFont = CreateFont(
        -12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

    HFONT hFont2 = CreateFont(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
    SendMessage(hDisplay, WM_SETFONT, (WPARAM)hFont2, TRUE);

    SendMessage(hInput, WM_SETFONT, (WPARAM)hFont, TRUE);
    //SendMessage(hThresholdDisplay, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hButton, WM_SETFONT, (WPARAM)hFont, TRUE);

    ShowWindow(hwnd, nCmdShow);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_RBUTTONUP: {
        HMENU hMenu = CreatePopupMenu();
        AppendMenu(hMenu, MF_STRING | (beepEnabled ? MF_CHECKED : MF_UNCHECKED), 1001, L"Enable Beep");
        AppendMenu(hMenu, MF_STRING | (paused ? MF_CHECKED : MF_UNCHECKED), 1002, paused ? L"Resume Timer" : L"Pause Timer");
        AppendMenu(hMenu, MF_STRING, 1003, L"Exit App");

        POINT pt;
        GetCursorPos(&pt);
        TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
        DestroyMenu(hMenu);
        break;
    }


    case WM_COMMAND:
        if (LOWORD(wp) == 1) {  // Go Button
            running = true;
            startTime = GetTickCount();

            wchar_t buffer[50];
            GetWindowText(hInput, buffer, 50);
            threshold = EvaluateExpression(buffer);

            wchar_t thresholdText[50];
            swprintf(thresholdText, 50, L"Alarm at: %.2lf", threshold);
            SetWindowText(hThresholdDisplay, thresholdText);

            SetTimer(hwnd, 1, 1000, nullptr);       // Timer for stopwatch
            SetTimer(hwnd, 2, 4000, nullptr);       // Timer for flash+beep every 2000ms
        }
        else if (LOWORD(wp) == 1001) {  // Toggle beep
            beepEnabled = !beepEnabled;
        }
        else if (LOWORD(wp) == 1002) {  // Pause/Resume
            paused = !paused;
        }
        else if (LOWORD(wp) == 1003) {  // Exit
            PostQuitMessage(0);
        }

        break;

    case WM_TIMER:
        if (wp == 1 && running && !paused) {
            UpdateTimerDisplay(hwnd);
        }
        else if (wp == 2 && running && !paused) {
            DWORD elapsed = (GetTickCount() - startTime) / 1000;
            if (elapsed >= threshold && threshold > 0) {
                PlayAlarmSound(hwnd);
            }
        }

        else if (wp == 3 && isFlashing) {
            isFlashing = false;

            displayBgColor = RGB(255, 255, 255); // White
            DeleteObject(hDisplayBrush);
            hDisplayBrush = CreateSolidBrush(displayBgColor);

            InvalidateRect(hDisplay, nullptr, TRUE);
            UpdateWindow(hDisplay);

            KillTimer(hwnd, 3);
        }


        break;
    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wp;
        HWND hCtrl = (HWND)lp;

        if (hCtrl == hDisplay) {
            SetBkColor(hdcStatic, displayBgColor);
            return (INT_PTR)hDisplayBrush;
        }
        break;
    }


    case WM_LBUTTONDOWN:
        ReleaseCapture();
        SendMessage(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);  // Drag window
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

void UpdateTimerDisplay(HWND hwnd) {
    DWORD elapsed = (GetTickCount() - startTime) / 1000;
    int hours = elapsed / 3600;
    int minutes = (elapsed % 3600) / 60;
    int seconds = elapsed % 60;

    std::wstringstream ss;
    ss << (hours < 10 ? L"0" : L"") << hours << L":"
        << (minutes < 10 ? L"0" : L"") << minutes << L":"
        << (seconds < 10 ? L"0" : L"") << seconds;

    SetWindowText(hDisplay, ss.str().c_str());
}

void PlayAlarmSound(HWND hWnd) {
    // Set flash flag
    isFlashing = true;

    // Change color to red
    displayBgColor = RGB(255, 0, 0);
    DeleteObject(hDisplayBrush); // Clean old brush
    hDisplayBrush = CreateSolidBrush(displayBgColor);

    InvalidateRect(hDisplay, nullptr, TRUE);
    UpdateWindow(hDisplay);

    // Optional beep
    if (beepEnabled) {
        Beep(750, 100);
    }

    // Set timer to revert color
    SetTimer(hWnd, 3, 300, nullptr);
}










double EvaluateExpression(const std::wstring& expression) {
    std::string expr(expression.begin(), expression.end());
    std::stringstream ss;
    for (char ch : expr) {
        if (!isspace(ch)) ss << ch;
    }

    std::stack<double> values;
    std::stack<char> ops;

    auto applyOp = [](double a, double b, char op) -> double {
        switch (op) {
        case '+': return a + b;
        case '-': return a - b;
        case '*': return a * b;
        case '/': return b != 0 ? a / b : 0;
        default: return 0;
        }
        };

    auto precedence = [](char op) -> int {
        if (op == '+' || op == '-') return 1;
        if (op == '*' || op == '/') return 2;
        return 0;
        };

    for (size_t i = 0; i < expr.size();) {
        if (isdigit(expr[i]) || expr[i] == '.') {
            double val = 0;
            size_t len = 0;
            sscanf_s(expr.c_str() + i, "%lf%n", &val, (int*)&len);
            values.push(val);
            i += len;
        }
        else {
            while (!ops.empty() && precedence(ops.top()) >= precedence(expr[i])) {
                double b = values.top(); values.pop();
                double a = values.top(); values.pop();
                char op = ops.top(); ops.pop();
                values.push(applyOp(a, b, op));
            }
            ops.push(expr[i]);
            ++i;
        }
    }

    while (!ops.empty()) {
        double b = values.top(); values.pop();
        double a = values.top(); values.pop();
        char op = ops.top(); ops.pop();
        values.push(applyOp(a, b, op));
    }

    return values.empty() ? 0 : values.top();
}
