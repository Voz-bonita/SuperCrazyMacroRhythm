#include <windows.h>
#include <mmsystem.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <algorithm>
#include <unordered_map>

#pragma comment(lib, "winmm.lib")

struct Event
{
    WORD keyCode;
    std::string keyName;
    int type; // 0 = up, 1 = down, 2 = tap
    double timestamp;
};

// Map string names to VK codes
WORD resolveKey(const std::string &key)
{
    static const std::unordered_map<std::string, WORD> keyMap = {
        {"Key.up", VK_UP},
        {"Key.left", VK_LEFT},
        {"Key.space", VK_SPACE}
        // ... add more if needed
    };

    if (keyMap.count(key))
    {
        return keyMap.at(key);
    }
    if (key.length() == 1)
    {
        char c = toupper(key[0]);
        return VkKeyScanA(c) & 0xFF;
    }

    std::cerr << "Unknown key: " << key << std::endl;
    return 0;
}

// Send key press/release using SendInput
void sendKey(WORD keyCode, bool isPress)
{
    INPUT input = {0};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = keyCode;
    input.ki.dwFlags = isPress ? 0 : KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT));
}

// Busy-wait for ultra-precise timing (<10ms)
void preciseWait(double ms)
{
    auto start = std::chrono::high_resolution_clock::now();
    while (true)
    {
        auto now = std::chrono::high_resolution_clock::now();
        if (std::chrono::duration<double, std::milli>(now - start).count() >= ms)
            break;
    }
}

int main()
{
    timeBeginPeriod(1); // Set high precision timer

    std::vector<Event> events;

    // Read keystrokes file
    std::ifstream infile("keystrokes.txt");
    if (!infile)
    {
        std::cerr << "Cannot open keystrokes.txt\n";
        return 1;
    }

    std::string line;
    while (std::getline(infile, line))
    {
        std::istringstream iss(line);
        std::string keyStr;
        int type;
        double ts;
        if (!(iss >> keyStr >> type >> ts))
            continue;

        WORD vk = resolveKey(keyStr);
        if (vk == 0)
            continue;

        events.push_back({vk, keyStr, type, ts});
    }

    if (events.empty())
    {
        std::cerr << "No events loaded.\n";
        return 1;
    }

    std::sort(events.begin(), events.end(),
              [](const Event &a, const Event &b)
              { return a.timestamp < b.timestamp; });

    std::cout << "Focus the target window. Press SPACE to start playback...\n";
    while (true)
    {
        if (GetAsyncKeyState(VK_SPACE) & 0x8000)
        {
            std::cout << "Starting playback...\n";
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    auto start = std::chrono::high_resolution_clock::now();

    for (const auto &e : events)
    {
        auto now = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration<double>(now - start).count();

        double waitTime = e.timestamp - elapsed;
        if (waitTime > 0)
        {
            if (waitTime < 0.010)
                preciseWait(waitTime * 1000); // <10ms spin
            else
                std::this_thread::sleep_for(std::chrono::duration<double>(waitTime));
        }

        if (e.type == 1)
        {
            sendKey(e.keyCode, true); // key down
            std::cout << e.keyName << " Hold at " << e.timestamp << "s\n";
        }
        else if (e.type == 0)
        {
            sendKey(e.keyCode, false); // key up
            std::cout << e.keyName << " Release at " << e.timestamp << "s\n";
        }
        else if (e.type == 2)
        { // tap
            sendKey(e.keyCode, true);
            preciseWait(10); // 10ms tap
            sendKey(e.keyCode, false);
            std::cout << e.keyName << " Tap at " << e.timestamp << "s\n";
        }
    }

    std::cout << "Playback finished.\n";
    timeEndPeriod(1); // Restore default timer
    return 0;
}
