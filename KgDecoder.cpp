#include "KgDecoder.h"
#include "KgFrame.h"

KgDecoder::KgDecoder ()
{
    std::cout << "Decoder exists " << std::endl;
}

KgDecoder::~KgDecoder ()
{
    if(gifData != nullptr){
        gifData = nullptr;
    }
    if (gloColTable != nullptr) {
        delete[] gloColTable;
        gloColTable = nullptr;
    }
    actTable = nullptr;
   
}

void KgDecoder::SetData(uint8_t* data, uint32_t fisi){
    if (  data == nullptr || fisi == 0) {
        return;
    }
    this->gifData = data;
    this->gifSize = fisi;
    if (this->ReadHeader ()) {
        this->ReadgifData ();
    }
}
                                                                                                                                                                 
uint32_t KgDecoder::ReadByte (void* byt, uint32_t bysi)
{
    if (gifData == nullptr || gifSize == 0)
        return 0;
    try{
        memcpy (byt, this->gifData, bysi);
    }
    catch (std::exception& excep) {
        return 0; // mean error no byte read
    }
    
    this->gifData += bysi;
    this->gifSize -= bysi;
    uint32_t temp = this->dataInd;
    this->dataInd += bysi;
    //return byte read to make sure u make it successful
    return this->dataInd - temp;
}

bool KgDecoder::ReadHeader ()
{
    //get define header in gif file
    uint8_t sigver [6];
    if ((this->ReadByte (&sigver[0], 6) != 6)) {
        //file so short
        return false;
    }
    else if (memcmp (&sigver, "GIF", 3) != 0) {
        // not gif
        return false;
    }
    //success then keep reading logical screen 
    this->ReadByte (&this->screenWidth, 2);
    this->ReadByte (&this->screenHeight, 2);
    this->ReadByte (&this->gifPacked, 1);
    this->ReadByte (&this->bgColInd, 1);
    this->ReadByte (&this->pixAspectRatio, 1);
    //calculate value from pack field
    gloColFlag = (this->gifPacked >> 7) & 0x01;
    // get sorted flag
    gloColSortFlag = this->gifPacked & 0x08;
    //if color table doesnt exist, then next byte will be extension
    if (gloColFlag) {
        // calculate size
        gloBpCol = (this->gifPacked & 0x07); //count from 0, therefore length need to + 1
        this->ReadColTable (gloBpCol, &gloColTable);
    }
    //u dont need to get color resolution right now
    return true;
}

void KgDecoder::ReadgifData () {
    //read byte by byte then switch the case
    while (1) {
        ReadByte (&gifByte, 1);
        switch(gifByte)
        {
            case IMG_DECLARE: {
                
                //this may move or copy the content of value
                break;
            }
            case INTRO_EXT: {
                uint8_t labelbyte = 0;
                this->ReadByte(&labelbyte, 1);
                switch (labelbyte)
                {
                    case APP_EXT: {
                        //blockfield [11]bytes-data blockfield appid data1 data2 terminator 
                        uint8_t applicationauthen [11];
                        this->ReadByte(&labelbyte, 1); // read block size field, fixed size 11
                        ReadByte(&applicationauthen[0], labelbyte);
                        if(0 == memcmp(&applicationauthen[0], "NETSCAPE2.0", 11)){
                            //read netscape application
                            ReadByte(&labelbyte, 1); // block size field after authen
                            std::fill_n(applicationauthen, 11, 0);
                            ReadByte(&applicationauthen[0], labelbyte);
                            if(applicationauthen[0] == 0x01){
                                this->loopcount = applicationauthen[2] << 8 | applicationauthen[1];
                            }
                        } else {
                            //skip byte
                            SkipBlock ();
                        }
                        // skip block terminator
                        ReadByte (&labelbyte, 1);
                        break;
                    }
   
                    case PLAIN_TEXT_EXT: 
                    {
                        //blockfield textgridleft textgridtop textwidth textheight charcellwidth charcellheight
            //text-fg-color-ind text-bg-color-ind [plain text data : sub data block] terminator
                        ReadByte(&labelbyte, 1); //block field fixed value 12
                        //skip it, cus we dont have enough info to process
                        gifData += labelbyte;
                        //now skip data block
                        SkipBlock ();
                        //read terminate byte of ext block
                        ReadByte (&labelbyte, 1);
                        break;
                    }
    
                    case COMM_EXT: {
                        SkipBlock ();
                        break;
                    }

                    case GRAPHIC_EXT: {
                        ReadExtnImg();
                        break;
                    }   
                    default: {
                        // 2nd switch
                        break;
                    }
                }
                //break for this case
                break;
            }
            case GIF_TERM:
            {
                return;
            }
            default: {
                //1st switch
                break;
            }
        }
    }
   
}

void KgDecoder::ReadColTable (uint8_t& bitpercol, kColor** whattable)
{
    //allocate new mem for color table
    uint32_t tabsize = 3L * (1L << (bitpercol + 1));
    //  or u can do like this : tabsize = 3 * (2 << bitpercol), it will return the same res
    *whattable = new kColor[tabsize];
    //read color table, 3 bytes for a color, 
        // the tabsize is size in byte
    //  *whattable = kcolor*,       **whattable = kcolor,           &(**whattable) = address of kcolor
    this->ReadByte(&**whattable, tabsize);
    //point active color to current color table
    actTable = *whattable;
}

void KgDecoder::SkipBlock ()                                
{
    uint8_t blosize = 0;
    //this will continue reading with current pointer position
    //till it reaches terminator block
    do {
        ReadByte (&blosize,1 );
        if (blosize == 0) {
            //is this the end of extension or just sub data block
            return;
        }
        else {
            gifData += blosize;
        }
    } while (1);
}

uint32_t KgDecoder::ReadBlock (std::array<uint8_t, 256>& blockcontainer, uint32_t size)
{
    uint32_t res = 0;
    res = ReadByte (&blockcontainer.at(0), size);
    return res;
}

void KgDecoder::ReadExtnImg ()
{
    KgFrame tempFrame;
    uint8_t dabyte;
    //read graphic
    this->ReadByte (&dabyte, 1);  //block field  - fixed size 4 bytes
    ReadByte (&dabyte, 1); // pack field
    //shifst right by 2, then perform (&0x07) to get disposal value
    tempFrame.disposal = ((dabyte & 0x1c) >> 2);
    tempFrame.transColFlag = (dabyte & 0x01);
    ReadByte (&tempFrame.delayTime, 2);
    ReadByte (&tempFrame.transColIndex, 1);  //doesn't matter will check when we need
    ReadByte (&dabyte, 1); //read in block terminator to advance the cur pos of pointer
    //read img data after graphic ext
    // make sure the next byte will be img declare byte
    ReadByte (&dabyte, 1);
    if (dabyte != 0x2c) {
        perror ("Gif file is corrupted");
        return;
    }
    this->ReadImg (&tempFrame);
    
}

void KgDecoder::ReadImg (KgFrame* inFrame)
{
    //if set it to nullptr, exp will be thrown cuz it is nullptr so no memory allocate
    KgFrame* frame = new KgFrame;
    uint8_t  imgbyte = 0;
    if (inFrame != nullptr) {
        //perform move action
        *frame = std::move (*inFrame);
        //call destructor on argument frame , so restore the resource after moving
        inFrame->~KgFrame ();
    }
    // if not just perform reading img descriptor and then lzw-img- data code
    ReadByte (&frame->imgLeft, 2);
    ReadByte (&frame->imgTop, 2);
    ReadByte (&frame->imgWidth, 2);
    ReadByte (&frame->imgHeight, 2);
    //read pack field of img descriptor, there will be no terminator block for this block, just like logical screen descriptor
    ReadByte (&imgbyte, 1);
    if (imgbyte != 0x00) {     //if packed == 0x00, then skip all  cuz all just false or 0
        frame->localColFlag = (imgbyte & 0x80) >> 7;
        frame->interlaceFlag = (imgbyte & 0x40) >> 6;
        if (frame->localColFlag) {
            //check sorted. if flag is false then skip this
            frame->sortedColFlag = (imgbyte & 0x20) >> 5;
            frame->bpc = (imgbyte & 0x07);
            ReadColTable (frame->bpc, &frame->localColTab);   //this fn will set active color table pointer
        }
    }
    //decode incoming code 
        //process then out put data for pixel
    /*
    only 1 CC in each table
        code table include CODE INDEX and VALUE INDEX   .  value index may be color or char index
            code index          value index  -> color or char value
                 0                            0
                 1                             1                ,,'',, init code table, aka, single value index sequence
                 2                             2
               ......                           .....
        clear code (CC) =   1 << lzw init code        , the clear code value = clear code index = 1 << lzw init
        end of info       =           CC + 1
            available      =            CC + 2          , this is the 1st code u can use, after initing code table, to add new value index sequence 
 
    */
    // clear code value = 1 << lzw init code size, 
    // ==> end of info code value = clearcode value + 1;

    //  TODO clear pixels after done
    uint32_t imgSize = frame->imgWidth * frame->imgHeight;
    uint8_t* pixels = new uint8_t[imgSize];     //for img data
    uint16_t* prefix = new uint16_t[MAX_STACK_SIZE];
    uint8_t* pixStack = new uint8_t[MAX_STACK_SIZE + 1];
    uint32_t stackCount = 0;
    uint8_t* suffix = new uint8_t[MAX_STACK_SIZE];

    
    uint32_t initCodesize = 0;
    //this is actually LZW INIT CODE  size
    ReadByte (&initCodesize, 1); 

    uint32_t   clearCo = 1 << initCodesize;  // code index == code value
    uint32_t    eofCo = clearCo + 1;
    uint32_t    newCo = eofCo + 1;
    uint32_t    maxCo = clearCo * 2 - 1; //the max index can be used before increasing num of bits per code
    //      available bits to represent code never go over 12 bits
    uint32_t    realCosize = initCodesize + 1;    //this is the code size actually used  , the index before is occupied by init value table

        //use code mask to return the actual code value need to read
    uint32_t    codeMask = (1 << realCosize) - 1;   //if  real code size : 3, then mask = 111

    uint32_t    bitBench = 0;    //read in data | current data
    uint32_t    bitsCount = 0;
    uint8_t     blockIter = 0;    //max for sub block is ff, so u dont need more then 16 bits
    uint32_t bloSiField = 0;
    uint32_t code = 0 , oldCod = -1, currCode =0 , lstColIndex = 0;

    for (int i = 0; i < clearCo; i++) {
        prefix[i] = 0;
        suffix[i] = i;
    }

    // MAKE IT A LOOP, keep read in sub block till the length of sub block is 0x00
    //read in SUB BLOCK DATA 
    do {
        ReadByte (&bloSiField, 1);   //read block size field
        if (bloSiField == 0 || ReadByte (&dablock->at (0), bloSiField) == 0) {
            return; // error or end of image data
        }
        blockIter = 0; // reset iterator
        //HOW TO KNOW IF BLOCK END
        while (blockIter != bloSiField) {  
            if (bitsCount < realCosize) {
                //if remaining bits < realcode size then u don't need to read in
                bitBench = ((uint32_t)dablock->at (blockIter)) << bitsCount;
                blockIter++;
                bitsCount += 8; // u read in 8 bits
                //extract the code
                code = (codeMask & bitBench);
                bitBench >>= realCosize;
                bitsCount -= realCosize;
            }
            else {
                //reuse earlier bit bench
                code = (codeMask & bitBench);
                bitBench >>= realCosize;
                bitsCount -= realCosize;
            }

            if (code == clearCo) {
                realCosize = initCodesize + 1;
                newCo = eofCo + 1;
                oldCod = -1;
                codeMask = (1 << realCosize) - 1;
                continue; //so it comes out then read next one
            }
            if (code != eofCo) {
                if (code == codeMask && realCosize != 12) {    //max number of bits is 12
                    realCosize++;
                    codeMask = codeMask * 2 + 1;     //another way to calculate the next limit
                }
                std::cout << std::hex <<  code << std::endl;
                continue;
            }
        }

    } while (bloSiField != 0);
     
}
