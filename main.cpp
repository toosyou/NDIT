#include <iostream>
#include "tomo_tiff.h"
#include <cstring>
#include <cstdlib>
#include <unistd.h>

using namespace std;

void print_usage(void){
    cout << "Usage: " <<endl;
    cout << "neuron_detection_in_tiff [-e][-w window_size][-t num_threads][-f result_folder_name][-s save_eigen_value_address] address_filelist/address_ev" <<endl;
    cout << "[-e] Eigen value mode which start from loading .ev file." <<endl;
    return;
}

int main(int argc, char **argv){

    int opt = 0;
    enum{ ORIGINAL_DATA, EIGEN_VALUE } mode = ORIGINAL_DATA;
    int window_size = 5;
    int num_threads = -1;
    char *address = NULL;
    string folder_name;
    string saving_ev_address;
    //parsing arguments
    while( (opt = getopt(argc, argv, "ew:t:f:s:")) != -1 ){
        switch(opt){
        case 'e':
            mode = EIGEN_VALUE;
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

    //loading & calculating
    tomo_super_tiff sample;
    if(mode == ORIGINAL_DATA){
        sample = tomo_super_tiff(address);
        sample.neuron_detection(window_size);
    }
    else if(mode == EIGEN_VALUE){
        sample.load_eigen_values_ev(address);
        sample.experimental_measurement(0);
    }

    //save result
    cout << "change directory to \"result\"" << endl;
    chdir("result");
    if(!folder_name.empty()){
        mkdir(folder_name.c_str(),0755);
        chdir(folder_name.c_str());
    }

    if(!saving_ev_address.empty()){
        cout << "saving eigen value ev..." <<endl;
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
