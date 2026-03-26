#include "../include/ImageProcessor.h"
#include <span>
#include <fstream>
#include <cstdio>
#include <exception>
#include <filesystem>

int main(int argc, char* argv[]){
    if(argc != 2 && argc != 3){
        printf("failure, arg count : %d\n", argc);
        for(std::size_t i = 0; i < argc; i++){
            printf("%d : %s\n", i, argv[i]);
        }
        return -1;
    }
    ImageProcessor::RawImage rawImg;
    std::ifstream inFile{argv[1], std::ios::binary | std::ios::ate};
    if(!inFile.is_open()){
        printf("failure, file opening : %s\n", argv[1]);
        return -1;
    }
    std::vector<std::byte> file_data{};
    file_data.resize(inFile.tellg());
    inFile.seekg(0, std::ios::beg);

    if(!inFile.read(reinterpret_cast<char*>(file_data.data()), file_data.size())){
        printf("failure, file read\n");
        return -1;
    }
    try{
        const bool processed_ret = rawImg.ProcessData(file_data);
        ImageProcessor::BMP bmp{rawImg.width, rawImg.height, rawImg.format.alpha_width != 0};
        std::filesystem::path bmp_path{argv[1]};
        if(argc == 2){
            bmp_path.replace_extension(".bmp");
        }
        else if(argc == 3){
            bmp_path = argv[2];
        }
        bmp.Write(bmp_path.string(), rawImg.processed_data, rawImg.format);
    }
    catch(std::exception& e){
        printf("failed to process : %s", e.what());
        return -1;
    }

    printf("success\n");
    return 0;
}