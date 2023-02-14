#include "videodecoder.h"
#include "CRender.h"
#include "CLog.h"
#define __STDC_CONSTANT_MACROS
#define WM_MODMSG_DECODEEND       (WM_USER + 2000)		               // 解码完成消息
#ifdef _WIN32

//Windows
extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
};
#else

//Linux...
#ifdef __cplusplus
extern "C"
{
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#ifdef __cplusplus
};
#endif
#endif
C264Decoder::C264Decoder(const char* inpath)
{
	m_FormatCtx = nullptr;
	m_videoindex = -1;
	m_Codec = nullptr;
	m_CodecCtx = nullptr;
	m_Frame = nullptr;
	m_FrameYUV = nullptr;
	m_out_buffer = nullptr;
	m_got_picture = -1;
	m_img_convert_ctx = nullptr;
	m_filepath = inpath;
	m_render = nullptr;
	m_ptr = nullptr;
}

int C264Decoder::Init()
{
 	av_register_all();
	avformat_network_init();
	m_FormatCtx = avformat_alloc_context();
	if (avformat_open_input(&m_FormatCtx, m_filepath, NULL, NULL) != 0)
	{
		return -1;
	}
	//读取一部分视音频数据并且获得一些相关的信息
	if (avformat_find_stream_info(m_FormatCtx, NULL) < 0)
	{
		return -2;
	}

	//查找视频编码索引
	for (int i = 0; i < m_FormatCtx->nb_streams; i++)
	{
		if (m_FormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
 			m_videoindex = i;
			break;
		}
	}

	if (m_videoindex == -1)
	{
		return -3;
	}

	//编解码上下文
	m_CodecCtx = m_FormatCtx->streams[m_videoindex]->codec;
	//查找解码器
	m_Codec = avcodec_find_decoder(m_CodecCtx->codec_id);
	if (m_Codec == NULL)
	{
		return -4;
	}
	//打开解码器
	if (avcodec_open2(m_CodecCtx, m_Codec, NULL) < 0)
	{
		return -5;
	}

	//申请AVFrame，用于原始视频
	m_Frame = av_frame_alloc();
	//申请AVFrame，用于yuv视频
	m_FrameYUV = av_frame_alloc();
	//分配内存，用于图像格式转换
	m_out_buffer = (unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, m_CodecCtx->width, m_CodecCtx->height, 1));
	av_image_fill_arrays(m_FrameYUV->data, m_FrameYUV->linesize, m_out_buffer, AV_PIX_FMT_YUV420P, m_CodecCtx->width, m_CodecCtx->height, 1);
	m_packet = (AVPacket *)av_malloc(sizeof(AVPacket));
	//av_dump_format(m_FormatCtx, 0, m_filepath, 0);
	//申请转换上下文
	m_img_convert_ctx = sws_getContext(m_CodecCtx->width, m_CodecCtx->height, m_CodecCtx->pix_fmt,
		m_CodecCtx->width, m_CodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
	
	int ret = -1;
	//读取数据
	while (av_read_frame(m_FormatCtx, m_packet) >= 0)
	{
		if (m_packet->stream_index == m_videoindex)
		{
			ret = avcodec_decode_video2(m_CodecCtx, m_Frame, &m_got_picture, m_packet);
			if (ret < 0)
			{
				return -1;
			}

			if (m_got_picture >= 1)
			{
				//成功解码一帧
				sws_scale(m_img_convert_ctx, (const unsigned char* const*)m_Frame->data, m_Frame->linesize, 0, m_CodecCtx->height,
					m_FrameYUV->data, m_FrameYUV->linesize);//转换图像格式

				int y_size = m_CodecCtx->width*m_CodecCtx->height;
				m_ptr = new unsigned char[m_CodecCtx->width * m_CodecCtx->height * 3 / 2];
				unsigned char* p = m_ptr;
				void* uptr = p + y_size;
				void* vptr = p + y_size * 5 / 4;
				memcpy(p, m_FrameYUV->data[0], y_size);
				memcpy(uptr, m_FrameYUV->data[1], y_size / 4);
				memcpy(vptr, m_FrameYUV->data[2], y_size / 4);
				if (!m_render)m_render = new YUVRender();
				m_render->Init(m_ptr, m_CodecCtx->width, m_CodecCtx->height);
			}
			else
			{
				//未解码到一帧，可能时结尾B帧或延迟帧，在后面做flush decoder处理
			}
		}
		av_free_packet(m_packet);
	}
	return 0;
}

C264Decoder::~C264Decoder()
{
	if (m_img_convert_ctx)sws_freeContext(m_img_convert_ctx);
	if (m_FrameYUV)av_frame_free(&m_FrameYUV);
	if (m_Frame)av_frame_free(&m_Frame);
	if (m_CodecCtx)avcodec_close(m_CodecCtx);
	if (m_FormatCtx)avformat_close_input(&m_FormatCtx);
	if (m_ptr)
	{
		delete m_ptr;
		m_ptr = nullptr;
	}
	if (m_render->GetRenderWindow())
	{
		m_render->Stop();
	}
	if (m_render)
	{
		delete m_render;
		m_render = nullptr;
	}
	LOG_I("DECODE EXIT");
}
