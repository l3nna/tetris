#include <iostream>
#include <thread>
#include <vector>
#include <stdio.h>
#include <windows.h>

using namespace std;

int nScreenWidth = 80;          // Console Screen Size X (columns)
int nScreenHeight = 30;         // Console Screen Size Y (rows)
wstring tetromino[7];
int nFieldWidth = 12;
int nFieldHeight = 18;
unsigned char *pField = nullptr;

int Rotate(int px, int py, int r)
{
    int pi = 0;
    switch (r % 4)
    {
    case 0: pi = py * 4 + px; break;
    case 1: pi = 12 + py - (px * 4); break;
    case 2: pi = 15 - (py * 4) - px; break;
    case 3: pi = 3 - py + (px * 4); break;
    }
    return pi;
}

bool DoesPieceFit(int nTetromino, int nRotation, int nPosX, int nPosY)
{
    for (int px = 0; px < 4; px++)
        for (int py = 0; py < 4; py++)
        {
            int pi = Rotate(px, py, nRotation);
            int fi = (nPosY + py) * nFieldWidth + (nPosX + px);

            if (nPosX + px >= 0 && nPosX + px < nFieldWidth)
            {
                if (nPosY + py >= 0 && nPosY + py < nFieldHeight)
                {
                    if (tetromino[nTetromino][pi] != L'.' && pField[fi] != 0)
                        return false;
                }
            }
        }
    return true;
}

int main()
{
    // Tetromino shapes
    tetromino[0].append(L"..X...X...X...X.");
    tetromino[1].append(L"..X..XX..X......");
    tetromino[2].append(L".X...XX...X.....");
    tetromino[3].append(L".....XX..XX.....");
    tetromino[4].append(L"..X..XX...X.....");
    tetromino[5].append(L".....XX...X...X.");
    tetromino[6].append(L".....XX..X...X..");

    pField = new unsigned char[nFieldWidth * nFieldHeight];
    for (int x = 0; x < nFieldWidth; x++)
        for (int y = 0; y < nFieldHeight; y++)
            pField[y * nFieldWidth + x] = (x == 0 || x == nFieldWidth - 1 || y == nFieldHeight - 1) ? 9 : 0;

    // Set standard stdout buffer size to prevent Windows Terminal auto-wrapping
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    
    // Create secondary screen buffer
    hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
    
    // Get actual console window dimensions to prevent repeated rendering
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (GetConsoleScreenBufferInfo(hStdOut, &csbi)) {
        nScreenWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        nScreenHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    }
    
    // Set buffer size to match console size exactly
    COORD coord = { (SHORT)nScreenWidth, (SHORT)nScreenHeight };
    SetConsoleScreenBufferSize(hConsole, coord);
    SetConsoleActiveScreenBuffer(hConsole);

    wchar_t *screen = new wchar_t[nScreenWidth * nScreenHeight];
    DWORD dwBytesWritten = 0;

    bool bGameOver = false;
    int nCurrentPiece = 0;
    int nCurrentRotation = 0;
    int nCurrentX = nFieldWidth / 2 - 2;
    int nCurrentY = 0;

    bool bKey[4];
    bool bRotateHold = false;

    int nSpeed = 20;
    int nSpeedCount = 0;
    bool bForceDown = false;
    int nPieceCount = 0;
    int nScore = 0;
    vector<int> vLines;

    while (!bGameOver)
    {
        // Reset screen frame buffer
        for (int i = 0; i < nScreenWidth * nScreenHeight; i++) screen[i] = L' ';

        this_thread::sleep_for(chrono::milliseconds(50));
        nSpeedCount++;
        bForceDown = (nSpeedCount == nSpeed);

        // Input
        for (int k = 0; k < 4; k++)
            bKey[k] = (GetAsyncKeyState((unsigned char)("\x27\x25\x28Z"[k])) & 0x8000) != 0;

        // Logic
        nCurrentX -= (bKey[1] && DoesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentX - 1, nCurrentY)) ? 1 : 0;
        nCurrentX += (bKey[0] && DoesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentX + 1, nCurrentY)) ? 1 : 0;
        nCurrentY += (bKey[2] && DoesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentX, nCurrentY + 1)) ? 1 : 0;

        if (bKey[3])
        {
            nCurrentRotation += (!bRotateHold && DoesPieceFit(nCurrentPiece, nCurrentRotation + 1, nCurrentX, nCurrentY)) ? 1 : 0;
            bRotateHold = true;
        }
        else
            bRotateHold = false;

        if (bForceDown)
        {
            nSpeedCount = 0;
            nPieceCount++;
            if (nPieceCount % 10 == 0)
                if (nSpeed >= 10) nSpeed--;

            if (DoesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentX, nCurrentY + 1))
                nCurrentY++;
            else
            {
                for (int px = 0; px < 4; px++)
                    for (int py = 0; py < 4; py++)
                        if (tetromino[nCurrentPiece][Rotate(px, py, nCurrentRotation)] != L'.')
                            pField[(nCurrentY + py) * nFieldWidth + (nCurrentX + px)] = nCurrentPiece + 1;

                for (int py = 0; py < 4; py++)
                    if (nCurrentY + py < nFieldHeight - 1)
                    {
                        bool bLine = true;
                        for (int px = 1; px < nFieldWidth - 1; px++)
                            bLine &= (pField[(nCurrentY + py) * nFieldWidth + px]) != 0;

                        if (bLine)
                        {
                            for (int px = 1; px < nFieldWidth - 1; px++)
                                pField[(nCurrentY + py) * nFieldWidth + px] = 8;
                            vLines.push_back(nCurrentY + py);
                        }
                    }

                nScore += 25;
                if (!vLines.empty()) nScore += (1 << vLines.size()) * 100;

                nCurrentX = nFieldWidth / 2 - 2;
                nCurrentY = 0;
                nCurrentRotation = 0;
                nCurrentPiece = rand() % 7;

                bGameOver = !DoesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentX, nCurrentY);
            }
        }

        // Draw Field (Using 2 characters per width unit to keep square aspect ratio)
        for (int x = 0; x < nFieldWidth; x++)
            for (int y = 0; y < nFieldHeight; y++)
            {
                wchar_t c = L" ABCDEFG=#"[pField[y * nFieldWidth + x]];
                screen[(y + 2) * nScreenWidth + (x * 2 + 2)] = c;
                screen[(y + 2) * nScreenWidth + (x * 2 + 3)] = c;
            }

        // Draw Current Piece
        for (int px = 0; px < 4; px++)
            for (int py = 0; py < 4; py++)
                if (tetromino[nCurrentPiece][Rotate(px, py, nCurrentRotation)] != L'.')
                {
                    wchar_t c = nCurrentPiece + 65;
                    screen[(nCurrentY + py + 2) * nScreenWidth + ((nCurrentX + px) * 2 + 2)] = c;
                    screen[(nCurrentY + py + 2) * nScreenWidth + ((nCurrentX + px) * 2 + 3)] = c;
                }

        // Draw Score
        swprintf_s(&screen[2 * nScreenWidth + nFieldWidth * 2 + 6], 16, L"SCORE: %8d", nScore);

        if (!vLines.empty())
        {
            WriteConsoleOutputCharacterW(hConsole, screen, nScreenWidth * nScreenHeight, { 0,0 }, &dwBytesWritten);
            this_thread::sleep_for(chrono::milliseconds(400));

            for (auto &v : vLines)
                for (int px = 1; px < nFieldWidth - 1; px++)
                {
                    for (int py = v; py > 0; py--)
                        pField[py * nFieldWidth + px] = pField[(py - 1) * nFieldWidth + px];
                    pField[px] = 0;
                }

            vLines.clear();
        }

        WriteConsoleOutputCharacterW(hConsole, screen, nScreenWidth * nScreenHeight, { 0,0 }, &dwBytesWritten);
    }

    CloseHandle(hConsole);
    cout << "Game Over!! Score: " << nScore << endl;
    system("pause");
    return 0;
}