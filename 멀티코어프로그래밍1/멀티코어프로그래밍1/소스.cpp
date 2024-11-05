#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK_ERROR(err) \
    if(err != CL_SUCCESS) { \
        printf("[%s:%d] OpenCL error %d\n", __FILE__, __LINE__, err); \
        exit(EXIT_FAILURE); \
    }

int main()
{
    cl_int err;
    int num_elements = 16384;
    // 플랫폼의 개수와 ID 얻기
    cl_uint num_platforms; //플랫폼개수
    clGetPlatformIDs(0, NULL, &num_platforms);
    cl_platform_id* platforms; //플랫폼 ID
    platforms = (cl_platform_id*)malloc(sizeof(cl_platform_id) * num_platforms);
    clGetPlatformIDs(num_platforms, platforms, NULL);

    // 디바이스 개수확인
    cl_uint num_devices;
    err = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL, 0, NULL, &num_devices); // 디바이스 개수를 저장
    
    // 디바이스 ID배열 할당 및 ID가져오기
    cl_device_id* devices = (cl_device_id*)malloc(sizeof(cl_device_id) * num_devices); //디바이스 ID를 num_devices개 만큼 저장
    err = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL, num_devices, devices, NULL); //디바이스 이름 저장

    //device ID확인
    for (int i = 0; i < num_devices; i++) {
        char device_name[128];
        err = clGetDeviceInfo(devices[i], CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
        if (err != CL_SUCCESS) {
            printf("Error: Unable to get device name. Error code: %d\n", err);
            continue;
        }
        printf("Device[%d]: %s\n", i, device_name);
    }


    //디바이스를 사용하는 context 만들기 -> context란 host와 device들을 관리하는 체제
    cl_context context; //디바이스를 관리하는 host가 가진 context
    context = clCreateContext(NULL, 4, devices, NULL, NULL, &err); //4개의 device들을 관리하는 context생성
    CHECK_ERROR(err);

    // 각 디바이스용 command queue를 만들기
    cl_command_queue queues[4]; //호스트는 각 디바이스의 큐에 커맨드를 넣어서 명령을 내린다
    queues[0] = clCreateCommandQueueWithProperties(context, devices[0], 0, &err);
    CHECK_ERROR(err);

    // 소스코드로부터 Program object 만들기 //소스코드는 openCL 각 디바이스에서 돌아가야하는 커널코드가 들어가야한다
    const char* source_code =
        "__kernel void vec_add(__global int* A, __global int* B, __global int* C) {\n"
        "    int i = get_global_id(0);\n"
        "    C[i] = A[i] + B[i];\n"
        "}";
    size_t source_size = strlen(source_code);
    cl_program program;

    program = clCreateProgramWithSource(context, 1, &source_code, &source_size, &err);
    CHECK_ERROR(err);

    // Program build
    err = clBuildProgram(program, num_devices, devices, "", NULL, NULL);

    //kernel Object
    cl_kernel kernel_vec_add;
    kernel_vec_add = clCreateKernel(program, "vec_add", &err);
    CHECK_ERROR(err);

    //vector addition
    int* A = (int*)malloc(sizeof(int) * 16384);
    int* B = (int*)malloc(sizeof(int) * 16384);
    int* C = (int*)malloc(sizeof(int) * 16384);
    int* ans = (int*)malloc(sizeof(int) * 16384);

    for (int i = 0; i < 16384; i++) {
        A[i] = rand();
        B[i] = rand();
        ans[i] = A[i] + B[i];
    }

    //Buffer Object만들기
    cl_mem buffer_A = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(int) * num_elements, A, &err);
    CHECK_ERROR(err);
    cl_mem buffer_B = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(int) * num_elements, B, &err);
    CHECK_ERROR(err);
    cl_mem buffer_C = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(int) * num_elements, NULL, &err);
    CHECK_ERROR(err);

    // 버퍼에 데이터 복사 (clEnqueueWriteBuffer 사용)
    err = clEnqueueWriteBuffer(queues[0], buffer_A, CL_TRUE, 0, sizeof(int) * num_elements, A, 0, NULL, NULL);
    CHECK_ERROR(err);
    err = clEnqueueWriteBuffer(queues[0], buffer_B, CL_TRUE, 0, sizeof(int) * num_elements, B, 0, NULL, NULL);
    CHECK_ERROR(err);

    // 커널에 인자 설정 -> 인자로 커널, 인덱스, 인자크기, 버퍼 를 받는다
    err = clSetKernelArg(kernel_vec_add, 0, sizeof(cl_mem), &buffer_A);
    CHECK_ERROR(err);
    err = clSetKernelArg(kernel_vec_add, 1, sizeof(cl_mem), &buffer_B);
    CHECK_ERROR(err);
    err = clSetKernelArg(kernel_vec_add, 2, sizeof(cl_mem), &buffer_C);
    CHECK_ERROR(err);

    //커널 실행
    size_t global_work_size = num_elements;
    err = clEnqueueNDRangeKernel(queues[0], kernel_vec_add, 1, NULL, &global_work_size, NULL, 0, NULL, NULL);
    CHECK_ERROR(err);

    // 결과를 buffer_c로 복사
    err = clEnqueueReadBuffer(queues[0], buffer_C, CL_TRUE, 0, sizeof(int) * num_elements, C, 0, NULL, NULL);
    CHECK_ERROR(err);

    for (int i = 0; i < 16384; i++) {
        if (C[i] != ans[i])
            printf("differnce\n");
    }
    printf("finish\n");
}
