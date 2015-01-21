#include <minlib/minlib.hpp>

struct Component {
   std::string id;
};


int main(void)
{
   SimpleMap<std::string, Component> sm;

   Component c;
   sm.add(c.id, c);
};