#include "minlib/minlib.hpp"

using namespace std;

int main(void)
{
    std::string s = readFile("test_text");
    D_ASSERT(s == "I am a testfile\nYes I am!\n\n");

    bool catcher = false;
    try
    {
        std::string s = readFile("fake_text");
    }
    catch(...)
    {
        catcher = true;
    }

    D_ASSERT(catcher);
}
