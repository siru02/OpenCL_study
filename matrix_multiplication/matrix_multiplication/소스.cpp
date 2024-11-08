#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define CHECK_ERROR(err) \
    if(err != CL_SUCCESS) { \
        printf("[%s:%d] OpenCL error %d\n", __FILE__, __LINE__, err); \
        exit(EXIT_FAILURE); \
    }

// 파일이름을 기준으로 kernel code를 가져오는 함수
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

// program object, device_id를 인자로 받아서 프로그램 build성공 여부를 확인 
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

void mat_mul_seq(float* A, float* B, float* C, int ROW_A, int COL_A, int COL_B) {
    int i, j, k;
    for (i = 0; i < ROW_A; i++) {
        for (j = 0; j < COL_B; j++) {
            C[i * COL_B + j] = 0.0f;
            for (k = 0; k < COL_A; k++) {
                C[i * COL_B + j] += C[i * COL_A + k] * B[k * COL_B + j];
            }
        }
    }
}

// OpenCL 설정함수
void opencl() {
    cl_int err;

    // Platform ID
    cl_platform_id platform;
    err = clGetPlatformIDs(1, &platform, NULL);
    CHECK_ERROR(err);

    // Device ID
    cl_device_id device;
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL); //GPU만 device로 추가한다
    CHECK_ERROR(err);

    // Create Context
    cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    CHECK_ERROR(err);

    // Create Command Queue
    cl_command_queue queue = clCreateCommandQueueWithProperties(context, device, 0, &err);
    CHECK_ERROR(err);

    // Create Program Object
    size_t kernel_source_size;
    char* kernel_source = get_source_code("kernel.cl", &kernel_source_size); //같은 폴더내의 kernel.cl 파일의 코드를 읽어온다
    cl_program program = clCreateProgramWithSource(context, 1, (const char**)&kernel_source, &kernel_source_size, &err);
    CHECK_ERROR(err);

    // Build Program
    err = clBuildProgram(program, 1, &device, "", NULL, NULL);
    build_error(program, device, err);
    CHECK_ERROR(err);

    /*
     * 커널 생성은 여기서 진행해주세요.
     * ex)
     *  cl_kernel kernel = clCreateKernel(program, "gemm", &err);
     *  CHECK_ERROR(err);
     */
    
    //커널 생성
    cl_kernel kernel = clCreateKernel(program, "matrix_multiplication", &err); //프로그램 오브젝트와 커널함수이름을 인자로 준다

    clock_t start = clock();

    //여기서부터 병렬 처리를 위한 나머지 호스트 코드를 작성하세요.
    int ROW_A = 5;
    int COL_A = 5;
    int ROW_B = 5;
    int COL_B = 5;
    int ROW_C = ROW_A;
    int COL_C = COL_B;

    float* A = (float*)malloc(sizeof(float) * 5 * 5);
    float* B = (float*)malloc(sizeof(float) * 5 * 5);
    float* C = (float*)malloc(sizeof(float) * 5 * 5);
    float* ans = (float*)malloc(sizeof(float) * 5 * 5);

    for (int i = 0; i < 5; i++) {
        for (int j = 0; i < 5; j++) {
            A[i * ROW_A + j] = rand();
            B[i * ROW_B + j] = rand();
        }
    }
    mat_mul_seq(A, B, ans, ROW_A, COL_A, COL_B);
    //여기까지 정답코드 생성

    // OpenCL사용을 위한 버퍼선언
    cl_mem buffer_A = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(float) * ROW_A * COL_A, A, &err);
    CHECK_ERROR(err);
    cl_mem buffer_B = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(float) * ROW_B * COL_B, B, &err);
    CHECK_ERROR(err);
    cl_mem buffer_C = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(float) * ROW_A * COL_B, NULL, &err);
    CHECK_ERROR(err);

    // 버퍼에 데이터 복사
    err = clEnqueueWriteBuffer(queue, buffer_A, CL_TRUE, 0, sizeof(float) * ROW_A * COL_A, A, 0, NULL, NULL);
    CHECK_ERROR(err);
    err = clEnqueueWriteBuffer(queue, buffer_B, CL_TRUE, 0, sizeof(float) * ROW_B * COL_B, B, 0, NULL, NULL);
    CHECK_ERROR(err);

    // 커널에 인자 설정
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &buffer_A);
    CHECK_ERROR(err);
    err = clSetKernelArg(kernel, 1, sizeof(cl_mem), &buffer_B);
    CHECK_ERROR(err);
    err = clSetKernelArg(kernel, 2, sizeof(cl_mem), &buffer_C);
    CHECK_ERROR(err);
    err = clSetKernelArg(kernel, 3, sizeof(cl_mem), &ROW_A);
    CHECK_ERROR(err);
    err = clSetKernelArg(kernel, 4, sizeof(cl_mem), &COL_B);
    CHECK_ERROR(err);

    // 커널 실행
    size_t global_work_size = ROW_A * COL_B;
    


    printf("Execution time: %lfsec\n", (float)(clock() - start) / CLOCKS_PER_SEC);

    err = clReleaseMemObject(buffer_A);
    err = clReleaseMemObject(buffer_B);
    err = clReleaseMemObject(buffer_C);
    err = clReleaseKernel(kernel);
    err = clReleaseProgram(program);
    err = clReleaseCommandQueue(queue);
    err = clReleaseContext(context);

    free(kernel_source);
}