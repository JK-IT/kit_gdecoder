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
        return 0xffffffff;  // = -1
    try{
        memcpy (byt, this->gifData, bysi);
    }
    catch (std::exception& excep) {
        return 0xffffffff; // =-1
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
                uint8_t introbyte = 0;
                this->ReadByte(&introbyte, 1);
                switch (introbyte)
                {
                    case APP_EXT: {
                        //blockfield [11]bytes-data blockfield appid data1 data2 terminator 
                        uint8_t applicationauthen [11]; //array on stack is freed automatically by system
                        this->ReadByte(&introbyte, 1); // read block size field, fixed size 11
                        ReadByte(&applicationauthen[0], introbyte);
                        if(0 == memcmp(&applicationauthen[0], "NETSCAPE2.0", 11)){
                            //read netscape application
                            ReadByte(&introbyte, 1); // block size field after authen
                            std::fill_n(applicationauthen, 11, 0);
                            ReadByte(&applicationauthen[0], introbyte);
                            if(applicationauthen[0] == 0x01){
                                this->loopcount = applicationauthen[2] << 8 | applicationauthen[1];
                            }
                        } else {
                            //skip byte
                            SkipBlocks (gifData);
                        }
                        // skip block terminator
                        ReadByte (&introbyte, 1);
                        break;
                    }
   
                    case PLAIN_TEXT_EXT: 
                    {
                        //blockfield textgridleft textgridtop textwidth textheight charcellwidth charcellheight
            //text-fg-color-ind text-bg-color-ind [plain text data : sub data block] terminator
                        ReadByte(&introbyte, 1); //block field fixed value 12
                        //skip it, cus we dont have enough info to process
                        gifData += introbyte;
                        //now skip data block
                        SkipBlocks (gifData);
                        //read terminate byte of ext block
                        ReadByte (&introbyte, 1);
                        break;
                    }
    
                    case COMM_EXT: {
                        SkipBlocks (gifData);
                        break;
                    }

                    case GRAPHIC_EXT: {
                        //always come before and followed by image data
                        KgFrame imgFrame;
                        if (!ReadImgControlExt (gifData ,imgFrame)) {
                            //not successful 
                            break;
                        }
                        ReadByte (&introbyte, 1);
                        if (introbyte != 0x2c) {
                            break;
                        }
                        ReadImg (gifData ,imgFrame);
                        introbyte = 0;
                        break;
                    }   
                    default: {
                        // 2nd switch
                        break;
                    }
                }
                //break for this intro ext case
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

bool KgDecoder::ReadImgControlExt (uint8_t*& imgDat, KgFrame& inframe)  //require 1st byte will be block size field
{
    /*          ...... ext id, graphic control id
                    Block Size[1 byte] -- packfield [1 byte] -- delay time [ 2 bytes] -- transColor ind [1 byte] -- block terminator[1 byte]   
                    default - 4 bytes
                                              [7-5]                 [4-2]                         [1]                            [0]
     <Packed Fields>  =     Reserved        Disposal Method    User Input Flag    Transparent Color Flag
                                             3 Bits                 3 Bits                         1 Bit                           1 Bit                                             
    */
    uint8_t extBlockfield = 0;  
    if (ReadByte (&extBlockfield, 1) == 0xffffffff) {
        return false;
    };
    if (extBlockfield != 0x04) {   //default block size field of this extension block
        return false;
    }
    ReadByte (&dablock->at (0), extBlockfield);
    uint8_t tempByte = 0;
    //process packed field
    tempByte = dablock->at (0);   //pack field
    inframe.disposal = tempByte & 0x1c;
    inframe.transColFlag = tempByte & 0x01;
    //get delay time
    memcpy (&inframe.delayTime, &dablock->at (1), 2);
    // get transparent color index
    memcpy (&inframe.transColIndex, &dablock->at (3), 1);
    //read next terminator block of this extention
    ReadByte (&extBlockfield, 1);
    return true;
}

void KgDecoder::SkipBlocks (uint8_t*& data)                                
{
    //read in next byte, then advanced the pointer pos to by the value of byte
    uint8_t blosize = 0;
    //this will continue reading with current pointer position
    //till it reaches terminator block
    do {
        memcpy (&blosize, data, 1);
        data += 1;
        if (blosize == 0) {
            //is this the end of extension or just sub data block
            return;
        }
        else {
            data += blosize;
        }
    } while (1);
}


void KgDecoder::ReadImg (uint8_t*& imgDat ,KgFrame& inFrame)
{    /*     img seperator byte already read before pass in here    
        (img seperator 0x2c)-- img_left(2 bytes)    img_top(2 bytes)   img_width(2 bytes)     img_height(2 bytes)     packed_field (1 byte)

<Packed Fields>  =  Local Color Table Flag - Interlace Flag -  Sort Flag -      Reserved -          Size of Local Color Table                                                          
                                            1 Bit[7]                    1 Bit [6]             1 Bit [5]        2 Bits[4-3]              3 Bits [2-0]
         //right after this will be local color table if exists
     */
    ReadByte (&inFrame.imgLeft, 2);
    ReadByte (&inFrame.imgTop, 2);
    ReadByte (&inFrame.imgWidth, 2);
    ReadByte (&inFrame.imgHeight, 2);
    uint8_t tempByte = 0;
    ReadByte (&tempByte, 1);
    inFrame.localColFlag = (tempByte & 0x80) >> 7;
    inFrame.interlaceFlag = (tempByte & 0x40) >> 6;
    inFrame.sortedColFlag = (tempByte & 0x20) >> 5;
    inFrame.bpc = tempByte & 0x07; // only last 3 bits are used , bits per color
    if (inFrame.localColFlag) {
        ReadColTable (inFrame.bpc, &inFrame.localColTab);
        //after this, the gif data pointer will be at next byte or initial code size of image
    }
    //if local table not exist then then next byte will be initial code size
     
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

    uint32_t imgSize = inFrame.imgWidth * inFrame.imgHeight;
    //image array
    uint8_t* imgPix = new uint8_t[imgSize];
    std::fill_n (&imgPix[0], imgSize, 0);   //init with 0 value
    uint32_t pixlocation = 0;
    //temporary image array
    std::vector<uint8_t> tmpPixels(MAX_BIT_SIZE, 0);
    uint32_t tmplocation = 0;
    // 2 columns of working code container
    /* position, aka code
         code xtracted   1st     2nd      3rd   CC   EOI   6th     7th     8th      9th
        main indexes [    #1     #2        #3    cc     eoi    1         3       3          2
        pre indexes   [     0       0           0      0      0       3        1       6         8  
    */
    std::vector<uint16_t> preIndexes(MAX_BIT_SIZE, 0);
    std::vector<uint8_t>mainIndexes (MAX_BIT_SIZE, 0);
    //read INIT CODE size of gif
    uint8_t initCosize = 0;
    ReadByte (&initCosize, 1);
    uint32_t clearcode = 1 << initCosize;
    uint32_t endofimg = clearcode + 1;
    uint32_t newEntry = endofimg + 1;
    uint32_t realCosize = initCosize + 1, firstInd = 0, preCode = 0xffffffff,
        currCode = 0, code = 0, bitbend = 0, bitcount = 0;
    uint32_t codemask = (1 << realCosize) - 1;
    //init main array with code, this will be init table with single sequence for each code
    for (int i = 0; i < clearcode; i++) {
        mainIndexes[i] = i;
    }
    while(pixlocation <= imgSize) {
        //read block data till end,
            //extract code then add code to indexes table and temp pix stack
            //adding pix from temp stack to img pix
        ReadByte (&tempByte, 1);    //get data block size
        if (tempByte == 0) {
            return; //done with img data
        }
        ReadByte (&dablock->at (0), tempByte);
        auto beginIt = dablock->begin ();
        auto endIt = dablock->end ();
        while (beginIt < endIt) {
            while (bitcount < realCosize) {
                bitbend |= *beginIt;
                beginIt++;
                bitcount += 8;
            }
                //extract code
            code = bitbend & codemask;
            bitbend >>= realCosize;
            bitcount -= realCosize;
                //process code
            if (code == clearcode) {
                //reset everything
                realCosize = initCosize + 1;
                codemask = (1 << realCosize) - 1;
                newEntry = endofimg + 1;
                preCode = 0xffffffff;
                continue;   //escape for next loop
            }
            if (preCode == 0xffffffff) {    //1st code after clear code
                preCode = code;
                firstInd = code;
                //output the code to code stream
                imgPix[pixlocation++] = static_cast<uint8_t>( code);
                continue;
            }
            currCode = code;
            if (code == newEntry) {//code doesn't exist
                //put 1st index of precode and value of precode  to temp stack
                tmpPixels[tmplocation++] = static_cast<uint8_t>(firstInd);
                code = preCode;
            }
            while (code > clearcode) {//loop over precode till single value code, aka not sequence
                tmpPixels[tmplocation++] = mainIndexes[code];
                code = preIndexes[code]; 
            }
            firstInd = static_cast<uint32_t>(mainIndexes[code]);
            tmpPixels[tmplocation++] = static_cast<uint8_t>(firstInd);
            //add to code book
            if (newEntry < MAX_BIT_SIZE) { // #0 -> #4095
                mainIndexes[newEntry] = firstInd;
                preIndexes[newEntry] = preCode;
                newEntry++;
            }
            //check if new entry pass the limit after increasing
            if (((newEntry & codemask) == 0) && (realCosize < 0x0c)) {
                //inc the number of bits will be read
                realCosize++;
                codemask = (codemask * 2) + 1;
            }
            //move value from temp pix to img pix
            do {
                if (tmplocation == 0) {
                    imgPix[pixlocation++] = tmpPixels[tmplocation];
                    break;
                }
                imgPix[pixlocation++] = tmpPixels[--tmplocation];
            } while (tmplocation >= 0);
        }
        
    }
    
    delete[] imgPix;
    imgPix = nullptr;
    preIndexes.clear ();
    mainIndexes.clear ();
}
