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
    // ==> end of image code value = clearcode value + 1;
    // new available code = end of image + 1;

    //  TODO clear pixels after done and array after done
    uint32_t imgSize = frame->imgWidth * frame->imgHeight;
    uint8_t* pixels = new uint8_t[imgSize];     //for img data
    uint8_t* suffix = new uint8_t[MAX_BIT_SIZE];
    std::fill_n (suffix, MAX_BIT_SIZE, 0);
    uint16_t* prefix = new uint16_t[MAX_BIT_SIZE];
    std::fill_n (prefix, MAX_BIT_SIZE, 0);
    uint8_t* pixStream = new uint8_t[MAX_BIT_SIZE];
    std::fill_n (pixStream, MAX_BIT_SIZE, 0);
    uint32_t streamPos = 0;

    //read init code size and initialize code
    uint8_t initSize = 0;   
    ReadByte (&initSize, 1);
    uint32_t clearcode = 1 << initSize;
    uint32_t endofimg = clearcode + 1;
    uint32_t newEntry = endofimg + 1;
    uint16_t realCoSize =(uint16_t) initSize + 1;  //max num of bits = 12
    uint32_t codeMask = (1 << realCoSize) - 1;

    for (uint32_t i = 0; i < clearcode; i++) {
        suffix[i] = i;
    }
    //read block data and process data
    uint8_t blosifield = 0;
    ReadByte (&blosifield, 1);
    if (blosifield == 0)    return; //no image data
    ReadByte (&dablock->at(0), (uint32_t)blosifield);
    uint8_t blockCur = 0, firstInd =0;
    uint32_t bitbench = 0; 
    uint16_t bitsCount = 0, code = 0, oldCo = 0xffff;
    while (blockCur <= dablock->size ()) {
        uint16_t currCode = 0;
        if (bitsCount < realCoSize) {
            bitbench |= ((uint32_t)dablock->at (blockCur)) << bitsCount;
            blockCur++;
            bitsCount += 8;
        }
        //extract code
        code = codeMask & bitbench;
        bitbench >>= realCoSize;
        bitsCount -= realCoSize;
        // process code
        if (code == clearcode) {
            realCoSize = initSize + 1;
            codeMask = (1 << realCoSize) - 1;
            oldCo = 0xffff;
            newEntry = endofimg + 1;
            continue; // perform next loop
        }
        if (code == endofimg)    return; //break out of the loop
        if (oldCo == 0xffff) {      // 1st code
            firstInd = code;
            oldCo = code;
            pixStream[streamPos++] = firstInd;
            continue; // perform next loop
        }
        currCode = code;
        if (code == newEntry) {      //new code not exist in table
            pixStream[streamPos++] = firstInd;
            code = oldCo;
        }
        //loop all the way back to the init table
        while (code > clearcode) {  //this haven't included the 1st index of sequence that code represents, cus it stops before the last 1
            pixStream[streamPos++] = suffix[code];
            code = prefix[code];
        }
        firstInd = suffix[code];
        pixStream[streamPos++] = firstInd;
        //add new code to prefix, suffix
        if (newEntry < MAX_BIT_SIZE) {
            suffix[newEntry] = firstInd;
            prefix[newEntry] = oldCo;
        }
        newEntry++;
        if ((newEntry & codeMask) == 0 && (newEntry < MAX_BIT_SIZE)) {
            realCoSize++;
            codeMask = (codeMask * 2) + 1;
        }
        oldCo = currCode;
    }

    delete[] suffix;
    suffix = nullptr;
    delete[] prefix;
    prefix = nullptr;
    delete[] pixStream;
    pixStream = nullptr;
    delete[] pixels;
    pixels = nullptr;

     
}
