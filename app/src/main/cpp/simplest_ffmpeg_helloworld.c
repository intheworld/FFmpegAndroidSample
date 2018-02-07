
#include <stdio.h>
 
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavfilter/avfilter.h"

//Log
#ifdef ANDROID
#include <jni.h>
#include <android/log.h>
#include <android/native_window_jni.h>
#include <android/native_window.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <unistd.h>

#define LOGE(format, ...)  __android_log_print(ANDROID_LOG_ERROR, "(>_<)", format, ##__VA_ARGS__)
#else
#define LOGE(format, ...)  printf("(>_<) " format "\n", ##__VA_ARGS__)
#endif


//FIX
struct URLProtocol;


JNIEXPORT jstring Java_win_intheworld_ffmpegandroidsample_MainActivity_avcodecinfo(JNIEnv *env, jobject obj)
{
	char info[40000] = { 0 };

	av_register_all();

	AVCodec *c_temp = av_codec_next(NULL);

	while(c_temp!=NULL){
		if (c_temp->decode!=NULL){
			sprintf(info, "%s[Dec]", info);
		}
		else{
			sprintf(info, "%s[Enc]", info);
		}
		switch (c_temp->type){
		case AVMEDIA_TYPE_VIDEO:
			sprintf(info, "%s[Video]", info);
			break;
		case AVMEDIA_TYPE_AUDIO:
			sprintf(info, "%s[Audio]", info);
			break;
		default:
			sprintf(info, "%s[Other]", info);
			break;
		}
		sprintf(info, "%s[%10s]\n", info, c_temp->name);

		
		c_temp=c_temp->next;
	}
	//LOGE("%s", info);

	return (*env)->NewStringUTF(env, info);
}

void custom_log(void *ptr, int level, const char* fmt, va_list vl){
	FILE *fp=fopen("/storage/emulated/0/av_log.txt","a+");
	if(fp){
		vfprintf(fp,fmt,vl);
		fflush(fp);
		fclose(fp);
	}
}


JNIEXPORT jint JNICALL Java_win_intheworld_ffmpegandroidsample_MainActivity_decode(JNIEnv *env, jobject obj, jstring input_jstr, jstring output_jstr) {
	AVFormatContext *pFormatCtx;
	int i, videoindex;
	AVCodecContext *pCodecCtx;
	AVCodec *pCodec;
	AVFrame *pFrame, *pFrameYUV;
	uint8_t *out_buffer;
	AVPacket *packet;
	int y_size;
	int ret, got_picture;
	struct SwsContext *img_convert_ctx;
	FILE *fp_yuv;
	int frame_cnt;
	clock_t time_start, time_finish;
	double time_duration = 0.0;

	char input_str[500] = {0};
	char output_str[500] = {0};
	char info[1000] = {0};

	sprintf(input_str, "%s", (*env)->GetStringUTFChars(env, input_jstr, NULL));

	sprintf(output_str, "%s", (*env)->GetStringUTFChars(env, output_jstr, NULL));

	av_log_set_callback(custom_log);

	av_register_all();

	avformat_network_init();

	pFormatCtx = avformat_alloc_context();

	if (avformat_open_input(&pFormatCtx, input_str, NULL, NULL)!=0) {
		LOGE("Could not open input stream.\n");
		return -1;
	}
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
		LOGE("Could not stream information.\n");
		return -1;
	}
	videoindex = -1;
	for (int i = 0; i < pFormatCtx->nb_streams; ++i) {
		if (pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
			videoindex=i;
			break;
		}
	}

	if (videoindex==-1) {
		LOGE("Could not find a video stream.\n");
		return -1;
	}
	pCodecCtx = pFormatCtx->streams[videoindex]->codec;
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);

	if (pCodec == NULL) {
		LOGE("Could not find Codec.\n");
		return -1;
	}
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
		LOGE("Could not open codec.\n");
		return -1;
	}

	pFrame = av_frame_alloc();
	pFrameYUV = av_frame_alloc();

	out_buffer = (unsigned char *)av_malloc((size_t)av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1));
	av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, out_buffer,
		AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1);

	packet = (AVPacket *)av_malloc(sizeof(AVPacket));
	img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
		pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height,
		AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

	sprintf(info, "[Input    ]%s\n", input_str);
	sprintf(info, "%s[Output   ]%s\n", info, output_str);
	sprintf(info, "%s[Format   ]%s\n", info, pFormatCtx->iformat->name);
	sprintf(info, "%s[Format   ]%s\n", info, pCodecCtx->codec->name);
	sprintf(info, "%s[Resolution]%dx%d\n", info, pCodecCtx->width, pCodecCtx->height);

	fp_yuv = fopen(output_str, "wb+");

	if (fp_yuv == NULL) {
		printf("Cannot open output file.\n");
		return -1;
	}

	frame_cnt=0;
	time_start = clock();

	while (av_read_frame(pFormatCtx, packet)>=0) {
		if (packet->stream_index==videoindex) {
			ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
			if (ret < 0) {
				LOGE("Decode Error.\n");
				return -1;
			}
			if (got_picture) {
				sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);

				y_size = pCodecCtx->width*pCodecCtx->height;
				fwrite(pFrameYUV->data[0], 1, y_size, fp_yuv);
				fwrite(pFrameYUV->data[1], 1, y_size/4, fp_yuv);
				fwrite(pFrameYUV->data[2], 1, y_size/4, fp_yuv);
			}

            char pictype_str[10] = {0};
            switch (pFrame->pict_type) {
                case AV_PICTURE_TYPE_I:
                    sprintf(pictype_str, "I");
                    break;
                case AV_PICTURE_TYPE_P:
                    sprintf(pictype_str, "P");
                    break;
                case AV_PICTURE_TYPE_B:
                    sprintf(pictype_str, "B");
                    break;
                default:
                    sprintf(pictype_str, "Other");
                    break;
            }
            LOGE("Frame Index: %5d. Type:%s", frame_cnt, pictype_str);
            frame_cnt++;
		}
        av_free_packet(packet);
	}

    //flush decoder
    //FIX: Flush Frames remained in Codec
    while (1) {
        ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
        if (ret < 0)
            break;
        if (!got_picture)
            break;
        sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
                  pFrameYUV->data, pFrameYUV->linesize);
        int y_size=pCodecCtx->width*pCodecCtx->height;
        fwrite(pFrameYUV->data[0],1,y_size,fp_yuv);    //Y
        fwrite(pFrameYUV->data[1],1,y_size/4,fp_yuv);  //U
        fwrite(pFrameYUV->data[2],1,y_size/4,fp_yuv);  //V
        //Output info
        char pictype_str[10]={0};
        switch(pFrame->pict_type){
            case AV_PICTURE_TYPE_I:sprintf(pictype_str,"I");break;
            case AV_PICTURE_TYPE_P:sprintf(pictype_str,"P");break;
            case AV_PICTURE_TYPE_B:sprintf(pictype_str,"B");break;
            default:sprintf(pictype_str,"Other");break;
        }
        LOGE("Frame Index: %5d. Type:%s",frame_cnt,pictype_str);
        frame_cnt++;
    }
    time_finish = clock();
    time_duration = (double)(time_finish - time_start);
    sprintf(info, "%s[Time   ]%fms\n", info, time_duration);
    sprintf(info, "%s[Count  ]%d\n", info, frame_cnt);

    sws_freeContext(img_convert_ctx);

    fclose(fp_yuv);
    av_frame_free(&pFrameYUV);
    av_frame_free(&pFrame);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);

    return 0;
}

JNIEXPORT void JNICALL
Java_win_intheworld_ffmpegandroidsample_MainActivity_render(JNIEnv *env, jobject obj,
                jstring inputStr_, jobject surface) {
    const char *inputPath = (*env)->GetStringUTFChars(env, inputStr_, JNI_FALSE);
    AVFormatContext *avFormatContext = avformat_alloc_context();
    int error;
    char buf[] = "";
    if ((error = avformat_open_input(&avFormatContext, inputPath, NULL, NULL)) < 0) {
        av_strerror(error, buf, 1024);
        LOGE("%s", inputPath);
        LOGE("Could't open file %s: %d(%s)", inputPath, error, buf);
        return;
    }

    if (avformat_find_stream_info(avFormatContext, NULL) < 0) {
        LOGE("%s", "Could't find stream.");
        return;
    }

    int video_index = -1;
    for (int i = 0; i < avFormatContext->nb_streams; i++) {
        if (avFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_index = i;
            break;
        }
    }

    if (video_index < 0) {
        LOGE("Can not find video stream.");
        return;
    }

    AVCodecContext *avCodecContext = avFormatContext->streams[video_index]->codec;

    AVCodec *avCodec = avcodec_find_decoder(avCodecContext->codec_id);

    if (avcodec_open2(avCodecContext, avCodec, NULL) < 0) {
        LOGE("open failed.");
        return;
    }

    AVPacket *packet = (AVPacket *) av_malloc(sizeof(AVPacket));
    av_init_packet(packet);

    AVFrame *frame = av_frame_alloc();
    AVFrame *rgb_frame = av_frame_alloc();

    uint8_t *out_buffer = (uint8_t *)av_malloc(avpicture_get_size(AV_PIX_FMT_RGBA, avCodecContext->width, avCodecContext->height));

    avpicture_fill((AVPicture *)rgb_frame, out_buffer, AV_PIX_FMT_RGBA, avCodecContext->width, avCodecContext->height);

    struct SwsContext* swsContext = sws_getContext(avCodecContext->width, avCodecContext->height, avCodecContext->pix_fmt,
                                        avCodecContext->width, avCodecContext->height, AV_PIX_FMT_RGBA,
                                        SWS_BICUBIC, NULL, NULL, NULL);

    ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, surface);
    if (nativeWindow==0) {
        LOGE("Can not find nativeWindow.");
        return;
    }

    ANativeWindow_Buffer native_outBuffer;
    int frameCount;
    int h = 0;

    while (av_read_frame(avFormatContext, packet) >= 0) {
        if (packet->stream_index == video_index) {
            avcodec_decode_video2(avCodecContext, frame, &frameCount, packet);
            if (frameCount) {
                ANativeWindow_setBuffersGeometry(nativeWindow, avCodecContext->width, avCodecContext->height, WINDOW_FORMAT_RGBA_8888);
                ANativeWindow_lock(nativeWindow, &native_outBuffer, NULL);

                sws_scale(swsContext, (const uint8_t *const *)frame->data, frame->linesize, 0,
                        frame->height, rgb_frame->data,
                        rgb_frame->linesize);
                uint8_t *dst = (uint8_t *)native_outBuffer.bits;
                int destStride = native_outBuffer.stride * 4;
                uint8_t *src = rgb_frame->data[0];
                int srcStride = rgb_frame->linesize[0];
                for (int i = 0; i < avCodecContext->height; ++i) {
                    memcpy(dst + i*destStride, src + i * srcStride, srcStride);
                }
                ANativeWindow_unlockAndPost(nativeWindow);
                usleep(1000 * 16);
            }
        }
        av_free_packet(packet);
    }
    ANativeWindow_release(nativeWindow);
    av_frame_free(&frame);
    av_frame_free(&rgb_frame);
    avcodec_close(avCodecContext);
    avformat_free_context(avFormatContext);
    (*env)->ReleaseStringUTFChars(env, inputStr_, inputPath);
}