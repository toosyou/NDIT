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
#include <cmath>

#include <omp.h>

using namespace std;

class tomo_super_tiff;

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
    tomo_tiff(vector< vector<float> >& data){
        this->gray_scale_ = data;
        this->height_ = data.size();
        this->width_ = data[0].size();
        this->bits_per_sample_ = 16;
        this->samples_per_pixel_ = 1;
    }

    tomo_tiff(const char* address);

    void save(const char* address);
    const vector<float>& operator [](int index_y)const;
    int size(void){return this->gray_scale_.size();}

    friend class tomo_super_tiff;
};

class matrix{

    vector< vector<float> > number_;

public:

    matrix(const int size = 0, const float number = 0.0){
        this->resize(size,number);
    }

    vector<float>& operator [](const int index_i){
        return this->number_[index_i];
    }
    void resize(const int size,const float number = 0.0){
        this->number_.resize(size);
        for(int i=0;i<size;++i){
            this->number_[i].resize(size,number);
        }
    }
    int size(void){
        return this->number_.size();
    }

    matrix operator *(int ratio){
        matrix rtn = *this;
        for(int i=0;i<rtn.size();++i){
            for(int j=0;j<rtn.size();++j){
                rtn[i][j] = this->number_[i][j] * ratio;
            }
        }
        return rtn;
    }
    void operator +=(matrix b){
        if(this->number_.size() != b.size()){
            cerr << "matrixes' size don't match" <<endl;
            return;
        }
        for(int i=0;i<this->number_.size();++i){
            for(int j=0;j<this->number_.size();++j){
                this->number_[i][j] += b[i][j];
            }
        }
    }
};

class tomo_super_tiff{

    string prefix_;
    vector<tomo_tiff> tiffs_;//[z][y][x]
    vector< vector< vector<float> > > gaussian_window_;
    vector< vector< vector<matrix> > >differential_matrix_;
    vector< vector< vector<matrix> > >tensor_;

    void make_gaussian_window_(const int size, const float standard_deviation);
    void make_differential_matrix_();
    void make_tensor_(const int window_size);
    float Ix_(int x, int y, int z);
    float Iy_(int x, int y, int z);
    float Iz_(int x, int y, int z);

public:

    tomo_super_tiff(const char* address_filelist);
    void neuron_detection(const int window_size, const float standard_deviation=0.8);
};

#endif // TOMO_TIFF

