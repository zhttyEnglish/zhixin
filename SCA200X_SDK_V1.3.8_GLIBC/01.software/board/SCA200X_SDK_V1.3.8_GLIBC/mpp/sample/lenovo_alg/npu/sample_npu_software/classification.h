#include<string>
#include<vector>

#include "sample_svp_postprocess.h"

using namespace std;
#ifndef __CLASSIFICATION_H_
#define __CLASSIFICATION_H_

class Classification
{
public:
    Classification(output_tensor *pTensorInfo);
    ~Classification();
    int classificationPostProcess(char *buff, vector<classification_output> &vOutput);
    int classificationPostProcess(short *buff, vector<classification_output> &vOutput);
    int classificationPostProcess(float *buff, vector<classification_output> &vOutput);
    int classificationChwPostProcess(float *buff, vector<classification_output> &vOutput);
private:
    output_tensor *pTensor;
    output_tensor *pTensorInfo;
    int tensorNum;
};

#endif
