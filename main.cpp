#include <iostream>
#include "tomo_tiff.h"
#include <cstring>
#include <cstdlib>
#include <unistd.h>


using namespace std;

void print_usage(void){
    cout << "Usage: " <<endl;
    cout << "neuron_detection_in_tiff" <<endl;
    cout << "[-w window_size]" << endl;
    cout << "[-t num_threads]" << endl;
    cout << "[-d] create experiment data" <<endl;
    cout << "*[-f result_folder_name]" <<endl;
    cout << "*[-s save_eigen_value_address]" <<endl;
    cout << "*[-e address_ev]" <<endl;
    cout << "*[-h threshold > 0]" <<endl;
    cout << "*[-b] bundle magnification" <<endl;
    cout << "[-m result_directory] merge measurements" <<endl;
    cout << "address_filelist" <<endl;
    return;
}

int main(int argc, char **argv){

    //argument
    int opt = 0;
    enum{ ORIGINAL_DATA, EIGEN_VALUE, EXPERIMENTAL_DATA, BUNDLE, MERGE } mode = ORIGINAL_DATA;
    int window_size = 5;
    int num_threads = -1;
    float threshold_measurement = -1.0;
    char *address = NULL;
    string folder_name;
    string saving_ev_address;
    string address_ev;

    //parsing arguments
    while( (opt = getopt(argc, argv, "e:w:t:f:s:dh:bm:")) != -1 ){
        switch(opt){
        case 'e':
            mode = EIGEN_VALUE;
            address_ev = string(optarg);
            break;

        case 'w':
            window_size = atoi(optarg);
            break;

        case 't':
            num_threads = atoi(optarg);
            break;

        case 'f':
            folder_name = string(optarg);
            break;

        case 's':
            saving_ev_address = string(optarg);
            break;

        case 'd':
            mode = EXPERIMENTAL_DATA;
            break;

        case 'h':
            sscanf(optarg,"%f",&threshold_measurement);
            if(threshold_measurement <= 0){
                print_usage();
                exit(-1);
            }
            break;

        case 'b':
            mode = BUNDLE;
            break;

        case 'm':
            mode = MERGE;
            folder_name = string(optarg);
            break;

        default:
            print_usage();
            exit(-1);
        }
    }
    if(optind >= argc){
        print_usage();
        exit(-1);
    }
    address = (char*)argv[optind];

    //set number of threads
    if(num_threads > 0){
        omp_set_dynamic(0);
        omp_set_num_threads(num_threads);
    }

    //do the bundle thing
    if(mode == BUNDLE){
        if( folder_name.empty() ){
            folder_name = string(address) + string("x");
        }
        if( address_ev.empty() ){
            address_ev = string("ev_") + string(address) + string("x");
        }
        string buffer(address);
        sprintf(address, "tomo_data_%sx.txt", buffer.c_str());

        cout << "-f " << folder_name <<endl;
        cout << "-e " << address_ev <<endl;
        cout << address <<endl;
    }

    //loading & calculating
    tomo_super_tiff sample;
    if(mode == ORIGINAL_DATA){
        sample = tomo_super_tiff(address);
        sample.neuron_detection(window_size, threshold_measurement);
        if(sample.size_original_data() >= TIFF_IMAGE_LARGE_SIZE) // the data is too large to care the -f & -s arguments, save anyway
            return 0;
    }
    else if(mode == EIGEN_VALUE || mode == BUNDLE){
        sample = tomo_super_tiff(address);
        sample.load_eigen_values_separated(address_ev.c_str());
        sample.experimental_measurement(threshold_measurement);
    }
    else if(mode == EXPERIMENTAL_DATA){
        create_experimental_data(address);
        return 0;
    }
    else if(mode == MERGE){
        merge_measurements( address, folder_name.c_str() );
        return 0;
    }

    //save result
    cout << "change directory to \"result\"" << endl;
    chdir("result");
    if(!folder_name.empty()){
        mkdir(folder_name.c_str(),0755);
        chdir(folder_name.c_str());
    }

    if(!saving_ev_address.empty()){
        sample.save_eigen_values_ev(saving_ev_address.c_str());
    }

    cout << "saving eigen value with rgb..." <<endl;
    sample.save_eigen_values_rgb("eigen_value");

    cout << "saving eigen value merged with rgb..." <<endl;
    sample.save_eigen_values_rgb_merge("eigen_value_merge");

    cout << "saving eigen value separated..."<<endl;
    sample.save_eigen_values_separated("eigen_value_separated");

    cout << "saving measurement..." <<endl;
    sample.save_measure("measurement");
    sample.save_measure_merge("measurement_merge");

    return 0;
}
