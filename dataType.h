#ifndef DATATYPE_H
#define DATATYPE_H

class UVector
{
public:
    UVector();
    UVector(float ux, float uy, float uz);

public:
    float x;
    float y;
    float z;
};
typedef struct UVector UVector;

class UQuaternion
{
public:
    UQuaternion();
    UQuaternion(float ux, float uy, float uz, float uw);

public:
    float x;
    float y;
    float z;
    float w;
};
typedef struct UQuaternion UQuaternion;

#endif // DATATYPE_H
