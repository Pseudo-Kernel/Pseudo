#pragma once

#include "prototype_base.h"

namespace prototype
{
    //
    // console.
    //

    class console
    {
    public:
        console(short size_x, short size_y, short color,
            void *conin = nullptr, void *conout = nullptr, void *conerr = nullptr) :
            x_(0),
            y_(0),
            sx_(size_x),
            sy_(size_y),
            color_(color),
            conin_(conin),
            conout_(conout),
            conerr_(conerr)
        {
            if (!conin_)
                conin_ = GetStdHandle(STD_INPUT_HANDLE);

            if (!conout_)
                conout_ = GetStdHandle(STD_OUTPUT_HANDLE);

            if (!conerr_)
                conerr_ = GetStdHandle(STD_ERROR_HANDLE);

            init();
        }

        void clear(bool reset_cursor)
        {
            COORD pos = { 0, 0 };
            DWORD dummy = 0;

            std::lock_guard<decltype(mutex_)> lock(mutex_);
            FillConsoleOutputAttribute(conout_, color_, sx_ * sy_, pos, &dummy);
            FillConsoleOutputCharacterA(conout_, ' ', sx_ * sy_, pos, &dummy);

            if (reset_cursor)
                set_xy(0, 0);
        }

        void printf(const char *format, ...)
        {
            char buffer[512];

            va_list args;
            va_start(args, format);
            vsprintf_s(buffer, format, args);
            va_end(args);

            print(buffer);
        }

        void printf_xy(short x, short y, const char *format, ...)
        {
            char buffer[512];

            va_list args;
            va_start(args, format);
            vsprintf_s(buffer, format, args);
            va_end(args);

            print_xy(x, y, buffer);
        }

        void print(char *text)
        {
            DWORD size = static_cast<DWORD>(strlen(text));
            DWORD dummy = 0;

            std::lock_guard<decltype(mutex_)> lock(mutex_);
            WriteConsoleA(conout_, text, size, &dummy, nullptr);

            CONSOLE_SCREEN_BUFFER_INFO sbi{};
            GetConsoleScreenBufferInfo(conout_, &sbi);
            x_ = sbi.dwCursorPosition.X, y_ = sbi.dwCursorPosition.Y;
        }

        void print_xy(short x, short y, char *text)
        {
            DWORD size = static_cast<DWORD>(strlen(text));
            DWORD dummy = 0;

            std::lock_guard<decltype(mutex_)> lock(mutex_);
            set_xy(x, y);
            WriteConsoleA(conout_, text, size, &dummy, nullptr);

            CONSOLE_SCREEN_BUFFER_INFO sbi{};
            GetConsoleScreenBufferInfo(conout_, &sbi);
            x_ = sbi.dwCursorPosition.X, y_ = sbi.dwCursorPosition.Y;
        }

        void set_x(short x)
        {
            COORD pos = { x, y_ };
            _set_xy(pos);
        }

        void set_y(short y)
        {
            COORD pos = { x_, y };
            _set_xy(pos);
        }

        void set_xy(short x, short y)
        {
            COORD pos = { x, y };
            _set_xy(pos);
        }

        void set_color(short color)
        {
            SetConsoleTextAttribute(conout_, color);
            color_ = color;
        }

        short x() const
        {
            return x_;
        }

        short y() const
        {
            return y_;
        }

        short size_x() const
        {
            return sx_;
        }

        short size_y() const
        {
            return sy_;
        }

        short color() const
        {
            return color_;
        }

    private:
        void init()
        {
            CONSOLE_FONT_INFOEX cfi{};
            cfi.cbSize = sizeof(cfi);
            cfi.nFont = 0;
            cfi.FontFamily = FF_DONTCARE;
            cfi.FontWeight = FW_NORMAL;
            //cfi.dwFontSize.X = 0;
            //cfi.dwFontSize.Y = 16;
            //wcscpy_s(cfi.FaceName, L"Consolas");
            cfi.dwFontSize.X = 9;
            cfi.dwFontSize.Y = 16;
            wcscpy_s(cfi.FaceName, L"Terminal");
            SetCurrentConsoleFontEx(conout_, FALSE, &cfi);

            COORD size = { sx_, sy_ };
            SetConsoleScreenBufferSize(conout_, size);
            SetConsoleTextAttribute(conout_, color_);

            clear(true);

            SetWindowPos(GetConsoleWindow(), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW);
        }

        void _set_xy(COORD pos)
        {
            std::lock_guard<decltype(mutex_)> lock(mutex_);

            SetConsoleCursorPosition(conout_, pos);
            x_ = pos.X, y_ = pos.Y;
        }

        std::recursive_mutex mutex_;
        short x_, y_;
        short sx_, sy_;
        short color_;
        //short text_color_;
        //short bk_color_;
        void *conin_;
        void *conout_;
        void *conerr_;
    };

    console con(120, 70, 0x07);
}
