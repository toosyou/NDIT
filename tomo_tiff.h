#ifndef TOMO_TIFF
#define TOMO_TIFF

#include <tiffio.h>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

class tomo_tiff{

    unsigned int height_;
    unsigned int width_;
    uint8_t bits_per_sample_;
    int samples_per_pixel_;
    int color_map_;
    int compression_;
    int photometric_;
    int xresolution_;
    int yresolution_;
    int resolutionunit_;
    int planarconfig_;
    int orientation_;

    vector< vector<float> > gray_scale_;

public:

    tomo_tiff(const char* address);

    void save(const char* address);
};

#endif // TIFF

