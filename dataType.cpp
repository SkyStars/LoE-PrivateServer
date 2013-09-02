#include "dataType.h"

UVector::UVector()
{
    x=0;
    y=0;
    z=0;
}

UVector::UVector(float ux, float uy, float uz)
{
    x=ux;
    y=uy;
    z=uz;
}

UQuaternion::UQuaternion()
{
    x=0;
    y=0;
    z=0;
    w=0;
}

UQuaternion::UQuaternion(float ux, float uy, float uz, float uw)
{
    x=ux;
    y=uy;
    z=uz;
    w=uw;
}
