#include <iostream>
#include <thread>
#include <vector>
#include <stdio.h>
#include <windows.h>

#define FOREGROUND_WHITE (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)

using namespace std;

int nScreenWidth = 80;      // screen width columns, dont change unless u want layout broken
int nScreenHeight = 30;     // rows height
wstring tetromino[7];
int nFieldWidth = 12;
int nFieldHeight = 18;
unsigned char *pField = nullptr;

// rotation math magic stolen from stackoverflow, dont touch it works fine
int Rotate(int px, int py, int r)
{
    int pi = 0;
    switch (r % 4)
    {
    case 0: pi = py * 4 + px; break;              // 0 degrees
    case 1: pi = 12 + py - (px * 4); break;       // 90 degrees
    case 2: pi = 15 - (py * 4) - px; break;       // 180 degrees
    case 3: pi = 3 - py + (px * 4); break;        // 270 degrees
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
                        return false; // hit something solid
                }
            }
        }
    return true;
}

// maps block type to actual windows console color attributes
WORD GetColorAttribute(int nID)
{
    switch (nID)
    {
    case 1: return FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY; // Cyan (I piece)
    case 2: return FOREGROUND_BLUE | FOREGROUND_INTENSITY;                    // Blue (J piece)
    case 3: return FOREGROUND_RED | FOREGROUND_INTENSITY;                     // Orange/Red (L piece)
    case 4: return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;  // Yellow (O piece)
    case 5: return FOREGROUND_GREEN | FOREGROUND_INTENSITY;                   // Green (S piece)
    case 6: return FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;   // Purple (T piece)
    case 7: return FOREGROUND_RED | FOREGROUND_INTENSITY;                     // Red (Z piece)
    case 8: return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY; // White flash for clearing lines
    case 9: return FOREGROUND_BLUE;                                           // Dark blue walls
    default: return 0;
    }
}

int main()
{
    // shape definitions - dots are empty spaces, X is block
    tetromino[0].append(L"..X...X...X...X."); // I
    tetromino[1].append(L"..X..XX..X......"); // J
    tetromino[2].append(L".X...XX...X....."); // L
    tetromino[3].append(L".....XX..XX....."); // O
    tetromino[4].append(L"..X..XX...X....."); // S
    tetromino[5].append(L".....XX...X...X."); // T
    tetromino[6].append(L".....XX..X...X.."); // Z

    pField = new unsigned char[nFieldWidth * nFieldHeight];
    for (int x = 0; x < nFieldWidth; x++)
        for (int y = 0; y < nFieldHeight; y++)
            pField[y * nFieldWidth + x] = (x == 0 || x == nFieldWidth - 1 || y == nFieldHeight - 1) ? 9 : 0;

    // setup console stuff so it doesn't flicker like crazy
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
    
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (GetConsoleScreenBufferInfo(hStdOut, &csbi)) {
        nScreenWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        nScreenHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    }
    
    COORD coord = { (SHORT)nScreenWidth, (SHORT)nScreenHeight };
    SetConsoleScreenBufferSize(hConsole, coord);
    SetConsoleActiveScreenBuffer(hConsole);

    // using CHAR_INFO instead of wchar_t so we can pack color attributes into the buffer
    CHAR_INFO *screen = new CHAR_INFO[nScreenWidth * nScreenHeight];
    DWORD dwBytesWritten = 0;

    bool bGameOver = false;
    
    srand(time(NULL));
    int nCurrentPiece = rand() % 7;
    int nNextPiece = rand() % 7; // added next piece queue so you can actually plan ahead
    
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
        // clear screen buffer to spaces with black background
        for (int i = 0; i < nScreenWidth * nScreenHeight; i++) {
            screen[i].Char.UnicodeChar = L' ';
            screen[i].Attributes = 0;
        }

        this_thread::sleep_for(chrono::milliseconds(50));
        nSpeedCount++;
        bForceDown = (nSpeedCount == nSpeed);

        // read inputs: right arrow, left arrow, down arrow, Z key for rotation
        for (int k = 0; k < 4; k++)
            bKey[k] = (GetAsyncKeyState((unsigned char)("\x27\x25\x28Z"[k])) & 0x8000) != 0;

        // move left/right/down if nothing's blocking
        nCurrentX -= (bKey[1] && DoesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentX - 1, nCurrentY)) ? 1 : 0;
        nCurrentX += (bKey[0] && DoesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentX + 1, nCurrentY)) ? 1 : 0;
        nCurrentY += (bKey[2] && DoesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentX, nCurrentY + 1)) ? 1 : 0;

        // handle rotation with debounce so it doesn't spin wildly on a single tap
        if (bKey[3])
        {
            nCurrentRotation += (!bRotateHold && DoesPieceFit(nCurrentPiece, nCurrentRotation + 1, nCurrentX, nCurrentY)) ? 1 : 0;
            bRotateHold = true;
        }
        else
            bRotateHold = false;

        // drop piece automatically based on game speed tick
        if (bForceDown)
        {
            nSpeedCount = 0;
            nPieceCount++;
            if (nPieceCount % 10 == 0)
                if (nSpeed >= 10) nSpeed--; // gets faster as you play more

            if (DoesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentX, nCurrentY + 1))
                nCurrentY++;
            else
                // locked in place, write to field grid
            {
                for (int px = 0; px < 4; px++)
                    for (int py = 0; py < 4; py++)
                        if (tetromino[nCurrentPiece][Rotate(px, py, nCurrentRotation)] != L'.')
                            pField[(nCurrentY + py) * nFieldWidth + (nCurrentX + px)] = nCurrentPiece + 1;

                // check for completed lines
                for (int py = 0; py < 4; py++)
                    if (nCurrentY + py < nFieldHeight - 1)
                    {
                        bool bLine = true;
                        for (int px = 1; px < nFieldWidth - 1; px++)
                            bLine &= (pField[(nCurrentY + py) * nFieldWidth + px]) != 0;

                        if (bLine)
                        {
                            for (int px = 1; px < nFieldWidth - 1; px++)
                                pField[(nCurrentY + py) * nFieldWidth + px] = 8; // flash effect ID
                            vLines.push_back(nCurrentY + py);
                        }
                    }

                nScore += 25;
                if (!vLines.empty()) nScore += (1 << vLines.size()) * 100;

                // spawn next piece
                nCurrentX = nFieldWidth / 2 - 2;
                nCurrentY = 0;
                nCurrentRotation = 0;
                nCurrentPiece = nNextPiece;
                nNextPiece = rand() % 7;

                // game over check
                bGameOver = !DoesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentX, nCurrentY);
            }
        }

        // draw playfield blocks with colors
        for (int x = 0; x < nFieldWidth; x++)
            for (int y = 0; y < nFieldHeight; y++)
            {
                int val = pField[y * nFieldWidth + x];
                wchar_t c = L" ABCDEFG=#"[val];
                WORD attr = GetColorAttribute(val);

                // double width characters to keep square look in console
                screen[(y + 2) * nScreenWidth + (x * 2 + 2)].Char.UnicodeChar = c;
                screen[(y + 2) * nScreenWidth + (x * 2 + 2)].Attributes = attr;
                screen[(y + 2) * nScreenWidth + (x * 2 + 3)].Char.UnicodeChar = c;
                screen[(y + 2) * nScreenWidth + (x * 2 + 3)].Attributes = attr;
            }

        // draw active piece flying down
        for (int px = 0; px < 4; px++)
            for (int py = 0; py < 4; py++)
                if (tetromino[nCurrentPiece][Rotate(px, py, nCurrentRotation)] != L'.')
                {
                    wchar_t c = nCurrentPiece + 65;
                    WORD attr = GetColorAttribute(nCurrentPiece + 1);

                    int idx1 = (nCurrentY + py + 2) * nScreenWidth + ((nCurrentX + px) * 2 + 2);
                    int idx2 = (nCurrentY + py + 2) * nScreenWidth + ((nCurrentX + px) * 2 + 3);

                    screen[idx1].Char.UnicodeChar = c;
                    screen[idx1].Attributes = attr;
                    screen[idx2].Char.UnicodeChar = c;
                    screen[idx2].Attributes = attr;
                }

        // print score UI text
        wchar_t scoreText[32];
        swprintf_s(scoreText, 32, L"SCORE: %8d", nScore);
        for (int i = 0; i < wcslen(scoreText); i++) {
            int idx = 2 * nScreenWidth + nFieldWidth * 2 + 6 + i;
            screen[idx].Char.UnicodeChar = scoreText[i];
            screen[idx].Attributes = FOREGROUND_WHITE | FOREGROUND_INTENSITY;
        }

        // print next piece preview box UI
        wchar_t nextTitle[] = L"NEXT PIECE:";
        for (int i = 0; i < wcslen(nextTitle); i++) {
            int idx = 5 * nScreenWidth + nFieldWidth * 2 + 6 + i;
            screen[idx].Char.UnicodeChar = nextTitle[i];
            screen[idx].Attributes = FOREGROUND_WHITE;
        }

        for (int px = 0; px < 4; px++)
            for (int py = 0; py < 4; py++)
            {
                wchar_t c = L' ';
                WORD attr = 0;
                if (tetromino[nNextPiece][Rotate(px, py, 0)] != L'.')
                {
                    c = nNextPiece + 65;
                    attr = GetColorAttribute(nNextPiece + 1);
                }
                int idx1 = (7 + py) * nScreenWidth + nFieldWidth * 2 + 8 + (px * 2);
                int idx2 = (7 + py) * nScreenWidth + nFieldWidth * 2 + 9 + (px * 2);
                
                screen[idx1].Char.UnicodeChar = c;
                screen[idx1].Attributes = attr;
                screen[idx2].Char.UnicodeChar = c;
                screen[idx2].Attributes = attr;
            }

        // line clear animation handling
        if (!vLines.empty())
        {
            COORD coordSize = { (SHORT)nScreenWidth, (SHORT)nScreenHeight };
            COORD coordCoord = { 0, 0 };
            SMALL_RECT rectRegion = { 0, 0, (SHORT)(nScreenWidth - 1), (SHORT)(nScreenHeight - 1) };
            WriteConsoleOutputW(hConsole, screen, coordSize, coordCoord, &rectRegion);
            
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

        // render full screen buffer to console window in one go
        COORD coordSize = { (SHORT)nScreenWidth, (SHORT)nScreenHeight };
        COORD coordCoord = { 0, 0 };
        SMALL_RECT rectRegion = { 0, 0, (SHORT)(nScreenWidth - 1), (SHORT)(nScreenHeight - 1) };
        WriteConsoleOutputW(hConsole, screen, coordSize, coordCoord, &rectRegion);
    }

    CloseHandle(hConsole);
    cout << "Game Over!! Final Score: " << nScore << endl;
    system("pause");
    return 0;
}
