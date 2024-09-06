#include "bisect/initializer.h"

using namespace bisect::gst;

initializer::initializer()
{
    gst_init(NULL, NULL);
}

initializer::~initializer()
{
    gst_deinit();
}