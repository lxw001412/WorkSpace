#include "Unicode.h"
#include "videodecoder.h"
#define __STDC_CONSTANT_MACROS
//Windows
extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavutil/error.h>
};
CH264Decoder::CH264Decoder(const char* inpath)
{
	m_filepath = inpath;
	m_pCodec = nullptr;
	m_pFrame = nullptr,
    m_pFrameYUV = nullptr;
	m_packet = nullptr;
	m_img_convert_ctx = nullptr;
	m_pFormatCtx = nullptr;
	m_pCodecCtx = nullptr;
	avcodec_register_all();
}

CH264Decoder::~CH264Decoder()
{
	Close();
	Deletefile();
}

int CH264Decoder::decode_h264()
{
	uint8_t *out_buffer;
	int    videoindex = -1;
	int y_size;
	int ret, got_picture;

	m_pFormatCtx = avformat_alloc_context();
	if (avformat_open_input(&m_pFormatCtx, m_filepath, NULL, NULL) != 0)
	{
		return -1;
	}
	if (avformat_find_stream_info(m_pFormatCtx, NULL) < 0)
	{
		return -1;
	}

	for (int i = 0; i < m_pFormatCtx->nb_streams; i++)
	{
		if (m_pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoindex = i;
			break;
		}
	}
	if (videoindex == -1)
	{
		return -1;
	}

	m_pCodecCtx = m_pFormatCtx->streams[videoindex]->codec;

	// 根据编码器的ID查找FFmpeg的解码器
	m_pCodec = avcodec_find_decoder(m_pCodecCtx->codec_id);
	if (m_pCodec == NULL)
	{
		return -1;
	}
	// 初始化一个视音频编解码器的AVCodecContext
	if (avcodec_open2(m_pCodecCtx, m_pCodec, NULL) < 0)
	{
		return -1;
	}

	// 一些内存分配
	m_packet = (AVPacket *)av_malloc(sizeof(AVPacket));
	if (!m_packet)return -1;
	m_pFrame = av_frame_alloc();
	if (!m_pFrame)return -1;
	m_pFrameYUV = av_frame_alloc();
	if (!m_pFrameYUV)return -1;
	out_buffer = (uint8_t *)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, m_pCodecCtx->width, m_pCodecCtx->height));
	if (!out_buffer)return -1;
	// 为已经分配空间的结构体AVPicture挂上一段用于保存数据的空间
	// AVFrame/AVPicture有一个data[4]的数据字段,buffer里面存放的只是yuv这样排列的数据，
	// 而经过fill 之后，会把buffer中的yuv分别放到data[0],data[1],data[2]中。
	avpicture_fill((AVPicture *)m_pFrameYUV, out_buffer, AV_PIX_FMT_YUV420P, m_pCodecCtx->width, m_pCodecCtx->height);
	// 初始化一个SwsContext
	// 参数：源图像的宽，源图像的高，源图像的像素格式，目标图像的宽，目标图像的高，目标图像的像素格式，设定图像拉伸使用的算法
	m_img_convert_ctx = sws_getContext(m_pCodecCtx->width, m_pCodecCtx->height, m_pCodecCtx->pix_fmt,
		m_pCodecCtx->width, m_pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
	if (!m_img_convert_ctx)return -1;
	int re;
	while (av_read_frame(m_pFormatCtx, m_packet) >= 0)
	{
		if (m_packet->stream_index == videoindex)
		{
			// 解码一帧视频数据。输入一个压缩编码的结构体AVPacket，输出一个解码后的结构体AVFrame
			ret = avcodec_decode_video2(m_pCodecCtx, m_pFrame, &got_picture, m_packet);
			if (ret < 0)
			{
				return -1;
			}
			if (got_picture)
			{
				// 转换像素
				sws_scale(m_img_convert_ctx, (const uint8_t* const*)m_pFrame->data, m_pFrame->linesize, 0, m_pCodecCtx->height,
					m_pFrameYUV->data, m_pFrameYUV->linesize);
				   y_size = m_pCodecCtx->width * m_pCodecCtx->height;
			}
		}
		re = Frame2JPG(m_pFrameYUV, m_pCodecCtx->width, m_pCodecCtx->height);
		av_free_packet(m_packet);
	}
	Close();
	return re;
}

void CH264Decoder::Close()
{
	if (m_img_convert_ctx)
	{
		sws_freeContext(m_img_convert_ctx);
		m_img_convert_ctx = nullptr;
	}
	if (m_pFrameYUV)
	{
		av_frame_free(&m_pFrameYUV);
		m_pFrameYUV = nullptr;
	}
	if (m_pFrame)
	{
		av_frame_free(&m_pFrame);
		m_pFrame = nullptr;
	}
	if (m_pCodecCtx)
	{
		avcodec_close(m_pCodecCtx);
		m_pCodecCtx = nullptr;
	}
	if (m_pFormatCtx)
	{
		avformat_close_input(&m_pFormatCtx);
		m_pFormatCtx = nullptr;
	}
}

int CH264Decoder::Frame2JPG(AVFrame* pFrame_, int width, int height) {
	AVCodec *pCodec;
	AVCodecContext *pCodecCtx = NULL;
	int i, ret, got_output;
	FILE *fp_out;
	AVFrame *pFrame;
	AVPacket pkt;
	int y_size;
	int framecnt = 0;
	struct SwsContext *img_convert_ctx;
	AVFrame *pFrame2;

	AVCodecID codec_id = AV_CODEC_ID_MJPEG;
	int framenum = 1;

	pCodec = avcodec_find_encoder(codec_id);
	if (!pCodec) {
		return -1;
	}
	pCodecCtx = avcodec_alloc_context3(pCodec);
	if (!pCodecCtx) {
		return -1;
	}
	pCodecCtx->bit_rate = 4000000;
	pCodecCtx->width = width;
	pCodecCtx->height = height;
	pCodecCtx->time_base.num = 1;
	pCodecCtx->time_base.den = 11;
	pCodecCtx->gop_size = 75;
	pCodecCtx->pix_fmt = AV_PIX_FMT_YUVJ420P;

	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
		return -1;
	}

	pFrame = av_frame_alloc();
	if (!pFrame) {
		return -1;
	}
	pFrame->format = pCodecCtx->pix_fmt;
	pFrame->width = pCodecCtx->width;
	pFrame->height = pCodecCtx->height;

	ret = av_image_alloc(pFrame->data, pFrame->linesize, pCodecCtx->width, pCodecCtx->height,
		pCodecCtx->pix_fmt, 16);
	if (ret < 0) {
		return -1;
	}

	pFrame2 = av_frame_alloc();
	if (!pFrame) {
		return -1;
	}
	pFrame2->format = AV_PIX_FMT_YUV420P;
	pFrame2->width = pCodecCtx->width;
	pFrame2->height = pCodecCtx->height;

	ret = av_image_alloc(pFrame2->data, pFrame2->linesize, pCodecCtx->width, pCodecCtx->height,
		AV_PIX_FMT_YUV420P, 16);
	if (ret < 0) {
		return -1;
	}

	img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUVJ420P, SWS_BICUBIC, NULL, NULL, NULL);
	if (!img_convert_ctx) return -1;
	//Output bitstream
	CString strpath = GetCurPath() + C2CString("test.jpg");
	Addfilepath(CString2C(strpath));
	fp_out = fopen(CString2C(strpath), "wb+");
	if (!fp_out) {
		return -1;
	}
	y_size = pCodecCtx->width * pCodecCtx->height;
	//Encode
	for (i = 0; i < framenum; i++) {
		av_init_packet(&pkt);
		pkt.data = NULL;    // packet data will be allocated by the encoder
		pkt.size = 0;
		//Read raw YUV data
		memcpy(pFrame2->data[0], pFrame_->data[0], y_size);
		memcpy(pFrame2->data[1], pFrame_->data[1], y_size / 4);
		memcpy(pFrame2->data[2], pFrame_->data[2], y_size / 4);
		sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame2->data, pFrame2->linesize, 0, pCodecCtx->height, pFrame->data, pFrame->linesize);

		pFrame->pts = i;
		/* encode the image */
		ret = avcodec_encode_video2(pCodecCtx, &pkt, pFrame, &got_output);
		if (ret < 0) {
			return -1;
		}
		if (got_output) {
			framecnt++;
			fwrite(pkt.data, 1, pkt.size, fp_out);
			av_free_packet(&pkt);
		}
	}
	//Flush Encoder
	for (got_output = 1; got_output; i++) {
		ret = avcodec_encode_video2(pCodecCtx, &pkt, NULL, &got_output);
		if (ret < 0) {
			return -1;
		}
		if (got_output) {
			fwrite(pkt.data, 1, pkt.size, fp_out);
			av_free_packet(&pkt);
		}
	}
	fclose(fp_out);
	if (pCodecCtx)
	{
		avcodec_close(pCodecCtx);
		av_free(pCodecCtx);
		pCodecCtx = nullptr;
	}
	if (pFrame->data[0])
	{
		av_freep(&pFrame->data[0]);
	}
	if (pFrame)
	{
		av_frame_free(&pFrame);
		pFrame = nullptr;
	}
	if (pFrame2)
	{
		av_frame_free(&pFrame2);
		pFrame2 = nullptr;
	}
	return 0;
}

void CH264Decoder::Addfilepath(std::string filename)
{
	if (filename.empty())return;
	if (m_imgpath.empty())
	{
		m_imgpath[filename]++;
		return;
	}
	for (const auto file : m_imgpath)
	{
		if (file.first == filename) return;
		else m_imgpath[filename]++;
	}
}

void CH264Decoder::Deletefile()
{
	if (m_imgpath.empty())return;
	for (const auto file : m_imgpath)
	{
		CString str = C2CString(file.first.c_str());
		SetFileAttributes(str, FILE_ATTRIBUTE_NORMAL);
		DeleteFile(str);
	}
}