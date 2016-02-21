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

    progressbar *progress = progressbar_new("Reading .tifs",size_tiffs);
    this->tiffs_.resize(size_tiffs);
#pragma omp parallel for
    for(int i=0;i<size_tiffs;++i){
        tiffs_[i] = tomo_tiff(addresses[i].c_str());
    #pragma omp critical
        {
            progressbar_inc(progress);
        }
    }
    progressbar_finish(progress);

    cout << "change working directory back to " << original_dir <<endl;
    chdir(original_dir);
    this->tiffs_[0].save("favicon.tif");
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
    progressbar *progress = progressbar_new("Initializing",this->tiffs_.size());
    differential_matrix_.resize(this->tiffs_.size());
#pragma omp parallel for
    for(int i=0;i<differential_matrix_.size();++i){
        this->differential_matrix_[i].resize(this->tiffs_[i].size());
        for(int j=0;j<differential_matrix_[i].size();++j){
            differential_matrix_[i][j].resize(this->tiffs_[i][j].size(),matrix(3,0));
        }
#pragma omp critical
        progressbar_inc(progress);
    }
    progressbar_finish(progress);

    /* differential_matrix      j->
     * __                           __
     * |    IxIx    IxIy    IxIz     |  i
     * |                             |  |
     * |    IxIy    IyIy    IyIz     |  v
     * |                             |
     * |    IxIz    IyIz    IzIz     |
     * L_                           _|
     */

    progress = progressbar_new("Calculating",this->tiffs_.size());
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
#pragma omp critical
        progressbar_inc(progress);
    }
    progressbar_finish(progress);

    return;
}

void tomo_super_tiff::make_tensor_(const int window_size){

    //init
    this->tensor_.resize(this->differential_matrix_.size());
    progressbar *progress = progressbar_new("Initializing",this->differential_matrix_.size());
#pragma omp parallel for
    for(int i=0;i<this->tensor_.size();++i){
        this->tensor_[i].resize(this->differential_matrix_[i].size());
        for(int j=0;j<this->tensor_[i].size();++j){
            this->tensor_[i][j].resize(this->differential_matrix_[i][j].size());
        }
#pragma omp critical
        progressbar_inc(progress);
    }
    progressbar_finish(progress);

    // struct tensor A = sum_u_v_w( gaussian(u,v,w) * differential(u,v,w) )

    //for every points
    progress = progressbar_new("Calculating",this->tensor_.size());
#pragma omp parallel for
    for(int z=0;z<this->tensor_.size();++z){
        for(int y=0;y<this->tensor_[z].size();++y){
            for(int x=0;x<this->tensor_[z][y].size();++x){

                matrix temp(3,0);
                //inside the window
                for(int k=z-window_size/2;k<z+(window_size+1)/2;++k){
                    for(int j=y-window_size/2;j<y+(window_size+1)/2;++j){
                        for(int i=x-window_size/2;i<x+(window_size+1)/2;++i){
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
#pragma omp critical
        progressbar_inc(progress);
    }
    progressbar_finish(progress);

    return;
}

void tomo_super_tiff::make_nobles_measure_(float measure_constant){

    cout << "making nobles measures..."<<endl;

    //init

    this->measure_.resize( this->tensor_.size() );
    progressbar *progress = progressbar_new("Initializing", this->tensor_.size());
#pragma omp parallel for
    for(int i=0;i<measure_.size();++i){
        this->measure_[i].resize(this->tensor_[i].size());
        for(int j=0;j<measure_[i].size();++j){
            this->measure_[i][j].resize(this->tensor_[i][j].size(),0.0);
        }
#pragma omp critical
        progressbar_inc(progress);
    }
    progressbar_finish(progress);

    //calculate measure
    // Noble's cornor measure :
    //      Mc = 2* det(tensor) / ( trace(tensor) + c )

    progress = progressbar_new("Calculating",this->measure_.size());
#pragma omp parallel for
    for(int i=0;i<measure_.size();++i){
        for(int j=0;j<measure_[i].size();++j){
            for(int k=0;k<measure_[i][j].size();++k){
                float trace = this->tensor_[i][j][k].trace();
                this->measure_[i][j][k] = 2 * this->tensor_[i][j][k].det();
                this->measure_[i][j][k] /= trace*trace + measure_constant;
            }
        }
#pragma omp critical
        progressbar_inc(progress);
    }
    progressbar_finish(progress);

    return;
}

void tomo_super_tiff::make_eigen_values_(){

    cout << "making eigen values..." <<endl;

    //init
    this->eigen_values_.resize(this->tensor_.size());

    progressbar *progress = progressbar_new("Initializing",this->eigen_values_.size());
#pragma omp parallel for
    for(int i=0;i<this->eigen_values_.size();++i){
        this->eigen_values_[i].resize( this->tensor_[i].size() );
        for(int j=0;j<this->eigen_values_[i].size();++j){
            this->eigen_values_[i][j].resize( this->tensor_[i][j].size() );
            for(int k=0;k<this->eigen_values_[i][j].size();++k){
                this->eigen_values_[i][j][k].resize(3,0.0);
            }
        }
#pragma omp critical
        progressbar_inc(progress);
    }
    progressbar_finish(progress);

    //using gsl for eigenvalue
    progress = progressbar_new("Calculating",this->tensor_.size());

#pragma omp parallel for
    for(int i=0;i<this->tensor_.size();++i){
        for(int j=0;j<this->tensor_[i].size();++j){
            for(int k=0;k<this->tensor_[i][j].size();++k){

                matrix &this_matrix = tensor_[i][j][k];

                //allocate needed
                gsl_matrix *tensor_matrix = gsl_matrix_alloc(3,3);
                gsl_vector *eigen_value = gsl_vector_alloc(3);
                gsl_matrix *eigen_vector = gsl_matrix_alloc(3,3);
                gsl_eigen_symmv_workspace *w = gsl_eigen_symmv_alloc(3);

                //convert tensor_[i][j][k] to gsl_matrix
                for(int x=0;x<3;++x){
                    for(int y=0;y<3;++y){
                        gsl_matrix_set(tensor_matrix,x,y,this_matrix[x][y]);
                    }
                }

                //do the eigenvalue thing
                gsl_eigen_symmv(tensor_matrix,eigen_value,eigen_vector,w);

                gsl_eigen_symmv_sort(eigen_value,eigen_vector,GSL_EIGEN_SORT_ABS_ASC);

                //save absolute of it to eigen_values_
                for(int x=0;x<3;++x){
                    float ev = gsl_vector_get(eigen_value,x);
                    this->eigen_values_[i][j][k][x] = ev > 0.0 ? ev : -ev;
                }

                //free everything
                gsl_matrix_free(tensor_matrix);
                gsl_matrix_free(eigen_vector);
                gsl_vector_free(eigen_value);
                gsl_eigen_symmv_free(w);
            }
        }
#pragma omp critical
        progressbar_inc(progress);
    }
    progressbar_finish(progress);

    return ;
}

void tomo_super_tiff::experimental_measurement(float threshold){

    cout << "making measurement..." <<endl;

    //resize & init
    this->measure_.resize(this->eigen_values_.size());
    progressbar *progress = progressbar_new("Initializing", this->measure_.size());
#pragma omp parallel for
    for(int i=0;i<this->measure_.size();++i){
        this->measure_[i].resize(this->eigen_values_[i].size());
        for(int j=0;j<this->measure_[i].size();++j){
            this->measure_[i][j].resize(this->eigen_values_[i][j].size(),0.0);
        }
#pragma omp critical
        progressbar_inc(progress);
    }
    progressbar_finish(progress);

    //measurement
    progress = progressbar_new("Calculating",this->measure_.size());
#pragma omp parallel for
    for(int i=0;i<this->measure_.size();++i){
        for(int j=0;j<this->measure_[i].size();++j){
            for(int k=0;k<this->measure_[i][j].size();++k){
                vector<float> &ev = this->eigen_values_[i][j][k];
                measure_[i][j][k] = 0.06 * ( ev[0] + ev[1] + ev[2]) * ( ev[0] + ev[1] + ev[2]) - ev[0] * ev[1] * ev[2];
            }
        }
#pragma omp critical
        progressbar_inc(progress);
    }
    progressbar_finish(progress);

    //normalize
    float maximum = 0.0;
    for(int i=0;i<this->measure_.size();++i){
        for(int j=0;j<this->measure_[i].size();++j){
            for(int k=0;k<this->measure_[i][j].size();++k){
                maximum = measure_[i][j][k] > maximum ? measure_[i][j][k] : maximum;
            }
        }
    }
    cout << "normalized by " << maximum <<endl;

#pragma omp parallel for
    for(int i=0;i<this->measure_.size();++i){
        for(int j=0;j<this->measure_[i].size();++j){
            for(int k=0;k<this->measure_[i][j].size();++k){
                measure_[i][j][k] /= maximum;
            }
        }
    }

    return ;

}

void tomo_super_tiff::neuron_detection(const int window_size, const float standard_deviation){

    cout << "making gaussian window with window_size : " << window_size;
    (cout << "\tstandard_deviation : " << standard_deviation ).flush();

    this->make_gaussian_window_(window_size,standard_deviation*(float)window_size/2.0);
    cout << "\tdone!"<<endl;

    cout << "making differential matrix..." <<endl;
    this->make_differential_matrix_();

    cout << "making struct tensor..." <<endl;
    this->make_tensor_(window_size);

    this->make_eigen_values_();

    this->experimental_measurement(0.00005 );

    return;
}

void tomo_super_tiff::save_measure(const char *prefix){

    char original_directory[100];
    getcwd(original_directory,100);
    mkdir(prefix, 0755);
    chdir(prefix);
    cout << "changing working directory to " << prefix <<endl;

    //normalize, merge & save
    progressbar *progress = progressbar_new("Saving",this->measure_.size());
#pragma omp parallel for
    for(int i=0;i<this->measure_.size();++i){
        //init, normalize & merge
        vector< vector<float> > output_image(this->measure_[i].size());
        for(int j=0;j<output_image.size();++j){
            output_image[j].resize(this->measure_[i][j].size());
            for(int k=0;k<output_image[j].size();++k){
                output_image[j][k] = this->measure_[i][j][k];
            }
        }
        //make address
        char number_string[50]={0};
        sprintf(number_string, "%d", i);
        string address = string(number_string) + string(".tif");
        //save
        tomo_tiff output_tiff(output_image);
        output_tiff.save(address.c_str());

#pragma omp critical
        progressbar_inc(progress);
    }
    progressbar_finish(progress);

    chdir(original_directory);
    cout << "changing working directory back to " << original_directory <<endl;

    return;

}

void tomo_super_tiff::save_measure_merge(const char *prefix){

    char original_directory[100];
    getcwd(original_directory,100);
    mkdir(prefix, 0755);
    chdir(prefix);
    cout << "changing working directory to " << prefix <<endl;

    //normalize, merge & save
    progressbar *progress = progressbar_new("Saving",this->measure_.size());
#pragma omp parallel for
    for(int i=0;i<this->measure_.size();++i){
        //init, normalize & merge
        vector< vector<float> > output_image(this->measure_[i].size());
        for(int j=0;j<output_image.size();++j){
            output_image[j].resize(this->measure_[i][j].size() + this->tiffs_[i][j].size());
            for(int k=0;k<output_image[j].size();++k){
                output_image[j][k] = k < this->tiffs_[i][j].size() ?
                            this->tiffs_[i][j][k] : this->measure_[i][j][k - this->tiffs_[i][j].size()];
            }
        }
        //making address
        char number_string[50]={0};
        sprintf(number_string, "%d", i);
        string address = string(number_string) + string(".tif");

        //save
        tomo_tiff output_tiff(output_image);
        output_tiff.save(address.c_str());

#pragma omp critical
        progressbar_inc(progress);
    }
    progressbar_finish(progress);

    chdir(original_directory);
    cout << "changing working directory back to " << original_directory <<endl;

    return;
}

void tomo_super_tiff::save_eigen_values_rgb(const char *prefix){

    //find maximum of eigen_values_
    float maximum = -1.0;
    for(int i=0;i<this->eigen_values_.size();++i){
        for(int j=0;j<this->eigen_values_[i].size();++j){
            for(int k=0;k<this->eigen_values_[i][j].size();++k){
                for(int m=0;m<this->eigen_values_[i][j][k].size();++m){
                    maximum = maximum > this->eigen_values_[i][j][k][m] ? maximum : this->eigen_values_[i][j][k][m];
                }
            }
        }
    }

    //save them
    char original_dir[100] = {0};
    mkdir(prefix,0755);
    getcwd(original_dir,100);
    chdir(prefix);

#pragma omp parallel for
    for(int i=0;i<this->eigen_values_.size();++i){
        //make file name
        char number_string[50] = {0};
        sprintf(number_string,"%d",i);
        string address = string(number_string) + string(".tiff");

        //open file for saving
        TIFF *tif = TIFFOpen(address.c_str(),"w");
        if(tif == NULL){
            cerr << "ERROR : cannot open to save " << prefix << "/" << address <<endl;
        }

        int height = this->eigen_values_[i].size();
        int width = this->eigen_values_[i][0].size();

        TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width);
        TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height);
        TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 16);
        TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 3);
        TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

        TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_DEFLATE);
        TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);

        vector<uint16_t> tmp_data(width * height * 3);
        int index_tmp = 0;
        for(int j=0;j<height;++j){
            for(int k=0;k<width;++k){
                for(int m=0;m<this->eigen_values_[i][j][k].size();++m){
                    tmp_data[index_tmp++] = (uint16_t)(this->eigen_values_[i][j][k][m] / maximum * 65535.0);
                }
            }
        }

        TIFFWriteEncodedStrip(tif, 0, &tmp_data[0], width * height * 6);

        TIFFClose(tif);
    }
    chdir(original_dir);

    return;
}

void tomo_super_tiff::save_eigen_values_rgb_merge(const char *prefix){
    //find maximum of eigen_values_
    float maximum = -1.0;
    for(int i=0;i<this->eigen_values_.size();++i){
        for(int j=0;j<this->eigen_values_[i].size();++j){
            for(int k=0;k<this->eigen_values_[i][j].size();++k){
                for(int m=0;m<this->eigen_values_[i][j][k].size();++m){
                    maximum = maximum > this->eigen_values_[i][j][k][m] ? maximum : this->eigen_values_[i][j][k][m];
                }
            }
        }
    }

    //save them
    char original_dir[100] = {0};
    mkdir(prefix,0755);
    getcwd(original_dir,100);
    chdir(prefix);

#pragma omp parallel for
    for(int i=0;i<this->eigen_values_.size();++i){
        //make file name
        char number_string[50] = {0};
        sprintf(number_string,"%d",i);
        string address = string(number_string) + string(".tiff");

        //open file for saving
        TIFF *tif = TIFFOpen(address.c_str(),"w");
        if(tif == NULL){
            cerr << "ERROR : cannot open to save " << prefix << "/" << address <<endl;
        }

        int width = this->eigen_values_[i].size() + this->tiffs_[i].size();
        int height = this->eigen_values_[i][0].size();

        TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width);
        TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height);
        TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 16);
        TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 3);
        TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

        TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_DEFLATE);
        TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);

        vector<uint16_t> tmp_data(width * height * 3);
        int index_tmp = 0;
        for(int j=0;j<height;++j){
            for(int k=0;k<width;++k){
                for(int m=0;m<3;++m){
                    if(k < this->tiffs_[i].size())
                        tmp_data[index_tmp++] = (uint16_t)(this->tiffs_[i][j][k] * 65535.0);
                    else
                        tmp_data[index_tmp++] = (uint16_t)(this->eigen_values_[i][j][k-tiffs_[i].size()][m] / maximum * 65535.0);
                }
            }
        }

        TIFFWriteEncodedStrip(tif, 0, &tmp_data[0], width * height * 6);

        TIFFClose(tif);
    }
    chdir(original_dir);

    return;
}

void tomo_super_tiff::save_eigen_values_separated(const char *prefix){
    //find maximum of eigen_values_
    float maximum = -1.0;
    for(int i=0;i<this->eigen_values_.size();++i){
        for(int j=0;j<this->eigen_values_[i].size();++j){
            for(int k=0;k<this->eigen_values_[i][j].size();++k){
                for(int m=0;m<this->eigen_values_[i][j][k].size();++m){
                    maximum = maximum > this->eigen_values_[i][j][k][m] ? maximum : this->eigen_values_[i][j][k][m];
                }
            }
        }
    }

    //save them
    char original_dir[100] = {0};
    mkdir(prefix,0755);
    getcwd(original_dir,100);
    chdir(prefix);

    //ev0
    for(int t=0;t<3;++t){
        char original_dir_t[100] = {0};
        char number_string[50] = {0};
        sprintf(number_string,"%d",t);
        mkdir(number_string,0755);
        getcwd(original_dir_t,100);
        chdir(number_string);

#pragma omp parallel for
        for(int i=0;i<this->eigen_values_.size();++i){
            //make file name
            sprintf(number_string,"%d",i);
            string address = string(number_string) + string(".tiff");

            vector< vector<float> > data(this->eigen_values_[i].size());
            for(int j=0;j<data.size();++j){
                data[j].resize(this->eigen_values_[i][j].size(),0.0);
                for(int k=0;k<data[j].size();++k){
                    data[j][k] = this->eigen_values_[i][j][k][t] / maximum;
                }
            }

            tomo_tiff tmp(data);
            tmp.save(address.c_str());
        }
        chdir(original_dir_t);
    }
    chdir(original_dir);

    return;
}

void tomo_super_tiff::save_eigen_values_ev(const char *address){

    cout << "saving " << address << "..." <<endl;

    fstream out_ev(address, fstream::out);
    if(out_ev.is_open() == false){
        cerr << "ERROR : cannot open " <<address <<endl;
        exit(-1);
    }

    //find maximum
    progressbar *progress = progressbar_new("Maximum",this->eigen_values_.size());
    float maximum = 0.0;
    for(int i=0;i<this->eigen_values_.size();++i){
        for(int j=0;j<this->eigen_values_[i].size();++j){
            for(int k=0;k<this->eigen_values_[i][j].size();++k){
                for(int m=0;m<this->eigen_values_[i][j][k].size();++m){
                    maximum = maximum > this->eigen_values_[i][j][k][m] ? maximum : this->eigen_values_[i][j][k][m];
                }
            }
        }
        progressbar_inc(progress);
    }
    progressbar_finish(progress);

    out_ev << "exyz-size " << this->eigen_values_[0][0][0].size() << " " << this->eigen_values_[0][0].size() << " " << this->eigen_values_[0].size() << " " << this->eigen_values_.size() <<endl;
    out_ev << "normalized " << fixed << setprecision(8) <<  maximum <<endl;
    out_ev << "order xyz"<<endl;

    progress = progressbar_new("Saving",this->eigen_values_.size());
    for(int i=0;i<this->eigen_values_.size();++i){
        for(int j=0;j<this->eigen_values_[i].size();++j){
            for(int k=0;k<this->eigen_values_[i][j].size();++k){
                for(int m=0;m<this->eigen_values_[i][j][k].size();++m){
                    out_ev << fixed << setprecision(8) << (float)(this->eigen_values_[i][j][k][m]/maximum) << " ";
                }
            }
        }
        progressbar_inc(progress);
    }
    progressbar_finish(progress);

    out_ev.close();
    return;
}

void tomo_super_tiff::load_eigen_values_ev(const char *address){

    cout << "reading " << address << "..." <<endl;

    fstream in_ev(address, fstream::in);
    if(in_ev.is_open() == false){
        cerr << "ERROR : cannot open " <<address <<endl;
        exit(-1);
    }

    string buffer;
    string order;
    float normalized = 0.0;
    int size_e = 0;
    int size_x = 0;
    int size_y = 0;
    int size_z = 0;

    //exyz-size
    in_ev >> buffer >> size_e >> size_x >> size_y >> size_z;
    //normalized
    in_ev >> buffer >> normalized;
    //order
    in_ev >> buffer >> order;

    //init
    this->eigen_values_.resize(size_z);
    progressbar *progress = progressbar_new("Initialize",this->eigen_values_.size());
#pragma omp parallel for
    for(int i=0;i<size_z;++i){
        this->eigen_values_[i].resize(size_y);
        for(int j=0;j<size_y;++j){
            this->eigen_values_[i][j].resize(size_x);
            for(int k=0;k<size_x;++k){
                this->eigen_values_[i][j][k].resize(size_e,0.0);
            }
        }
#pragma omp critical
        progressbar_inc(progress);
    }
    progressbar_finish(progress);

    //read data
    progress = progressbar_new("Reading",this->eigen_values_.size());
    if(order == "xyz"){
        for(int i=0;i<this->eigen_values_.size();++i){
            for(int j=0;j<this->eigen_values_[i].size();++j){
                for(int k=0;k<this->eigen_values_[i][j].size();++k){
                    for(int m=0;m<this->eigen_values_[i][j][k].size();++m){
                        in_ev >> this->eigen_values_[i][j][k][m] ;
                        this->eigen_values_[i][j][k][m] *= normalized;
                    }
                }
            }
            progressbar_inc(progress);
        }
        progressbar_finish(progress);
    }
    else{
        cout << "ERROR : order " << order << " not handled" <<endl;
        exit(-1);
    }

    return;
}

