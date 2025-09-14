/*
    Turbo C:
        tcc cgasamp.c

    Watcom 11:
        wcl cgasamp.c

    Keys:
        Escape  - quit
        0       - select palette 0 green/red/brown (video mode 4)
        1       - select palette 1 cyan/magenta/light grey (video mode 4)
        2       - select palette 2 cyan/red/light grey (video mode 5)
        I       - toggle intensity palette
        Up/Down - cycle the background colour through the 16 colours of the full CGA palette
*/

#include <dos.h>
#include <mem.h>
#include <conio.h>

// #define VGA_COMPATIBLE

enum Palette
{
    Palette0 = 0,                           // bg/green/red/brown - only available in mode 4
    Palette1,                               // bg/cyan/magenta/light grey - only available in mode 4
    Palette2                                // bg/cyan/red/light grey - only available in mode 5
};

// Video modes
#define VIDEO_MODE_TEXT             0x03    // 80x25 text mode
#define VIDEO_MODE_320X200_4        0x04    // 320x200, 4 colours, only Palette0 and Palette1 available
#define VIDEO_MODE_320X200_5        0x05    // 320x200, 4 colours, only Palette2 available

// CGA registers
#define CGA_COLOR_SELECT_REGISTER   0x3D9
#define CGA_PALETTE_BIT             (1<<5)  // 1 = cyan/magenta/white, 0 = green/red/brown
#define CGA_PALETTE_INTENSITY_BIT   (1<<4)  // 1 = select high intensity palette, 0 = select low intensity palette
#define CGA_BACKGROUND_MASK         0x0F    // background colour bits are 0-3 and effectively selects from the 16 colour palette

#define VIDEO_MEMORY_SEGMENT        0xB800
#define VIDEO_MEMORY_SIZE           (16*1024)

#define KB_DATA_REGISTER            0x60
#define KB_ESCAPE_SEQUENCE          0xE0
#define KB_KEY_ESCAPE               0x1B
#define KB_KEY_0                    '0'
#define KB_KEY_1                    '1'
#define KB_KEY_2                    '2'
#define KB_KEY_I                    'I'
#define KB_KEY_UP                   0x48
#define KB_KEY_DOWN                 0x50

// Sets the current video mode.
// Returns 1 if the mode was changed, or 0 if the current mode was the same as the new mode.
char SetVideoMode(unsigned char mode)
{
    static unsigned char currentMode = 0xFF;
    union REGS regs;

    if (currentMode != mode)
    {
        regs.h.ah = 0x00;  // ah = 0 is "set video mode" function
        regs.h.al = mode;
        int86(0x10, &regs, &regs);
        currentMode = mode;
        return 1;
    }

    return 0;
}

// Sets the current CGA palette, intensity, and background colour.
// Returns 1 if the video mode was changed, or 0 if not.
char SetCGAPalette(enum Palette palette, char highIntensity, unsigned char backgroundColour)
{
    char modeChanged = 0;
    unsigned char value = 0;
#if defined(VGA_COMPATIBLE)
    union REGS regs;
#endif

#if defined(VGA_COMPATIBLE)
    regs.h.ah = 0x0B; // set color palette
#endif

    if (palette == Palette2)
    {
        // palette 2 requires mode 5
        modeChanged = SetVideoMode(VIDEO_MODE_320X200_5);
    }
    else
    {
        // Should be palette 0 or 1. These require mode 4
        modeChanged = SetVideoMode(VIDEO_MODE_320X200_4);

#if defined(VGA_COMPATIBLE)
        regs.h.bh = 0x01; // select palette
        regs.h.bl = 0x00; // default to green/red/brown
#endif

        if (palette == Palette1)
        {
#if defined(VGA_COMPATIBLE)
            regs.h.bl = 0x01; // select cyan/magenta/white
#else
            // Select alternate palette
            value |= CGA_PALETTE_BIT;
#endif
        }

#if defined(VGA_COMPATIBLE)
        int86(0x10, &regs, &regs);
#endif
    }

    // Set background color
    value |= (backgroundColour & CGA_BACKGROUND_MASK);

    if (highIntensity)
    {
        // Select the high intensity palette
        value |= CGA_PALETTE_INTENSITY_BIT;
    }

#if defined(VGA_COMPATIBLE)
    regs.h.bh = 0x00; // set background colour
    regs.h.bl = value;
    int86(0x10, &regs, &regs);
#else
    outp(CGA_COLOR_SELECT_REGISTER, value);
#endif

    return modeChanged;
}

void SetPixel(unsigned int x, unsigned int y, unsigned char colour, unsigned char* buffer)
{
    //unsigned char far *vidmem = MK_FP(0xb800, 0);
    unsigned int byteIndex = ((y & 1) * 8192) + ((x + ((y >> 1) * 320)) >> 2);
    unsigned char pixelBitIndex = 6 - (2 * (x & 3));
    unsigned char pixelMask = ~(3 << pixelBitIndex);
    unsigned char value = buffer[byteIndex];

    // First clear the existing bits
    value &= pixelMask;

    // Now set the colour
    value |= (colour << pixelBitIndex);

    // Store back into the buffer
    buffer[byteIndex] = value;
}

void DrawPattern(unsigned char* buffer)
{
    #define BLOCK_SIZE 20
    unsigned int x;
    unsigned char colour = 0;
    for (x = 0; x < 320; x += BLOCK_SIZE)
    {
        unsigned int bx, by;
        for (by = 0; by < 200; by++)
        {
            for (bx = x; bx < x + BLOCK_SIZE; bx++)
            {
                SetPixel(bx, by, colour, buffer);
            }
        }

        colour = (colour + 1) & 3;

        printf(".");
        fflush(0);
    }
}

void CopyBufferToVideoMemory(unsigned char* buffer)
{
    movedata(FP_SEG(buffer), FP_OFF(buffer), VIDEO_MEMORY_SEGMENT, 0, VIDEO_MEMORY_SIZE);
}

static unsigned char s_pattern[VIDEO_MEMORY_SIZE];

int main()
{
    enum Palette currentPalette = Palette0;
    unsigned char highIntensity = 0;
    unsigned char backgroundColour = 0;

    printf("Initialising");
    DrawPattern(s_pattern);

    SetCGAPalette(currentPalette, highIntensity, backgroundColour);
    CopyBufferToVideoMemory(s_pattern);

    while (1)
    {
        unsigned char refreshPalette = 0;
        int key = toupper(getch());

        if (key == KB_ESCAPE_SEQUENCE)
        {
            // Ignore escape sequences and read the next key code
            key = getch();
        }

        if (key == KB_KEY_ESCAPE)
            break;

        switch (key)
        {
            case KB_KEY_0:
            {
                // Switch to palette 0
                currentPalette = Palette0;
                refreshPalette = 1;
                break;
            }
            case KB_KEY_1:
            {
                // Switch to palette 1
                currentPalette = Palette1;
                refreshPalette = 1;
                break;
            }
            case KB_KEY_2:
            {
                // Switch to palette 2
                currentPalette = Palette2;
                refreshPalette = 1;
                break;
            }
            case KB_KEY_I:
            {
                // Toggle the intensity palette
                highIntensity ^= 1;
                refreshPalette = 1;
                break;
            }
            case KB_KEY_UP:
            {
                backgroundColour = (backgroundColour + 1) & 0xF;
                refreshPalette = 1;
                break;
            }
            case KB_KEY_DOWN:
            {
                backgroundColour = (backgroundColour - 1) & 0xF;
                refreshPalette = 1;
                break;
            }
        }

        if (refreshPalette)
        {
            if (SetCGAPalette(currentPalette, highIntensity, backgroundColour))
            {
                // Video mode changed, redraw the pattern because video memory may have been trashed
                CopyBufferToVideoMemory(s_pattern);
            }
        }
    }

    SetVideoMode(VIDEO_MODE_TEXT);

    return 0;
}
