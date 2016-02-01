#include <iostream>
#include "tomo_tiff.h"
#include <cstring>
#include <cstdlib>

using namespace std;

int main(int argc, char **argv){
    if(argc < 2){
        return -1;
    }

    tomo_super_tiff sample((char*)argv[1]);

    //do the down size to 32x
    /*for(int mag = 2;mag <= 32;mag *= 2){
        string prefix("./tomo_data_");
        char number_string[50]={0};
        sprintf(number_string,"%d",mag);
        prefix += number_string + string("x/");

        sample.down_size(mag, prefix.c_str());
    }*/
    sample.neuron_detection(5);
    return 0;
}
