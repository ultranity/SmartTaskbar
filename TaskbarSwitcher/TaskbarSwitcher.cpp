#include "stdafx.h"

APPBARDATA msgData = { sizeof(APPBARDATA) };

WINDOWPLACEMENT placement = { sizeof(WINDOWPLACEMENT) };

POINT cursor;

HWND maxWindow;

BOOL cloakedVal = TRUE;

bool tryShowBar = true;

bool IsCursorOverTaskbar();

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    if (IsWindowVisible(hwnd) == FALSE) // Skip hidden windows, IsWindowVisible is much faster than GetWindowPlacement (approximately 4 times).
        return TRUE;                    // 跳过隐藏的窗体，IsWindowVisible 比 GetWindowPlacement 大约快4倍
    GetWindowPlacement(hwnd, &placement);
    if (placement.showCmd != SW_MAXIMIZE) // Skip not maximized windows.
        return TRUE;                      // 跳过不是最大化的窗体
    // The IsVisible property of some applications is true but the user cannot see these applications.
    // These applications may be:

    // 1. In another virtual desktop.
    // 2. is UWP application (Application Frame Host is always in maximized and visible state whenever there is a UWP application (even if it's suspend) is maximized).
    // DwmGetWindowAttribute therefore must be used to further determine whether the window is truly visible.

    // 有些窗体在Visible属性为true时，用户仍然看不见它们
    // 这可能是因为：

    // 1. 它在另一个虚拟桌面
    // 2. 它是一个UWP应用，UWP应用的宿主Application Frame Host，会保持可见属性和最大化属性，只要有任何一个UWP应用是最大化的
    // 因而必须使用DwmGetWindowAttribute对窗体是否可见作进一步判断
    DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloakedVal, sizeof(cloakedVal)); 
    if (cloakedVal)
        return TRUE;
    maxWindow = hwnd;
    return FALSE;
}


int main()
{
    SHAppBarMessage(ABM_GETTASKBARPOS, &msgData);
    while (true) {
        while (IsCursorOverTaskbar())
            Sleep(250); // In order to reduce CPU usage, the following thread sleep times are integer multiples of 1/64 seconds.
                        // 为了减少CPU开销，这里的线程睡眠时间都是1/64秒的整数倍
        EnumWindows(EnumWindowsProc, NULL);
        if (maxWindow == NULL) {
            if (tryShowBar == false) {
                Sleep(375);
                continue;
            }
            tryShowBar = false;
            msgData.lParam = ABS_ALWAYSONTOP;
            SHAppBarMessage(ABM_SETSTATE, &msgData);
            Sleep(500); // Considering the duration of the taskbar animation and the probability of the user changing the status of a window extremely quickly, 
                        // setting it to 500 or 750 milliseconds is the most reasonable.
            continue;
        }
        msgData.lParam = ABS_AUTOHIDE;
        SHAppBarMessage(ABM_SETSTATE, &msgData);
        do {
            Sleep(500); // When a user maximizes a window, the window is likely to remain maximized for a long time. So it is unnecessary to check the status too often.
            cloakedVal = TRUE;
            DwmGetWindowAttribute(maxWindow, DWMWA_CLOAKED, &cloakedVal, sizeof(cloakedVal));
            if (cloakedVal)
                break; // break when 1. the current window is closed 2. the current window is on another virtual desktop.
            GetWindowPlacement(maxWindow, &placement);
        } while (placement.showCmd == SW_MAXIMIZE); // End the loop when current window no longer maximized.
        tryShowBar = true;
        maxWindow = NULL;
        SHAppBarMessage(ABM_GETTASKBARPOS, &msgData);
    }
    return 0;
}

inline bool IsCursorOverTaskbar()
{
    // Do not change the status of the taskbar when the mouse is over it.
        
    // 1. Users may switch between multiple tasks at this time, switching the status of the Taskbar will affect the user experience.
    // 2. In win10, switching the status of the Taskbar to Auto-Hide Mode while the mouse is over it will causes a bug.
    //      which somehow causes the taskbar to blocks the content below it.
    //      (The taskbar cannot be hidden at this situation unless the user clicks on the taskbar and then clicks on another window.)

    // For the above reasons, at the beginning of this infinite loop, we must first determine whether the mouse is above the Taskbar.

    // 当鼠标在任务栏上方时，不应该改变任务栏的状态

    // 1. 此时用户可能在多个窗体之间切换不定，这时候改变任务栏状态会影响用户体验
    // 2. 在win10中，当鼠标在任务栏上方时切换任务栏到自动隐藏模式会导致bug，这个bug会使得任务栏遮挡住它下方的内容，而不能正确的隐藏
    //      要解决这个问题，需要用户点击一次任务栏，再点击一次其他窗口，才能使得任务栏正常隐藏

    // 综上，在循环的开始，需要检测鼠标是否在任务栏上方
    GetCursorPos(&cursor);
    switch (msgData.uEdge)
    {
    case ABE_BOTTOM:
        if (cursor.y >= msgData.rc.top)
            return true;
        return false;
    case ABE_LEFT:
        if (cursor.x <= msgData.rc.right)
            return true;
        return false;
    case ABE_TOP:
        if (cursor.y <= msgData.rc.bottom)
            return true;
        return false;
    default:
        if (cursor.x >= msgData.rc.left)
            return true;
        return false;
    }
}


