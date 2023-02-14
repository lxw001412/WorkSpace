#include <WinSock2.h>
#include <stdio.h>
#include <time.h>
#include <process.h>
#include <list>
#include <string>
#include "CLog.h"
#pragma comment(lib,"ws2_32.lib")
using namespace std;

//根据 IP:PORT 来区分每个客户请求
struct tftpd_client
{
    unsigned int   c_ip;
    unsigned short c_port;
    ///
    int op_code;  // 1 read file; 2 write file
    string fullpath; //文件完整路径
    FILE* fp;
    bool  b_trans_complete; //传输是否成功完成
    time_t    last_time; //最后一次活动的时间，用来进行超时判断
    char buf[550];
    int  send_size; //重试的时候发生的数据包长度，一般都是 516，除非是文件结束 < 516
    unsigned short block_id; //当前的块ID
    ///

    void close() {
        if (fp) {
            fclose(fp); fp = NULL;
        }
        if (!b_trans_complete && op_code == 2) {//是否应该删除未上传成功的文件?
            remove(fullpath.c_str()); //删除未成功的文件
        }
        ///
        b_trans_complete = false;
        block_id = 0;
    }

    tftpd_client(int op_code, unsigned int c_ip, unsigned short c_port) {
        this->op_code = op_code;
        this->c_ip = c_ip;
        this->c_port = c_port;

        fp = NULL;
        b_trans_complete = false;
        last_time = time(0);
        block_id = 0;
    }
    ~tftpd_client() {
        close();
    }
};

struct tftpd
{
protected:
    bool quit;
    int sock;
    unsigned int bind_localip;
    int bind_localport;
    char root_path[512];

    ///
    list< tftpd_client* > clients;  //正在传输文件的客户端

    int create_tftpd_sock();
    void check_timeout();
    int recv_packet(char* buf, int len, sockaddr_in* from);
    void parse_packet(char* buf, int len, sockaddr_in* from);
    void start_transfile(tftpd_client* c, char* path, sockaddr_in* from);
    void reply_error(int err_code, const char* errmsg, sockaddr_in* from);
    void send_data_block(tftpd_client* c, sockaddr_in* from);
    void recv_data_block(tftpd_client* c, int block_id, char* data, int data_size, sockaddr_in* from);

    HANDLE hthread;
    static unsigned int __stdcall __tftpd_thread(void* p) {
        tftpd* svr = (tftpd*)p;
        svr->tftpd_loop();
        return 0;
    }
    void tftpd_loop();

public:
    tftpd();
    ~tftpd();
    ///
    int create(const char* strip, int port, const char* root_path);
};

tftpd::tftpd()
{
    quit = false;
    sock = -1;
    bind_localip = 0; bind_localport = 69; // TFTP
    strcpy(root_path, "/");
}
tftpd::~tftpd()
{
    quit = true;
    closesocket(sock);
    WaitForSingleObject(hthread, 30 * 000);
    CloseHandle(hthread);
    for (list<tftpd_client*>::iterator it = clients.begin(); it != clients.end(); ++it) {
        delete* it;
    }
    clients.clear();
	LOG_I("TFTP EXIT");
}

int tftpd::create_tftpd_sock()
{
    if (sock >= 0) closesocket(sock);

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
        return -1;
    sockaddr_in addr; memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET; addr.sin_port = htons(bind_localport); //
    memcpy(&addr.sin_addr, &bind_localip, 4);
    if (::bind(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        closesocket(sock);
        return -1;
    }
    return 0;
}

void tftpd::tftpd_loop()
{

    time_t last = time(0);
    while (!quit) {
        char buf[4096];
        sockaddr_in from;
        time_t cur = time(0);
        if (abs(cur - last) >= 10) {
            last = cur;
            check_timeout(); //定时超时检查
        }
        //
        int r = recv_packet(buf, 4096, &from);
        if (r < 4) { // TFTP 头必须 >= 4
            continue;
        }

        buf[4095] = 0; ///
        parse_packet(buf, r, &from);
        //
    }
}
int tftpd::recv_packet(char* buf, int len, sockaddr_in* from)
{
    ///
    while (!quit && sock < 0) {
        if (create_tftpd_sock() == 0)break;
        Sleep(3000);
    }
    ///
    fd_set rdst; FD_ZERO(&rdst);
    FD_SET(sock, &rdst);
    timeval tmo; tmo.tv_usec = 0; tmo.tv_sec = 4;
    int r = select(sock + 1, &rdst, NULL, NULL, &tmo);
    if (r < 0) {
        printf("UDP select error.\n");
        Sleep(200);
        return -1;
    }
    if (r == 0) {
        return 0;
    }
    int socklen = sizeof(sockaddr_in);
    r = recvfrom(sock, buf, 4096, 0, (sockaddr*)from, &socklen);
    if (r <= 0) {
        printf("UDP TFTP recvfrom error\n");
        return r;
    }
    return r;
}
/*****
分析TFTP的数据包，具体的协议格式可参阅 RFC文档
******/
void tftpd::parse_packet(char* buf, int len, sockaddr_in* from)
{
    unsigned int c_ip; memcpy(&c_ip, &from->sin_addr, 4);
    unsigned short c_port = from->sin_port;
    unsigned short op_code = ntohs(*(unsigned short*)buf);
    ///
    tftpd_client* c = NULL;
    list<tftpd_client*>::iterator it;
    for (it = clients.begin(); it != clients.end(); ++it) {
        tftpd_client* cc = *it;
        if (cc->c_ip == c_ip && cc->c_port == c_port) {
            c = cc;
            c->last_time = time(0); //刷新最后一次访问时间
            break;
        }
    }

    if (op_code == 1 || op_code == 2) { // get file 从服务器端拉取文件 //put file 上传文件到服务器端
        if (c) {
            c->close();//首先关闭原来的
            c->op_code = op_code;
        }
        else {
            c = new tftpd_client(op_code, c_ip, c_port);
            clients.push_back(c);
        }

        start_transfile(c, buf + 2, from); // 从 2 的位置开始是文件路径,可参看TFTP RFC

        return;
    }

    if (!c) {
        reply_error(1, "client not exists or timeout", from);
        return;
    }
    unsigned short block_id = ntohs(*(unsigned short*)(buf + 2));

    if (op_code == 3) { // DATA
        int data_size = len - 4; //
        if (c->op_code != 2) { //如果不是写请求，肯定是哪里出了问题
            printf("*** Shit OOPS.\n");
        }
        else {
            recv_data_block(c, block_id, buf + 4, data_size, from); //
        }
    }
    else if (op_code == 4) { // ACK
        if (c->op_code != 1) { //如果不是读请求，肯定是哪里出了问题
            printf("---Shit OOPS.\n");
        }
        else if (block_id == c->block_id) { //回答的包正确，才能进行下一个数据块的传输
            send_data_block(c, from);
        }
    }
}
void tftpd::reply_error(int err_code, const char* errmsg, sockaddr_in* from)
{
    char buf[1024];
    *(unsigned short*)buf = htons(5); //回答错误
    *(unsigned short*)(buf + 2) = htons(err_code); //错误码
    strcpy(buf + 4, errmsg);
    ///
    sendto(sock, buf, 4 + strlen(buf + 4) + 1, 0, (sockaddr*)from, sizeof(*from)); //
}
void tftpd::start_transfile(tftpd_client* c, char* filename, sockaddr_in* from)
{
    char path[512]; sprintf(path, "%s/%s", root_path, filename); printf("TRANS: %s\n", path);
    c->fullpath = path;

    FILE* fp = fopen(path, c->op_code == 1 ? "rb" : "wb"); //这里不考虑传输模式，都当成2进制文件传输
    if (!fp) {
        reply_error(1, "open file error", from);
        return;
    }
    char* buf = c->buf;
    c->block_id = 0;
    c->fp = fp;

    if (c->op_code == 1) { //准备发送数据块

        send_data_block(c, from);
    }
    else {

        *(unsigned short*)buf = htons(4); // ACK
        *(unsigned short*)(buf + 2) = 0;      // BLOCK ID=0

        sendto(sock, buf, 4, 0, (sockaddr*)from, sizeof(*from));
    }

}
void tftpd::recv_data_block(tftpd_client* c, int block_id, char* data, int data_size, sockaddr_in* from)
{
    if (c->block_id == block_id) { //这是重复上传的数据块,直接回复 ACK
    L:        *(unsigned short*)data = htons(4); // ACK
        *(unsigned short*)(data + 2) = htons(block_id);

        sendto(sock, data, 4, 0, (sockaddr*)from, sizeof(*from));

        return;
    }
    ///
    if (!c->fp) {
        reply_error(1, "file not opened.", from);
        return;
    }

    c->block_id = block_id;

    if (data_size > 0) {
        fwrite(data, 1, data_size, c->fp);
    }
    ///
    if (data_size < 512) { //文件已经上传完成
        c->b_trans_complete = true;
        fclose(c->fp); c->fp = NULL;
    }

    goto L;
}
void tftpd::send_data_block(tftpd_client* c, sockaddr_in* from)
{
    if (c->b_trans_complete) { //文件已经传输完成
        if (c->fp) {
            fclose(c->fp); c->fp = NULL;
        }
        return;
    }

    if (!c->fp) {
        reply_error(2, "file had closed", from);
        return;
    }

    char* buf = c->buf;
    *(unsigned short*)buf = htons(3); // DATA
    *(unsigned short*)(buf + 2) = htons(++c->block_id); ///
    int cnt = fread(buf + 4, 1, 512, c->fp);
    if (cnt < 0) {
        fclose(c->fp); c->fp = NULL;
        c->b_trans_complete = false;
        reply_error(3, "read file data error", from);
        return;
    }
    if (cnt != 512) { //传输完成，暂时不关闭 fp，因为等待检查最后的ACK到来,以及超时检查

        c->b_trans_complete = true;
    }
    ///
    c->send_size = 4 + cnt;
    sendto(sock, buf, 4 + cnt, 0, (sockaddr*)from, sizeof(*from));
}

void tftpd::check_timeout()
{
    list<tftpd_client*>::iterator it, tt;
    time_t cur = time(0);

    for (it = clients.begin(); it != clients.end(); ) {

        tftpd_client* c = *it;
        bool b_del = false;
        if (c->op_code == 1) { // get file
            if (c->fp && !c->b_trans_complete && abs(cur - c->last_time) >= 15) { //重发
                sockaddr_in to; memset(&to, 0, sizeof(to)); to.sin_family = AF_INET;
                to.sin_port = c->c_port;
                memcpy(&to.sin_addr, &c->c_ip, 4);
                sendto(sock, c->buf, c->send_size, 0, (sockaddr*)&to, sizeof(to));
                //
                printf("ReSend [%s] BlockID=%d.\n", c->fullpath.c_str(), c->block_id);
            }
            else if (abs(cur - c->last_time) >= 45) { //超过30秒
                b_del = true;
            }
        }
        else { // put file
            if (abs(cur - c->last_time) >= 45) {
                b_del = true;
            }
        }
        ///
        if (b_del) {
            tt = it; ++it;
            clients.erase(tt);
            printf("%s: [%s] %s\n", c->op_code == 1 ? "READ" : "WRITE", c->fullpath.c_str(), c->b_trans_complete ? "OK" : "FAIL");
            delete c;
        }
        else {
            ++it;
        }
    }
}


int tftpd::create(const char* strip, int port, const char* root_path)
{
    if (strip) {
        this->bind_localip = inet_addr(strip);
    }
    if (port > 0) this->bind_localport = port;
    strcpy(this->root_path, root_path);

    this->create_tftpd_sock();

    unsigned int tid;
    hthread = (HANDLE)_beginthreadex(NULL, 0, __tftpd_thread, this, 0, &tid);
    return 0;
}

void* tftpd_create(const char* strip, int port, const char* root_path)
{
    tftpd* svr = new tftpd;
    svr->create(strip, port, root_path);

    return svr;
}
void tftpd_close(void* handle)
{
    if (!handle)return;
    tftpd* svr = (tftpd*)handle;
    delete svr;
}
