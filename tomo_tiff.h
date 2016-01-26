#ifndef TOMO_TIFF
#define TOMO_TIFF

#include <tiffio.h>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstdio>
#include <cstdlib>
#include <math.h>

#include <omp.h>

using namespace std;

class tomo_tiff{

    string address_;
    unsigned int height_;
    unsigned int width_;
    uint8_t bits_per_sample_;
    int samples_per_pixel_;

    /*int color_map_;
    int compression_;
    int photometric_;
    int xresolution_;
    int yresolution_;
    int resolutionunit_;
    int planarconfig_;
    int orientation_;*/

    vector< vector<float> > gray_scale_;

public:

    tomo_tiff(){
        this->height_ = -1;
        this->width_ = -1;
        this->bits_per_sample_ = 0;
        this->samples_per_pixel_ = -1;
        this->gray_scale_.clear();
    }

    tomo_tiff(const char* address);

    void save(const char* address);
    const vector<float>& operator [](int index_i)const;
};

class tomo_super_tiff{

    string prefix_;
    vector<tomo_tiff> tiffs_;
    vector< vector< vector<float> > > gaussian_window;
    void make_gaussian_window_(const int size, const float standard_deviation);

public:

    tomo_super_tiff(const char* address_filelist);
    void neuron_detection(int window_size);
};

#endif // TOMO_TIFF

