#ifndef K_GIF_DECODER_H
#define K_GIF_DECODER_H

#include "kommon.h"
#include "KgFrame.h"

#define INTRO_EXT 0x21
#define GRAPHIC_EXT 0xf9
#define APP_EXT 0xff
#define PLAIN_TEXT_EXT 0x01
#define COMM_EXT 0xfe

#define IMG_DECLARE 0x2c
#define GIF_TERM 0x3b


class KgDecoder {
public:
    KgDecoder ();
    ~KgDecoder ();

    void SetData (uint8_t* data, uint32_t  fisi);


private:

    static const uint32_t MAX_BIT_SIZE = (uint32_t) (4096);
    static const uint32_t MAX_BIT = (uint32_t)( 0x0c);
    uint8_t* gifData = nullptr;
    uint32_t gifSize = 0;
    uint8_t gifByte = 0;
    kColor* gloColTable = nullptr;
    kColor* actTable = nullptr;
    std::array<uint8_t, 256>* dablock = new std::array<uint8_t, 256>;
    std::vector<KgFrame> frameContainer;
    uint32_t frameCount = 0;
    /*
        information of gif file header
    */
    uint16_t screenWidth = 0, screenHeight =0 ;
    // [7] glo color tab flag   
    // [6-4] color resolution, num of bits per primary color -1 to orginal img
        // 001 = 2 bits/pix, 111= 8 bits/pixel
    // [3] sorted flag [2-0] size of glo color table
    uint8_t gifPacked = 0, bgColInd = 0, pixAspectRatio = 0;
    bool gloColSortFlag = false, gloColFlag = false;
    uint8_t gloBpCol = 0;
    //uint32_t gloColTabSize = 0;
    
    /*
   applcation extension information
            1 byte count, 11 byte flowing , 1 byte count, app id, lo-by hi-by of num of loop     
    */
    uint16_t loopcount = 0;
   /*
    minion method
   */
  uint32_t dataInd = 0; //this use to keep track of bytes already read
  uint32_t ReadByte(void* byt, uint32_t bysi);
  //to modify a pointer value, aka address pointed to by pointer , 
        //u need to use *& - reference to pointer, for easier think of it like reference to value
  void SkipBlocks (uint8_t*& imgDat);

  bool ReadHeader ();
  void ReadgifData ();
  //modify original pointer => make it as a value, 
  //        this case "whattable" is a pointer to 'pointer to kcolor', so when modifying whattable
  //        need to dereference to return the underlying value - pointer to kcolor
  //        *whattable = kcolor*, aka the kcolor* u passed in
  void ReadColTable (uint8_t& bitpercol, kColor ** whattable);
  bool ReadImgControlExt (uint8_t* & imgDat, KgFrame& inframe);     //pass in directly the obj cuz inframe is reference so it already dereferenced
  void ReadImg (uint8_t* & imgDat , KgFrame& inframe); //pass in address of obj

};
#endif // !K_GIF_DECODER_H
