# ? NetvedioAgent - ������Ƶ����ϵͳ

<div align="center">
  <img src="https://img.shields.io/badge/Platform-Luckfox%20PICO%20RV1106-blue" alt="Platform">
  <img src="https://img.shields.io/badge/AI-RetinaFace%20+%20FaceNet-green" alt="AI">
  <img src="https://img.shields.io/badge/Protocol-RTSP-orange" alt="Protocol">
  <img src="https://img.shields.io/badge/License-MIT-yellow" alt="License">
</div>

## ? Ӳ��ƽ̨

| ��� | �ͺ� | ���� |
|------|------|------|
| ?? **���ذ�** | Luckfox-PICO RV1106 | ������Ƕ��ʽAI������ |
| ? **����ͷ** | SC3366 | ����ͼ�񴫸��� |
| ?? **��̨** | SG90�����̨ | ����ת����̨ϵͳ |

## ? ��Ŀ����

**NetvedioAgent** ��һ������ Luckfox-PICO RV1106 ƽ̨��������Ƶ���ٷ����ϵͳ������Ŀ�������Ƚ�������ʶ��������Ч����Ƶ��������紫��������Ϊ�û��ṩ���������ܼ�ؽ��������

### ? ��������

- ? **˫ģ��Э��**: RetinaFace ������� + FaceNet ������ȡ
- ? **ʵʱ��Ƶ��**: RKMPI Ӳ������ + RTSP ��������  
- ? **���ܸ���**: PID �����㷨ʵ�־�׼��̨����
- ? **˫����ģʽ**: �Զ����� + �ֶ������޷��л�
- ? **����ͨ��**: TCP Э��ʵ�ֿͻ���������ʵʱ����

## ? ϵͳ����ԭ��

### ? ��ʼ���׶�
���� RetinaFace �� FaceNet ģ�ͣ����� RKMPI�����������TCP ���񣬳�ʼ�� PID ��������ʹ�� FaceNet ��ȡĿ����������������

### ? ���̼ܹ߳�
- **? ͼ�񲶻��߳�**: ͨ�� RK_MPI_VI ��ȡ����ͷ֡������ `frame_queue` ���У�������ʱ�Զ���������֡
- **? ͼ�����߳�**: �Ӷ��л�ȡ����֡������ RetinaFace �������� FaceNet ����ƥ�䣬PID ������̨����Ŀ�꣬RKMPI ���벢 RTSP ����
- **? TCP ͨ���߳�**: �����ͻ��˿���ָ�����ģʽ�л����Զ�/�ֶ�������̨��������

### ? �̰߳�ȫ
`frame_queue` ����Ϊ������Դ��ʹ�û�����������������֤�̰߳�ȫ����ֹ���ݾ�����ϵͳ����ʱ�ȴ������߳������������ͷ� RKMPI ��Դ��ģ�������ġ�


## ? AI ģ�͹�������

### ? ģ�ͳ�ʼ��
```cpp
init_retinaface_facenet_model(model_path, model_path2, &app_retinaface_ctx, &app_facenet_ctx);
```

**��Ҫ����:**
1. ���� RetinaFace �� FaceNet ģ���ļ�
2. ��ȡģ�����������������
3. ����ģ�������ڴ�ռ�
4. ���������Ľṹ�����

### ? Ŀ��������ȡ
1. **ͼ��Ԥ����**: Letterbox ���ţ����ֿ�߱�
2. **FaceNet ����**: ��ȡĿ��������������
3. **������һ��**: ���� `reference_out_fp32` �ο�����

### ? ʵʱ��������
1. **�������**: RetinaFace ���ͼ������������
2. **������ȡ**: ��ÿ����⵽��������ȡ��������
3. **���ƶȼ���**: ������Ŀ��������ŷ�Ͼ���
4. **Ŀ��ѡ��**: ѡ�������С��������Ϊ����Ŀ��

### ?? ��������
- **����**: ��ʶ����������
- **�������**: 
  - ����ŷ�Ͼ�����ֵ `min_norm`
  - ʹ�ø�������Ŀ��������Ƭ


## ? ���ٲ���

### ? ����׼��
- Ubuntu 18.04/20.04 LTS
- Luckfox-Pico SDK ����
- ������빤����

### ? SDK �����
�ο��ٷ��ĵ�: [Luckfox-Pico-SDK](https://wiki.luckfox.com/zh/Luckfox-Pico/Luckfox-Pico-SDK)

### ? ��ȡԴ��
```bash
git clone git@github.com:Jay-ms/NetvedioAgent.git
cd NetvedioAgent
```

### ?? ���ñ���
�޸� `CMakeLists.txt`:
```cmake
# ������Ŀ·��
set(PROJECT_DIR "/your/project/path/NetvedioAgent")

# ���ý��������
set(CMAKE_C_COMPILER "/your/toolchain/path/arm-rockchip830-linux-uclibcgnueabihf-gcc")
set(CMAKE_CXX_COMPILER "/your/toolchain/path/arm-rockchip830-linux-uclibcgnueabihf-g++")
```

### ?? ������Ŀ
```bash
chmod +x make.sh
./make.sh
```

### ? �����豸
```bash
# �����ִ���ļ�
scp NetvedioAgent root@[�豸IP]:/

# ����ģ���ļ�
scp -r model root@[�豸IP]:/

# �滻Ŀ������ͼƬ
scp your_target_face.jpg root@[�豸IP]:/model/test.jpg
```

### ?? ����ϵͳ
```bash
ssh root@[�豸IP]
./NetvedioAgent ./model/RetinaFace.rknn ./model/mobilefacenet.rknn ./model/test.jpg
```

### ? ��֤����
- **RTSP ��Ƶ��**: `rtsp://[�豸IP]:8554/live`
- **TCP ���ƶ˿�**: `[�豸IP]:8080`

---

## ? ��ԴЭ��

����Ŀ���� [MIT License](https://opensource.org/licenses/MIT) ��ԴЭ�顣

---

<div align="center">
  
### ? ��������Ŀ�����а���������� Star ֧��һ�£�

**? ��Ŀ��ַ**: [NetvedioAgent](https://github.com/Jay-ms/NetvedioAgent)

</div>