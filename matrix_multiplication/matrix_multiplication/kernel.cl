__kernel void matrix_multiplication(__global float* A, __global float* B, __global float* C, int ROW, int COL) {
    int x = get_global_id(0); // ���� PE�� x��ǥ�� ���� global_id�� ���´�
    int y = get_global_id(1); // ���� PE�� y��ǥ�� ���� global_id�� ���´�
    
    // C[x][y]�� ����ϱ� ���ؼ�
    // A�� x�� ���ҵ��� ���� ��������
    // B�� y�� ���ҵ��� ���� �����´�

    int i, j, k;
    for (i = 0; i < ROW; i++) {
        for (j = 0; j < COL; j++) {
            for (k = 0; k < COL; k++) {
                C[i * COL + j] += A[i * COL + k] * B[k * COL + j];
            }
        }
    }
}