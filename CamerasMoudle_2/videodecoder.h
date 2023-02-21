#include <string>
#include <map>
struct AVFrame;
struct AVFormatContext;
struct AVCodecContext;
struct AVCodec;
struct AVFrame;
struct AVPacket;
struct SwsContext;
class CH264Decoder
{
public:
	CH264Decoder(const char* filepath);
	CH264Decoder(const CH264Decoder&) = delete;
	CH264Decoder& operator = (const CH264Decoder&) = delete;
	int decode_h264();
	void Close();
	~CH264Decoder();
protected:
	void Addfilepath(std::string filename);
	void Deletefile();
	int Frame2JPG(AVFrame* pFrame_, int width, int height);
private:
	const char* m_filepath;
	AVCodec            *m_pCodec;
	AVFrame            *m_pFrame, *m_pFrameYUV;
	AVPacket           *m_packet;
	SwsContext         *m_img_convert_ctx;
	AVFormatContext    *m_pFormatCtx;
	AVCodecContext     *m_pCodecCtx;
	std::map<std::string, int> m_imgpath;
};