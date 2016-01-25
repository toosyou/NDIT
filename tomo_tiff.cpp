#include "tomo_tiff.h"

tomo_tiff::tomo_tiff(const char* address){
    TIFF *tif = TIFFOpen( address, "r" );
    if(tif == NULL){
        cerr << "ERROR : cannot open " << address << endl;
        return;
    }

    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &this->height_);
    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH,  &this->width_);
    TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &this->bits_per_sample_);
    TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &this->samples_per_pixel_);

    //init
    this->address_ = string(address);
    this->gray_scale_.resize(this->height_);
    for(int i=0;i<this->height_;++i){
        this->gray_scale_[i].resize(this->width_,0.0);
    }

    //read raw-data
    unsigned int line_size = TIFFScanlineSize(tif);
    char* buf = new char[ line_size * this->height_];
    for(int i=0;i<this->height_;++i){
        TIFFReadScanline( tif, &buf[i*line_size], i );
    }

    if(this->bits_per_sample_ == 16 && this->samples_per_pixel_ == 1){
        for(int i=0;i<this->height_;++i){
            for(int j=0;j<this->width_;++j){
                this->gray_scale_[i][j] = (float)((uint16_t*)buf)[ i*this->width_ + j ] / 65535.0;
            }
        }
    }
    else{
        cerr << "ERROR : " << address << " not handled!" <<endl;
        cerr << "bits_per_sample : " << this->bits_per_sample_ << " ";
        cerr << "samples_per_pixel : " << this->samples_per_pixel_ <<endl;
    }

    delete [] buf;
    TIFFClose(tif);

    return;
}

void tomo_tiff::save(const char* address){
    TIFF *tif = TIFFOpen(address, "w");
    if(tif == NULL){
        cerr << "ERROR : cannot create file " << address <<endl;
        return;
    }

    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, this->width_);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, this->height_);

    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, this->bits_per_sample_);
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, this->samples_per_pixel_);

    TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);

    TIFFSetField(tif, TIFFTAG_XRESOLUTION, 0);
    TIFFSetField(tif, TIFFTAG_YRESOLUTION, 0);
    TIFFSetField(tif, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);

    TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);

    if(this->bits_per_sample_ == 16 && this->samples_per_pixel_ == 1){
        vector<uint16_t> data;
        for(int i=0;i<this->height_;++i){
            for(int j=0;j<this->width_;++j){
                data.push_back( gray_scale_[i][j] * 65535.0 );
            }
        }
        TIFFWriteEncodedStrip(tif, 0, &data[0], this->height_*this->width_*2);
    }
    else{
        cerr << "ERROR : " << address << " not handled!" <<endl;
        cerr << "bits_per_sample : " << this->bits_per_sample_ << " ";
        cerr << "samples_per_pixel : " << this->samples_per_pixel_ <<endl;
    }

    TIFFClose(tif);
    return;
}

tomo_super_tiff::tomo_super_tiff(const char *address_filelist){

    fstream in_filelist(address_filelist,fstream::in);

    int size_tiffs = -1;
    char prefix[50]={0};
    char original_dir[50]={0};
    getcwd(original_dir,50);

    in_filelist >> size_tiffs;
    in_filelist >> prefix;
    cout << "change working directory to " << prefix <<endl;
    this->prefix_ = string(prefix);
    chdir(prefix);

    this->tiffs_.resize(size_tiffs);
    for(int i=0;i<size_tiffs;++i){
        char address_buffer[100];
        in_filelist >> address_buffer;
        (cout << "reading " << address_buffer ).flush();
        tiffs_[i] = tomo_tiff(address_buffer);
        cout << "\t\t\tdone!" <<endl;
    }

    cout << "change working directory back to " << original_dir <<endl;
    chdir(original_dir);
    return;
}
