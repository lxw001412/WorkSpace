#include "CThread.h"
#include <stdio.h>
struct AVFormatContext;
struct AVCodecContext;
struct AVCodec;
struct AVFrame;
struct AVPacket;
struct SwsContext;
class YUVRender;
class C264Decoder
{
public:
	C264Decoder(const char* filepath);
	int Init();
	~C264Decoder();
private:
	AVFormatContext	*m_FormatCtx;
	int	 m_videoindex;
	AVCodecContext	*m_CodecCtx;
	AVCodec			*m_Codec;
	AVFrame	*m_Frame, *m_FrameYUV;
	unsigned char *m_out_buffer;
	AVPacket *m_packet;
	int m_got_picture;
	SwsContext *m_img_convert_ctx;
	const char* m_filepath;
	unsigned char* m_ptr;
	YUVRender* m_render;
};