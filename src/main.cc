// Copyright (c) 2023 by Rockchip Electronics Co., Ltd. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/*-------------------------------------------
                Includes
-------------------------------------------*/
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>   
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include "rtsp_demo.h"
#include "luckfox_mpi.h"

#include "retinaface_facenet.h"
#include <time.h>
#include <sys/time.h>

#include "dma_alloc.cpp"

#include "pwm.h"
#include "pid.h"
#include "tcpserver.h"
#include "tcpclient.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <queue>
#include "ThreadPool.hpp"

#define USE_DMA    0

#define DISP_WIDTH  640
#define DISP_HEIGHT 480

//Threads
std::queue<VIDEO_FRAME_INFO_S> frame_queue; // Frame queue for multi-threading
std::mutex mtx_frame_queue;
std::condition_variable cv_frame_queue;
std::atomic<bool> stop_flag(false);
const size_t MAX_QUEUE_SIZE = 10;

// PID
PID yaw_pid={0};         
PID pitch_pid={0};

// PWM
std::atomic<int> yaw_servo_angle(45);   // Initial yaw servo angle
std::atomic<int> pitch_servo_angle(90);   // Initial pitch servo angle
std::atomic<bool> servo_mode(false); // Servo mode flag

//TCP Server
int sockfd = 0; // Global socket file descriptor

/*-------------------------------------------
                  Sub Thread
-------------------------------------------*/
void capture_thread()
{
    RK_S32 s32Ret;
    clock_t start_time;
    clock_t end_time;
    float capture_fps;
    while (!stop_flag)
    {
        start_time = clock();
        VIDEO_FRAME_INFO_S stViFrame;
        s32Ret = RK_MPI_VI_GetChnFrame(0, 0, &stViFrame, -1);          //  Get Frame from VI   
        if(s32Ret == RK_SUCCESS)
        {
            std::unique_lock<std::mutex> lock(mtx_frame_queue);
            if (frame_queue.size() >= MAX_QUEUE_SIZE)
            {
                RK_MPI_VI_ReleaseChnFrame(0, 0, &frame_queue.front());  //  Release Frame from VI
                frame_queue.pop();
            }
            frame_queue.push(stViFrame);
            lock.unlock();
            cv_frame_queue.notify_one();
        }
        end_time = clock();
        capture_fps = ((float)CLOCKS_PER_SEC / (end_time - start_time));
        // printf("Capture FPS: %.2f\n", capture_fps);
    }
}
void process_thread(rknn_app_context_t* app_retinaface_ctx, rknn_app_context_t* app_facenet_ctx,
                    float* reference_out_fp32, int width, int height, int facenet_width, int facenet_height, 
                    unsigned char* data, cv::Mat* frame, VENC_STREAM_S* stFrame, VIDEO_FRAME_INFO_S* h264_frame,
                    RK_U32* H264_TimeRef, rtsp_demo_handle g_rtsplive,rtsp_session_handle g_rtsp_session)
{

    float min_norm = 2.0f;
    int min_norm_index = 0;
    //Retinaface
    int retina_width    = 640;
    int retina_height   = 640;

    float scale_x = (float)width / (float)retina_width;  
	float scale_y = (float)height / (float)retina_height;
    int sX,sY,eX,eY; 

    int ret;
    RK_S32 s32Ret;
    object_detect_result_list od_results;
    char show_text[12];
    char fps_text[32]; 

    float* out_fp32 = (float*)malloc(sizeof(float) * 128); 

    clock_t start_time;
    clock_t end_time;
    float process_fps;

    while(!stop_flag)
    {
        start_time = clock();
        VIDEO_FRAME_INFO_S stViFrame;
        std::unique_lock<std::mutex> lock(mtx_frame_queue);
        cv_frame_queue.wait(lock, [] {return !frame_queue.empty() || stop_flag; });
        if(stop_flag) break;
        while(frame_queue.size() > 1)
        {
            RK_MPI_VI_ReleaseChnFrame(0, 0, &frame_queue.front());
            frame_queue.pop();
        }
        stViFrame = frame_queue.front();
        frame_queue.pop();
        h264_frame->stVFrame.u32TimeRef = (*H264_TimeRef)++;
		h264_frame->stVFrame.u64PTS = TEST_COMM_GetNowUs();

        // infer process
        void *vi_data = RK_MPI_MB_Handle2VirAddr(stViFrame.stVFrame.pMbBlk);
        cv::Mat yuv420sp(height + height / 2, width, CV_8UC1, vi_data);
        cv::Mat bgr(height, width, CV_8UC3, data);			
        cv::Mat model_bgr(retina_height, retina_width, CV_8UC3);
        
        cv::cvtColor(yuv420sp, bgr, cv::COLOR_YUV420sp2BGR);
        cv::resize(bgr, *frame, cv::Size(width ,height), 0, 0, cv::INTER_LINEAR);
        cv::resize(bgr, model_bgr, cv::Size(retina_width ,retina_height), 0, 0, cv::INTER_LINEAR);
        memcpy(app_retinaface_ctx->input_mems[0]->virt_addr, model_bgr.data, retina_width * retina_height * 3);
        ret = inference_retinaface_model(app_retinaface_ctx, &od_results);
        if (ret != 0)
        {
            printf("init_retinaface_model fail! ret=%d\n", ret);
            RK_MPI_VI_ReleaseChnFrame(0, 0, &stViFrame); 
            continue;
        }

        for (int i = 0; i < od_results.count; i++) {
            if (od_results.count >= 1) {
                object_detect_result *det_result = &(od_results.results[i]);
                sX = (int)((float)det_result->box.left * scale_x);
                sY = (int)((float)det_result->box.top * scale_y);
                eX = (int)((float)det_result->box.right * scale_x);
                eY = (int)((float)det_result->box.bottom * scale_y);

                cv::Rect roi(sX, sY, (eX - sX), (eY - sY));
                cv::Mat face_img = bgr(roi);

                cv::Mat facenet_input(facenet_height, facenet_width, CV_8UC3, app_facenet_ctx->input_mems[0]->virt_addr);
                letterbox(face_img, facenet_input);
                ret = rknn_run(app_facenet_ctx->rknn_ctx, nullptr);
                if (ret < 0) {
                    printf("rknn_run fail! ret=%d\n", ret);
                    continue;
                }
                uint8_t *output = (uint8_t *)(app_facenet_ctx->output_mems[0]->virt_addr);
                output_normalization(app_facenet_ctx, output, out_fp32);
                
                // calculate target_face with cur_face distance
                float norm = get_duclidean_distance(reference_out_fp32, out_fp32);
                min_norm = norm < min_norm ? norm : min_norm;
                min_norm_index = norm < min_norm ? i : min_norm_index;
            }
        }

        // Servo Control
        if(od_results.count > 0)    
        {
            int index = min_norm_index;
            object_detect_result *det_result = &(od_results.results[index]);

            sX = (int)((float)det_result->box.left 	 *scale_x);	
            sY = (int)((float)det_result->box.top 	 *scale_y);	
            eX = (int)((float)det_result->box.right  *scale_x);	
            eY = (int)((float)det_result->box.bottom *scale_y);

            cv::rectangle(*frame,cv::Point(sX,sY),cv::Point(eX,eY),cv::Scalar(0,255,0),3);

            printf("@ (%d %d %d %d) %.3f\n",sX,sY,eX,eY,min_norm);      
            sprintf(show_text,"norm=%f",min_norm);
            cv::putText(*frame, show_text, cv::Point(sX, sY - 8),cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0,255,0), 1);

            int center_x = (sX + eX) / 2;
            int center_y = (sY + eY) / 2;
            PID_Calc(&yaw_pid, 320, center_x);
            PID_Calc(&pitch_pid, 240, center_y);

            printf("yaw PID output: %.2f, Servo angle: %d\n", yaw_pid.output, yaw_servo_angle.load());
            printf("pitch PID output: %.2f, Servo angle: %d\n", pitch_pid.output, pitch_servo_angle.load());

            if(!servo_mode)
            {
                yaw_pid.output >= 0 ? yaw_servo_angle = 45 + 0.15 * yaw_pid.output
                            : yaw_servo_angle = 45 + yaw_pid.output * 0.15;
                pitch_pid.output >= 0 ? pitch_servo_angle = 90 - 0.2 * pitch_pid.output
                                    : pitch_servo_angle = 90 - pitch_pid.output * 0.2;
                
                yaw_servo_set_angle(yaw_servo_angle);
                pitch_servo_set_angle(pitch_servo_angle);
                printf("yaw_servo_angle: %d, pitch_servo_angle: %d\n", yaw_servo_angle.load(), pitch_servo_angle.load());
            }
            
            min_norm = 2.0f;
            min_norm_index = 0;
        }

        sprintf(fps_text, "FPS: %.1f", process_fps);
        cv::putText(*frame,fps_text,cv::Point(0, 20),
                    cv::FONT_HERSHEY_SIMPLEX,0.5,
                    cv::Scalar(0,255,0),1);

        memcpy(data, frame->data, width * height * 3);

        // encode H264
        RK_MPI_VENC_SendFrame(0,  h264_frame ,-1);
        // Get encode rtsp
        s32Ret = RK_MPI_VENC_GetStream(0, stFrame, -1);
        if(s32Ret == RK_SUCCESS)
		{
			if(g_rtsplive && g_rtsp_session)
			{			
				void *pData = RK_MPI_MB_Handle2VirAddr(stFrame->pstPack->pMbBlk);
				rtsp_tx_video(g_rtsp_session, (uint8_t *)pData, stFrame->pstPack->u32Len,
							  stFrame->pstPack->u64PTS);
				rtsp_do_event(g_rtsplive);
			}
		}

        // release frame
        RK_MPI_VI_ReleaseChnFrame(0, 0, &stViFrame);
        RK_MPI_VENC_ReleaseStream(0, stFrame);

        end_time = clock();
        process_fps = ((float)CLOCKS_PER_SEC / (end_time - start_time));
        // printf("Process FPS: %.2f\n", process_fps);
    }
    free(out_fp32);
}

/*-------------------------------------------
                  Main Function
-------------------------------------------*/
int main(int argc, char **argv)
{
    if (argc != 4)
    {
        printf("%s <retinaface model_path> <facenet model_path> <reference pic_path>\n", argv[0]);
        return -1;
    }
    system("RkLunch-stop.sh");
    // RK_S32 s32Ret = 0;
    
    const char *model_path  = argv[1];
    const char *model_path2 = argv[2];
    const char *image_path  = argv[3]; 

    clock_t start_time;
    clock_t end_time;

    int width    = DISP_WIDTH;
    int height   = DISP_HEIGHT;

    //Facenet
    int facenet_width   = 160;
    int facenet_height  = 160;

    rknn_app_context_t app_retinaface_ctx;
    rknn_app_context_t app_facenet_ctx; 

    memset(&app_retinaface_ctx, 0, sizeof(rknn_app_context_t));
    memset(&app_facenet_ctx, 0, sizeof(rknn_app_context_t));

    //Init Model
    init_retinaface_facenet_model(model_path, model_path2, &app_retinaface_ctx, &app_facenet_ctx);

    int framebuffer_fd = 0; //for DMA
  
    //h264_frame	
	VENC_STREAM_S stFrame;	
	stFrame.pstPack = (VENC_PACK_S *)malloc(sizeof(VENC_PACK_S));
	RK_U64 H264_PTS = 0;
	RK_U32 H264_TimeRef = 0; 

    // Create Pool
	MB_POOL_CONFIG_S PoolCfg;
	memset(&PoolCfg, 0, sizeof(MB_POOL_CONFIG_S));
	PoolCfg.u64MBSize = width * height * 3 ;
	PoolCfg.u32MBCnt = 1;
	PoolCfg.enAllocType = MB_ALLOC_TYPE_DMA;
	//PoolCfg.bPreAlloc = RK_FALSE;
	MB_POOL src_Pool = RK_MPI_MB_CreatePool(&PoolCfg);
	printf("Create Pool success !\n");	

	// Get MB from Pool 
	MB_BLK src_Blk = RK_MPI_MB_GetMB(src_Pool, width * height * 3, RK_TRUE);
	
	// Build h264_frame
	VIDEO_FRAME_INFO_S h264_frame;
	h264_frame.stVFrame.u32Width = width;
	h264_frame.stVFrame.u32Height = height;
	h264_frame.stVFrame.u32VirWidth = width;
	h264_frame.stVFrame.u32VirHeight = height;
	h264_frame.stVFrame.enPixelFormat =  RK_FMT_RGB888; 
	h264_frame.stVFrame.u32FrameFlag = 160;
	h264_frame.stVFrame.pMbBlk = src_Blk;
	unsigned char *data = (unsigned char *)RK_MPI_MB_Handle2VirAddr(src_Blk);
	cv::Mat frame(cv::Size(width,height),CV_8UC3,data);
	
	// rkaiq init
	RK_BOOL multi_sensor = RK_FALSE;	
	const char *iq_dir = "/etc/iqfiles";
	rk_aiq_working_mode_t hdr_mode = RK_AIQ_WORKING_MODE_NORMAL;
	//hdr_mode = RK_AIQ_WORKING_MODE_ISP_HDR2;
	SAMPLE_COMM_ISP_Init(0, hdr_mode, multi_sensor, iq_dir);
	SAMPLE_COMM_ISP_Run(0);

	// rkmpi init
	if (RK_MPI_SYS_Init() != RK_SUCCESS) {
		RK_LOGE("rk mpi sys init fail!");
		return -1;
	}

	// rtsp init	
	rtsp_demo_handle g_rtsplive = NULL;
	rtsp_session_handle g_rtsp_session;
	g_rtsplive = create_rtsp_demo(554);
	g_rtsp_session = rtsp_new_session(g_rtsplive, "/live/0");
	rtsp_set_video(g_rtsp_session, RTSP_CODEC_ID_VIDEO_H264, NULL, 0);
	rtsp_sync_video_ts(g_rtsp_session, rtsp_get_reltime(), rtsp_get_ntptime());
	
	// vi init
	vi_dev_init();
	vi_chn_init(0, width, height);

	// venc init
	RK_CODEC_ID_E enCodecType = RK_VIDEO_ID_AVC;
	venc_init(0, width, height, enCodecType);
	
	printf("init success\n");	

    //Get referencve img feature
    cv::Mat image = cv::imread(image_path);
    cv::Mat facenet_input(facenet_height, facenet_width, CV_8UC3, app_facenet_ctx.input_mems[0]->virt_addr);
    letterbox(image,facenet_input); 
    int ret = rknn_run(app_facenet_ctx.rknn_ctx, nullptr);
    if (ret < 0) {
        printf("rknn_run fail! ret=%d\n", ret);
        return -1;
    }
    uint8_t  *output = (uint8_t *)(app_facenet_ctx.output_mems[0]->virt_addr);
    float* reference_out_fp32 = (float*)malloc(sizeof(float) * 128); 
    output_normalization(&app_facenet_ctx,output,reference_out_fp32);
    //memset(facenet_input.data, 0, facenet_width * facenet_height * channels);

    // Servo Init
    servo_init();
    yaw_servo_set_angle(yaw_servo_angle); 
    pitch_servo_set_angle(pitch_servo_angle);

    // PID Init
    PID_Init(&yaw_pid, 0.75, 0.04, 0.12, 50, 300.0); 
    PID_Init(&pitch_pid, 0.80, 0.01, 0.10, 30, 220.0);

    // tcp client init
    int sockfd_client = tcpclient_init();

    // tcp server init
    // int connfd = tcpserver_init();
    // if (connfd < 0) {
    //     printf("TCP server init failed!\n");
    //     return -1;
    // }
    // printf("TCP server is running...\n");
    // Start threads
    std::thread t_tcpclient(tcpclient_run, sockfd_client, std::ref(servo_mode), std::ref(yaw_servo_angle), std::ref(pitch_servo_angle));
    // std::thread t_tcpserver(tcpserver_run, connfd, std::ref(servo_mode), std::ref(yaw_servo_angle), std::ref(pitch_servo_angle));
    std::thread t_capture(capture_thread);
    std::thread t_process(process_thread, &app_retinaface_ctx, &app_facenet_ctx, reference_out_fp32, width, height, facenet_width, facenet_height,
                          data, &frame, &stFrame, &h264_frame, &H264_TimeRef, g_rtsplive, g_rtsp_session);

    // wait for threads to finish
    t_capture.join();
    t_process.join();
    // t_tcpserver.join();
    t_tcpclient.join();
 
    free(reference_out_fp32);

    // Destory MB
	RK_MPI_MB_ReleaseMB(src_Blk);
	// Destory Pool
	RK_MPI_MB_DestroyPool(src_Pool);
	
	RK_MPI_VI_DisableChn(0, 0);
	RK_MPI_VI_DisableDev(0);
	
	SAMPLE_COMM_ISP_Stop(0);
		
	RK_MPI_VENC_StopRecvFrame(0);
	RK_MPI_VENC_DestroyChn(0);

	free(stFrame.pstPack);

	if (g_rtsplive)
		rtsp_del_demo(g_rtsplive);

	RK_MPI_SYS_Exit();

    release_facenet_model(&app_facenet_ctx);
    release_retinaface_model(&app_retinaface_ctx);
    // Deinit Servo
    servo_deinit();

    return 0;
}