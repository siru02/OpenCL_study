#define _USE_MATH_DEFINES
#define _CRT_SECURE_NO_WARNINGS

#include "rotation.h"
#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

char* get_source_code(const char* file_name, size_t* len) {
    FILE* file = fopen(file_name, "rb");
    if (file == NULL) {
        printf("[%s:%d] Failed to open %s\n", __FILE__, __LINE__, file_name);
        exit(EXIT_FAILURE);
    }

    fseek(file, 0, SEEK_END);
    size_t length = (size_t)ftell(file);
    rewind(file);

    char* source_code = (char*)malloc(length + 1);
    fread(source_code, length, 1, file);
    source_code[length] = '\0';
    fclose(file);
    *len = length;

    return source_code;
}

void build_error(cl_program program, cl_device_id device, cl_int err) {
    if (err == CL_BUILD_PROGRAM_FAILURE) {
        size_t log_size;
        char* log;

        err = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        CHECK_ERROR(err);

        log = (char*)malloc(log_size + 1);
        err = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        CHECK_ERROR(err);

        log[log_size] = '\0';
        printf("Compiler error:\n%s\n", log);
        free(log);
        exit(0);
    };
}

void rotate(const float* input, float* output, const int width, const int height, char* degree) {
    cl_int err;

    // Platform ID
    cl_platform_id platform;
    err = clGetPlatformIDs(1, &platform, NULL);
    CHECK_ERROR(err);

    // Device ID
    cl_device_id device;
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    CHECK_ERROR(err);

    // Create Context
    cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    CHECK_ERROR(err);

    // Create Command Queue
    cl_command_queue queue = clCreateCommandQueueWithProperties(context, device, 0, &err);
    CHECK_ERROR(err);

    // Create Program Object
    size_t kernel_source_size;
    char* kernel_source = get_source_code("kernel.cl", &kernel_source_size);
    cl_program program = clCreateProgramWithSource(context, 1, (const char**)&kernel_source, &kernel_source_size, &err);
    CHECK_ERROR(err);

    // Build Program
    err = clBuildProgram(program, 1, &device, "", NULL, NULL); //(program, num_devices, &device, option, ...)
    build_error(program, device, err);
    CHECK_ERROR(err);

    // 여기서부터 병렬 처리를 위한 호스트 코드를 작성하세요.
    /*
    1. 이미지는 픽셀값은 float으로 저장되어있으므로 float배열인 버퍼를 준비한다
    2. 커널함수에 인자로 global하게 float배열을 넘겨준다
    3. 각 work-item은 하나의 (dest_x, dest_y)좌표에 대해 원본으로부터 값을 가져오도록 한다
    4. Index공간은 2차원(width, height)로 설정 -> pixel[dest_y * width + dest_x]와 같이 접근하도록
    5. work_group도 설정해야한다 //이건 나중에 추가하자
    */

    // 임시로 2개의 버퍼만 사용하게 설정 -> work_group을 사용하려면 dest buffer가 여러개이어야할듯
    cl_mem buffer_src = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(float) * width * height, input, &err);
    CHECK_ERROR(err);
    cl_mem buffer_dest = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(float) * width * height, NULL, &err);
    CHECK_ERROR(err);

    // 버퍼에 데이터 복사
    err = clEnqueueWriteBuffer(queue, buffer_src, CL_TRUE, 0, sizeof(float) * width * height, input, 0, NULL, NULL);
    CHECK_ERROR(err);

    // 인자 변수들 설정
    // (x0, y0)은 회전의 중심좌표
    float x0 = width / 2.0f; //이미지의 너비의 절반
    float y0 = height / 2.0f; //이미지 높이의 절반

    // 회전각에 따라 sin, cos 계산하는 부분
    const float theta = atof(degree) * M_PI / 180;
    const float sin_theta = sinf(theta);
    const float cos_theta = cosf(theta);

    // Kernel 생성 및 인자 설정
    cl_kernel rotate_image = clCreateKernel(program, "rotate_image", &err); //(프로그램객체, 커널함수명,err
    CHECK_ERROR(err);
    clSetKernelArg(rotate_image, 0, sizeof(cl_mem), &buffer_src); //첫 인자로 buffer_src
    clSetKernelArg(rotate_image, 1, sizeof(cl_mem), &buffer_dest);
    clSetKernelArg(rotate_image, 2, sizeof(float), &sin_theta);
    clSetKernelArg(rotate_image, 3, sizeof(float), &cos_theta);
    clSetKernelArg(rotate_image, 4, sizeof(float), &x0);
    clSetKernelArg(rotate_image, 5, sizeof(float), &y0);
    clSetKernelArg(rotate_image, 6, sizeof(int), &width);
    clSetKernelArg(rotate_image, 7, sizeof(int), &height);

    // 커널 실행
    size_t global_work_size[2] = { width, height }; //작업공간을 2차원으로 구성할 것이므로
    size_t local_work_size[2] = { 16, 16 }; // work-group 사이즈를 설정한다
    clEnqueueNDRangeKernel(queue, rotate_image, 2, NULL, global_work_size, local_work_size, 0, NULL, NULL);

    // buffer_dest에 있는 정보를 output에 옮겨적는다
    err = clEnqueueReadBuffer(queue, buffer_dest, CL_TRUE, 0, sizeof(float) * width * height, output, 0, NULL, NULL);
    CHECK_ERROR(err);
    clFinish(queue);

    // 릴리즈
    clReleaseMemObject(buffer_src);
    clReleaseMemObject(buffer_dest);
    clReleaseKernel(rotate_image);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    free(kernel_source);
    printf("Release Fin\n");
}