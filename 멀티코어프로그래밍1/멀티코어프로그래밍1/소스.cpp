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
    // �÷����� ������ ID ���
    cl_uint num_platforms; //�÷�������
    clGetPlatformIDs(0, NULL, &num_platforms);
    cl_platform_id* platforms; //�÷��� ID
    platforms = (cl_platform_id*)malloc(sizeof(cl_platform_id) * num_platforms);
    clGetPlatformIDs(num_platforms, platforms, NULL);

    // ����̽� ����Ȯ��
    cl_uint num_devices;
    err = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL, 0, NULL, &num_devices); // ����̽� ������ ����
    
    // ����̽� ID�迭 �Ҵ� �� ID��������
    cl_device_id* devices = (cl_device_id*)malloc(sizeof(cl_device_id) * num_devices); //����̽� ID�� num_devices�� ��ŭ ����
    err = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL, num_devices, devices, NULL); //����̽� �̸� ����

    //device IDȮ��
    for (int i = 0; i < num_devices; i++) {
        char device_name[128];
        err = clGetDeviceInfo(devices[i], CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
        if (err != CL_SUCCESS) {
            printf("Error: Unable to get device name. Error code: %d\n", err);
            continue;
        }
        printf("Device[%d]: %s\n", i, device_name);
    }


    //����̽��� ����ϴ� context ����� -> context�� host�� device���� �����ϴ� ü��
    cl_context context; //����̽��� �����ϴ� host�� ���� context
    context = clCreateContext(NULL, 4, devices, NULL, NULL, &err); //4���� device���� �����ϴ� context����
    CHECK_ERROR(err);

    // �� ����̽��� command queue�� �����
    cl_command_queue queues[4]; //ȣ��Ʈ�� �� ����̽��� ť�� Ŀ�ǵ带 �־ ����� ������
    queues[0] = clCreateCommandQueueWithProperties(context, devices[0], 0, &err);
    CHECK_ERROR(err);

    // �ҽ��ڵ�κ��� Program object ����� //�ҽ��ڵ�� openCL �� ����̽����� ���ư����ϴ� Ŀ���ڵ尡 �����Ѵ�
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

    //Buffer Object�����
    cl_mem buffer_A = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(int) * num_elements, A, &err);
    CHECK_ERROR(err);
    cl_mem buffer_B = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(int) * num_elements, B, &err);
    CHECK_ERROR(err);
    cl_mem buffer_C = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(int) * num_elements, NULL, &err);
    CHECK_ERROR(err);

    // ���ۿ� ������ ���� (clEnqueueWriteBuffer ���)
    err = clEnqueueWriteBuffer(queues[0], buffer_A, CL_TRUE, 0, sizeof(int) * num_elements, A, 0, NULL, NULL);
    CHECK_ERROR(err);
    err = clEnqueueWriteBuffer(queues[0], buffer_B, CL_TRUE, 0, sizeof(int) * num_elements, B, 0, NULL, NULL);
    CHECK_ERROR(err);

    // Ŀ�ο� ���� ���� -> ���ڷ� Ŀ��, �ε���, ����ũ��, ���� �� �޴´�
    err = clSetKernelArg(kernel_vec_add, 0, sizeof(cl_mem), &buffer_A);
    CHECK_ERROR(err);
    err = clSetKernelArg(kernel_vec_add, 1, sizeof(cl_mem), &buffer_B);
    CHECK_ERROR(err);
    err = clSetKernelArg(kernel_vec_add, 2, sizeof(cl_mem), &buffer_C);
    CHECK_ERROR(err);

    //Ŀ�� ����
    size_t global_work_size = num_elements;
    err = clEnqueueNDRangeKernel(queues[0], kernel_vec_add, 1, NULL, &global_work_size, NULL, 0, NULL, NULL);
    CHECK_ERROR(err);

    // ����� buffer_c�� ����
    err = clEnqueueReadBuffer(queues[0], buffer_C, CL_TRUE, 0, sizeof(int) * num_elements, C, 0, NULL, NULL);
    CHECK_ERROR(err);

    for (int i = 0; i < 16384; i++) {
        if (C[i] != ans[i])
            printf("differnce\n");
    }
    printf("finish\n");
}
