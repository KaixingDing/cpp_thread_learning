#include "resource.hpp"

Resource::Resource(int i) : id(i) {}

std::mutex &Resource::getMutex()
{
    return mutex;
}

int Resource::getId() const
{
    return id;
}