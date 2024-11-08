__kernel void rotate_image(__global float* buffer_src, __global float* buffer_dest, 
    const float sin_theta, const float cos_theta, 
    const float x0, const float y0, const int width, const int height) 
{
    // ���� �������� PE�� global_size������ �� ��ҿ� ���ؼ� �����ϹǷ� ���ϰ����ϴ� ��ǥ�� x, y ��ǥ�� ���´�
    int dest_x = get_global_id(0); // ���� PE�� x��ǥ�� ���� global_id�� ���´�
    int dest_y = get_global_id(1); // ���� PE�� y��ǥ�� ���� global_id�� ���´�

    // output[x][y] = input[src_x][src_y]�̴�
    float xOff = dest_x - x0;
    float yOff = dest_y - y0;

    // (src_x, src_y)�� (dest_x, dest_y)�� theta��ŭ ȸ���ϱ� ���� ��ǥ
    int src_x = (int)(xOff * cos_theta + yOff * sin_theta + x0);
    int src_y = (int)(yOff * cos_theta - xOff * sin_theta + y0);
    if ((src_x >= 0) && (src_x < width) && (src_y >= 0) && (src_y < height))
        buffer_dest[dest_y * width + dest_x] = buffer_src[src_y * width + src_x];
    else
        buffer_dest[dest_y * width + dest_x] = 0.0f;
}