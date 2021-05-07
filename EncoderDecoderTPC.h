#pragma once
#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>

#include "boost/boost/dynamic_bitset/dynamic_bitset.hpp"//notice that for some reason bitset stores data inverted, so LSB is to the left and MSB to the right, but when we call them everything is good

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image/stb_image_write.h"

using namespace std;//to avoid putting std:: before each string declaration


enum allocation_type{
    NO_ALLOCATION,SELF_ALLOCATED,STB_ALLOCATED
};

typedef struct{
    int width;
    int height;
    int channels;
    size_t size; 
    unsigned char *data;
    //enum allocation_type allocation_;
} Image;

void encodeTPC(string fileName);
void imageLoad(string fileName,Image &img);
void imageSave(string fileName,string dataString);
//tpc header?
string TPC_v1(Image *img);
string TPC_v2(Image *img);
//compare both
void decodeTPC(string fileName);

string getBits(boost::dynamic_bitset<>&bitset,int pointerPos,int numberOfBits);
void setBits(string &dataString,string data,int numberOfBits);
//void stringToBitset
void splitString(string &inputString,vector<string>*segmentsList,char delimiter);



void encodeTPC(string fileName){
  string dataString;
  Image img;
  std::cout<<"starting encoding..."<<endl;
  imageLoad(fileName,img);
  setBits(dataString,std::bitset<16>(img.width).to_string(),16);//that method migth cause bugs without the terminal complaining e.g. "std::bistet<8>(500).to_string()" will run even if you can't store "500" binary representation in 8 bits
  setBits(dataString,std::bitset<16>(img.height).to_string(),16);
  imageSave(fileName,dataString);
  std::cout<<"encoding finished"<<endl<<endl;
    return;
}

void decodeTPC(string fileName){
  FILE *pFile;
  long lSize;
  char *buffer;
  size_t result;
  int32_t pointerPos=0;//jpeg max size is 65535*65535pixels so 4 294 836 225pixels (image will be square but who cares no one make above 8k pictures), a 32bits int can handle that

  std::cout<<"starting decoding..."<<endl;
  pFile = fopen ( fileName.c_str() , "rb" );//open for binary reading, convert from string to char array
  if (pFile==NULL) {
    fputs ("File error",stderr); exit (1);
  }

  // obtain file size:
  fseek (pFile , 0 , SEEK_END);//put the position indicator at the end of the file
  lSize = ftell (pFile);//Returns the current value of the position indicator of the stream.
  rewind (pFile);//put the position indicator at the beginning of the file

  // allocate memory to contain the whole file:
  buffer = (char*) malloc (lSize);//create a buffer that is the same size of the file
  if (buffer == NULL) {fputs ("Memory error",stderr); exit (2);}

  // copy the file into the buffer:
  result = fread (buffer,1,lSize,pFile);//put the content off the file in buffer
  if (result != lSize) {fputs ("Reading error",stderr); exit (3);}

  /* the whole file is now loaded in the memory buffer. */
  int bufferSize=strlen(buffer)*8;//number of char*8 to get number of bits

  boost::dynamic_bitset<>bufferBinary(bufferSize);//create an empty bitset with 'bufferSize' bits, it will store all the data from the file

  for(int i=0;i<strlen(buffer);i++){//set bufferBinary, byte per byte
    std::bitset<8>charBinary(buffer[i]);//create a 8 bit array and store char binary
    //std::cout<<charBinary;//print byte content
    for(int j=0;j<8;j++){//for every bit of the byte
      bufferBinary.set(i*8+j,charBinary[7-j]);
    }
  }
  unsigned short width=std::stoi(getBits(bufferBinary,pointerPos,16),0,2);//convert string of bianry to int,stored in 16 bits,so maximum is 65 535
  pointerPos+=16;
  unsigned short height=std::stoi(getBits(bufferBinary,pointerPos,16),0,2);
  pointerPos+=16;
  std::cout<<"image size: "<<width<<"x"<<height<<endl;//print image size
  std::cout<<buffer<<endl;
  fclose (pFile);
  free (buffer);
  std::cout<<"decoding finished"<<endl;
  return;
}

void imageLoad(string fileName,Image &img){
  img.data=stbi_load(fileName.c_str(),&img.width,&img.height,&img.channels,0);
  if(img.data == NULL) {
    std::cout<<"Error in loading the image";
    exit(1);
  std::cout<<"image size: "<<img.width<<"x"<<img.height<<endl;
  }
}

void imageSave(string fileName,string dataString){
  FILE *pFile;
  if(dataString.length()%8!=0){
    std::cout<<"the file is not written in bytes or incomplete byte"<<endl;
    exit(1);
  }
  std::filesystem::path newFileName=fileName;
  newFileName.replace_extension(".tpc");
  pFile=fopen(newFileName.string().c_str(),"w");
  string temp;
  for(int i=0;i<=(dataString.length()/8)-1;i++){//for each byte
    temp.clear();
    for(int j=0;j<8;j++){
      temp+=dataString.at(i*8+j);
    }
    cout<<"bin: "<<temp<<" char: "<<stoi(temp,nullptr,2)<<endl;//show binary and int representation of each byte processed
    fputc(stoi(temp,nullptr,2),pFile);
  }
  fclose(pFile);
  std::cout<<"image saved"<<endl;
}

string getBits(boost::dynamic_bitset<>&bitset,int pointerPos,int numberOfBits){//concern bitset
  string bitsReturned;
  for(int i=pointerPos;i<pointerPos+numberOfBits;i++){//get numbers of bits from pointerPos,included
    if (bitset[i]==0){//for some reason if method is faster than bitsReturned+=to_string(bitset[i]);
      bitsReturned+="0";
    }
    else{
      bitsReturned+="1";
    }
  }
  return bitsReturned;
}

void setBits(string &dataString,string data,int numberOfBits){//concern string
  if(data.length()<numberOfBits){
    for(int i=0;i<numberOfBits-data.length();i++){
      dataString+="0";
    }
  }
  else if(data.length()>numberOfBits){
    std::cout<<"error: the data to write is too long"<<endl;
    return;
  }
  dataString+=data;
  return;
}