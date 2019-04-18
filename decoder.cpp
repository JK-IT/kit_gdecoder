// decoder.cpp : Defines the entry point for the application.
//


#include "decoder.h"
#include "KgDecoder.h"

using namespace std;

int main()
{
	cout << "Hello CMake." << endl;
    KgDecoder kgifde;
    FILE* giffile;
    //this is how you open file in vs c++
    giffile = fopen ("C:\\Users\\Kit\\Desktop\\Kit Gif Decoder\\decoder\\tenor.gif", "rb");
    if (giffile == nullptr) {
        std::cout << " file cannot open " << std::endl;
    }
    else {
        fseek (giffile, 0, SEEK_END);
        uint32_t filesi = (uint32_t) ftell (giffile); //in byte unit
        rewind (giffile);
        std::cout << filesi << std::endl;
        uint8_t* datbuff = new uint8_t[filesi];
        fread (datbuff, filesi, 1, giffile);
        kgifde.SetData (datbuff, filesi);

        //clear data and close file
        delete[] datbuff;
        datbuff = nullptr;
        fclose (giffile);
    }
    

	return 0;
}
