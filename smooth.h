#ifndef _SMOOTH_H_
#define _SMOOTH_H_

class smooth
{
public:
    smooth(int c);
    float calc(float rs);
    void setThreshold(float v);
    void setmax2value(float v);
    void setdif2value(float v);

private:
    int current;
    float threshold;
    float max2value;
    float dif2value;
    float rs1,rs2,rs3;
};

#endif // SMOOTH_H
