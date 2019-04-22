#include "KgFrame.h"

KgFrame::KgFrame ()
{
    std::cout << "Kframe is created " << std::endl;
}

KgFrame::~KgFrame ()
{
    std::cout << "Frame destructor is called " << std::endl;
    std::cout << " -----------||||" << std::endl;
    Cleanup ();
}

KgFrame::KgFrame (const KgFrame& src)
{
    //copy
    //clear the local color table 1st if exist
    if (localColFlag) {
        delete[] localColTab;
        localColTab = nullptr;
    }

    disposal = src.disposal;
    transColFlag = src.transColFlag;
    localColFlag = src.localColFlag;
    interlaceFlag = src.interlaceFlag;
    sortedColFlag = src.sortedColFlag;
    delayTime = src.delayTime;
    imgLeft = src.imgLeft;
    imgTop = src.imgTop;
    imgWidth = src.imgWidth;
    imgHeight = src.imgHeight;
    transColIndex = src.transColIndex;
    bpc = src.bpc;
    localColTab = src.localColTab; //pointing to the table of sources
}

KgFrame& KgFrame::operator=(const KgFrame& rhs)
{
    // TODO: insert return statement here
    if (this == &rhs) {
        return *this;
    }
    Cleanup ();
    disposal = rhs.disposal;
    transColFlag = rhs.transColFlag;
    localColFlag = rhs.localColFlag;
    interlaceFlag = rhs.interlaceFlag;
    sortedColFlag = rhs.sortedColFlag;
    delayTime = rhs.delayTime;
    imgLeft = rhs.imgLeft;
    imgTop = rhs.imgTop;
    imgWidth = rhs.imgWidth;
    imgHeight = rhs.imgHeight;
    transColIndex = rhs.transColIndex;
    bpc = rhs.bpc;
    localColTab = rhs.localColTab;
    return *this;
}

KgFrame& KgFrame::operator=(KgFrame&& rhs) noexcept
{
    if (this == &rhs) {   //rhs is a temp obj that content value , so it is lvalue
        return *this;
    }
    Cleanup ();
    MoveFrom (rhs);
    return *this;
}

KgFrame::KgFrame (KgFrame&& src) noexcept
{
    MoveFrom (src);
}

void KgFrame::Cleanup () noexcept
{
    if (localColFlag) {
       delete[] localColTab;
        localColTab = nullptr;
    }
    disposal = 0;
    transColFlag = localColFlag = interlaceFlag = sortedColFlag = false;
    delayTime = imgLeft = imgTop = imgWidth = imgHeight = 0;
    transColIndex = bpc = 0;
}

void KgFrame::MoveFrom (KgFrame& src) noexcept
{
    //perform data copy
    disposal = src.disposal;
    transColFlag = src.transColFlag;
    localColFlag = src.localColFlag;
    interlaceFlag = src.interlaceFlag;
    sortedColFlag = src.sortedColFlag;
    delayTime = src.delayTime;
    imgLeft = src.imgLeft;
    imgTop = src.imgTop;
    imgWidth = src.imgWidth;
    imgHeight = src.imgHeight;
    transColIndex = src.transColIndex ;
    bpc = src.bpc;
    localColTab = src.localColTab;

    //reset data of source
    src.disposal = 0;
    src.transColFlag = src.localColFlag = src.interlaceFlag = src.sortedColFlag = false;
    src.delayTime = src.imgLeft = src.imgTop = src.imgWidth = src.imgHeight = 0;
    src.transColIndex = src.bpc = 0;
    localColTab = nullptr;
}
