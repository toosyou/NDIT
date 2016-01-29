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

    this->height_ = this->gray_scale_.size();
    this->width_ = this->gray_scale_[0].size();

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

vector<float>& tomo_tiff::operator [](int index_y){
    return this->gray_scale_[index_y];
}

void tomo_super_tiff::make_gaussian_window_(const int size, const float standard_deviation){

    //init
    this->gaussian_window_.resize(size);
    for(int i=0;i<size;++i){
        this->gaussian_window_[i].resize(size);
        for(int j=0;j<size;++j){
            this->gaussian_window_[i][j].resize(size,0.0);
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

                float fi = (float)i - (float)(size-1) / 2.0;
                float fj = (float)j - (float)(size-1) / 2.0;
                float fk = (float)k - (float)(size-1) / 2.0;

                float exp_part = -( fi*fi + fj*fj + fk*fk ) / (standard_deviation*standard_deviation);
                gaussian_window_[i][j][k] = N * exp(exp_part);
                //maximum = max( maximum, gaussian_window[i][j][k] );
                summation += gaussian_window_[i][j][k];
            }
        }
    }

    //normalize the summation of gaussian_window to 1.0
    float ratio = 1.0/summation;
    for(int i=0;i<size;++i){
        for(int j=0;j<size;++j){
            for(int k=0;k<size;++k){
                gaussian_window_[i][j][k] *= ratio;
                maximum = max( maximum, gaussian_window_[i][j][k] );
            }
        }
    }

    //normalize the maximum to 1 for output
    float normalize_ratio = 1.0 / maximum;
    vector< vector< vector<float> > >output_test( gaussian_window_ );
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

float tomo_super_tiff::summation_within_window_gaussianed_(int x, int y, int z, int size){

    float summation = 0.0;

    for(int i=0;i<size;++i){ // x
        for(int j=0;j<size;++j){ // y
            for(int k=0;k<size;++k){ // z

                int sx = x+i;
                int sy = y+j;
                int sz = z+k;

                sz = sz < 0 ? 0 : sz;
                sy = sy < 0 ? 0 : sy;
                sx = sx < 0 ? 0 : sx;

                sz = sz >= this->tiffs_.size() ? this->tiffs_.size()-1 : sz;
                sy = sy >= this->tiffs_[sz].size() ? this->tiffs_[sz].size()-1 : sy;
                sx = sx >= this->tiffs_[sz][sy].size() ? this->tiffs_[sz][sy].size()-1 : sx;

                summation += this->tiffs_[sz][sy][sx] * this->gaussian_window_[k][j][i];
            }
        }
    }

    return summation;
}

void tomo_super_tiff::down_size(int magnification, const char *save_prefix, float sample_sd){

    vector< tomo_tiff > result;
    int process = 0;

    //init
    cout << "allocting result of down_size..." <<endl;
    result.resize( this->tiffs_.size()/magnification );
#pragma omp parallel for
    for(int i=0;i<result.size();++i){
        result[i].resize( this->tiffs_[i*magnification].size()/magnification );
        for(int j=0;j<result[i].size();++j){
            result[i][j].resize( this->tiffs_[i*magnification][j*magnification].size()/magnification, 0.0 );
        }
        cout << process << " / " << result.size() <<endl;
#pragma omp critical
        {
            process++;
        }
    }
    for(int i=0;i<result.size();++i){
        result[i].bits_per_sample_ = 16;
        result[i].samples_per_pixel_ = 1;
    }
    cout << "\t\tdone!" <<endl;

    //gaussian & sampling
    cout << "gaussian and sampling..." <<endl;

    this->make_gaussian_window_(magnification, sample_sd*(float)magnification/2.0);

    process = 0;
#pragma omp parallel for
    for(int z=0;z<result.size();++z){
        for(int y=0;y<result[z].size();++y){
            for(int x=0;x<result[z][y].size();++x){

                int sx = x*magnification;
                int sy = y*magnification;
                int sz = z*magnification;

                result[z][y][x] = this->summation_within_window_gaussianed_( sx-magnification/2,
                                                                             sy-magnification/2,
                                                                             sz-magnification/2,
                                                                             magnification);
            }
        }
        cout << process << " / " << result.size() <<endl;
#pragma omp critical
        {
            process++;
        }
    }
    cout << "\t\tdone!" <<endl;

    //save it to save_prefix
    cout << "saving files..." <<endl;
    mkdir(save_prefix, 0755);

    process = 0;
#pragma omp parallel for
    for(int i=0;i<result.size();++i){
        //make address
        char number_string[50]={0};
        string address(save_prefix);
        sprintf(number_string,"%d",i);
        address += string(number_string) + string(".tiff");
        //save
        result[i].save( address.c_str() );
        cout << address << " saved\t\t" << process << " / " << result.size() <<endl;
#pragma omp critical
        {
            process++;
        }
    }
    cout << "\t\tdone!" <<endl;

    return;
}

void tomo_super_tiff::make_differential_matrix_(){

    //init
    cout << endl << "Initializing, trying to allocate memory..." <<endl ;
    int process = 0;
    differential_matrix_.resize(this->tiffs_.size());
#pragma omp parallel for
    for(int i=0;i<differential_matrix_.size();++i){
        this->differential_matrix_[i].resize(this->tiffs_[i].size());
        for(int j=0;j<differential_matrix_[i].size();++j){
            differential_matrix_[i][j].resize(this->tiffs_[i][j].size(),matrix(3,0));
        }
        cout << process << " / " << differential_matrix_.size() <<endl;
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

                matrix &this_matrix = differential_matrix_[z][y][x];
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

void tomo_super_tiff::make_tensor_(const int window_size){

    //init
    cout << endl << "Initializing, trying to allocate memory..." <<endl ;
    int process = 0;
    this->tensor_.resize(this->differential_matrix_.size());
#pragma omp parallel for
    for(int i=0;i<this->tensor_.size();++i){
        this->tensor_[i].resize(this->differential_matrix_[i].size());
        for(int j=0;j<this->tensor_[i].size();++j){
            this->tensor_[i][j].resize(this->differential_matrix_[i][j].size());
        }
        cout << process << " / " << this->tensor_.size() <<endl;
#pragma omp critical
        {
            process++;
        }
    }
    cout << "done!" <<endl;

    // struct tensor A = sum_u_v_w( gaussian(u,v,w) * differential(u,v,w) )

    //for every points
    cout << "calculating tensor matrixes..." <<endl;
    process = 0;
#pragma omp parallel for
    for(int z=0;z<this->tensor_.size();++z){
        for(int y=0;y<this->tensor_[z].size();++y){
            for(int x=0;x<this->tensor_[z][y].size();++x){

                matrix temp(3,0);
                //inside the window
                for(int k=z-window_size/2;k<z+window_size/2;++k){
                    for(int j=y-window_size/2;j<y+window_size/2;++j){
                        for(int i=x-window_size/2;i<x+window_size/2;++i){
                            //check boundary
                            if( k < 0 || k >= this->tensor_.size() ||
                                    j < 0 || j >= this->tensor_[k].size() ||
                                    i < 0 || i >= this->tensor_[k][j].size())
                                continue;
                            //sum it up with gaussian ratio
                            int k_g = k - (z-window_size/2);
                            int j_g = j - (y-window_size/2);
                            int i_g = i - (x-window_size/2);
                            temp += differential_matrix_[k][j][i] * gaussian_window_[k_g][j_g][i_g];
                        }
                    }
                }
                this->tensor_[z][y][x] = temp;
            }
        }
        cout << process << " / " << this->tensor_.size() <<endl;
#pragma omp critical
        {
            process++;
        }
    }
    cout << "done!" <<endl;
    return;
}

void tomo_super_tiff::neuron_detection(const int window_size, const float standard_deviation){

    cout << "making gaussian window with window_size : " << window_size;
    (cout << "\tstandard_deviation : " << standard_deviation ).flush();

    this->make_gaussian_window_(window_size,standard_deviation*(float)window_size/2.0);
    cout << "\tdone!"<<endl;

    (cout << "making differential matrix..." ).flush();
    this->make_differential_matrix_();
    cout << "done!"<<endl;

    cout << "cleaning original data...";
    cout.flush();
    this->tiffs_.clear();
    cout << "\t\tdone!";

    cout << "making struct tensor...";
    cout.flush();
    this->make_tensor_(window_size);
    cout << "\t\tdone!" <<endl;

    return;
}

