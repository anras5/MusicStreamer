#include <iostream>
#include <irrKlang.h>

using namespace irrklang;
using namespace std;

int main(){
    ISoundEngine* engine = createIrrKlangDevice();

    if(!engine)
        return 0;

    cout << "\nHello World!\n";

    char i = 0;

    while(i != 'q')
    {
        std::cout << "Press any key to play some sound, press 'q' to quit.\n";

        // play a single sound
        ISound* sound = engine->play2D("../music/avatar.wav", true, false, true, ESM_AUTO_DETECT, false);

        std::cin >> i; 

    }
    engine->drop();
    return 0;
}
