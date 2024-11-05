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

    /////////////////////////////////////////////////////////////////////////////////////
    // 플랫폼의 개수와 ID얻기 -> 플랫폼이란 디바이스를 구현한 벤더회사를 의미한다
    cl_uint num_platforms; //플랫폼개수
    clGetPlatformIDs(0, NULL, &num_platforms);
    cl_platform_id* platforms; //플랫폼 ID
    platforms = (cl_platform_id*)malloc(sizeof(cl_platform_id) * num_platforms);
    clGetPlatformIDs(num_platforms, platforms, NULL);
    for (int i = 0; i < num_platforms; i++) {
        char platform_name[128];
        err = clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, sizeof(platform_name), platform_name, NULL);
        printf("platform[%d]: %s\n", i, platform_name);
    }
    ////////////////////////////////////////////////////////////////////////////////////////
    

    ////////////////////////////////////////////////////////////////////////////////////////
    // 디바이스 개수확인
    cl_uint num_devices;
    err = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL, 0, NULL, &num_devices); // 디바이스 개수를 저장
    
    // 디바이스 ID배열 할당 및 ID가져오기
    cl_device_id* devices = (cl_device_id*)malloc(sizeof(cl_device_id) * num_devices); //디바이스 ID를 num_devices개 만큼 저장
    err = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL, num_devices, devices, NULL); //디바이스 이름 저장

    //device ID및 device_name확인
    for (int i = 0; i < num_devices; i++) {
        char device_name[128];
        err = clGetDeviceInfo(devices[i], CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
        if (err != CL_SUCCESS) {
            printf("Error: Unable to get device name. Error code: %d\n", err);
            continue;
        }
        printf("Device[%d]: %s\n", i, device_name);
    }
    ////////////////////////////////////////////////////////////////////////////////////////


    ////////////////////////////////////////////////////////////////////////////////////////
    //디바이스를 사용하는 context 만들기 -> context란 host와 device들을 관리하는 체제
    // 디바이스 상에서 실행할 작업을 설정하기 위한 환경을 정의하는 객체
    cl_context context; //디바이스를 관리하는 host가 가진 context를 저장할 변수
    context = clCreateContext(NULL, num_devices, devices, NULL, NULL, &err); //4개의 device들을 관리하는 context생성
    CHECK_ERROR(err);
    ////////////////////////////////////////////////////////////////////////////////////////


    ////////////////////////////////////////////////////////////////////////////////////////
    // 각 디바이스용 command queue를 만들기
    // Command Queue는 호스트(Host)가 디바이스(Device)에 작업을 요청하는 대기열 역할을 수행
    cl_command_queue queues[4]; //호스트는 각 디바이스의 큐에 커맨드를 넣어서 명령을 내린다
    queues[0] = clCreateCommandQueueWithProperties(context, devices[0], 0, &err);
    CHECK_ERROR(err);
    ////////////////////////////////////////////////////////////////////////////////////////


    // 소스코드로부터 Program object 만들기 
    //소스코드는 openCL 각 디바이스에서 돌아가야하는 커널코드가 들어가야한다
    // OpenCL에서는 프로그램을 실행하기 위해 커널 소스 코드를 작성하고
    // 이를 컴파일하여 program object로 만들고
    // 디바이스에서 실행할 kernel object를 생성한다
    const char* source_code =
        "__kernel void vec_add(__global int* A, __global int* B, __global int* C) {\n"
        "    int i = get_global_id(0);\n" //i는 글로벌 작업 ID를 가져와 현재 워크 아이템이 처리할 인덱스를 결정
        "    C[i] = A[i] + B[i];\n" //1차원 벡터의 끼리의 연산이므로 i를 바로 적용
        "}";
    size_t source_size = strlen(source_code);
    cl_program program;

    //Program object생성
    // 소스코드가 program object로 전달되고 OpenCL 런타임이 이를 관리하게 된다
    program = clCreateProgramWithSource(context, 1, &source_code, &source_size, &err);
    CHECK_ERROR(err);

    // Program build -> 프로그램 객체를 컴파일하고 링크하여 커널로 실행할 수 있는 상태로 만든다
    err = clBuildProgram(program, num_devices, devices, "", NULL, NULL);
    //OpenCL런타임이 program을 컴파일하여 바이너리형태로 만들고 이를 메모리 영역에 저장
    // OpenCL런타임이 이를 호스트 메모리나 디바이스 메모리에 랜덤하게(?) 저장

    //kernel Object
    // OpenCL에서는 kernel function이 디바이스에서 실행하는 기본단위이므로
    // 바이너리를 그냥 실행해도 되는게 아닌가?
    cl_kernel kernel_vec_add;
    kernel_vec_add = clCreateKernel(program, "vec_add", &err);// 빌드한 program으로 kernel생성
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
    // buffer객체를 생성하고, 데이터 복사를 통해 호스트 메모리에서 디바이스 메모리로 데이터를 전송
    // clCreateBuffer로 buffer객체를 생성하면 디바이스 메모리에 메모리 공간을 할당한다
    cl_mem buffer_A = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(int) * num_elements, A, &err);
    CHECK_ERROR(err);
    cl_mem buffer_B = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(int) * num_elements, B, &err);
    CHECK_ERROR(err);
    cl_mem buffer_C = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(int) * num_elements, NULL, &err);
    CHECK_ERROR(err);

    // 버퍼에 데이터 복사 (clEnqueueWriteBuffer 사용)
    // 호스트 메모리에서 디바이스 메모리로 데이터를 복사
    err = clEnqueueWriteBuffer(queues[0], buffer_A, CL_TRUE, 0, sizeof(int) * num_elements, A, 0, NULL, NULL);
    CHECK_ERROR(err);
    err = clEnqueueWriteBuffer(queues[0], buffer_B, CL_TRUE, 0, sizeof(int) * num_elements, B, 0, NULL, NULL);
    CHECK_ERROR(err);
    // queues[0] -> 커맨드큐를 통해 디바이스에 명령을 전달하고, 디바이스 버퍼에 write를 수행하는 명령을 내린다

    // 커널에 인자 설정 -> 인자로 `커널, 인덱스, 인자크기, 버퍼` 를 받는다
    // clSetKernelArg는 특정 커널의 인자를 설정하는 함수
    // 커널함수의 첫 인자에는 buffer_A가 두 번째 인자에는 buffer_B가 세 번째 인자에는 buffer_C가 할당된다
    err = clSetKernelArg(kernel_vec_add, 0, sizeof(cl_mem), &buffer_A);
    CHECK_ERROR(err);
    err = clSetKernelArg(kernel_vec_add, 1, sizeof(cl_mem), &buffer_B);
    CHECK_ERROR(err);
    err = clSetKernelArg(kernel_vec_add, 2, sizeof(cl_mem), &buffer_C);
    CHECK_ERROR(err);

    //커널 실행 -> kernel_vec_add 명령을 큐에 넣어서 실행
    size_t global_work_size = num_elements;
    err = clEnqueueNDRangeKernel(queues[0], kernel_vec_add, 1, NULL, &global_work_size, NULL, 0, NULL, NULL);
    CHECK_ERROR(err);

    // 결과를 buffer_C에서 호스트 메모리인 배열C로 복사
    err = clEnqueueReadBuffer(queues[0], buffer_C, CL_TRUE, 0, sizeof(int) * num_elements, C, 0, NULL, NULL);
    CHECK_ERROR(err);

    for (int i = 0; i < 16384; i++) {
        if (C[i] != ans[i])
            printf("differnce\n");
        else
        {
            printf(" (%d, %d)", C[i], ans[i]);
        }
    }
    /*printf("C  : ");
    for (int i = 0; i < 16384; i++) {
        printf("%d ", C[i]);
    }
    printf("\nans: ");
    for (int i = 0; i < 16384; i++) {
        printf("%d ", ans[i]);
    }*/
    printf("finish\n");
}
