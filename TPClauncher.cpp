//In "settings.json" of code runner extension, replace the custom command by "cd $dir && g++ $fileName -Iboost -lstdc++fs -o $fileNameWithoutExt -std=c++17 && $dir$fileNameWithoutExt" to make sure g++ compile with boost,stb,filesystem and my custom library with c++17
//notice that filesystem could not work, in that case consider updating you gcc compiler, in my case I did with MSYS2 to install gcc 10.3.0
//In order to compile, you will need to have a complete boost distribution (1Gb, too big to put in my repo, you can download it aside: https://www.boost.org/) placed in the project folder and renamed "boost", that way you get "XXX/EncoderDecoderTPC/boost"
#include "EncoderDecoderTPC.h"

int main(){
    encodeTPC("r6icons.png");
    decodeTPC("r6icons.tpc");
    return 0;
}