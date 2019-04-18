#ifndef K_GIF_FRAME_H
#define K_GIF_FRAME_H

#include "kommon.h"

class KgFrame {
public:
    KgFrame ();
    ~KgFrame ();

    KgFrame (const KgFrame& src);
    KgFrame& operator=(const KgFrame& rhs);

    //move and move assignment
    KgFrame& operator=(KgFrame&& rhs) noexcept;
    KgFrame (KgFrame&& src) noexcept;

    /*
        graphic control extension information
            1 byte count, packed, delayed time u16 bits, trans color ind byte
            packed : [7-5] reserved [4-2] disposal method [1] user input flag [0] transparent color flag
    */
    uint32_t disposal = 0;
    bool transColFlag = false;
    uint16_t delayTime = 0; // 1/100 unit of sec
    uint8_t transColIndex = 0; // in graphic extension block

    /*
        these fields are from local image descriptor
        2c imgleft imgtop imgwidth imgheight packfield
    */
    uint16_t imgLeft = 0;
    uint16_t imgTop = 0, imgWidth =0, imgHeight = 0;
    /*
        [7] local color tab flag, [6] interlace flag, [5] sort flag, [4-3] reserved
        [2-0] size of local col table
    */
    //uint8_t imgPacked = 0;
    bool localColFlag = false;
    bool interlaceFlag = false;
    bool sortedColFlag = false;
    uint8_t bpc = 0; // bits per color, count from 0
    //uint32_t localColTableSize = 0;
    kColor* localColTab = nullptr;

    //image pixel data or 

private:
    void Cleanup () noexcept;
    void MoveFrom (KgFrame& src) noexcept;
};

#endif // !K_GIF_FRAME_H
