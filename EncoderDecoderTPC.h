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

struct Image{
  int width;
  int height;
  int channels;
  size_t size; 
  unsigned char *data;
  //enum allocation_type allocation_;
};
struct pixelCoulour{//red,green,blue,alpha
  unsigned char r;
  unsigned char g;
  unsigned char b;
  unsigned char a;
};


//basic commands
void encodeTPC(string fileName);
void decodeTPC(string fileName);

//random functions
void imageLoad(string fileName,Image &img);
void imageSave(string fileName,string dataString);
void makeRGB(Image &img,uint16_t x,uint16_t y,std::vector<int> &valuesBuffer,std::vector<pixelCoulour> &pixelsBuffer);
int searchPixelCoulour(std::vector<pixelCoulour> pixelsBuffer, pixelCoulour& pixelCoulour);
//tpc header?

//tpc encode modes
string TPC_em0(Image *img,string& dataString);
string TPC_em1(Image *img,string& dataString);
string TPC_em2(Image *img,string& dataString);
string TPC_em3(Image *img,string& dataString);

//tpc decode modes
string TPC_dm0(Image *img,string& dataString);
string TPC_dm1(Image *img,string& dataString);
string TPC_dm2(Image *img,string& dataString);
string TPC_dm3(Image *img,string& dataString);

string getBits(boost::dynamic_bitset<>&bitset,int pointerPos,int numberOfBits);
void addBits(string &dataString,string data,int numberOfBits);
void replaceBits(string &dataString,string data,int pointerPos,int numberOfBits);
//void stringToBitset



void encodeTPC(string fileName){
  int32_t pixelPointer=0;//jpeg max size is 65535*65535pixels so 4 294 836 225pixels (image will be square but who cares no one make above 8k pictures), a 32bits int can handle that
  string dataString;
  Image img;
  std::cout<<"starting encoding..."<<endl;
  imageLoad(fileName,img);

  addBits(dataString,std::bitset<16>(img.width).to_string(),16);//that method migth cause bugs without the terminal complaining e.g. "std::bistet<8>(500).to_string()" will run even if you can't store "500" binary representation in 8 bits
  addBits(dataString,std::bitset<16>(img.height).to_string(),16);
  addBits(dataString,"00000000",8);//add 8 bits in prevision of values buffer length(max val=255)
  addBits(dataString,"000",3);//add 3 bits in prevision of values max bits size (max val =7 so 8 possibility)
  addBits(dataString,"00000",5);//fill for the file to be in bytes

  std::vector<int> valuesBuffer;
  std::vector<pixelCoulour> pixelsBuffer;

  for(uint16_t x=0;x<img.width;x++){
    for(uint16_t y=0;y<img.height;y++){
      makeRGB(img,x,y,valuesBuffer,pixelsBuffer);
    }
  }

  imageSave(fileName,dataString);
  std::cout<<"encoding finished"<<endl<<endl;
  return;
}

void decodeTPC(string fileName){
  FILE *pFile;
  long lSize;
  char *buffer;
  size_t result;
  int64_t pointerPos=0;//represent pointer position in BINARY representation, 64 is overkill but I don't want to calculate max size of the file

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
  int valuesBufferSize=std::stoi(getBits(bufferBinary,pointerPos,8),0,2); 
  std::cout<<"image size: "<<width<<"x"<<height<<endl;//print image size
  std::cout<<"values buffer size: "<<valuesBufferSize<<endl;
  std::cout<<"char buffer: "<<buffer<<endl;
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
  return;
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
  return;
}
void makeRGB(Image &img,uint16_t x,uint16_t y,std::vector<int>& valuesBuffer,std::vector<pixelCoulour>& pixelsBuffer){
  unsigned char* pixel = img.data + (y * img.width + x) * img.channels;
  unsigned char r = pixel[0];
  unsigned char g = pixel[1];
  unsigned char b = pixel[2];
  unsigned char a = img.channels >= 4 ? pixel[3] : 0; //unsigned char a = img.channels >= 4 ? pixel[3] : 0xff;

  //add unused value to valuesBuffer
  if(std::find(valuesBuffer.begin(), valuesBuffer.end(), (int)r) == valuesBuffer.end()){//if red value not in valuesBuffer
    valuesBuffer.push_back((int)r);
  }
  if(std::find(valuesBuffer.begin(), valuesBuffer.end(), (int)g) == valuesBuffer.end()){//if green value not in valuesBuffer
    valuesBuffer.push_back((int)g);
  }
  if(std::find(valuesBuffer.begin(), valuesBuffer.end(), (int)b) == valuesBuffer.end()){//if blue value not in valuesBuffer
    valuesBuffer.push_back((int)b);
  }
  if(std::find(valuesBuffer.begin(), valuesBuffer.end(), (int)a) == valuesBuffer.end()){//if alpha value not in valuesBuffer
    valuesBuffer.push_back((int)a);
  }

  pixelCoulour temp{r,g,b,a};
  if(std::find(pixelsBuffer.begin(), pixelsBuffer.end(),temp) == pixelsBuffer.end()){
    pixelsBuffer.push_back(temp);
  }
  //cout<<"r: "<<(int)r<<"  g: "<<(int)g<<"  b: "<<(int)b<<endl;
}
/*
int searchPixelCoulour(std::vector<pixelCoulour> pixelsBuffer, pixelCoulour& pixelCoulour) {
  int index{};
  for (auto& value : pixelsBuffer) {      
    if (value == pixelCoulour) {
      return index;
    }
  index++;
  }
  return -1;
}
*/

string getBits(boost::dynamic_bitset<>&bitset,int pointerPos,int numberOfBits){//concern bitset e.g.  getBits(bitset,16,8); would read from 16 to 23
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
void addBits(string &dataString,string data,int numberOfBits=0){//concern string
  if(numberOfBits!=0 && data.length()<numberOfBits){
    for(int i=0;i<numberOfBits-data.length();i++){//fill with 0 to have a multiple of 8 bits
      dataString+="0";
    }
  }
  else if(numberOfBits!=0 && data.length()>numberOfBits){
    std::cout<<"error: the data to write is too long"<<endl;
    return;
  }
  dataString+=data;
  return;
}
void replaceBits(string &dataString,string data,int pointerPos,int numberOfBits){//if you replace bits you need to know the number of bit replaced replaced
    string temp;
    if(data.length()<numberOfBits){
    for(int i=0;i<numberOfBits-data.length();i++){//fill with 0 to have numberOfBits bits
      temp+="0";
    }
  }
  else if(  data.length()>numberOfBits){
    std::cout<<"error: the data to write is too long"<<endl;
    return;
  }
  temp+=data;
  int counter=0;
  for (int i=pointerPos;i<pointerPos+numberOfBits;i++){//replace NumberOfBits bits from pointerPos,included e.g. replaceBits(dataString,data,4,10); would replace from 4 to 13
    dataString[i]=temp[counter];
    counter++;
  }
  return;
}