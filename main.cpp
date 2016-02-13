#include <iostream>
#include "tomo_tiff.h"
#include <cstring>
#include <cstdlib>
#include <unistd.h>

using namespace std;

int main(int argc, char **argv){
    if(argc < 2){
        return -1;
    }

    tomo_super_tiff sample((char*)argv[1]);
    sample.neuron_detection(5);
    //cout << "saving measure..." <<endl;
    //sample.save_measure("result");
    //cout << "saving merged measure..." <<endl;
    //sample.save_measure_merge("result_merged");
    cout  << "saving eigen value with rgb..." <<endl;
    sample.save_eigen_values_rgb("eigen_value");
    cout  << "saving eigen value merged with rgb..." <<endl;
    sample.save_eigen_values_rgb_merge("eigen_value_merge");

    return 0;
}
