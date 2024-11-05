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
    // �÷����� ������ ID��� -> �÷����̶� ����̽��� ������ ����ȸ�縦 �ǹ��Ѵ�
    cl_uint num_platforms; //�÷�������
    clGetPlatformIDs(0, NULL, &num_platforms);
    cl_platform_id* platforms; //�÷��� ID
    platforms = (cl_platform_id*)malloc(sizeof(cl_platform_id) * num_platforms);
    clGetPlatformIDs(num_platforms, platforms, NULL);
    for (int i = 0; i < num_platforms; i++) {
        char platform_name[128];
        err = clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, sizeof(platform_name), platform_name, NULL);
        printf("platform[%d]: %s\n", i, platform_name);
    }
    ////////////////////////////////////////////////////////////////////////////////////////
    

    ////////////////////////////////////////////////////////////////////////////////////////
    // ����̽� ����Ȯ��
    cl_uint num_devices;
    err = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL, 0, NULL, &num_devices); // ����̽� ������ ����
    
    // ����̽� ID�迭 �Ҵ� �� ID��������
    cl_device_id* devices = (cl_device_id*)malloc(sizeof(cl_device_id) * num_devices); //����̽� ID�� num_devices�� ��ŭ ����
    err = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL, num_devices, devices, NULL); //����̽� �̸� ����

    //device ID�� device_nameȮ��
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
    //����̽��� ����ϴ� context ����� -> context�� host�� device���� �����ϴ� ü��
    // ����̽� �󿡼� ������ �۾��� �����ϱ� ���� ȯ���� �����ϴ� ��ü
    cl_context context; //����̽��� �����ϴ� host�� ���� context�� ������ ����
    context = clCreateContext(NULL, num_devices, devices, NULL, NULL, &err); //4���� device���� �����ϴ� context����
    CHECK_ERROR(err);
    ////////////////////////////////////////////////////////////////////////////////////////


    ////////////////////////////////////////////////////////////////////////////////////////
    // �� ����̽��� command queue�� �����
    // Command Queue�� ȣ��Ʈ(Host)�� ����̽�(Device)�� �۾��� ��û�ϴ� ��⿭ ������ ����
    cl_command_queue queues[4]; //ȣ��Ʈ�� �� ����̽��� ť�� Ŀ�ǵ带 �־ ����� ������
    queues[0] = clCreateCommandQueueWithProperties(context, devices[0], 0, &err);
    CHECK_ERROR(err);
    ////////////////////////////////////////////////////////////////////////////////////////


    // �ҽ��ڵ�κ��� Program object ����� 
    //�ҽ��ڵ�� openCL �� ����̽����� ���ư����ϴ� Ŀ���ڵ尡 �����Ѵ�
    // OpenCL������ ���α׷��� �����ϱ� ���� Ŀ�� �ҽ� �ڵ带 �ۼ��ϰ�
    // �̸� �������Ͽ� program object�� �����
    // ����̽����� ������ kernel object�� �����Ѵ�
    const char* source_code =
        "__kernel void vec_add(__global int* A, __global int* B, __global int* C) {\n"
        "    int i = get_global_id(0);\n" //i�� �۷ι� �۾� ID�� ������ ���� ��ũ �������� ó���� �ε����� ����
        "    C[i] = A[i] + B[i];\n" //1���� ������ ������ �����̹Ƿ� i�� �ٷ� ����
        "}";
    size_t source_size = strlen(source_code);
    cl_program program;

    //Program object����
    // �ҽ��ڵ尡 program object�� ���޵ǰ� OpenCL ��Ÿ���� �̸� �����ϰ� �ȴ�
    program = clCreateProgramWithSource(context, 1, &source_code, &source_size, &err);
    CHECK_ERROR(err);

    // Program build -> ���α׷� ��ü�� �������ϰ� ��ũ�Ͽ� Ŀ�η� ������ �� �ִ� ���·� �����
    err = clBuildProgram(program, num_devices, devices, "", NULL, NULL);
    //OpenCL��Ÿ���� program�� �������Ͽ� ���̳ʸ����·� ����� �̸� �޸� ������ ����
    // OpenCL��Ÿ���� �̸� ȣ��Ʈ �޸𸮳� ����̽� �޸𸮿� �����ϰ�(?) ����

    //kernel Object
    // OpenCL������ kernel function�� ����̽����� �����ϴ� �⺻�����̹Ƿ�
    // ���̳ʸ��� �׳� �����ص� �Ǵ°� �ƴѰ�?
    cl_kernel kernel_vec_add;
    kernel_vec_add = clCreateKernel(program, "vec_add", &err);// ������ program���� kernel����
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
    // buffer��ü�� �����ϰ�, ������ ���縦 ���� ȣ��Ʈ �޸𸮿��� ����̽� �޸𸮷� �����͸� ����
    // clCreateBuffer�� buffer��ü�� �����ϸ� ����̽� �޸𸮿� �޸� ������ �Ҵ��Ѵ�
    cl_mem buffer_A = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(int) * num_elements, A, &err);
    CHECK_ERROR(err);
    cl_mem buffer_B = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(int) * num_elements, B, &err);
    CHECK_ERROR(err);
    cl_mem buffer_C = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(int) * num_elements, NULL, &err);
    CHECK_ERROR(err);

    // ���ۿ� ������ ���� (clEnqueueWriteBuffer ���)
    // ȣ��Ʈ �޸𸮿��� ����̽� �޸𸮷� �����͸� ����
    err = clEnqueueWriteBuffer(queues[0], buffer_A, CL_TRUE, 0, sizeof(int) * num_elements, A, 0, NULL, NULL);
    CHECK_ERROR(err);
    err = clEnqueueWriteBuffer(queues[0], buffer_B, CL_TRUE, 0, sizeof(int) * num_elements, B, 0, NULL, NULL);
    CHECK_ERROR(err);
    // queues[0] -> Ŀ�ǵ�ť�� ���� ����̽��� ����� �����ϰ�, ����̽� ���ۿ� write�� �����ϴ� ����� ������

    // Ŀ�ο� ���� ���� -> ���ڷ� `Ŀ��, �ε���, ����ũ��, ����` �� �޴´�
    // clSetKernelArg�� Ư�� Ŀ���� ���ڸ� �����ϴ� �Լ�
    // Ŀ���Լ��� ù ���ڿ��� buffer_A�� �� ��° ���ڿ��� buffer_B�� �� ��° ���ڿ��� buffer_C�� �Ҵ�ȴ�
    err = clSetKernelArg(kernel_vec_add, 0, sizeof(cl_mem), &buffer_A);
    CHECK_ERROR(err);
    err = clSetKernelArg(kernel_vec_add, 1, sizeof(cl_mem), &buffer_B);
    CHECK_ERROR(err);
    err = clSetKernelArg(kernel_vec_add, 2, sizeof(cl_mem), &buffer_C);
    CHECK_ERROR(err);

    //Ŀ�� ���� -> kernel_vec_add ����� ť�� �־ ����
    size_t global_work_size = num_elements;
    err = clEnqueueNDRangeKernel(queues[0], kernel_vec_add, 1, NULL, &global_work_size, NULL, 0, NULL, NULL);
    CHECK_ERROR(err);

    // ����� buffer_C���� ȣ��Ʈ �޸��� �迭C�� ����
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
