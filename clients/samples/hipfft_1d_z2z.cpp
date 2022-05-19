// Copyright (C) 2019 - 2022 Advanced Micro Devices, Inc. All rights
// reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include <complex>
#include <iostream>
#include <vector>

#include <hip/hip_runtime_api.h>
#include <hipfft.h>

int main()
{
    std::cout << "hipfft 1D double-precision complex-to-complex transform\n";

    const int Nx        = 8;
    int       direction = HIPFFT_FORWARD; // forward=-1, backward=1

    std::vector<std::complex<double>> cdata(Nx);
    size_t complex_bytes = sizeof(decltype(cdata)::value_type) * cdata.size();

    // Create HIP device object and copy data to device
    // Use hipfftComplex for single-precision
    hipError_t           hip_rt;
    hipfftDoubleComplex* x;
    hip_rt = hipMalloc(&x, complex_bytes);
    if(hip_rt != hipSuccess)
        throw std::runtime_error("hipMalloc failed");

    // Inititalize the data
    for(size_t i = 0; i < Nx; i++)
    {
        cdata[i] = i;
    }
    std::cout << "input:\n";
    for(size_t i = 0; i < cdata.size(); i++)
    {
        std::cout << cdata[i] << " ";
    }
    std::cout << std::endl;
    hip_rt = hipMemcpy(x, cdata.data(), complex_bytes, hipMemcpyHostToDevice);
    if(hip_rt != hipSuccess)
        throw std::runtime_error("hipMemcpy failed");

    // Create the plan
    hipfftHandle plan      = NULL;
    hipfftResult hipfft_rt = hipfftCreate(&plan);
    if(hipfft_rt != HIPFFT_SUCCESS)
        throw std::runtime_error("failed to create plan");

    hipfft_rt = hipfftPlan1d(&plan, // plan handle
                             Nx, // transform length
                             HIPFFT_Z2Z, // transform type (HIPFFT_C2C for single-precision)
                             1); // number of transforms
    if(hipfft_rt != HIPFFT_SUCCESS)
        throw std::runtime_error("hipfftPlan1d failed");

    // Execute plan:
    // hipfftExecZ2Z: double precision, hipfftExecC2C: for single-precision
    hipfft_rt = hipfftExecZ2Z(plan, x, x, direction);
    if(hipfft_rt != HIPFFT_SUCCESS)
        throw std::runtime_error("hipfftExecZ2Z failed");

    std::cout << "output:\n";
    hip_rt = hipMemcpy(cdata.data(), x, complex_bytes, hipMemcpyDeviceToHost);
    if(hip_rt != hipSuccess)
        throw std::runtime_error("hipMemcpy failed");
    for(size_t i = 0; i < cdata.size(); i++)
    {
        std::cout << cdata[i] << " ";
    }
    std::cout << std::endl;

    hipfftDestroy(plan);
    hipFree(x);

    return 0;
}
