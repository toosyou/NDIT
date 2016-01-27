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
    for(unsigned int i=0;i<this->height_;++i){
        this->gray_scale_[i].resize(this->width_,0.0);
    }

    //read raw-data
    unsigned int line_size = TIFFScanlineSize(tif);
    char* buf = new char[ line_size * this->height_];
    for(unsigned int i=0;i<this->height_;++i){
        TIFFReadScanline( tif, &buf[i*line_size], i );
    }

    if(this->bits_per_sample_ == 16 && this->samples_per_pixel_ == 1){
        for(unsigned int i=0;i<this->height_;++i){
            for(unsigned int j=0;j<this->width_;++j){
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
        for(unsigned int i=0;i<this->height_;++i){
            for(unsigned int j=0;j<this->width_;++j){
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

const vector<float>& tomo_tiff::operator [](int index_y)const{
    return this->gray_scale_[index_y];
}

void tomo_super_tiff::make_gaussian_window_(const int size, const float standard_deviation){

    //init
    this->gaussian_window.resize(size);
    for(int i=0;i<size;++i){
        this->gaussian_window[i].resize(size);
        for(int j=0;j<size;++j){
            this->gaussian_window[i][j].resize(size,0.0);
        }
    }

    //sd : standard_deviation
    //g(x,y,z) = N * exp[ -(x^2 + y^2 + z^2)/sd^2 ];
    //where N = 1 / ( sd^3 * (2pi)^(3/2) )

    float maximum = 0.0;
    float summation = 0.0;

    //make N first
    float N = 1.0 / ( pow(standard_deviation,3) * pow( (2.0 * M_PI), 1.5 ) );

    //do g(x,y,z)
    for(int i=0;i<size;++i){
        for(int j=0;j<size;++j){
            for(int k=0;k<size;++k){

                float fi = (float)i - (float)size / 2.0;
                float fj = (float)j - (float)size / 2.0;
                float fk = (float)k - (float)size / 2.0;

                float exp_part = -( fi*fi + fj*fj + fk*fk ) / (standard_deviation*standard_deviation);
                gaussian_window[i][j][k] = N * exp(exp_part);
                //maximum = max( maximum, gaussian_window[i][j][k] );
                summation += gaussian_window[i][j][k];
            }
        }
    }

    //normalize the summation of gaussian_window to 1.0
    float ratio = 1.0/summation;
    for(int i=0;i<size;++i){
        for(int j=0;j<size;++j){
            for(int k=0;k<size;++k){
                gaussian_window[i][j][k] *= ratio;
                maximum = max( maximum, gaussian_window[i][j][k] );
            }
        }
    }

    //normalize the maximum to 1 for output
    float normalize_ratio = 1.0 / maximum;
    vector< vector< vector<float> > >output_test( gaussian_window );
    for(int i=0;i<size;++i){
        for(int j=0;j<size;++j){
            for(int k=0;k<size;++k){
                output_test[i][j][k] *= normalize_ratio;
            }
        }
    }
    mkdir("gaussian",0755);
    string prefix_test("gaussian/");
    for(int i=0;i<size;++i){
        tomo_tiff test_tiff( output_test[i] );
        char number_string[50];
        sprintf(number_string,"%d",i);
        string address_test = prefix_test+string(number_string)+string(".tiff");
        test_tiff.save( address_test.c_str() );
    }

    return;
}


tomo_super_tiff::tomo_super_tiff(const char *address_filelist){

    fstream in_filelist(address_filelist,fstream::in);

    int size_tiffs = -1;
    vector<string> addresses;
    char prefix[100]={0};
    char original_dir[100]={0};
    getcwd(original_dir,100);

    in_filelist >> size_tiffs;
    in_filelist >> prefix;
    //read filelist first for parallel
    addresses.resize(size_tiffs);
    for(int i=0;i<size_tiffs;++i){
        in_filelist >> addresses[i];
    }

    cout << "change working directory to " << prefix <<endl;
    this->prefix_ = string(prefix);
    chdir(prefix);

    int number_done = 0;
    this->tiffs_.resize(size_tiffs);
#pragma omp parallel for
    for(int i=0;i<size_tiffs;++i){
        tiffs_[i] = tomo_tiff(addresses[i].c_str());
        cout << "reading " << addresses[i] << "\t\t\tdone!\t\t" << number_done <<endl;
    #pragma omp critical
        {
            number_done++;
        }
    }

    cout << "change working directory back to " << original_dir <<endl;
    chdir(original_dir);
    return;
}

float tomo_super_tiff::Ix_(int x, int y, int z){
    if( x+1 >= this->tiffs_[z][y].size() )
        return this->tiffs_[z][y][x] - this->tiffs_[z][y][x-1];
    else if( x-1 < 0)
        return this->tiffs_[z][y][x+1] - this->tiffs_[z][y][x];
    else
        return ( this->tiffs_[z][y][x+1] - this->tiffs_[z][y][x-1] ) / 2.0;
}

float tomo_super_tiff::Iy_(int x, int y, int z){
    if( y+1 >= this->tiffs_[z].size() )
        return this->tiffs_[z][y][x] - this->tiffs_[z][y-1][x];
    else if( y-1 < 0)
        return this->tiffs_[z][y+1][x] - this->tiffs_[z][y][x];
    else
        return ( this->tiffs_[z][y+1][x] - this->tiffs_[z][y-1][x] ) / 2.0;
}

float tomo_super_tiff::Iz_(int x, int y, int z){
    if( z+1 >= this->tiffs_.size() )
        return this->tiffs_[z][y][x] - this->tiffs_[z-1][y][x];
    else if( z-1 < 0)
        return this->tiffs_[z+1][y][x] - this->tiffs_[z][y][x];
    else
        return ( this->tiffs_[z+1][y][x] - this->tiffs_[z-1][y][x] ) / 2.0;
}

void tomo_super_tiff::make_differential_matrix(){

    //init
    cout << endl << "Initializing, trying to allocate memory..." <<endl ;
    int process = 0;
    differential_matrix.resize(this->tiffs_.size());
#pragma omp parallel for
    for(int i=0;i<differential_matrix.size();++i){
        this->differential_matrix[i].resize(this->tiffs_[i].size());
        for(int j=0;j<differential_matrix[i].size();++j){
            differential_matrix[i][j].resize(this->tiffs_[i][j].size(),matrix(3,0));
        }
        cout << process << " / " << differential_matrix.size() <<endl;
#pragma omp critical
        {
            process++;
        }
    }
    cout << "done!" <<endl;

    /* differential_matrix      j->
     * __                           __
     * |    IxIx    IxIy    IxIz     |  i
     * |                             |  |
     * |    IxIy    IyIy    IyIz     |  v
     * |                             |
     * |    IxIz    IyIz    IzIz     |
     * L_                           _|
     */

    cout << "calculating differential matrixes..." <<endl;
    process = 0;
#pragma omp parallel for
    for(int z=0;z<this->tiffs_.size();++z){
        for(int y=0;y<this->tiffs_[z].size();++y){
            for(int x=0;x<this->tiffs_[z][y].size();++x){

                matrix &this_matrix = differential_matrix[z][y][x];
                float Ix = this->Ix_(x,y,z);
                float Iy = this->Iy_(x,y,z);
                float Iz = this->Iz_(x,y,z);

                this_matrix[0][0] = Ix*Ix;
                this_matrix[1][1] = Iy*Iy;
                this_matrix[2][2] = Iz*Iz;

                this_matrix[0][1] = this_matrix[1][0] = Ix*Iy;
                this_matrix[0][2] = this_matrix[2][0] = Ix*Iz;
                this_matrix[1][2] = this_matrix[2][1] = Iy*Iz;
            }
        }
        cout << process << " / " << this->tiffs_.size() <<endl;
#pragma omp critical
        {
            process++;
        }
    }
}

void tomo_super_tiff::neuron_detection(const int window_size, const float standard_deviation){

    cout << "making gaussian window with window_size : " << window_size;
    (cout << "\tstandard_deviation : " << standard_deviation ).flush();

    this->make_gaussian_window_(window_size,standard_deviation);
    cout << "\tdone!"<<endl;

    (cout << "making differential matrix..." ).flush();
    this->make_differential_matrix();
    cout << "done!"<<endl;

    return;
}
