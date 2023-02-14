#include <WinSock2.h>
#include <stdio.h>
#include <time.h>
#include <process.h>
#include <list>
#include <string>
#pragma comment(lib,"ws2_32.lib")
using namespace std;

//���� IP:PORT ������ÿ���ͻ�����
struct tftpd_client
{
    unsigned int   c_ip;
    unsigned short c_port;
    ///
    int op_code;  // 1 read file; 2 write file
    string fullpath; //�ļ�����·��
    FILE* fp;
    bool  b_trans_complete; //�����Ƿ�ɹ����
    time_t    last_time; //���һ�λ��ʱ�䣬�������г�ʱ�ж�
    char buf[550];
    int  send_size; //���Ե�ʱ���������ݰ����ȣ�һ�㶼�� 516���������ļ����� < 516
    unsigned short block_id; //��ǰ�Ŀ�ID
    ///

    void close() {
        if (fp) {
            fclose(fp); fp = NULL;
        }
        if (!b_trans_complete && op_code == 2) {//�Ƿ�Ӧ��ɾ��δ�ϴ��ɹ����ļ�?
            remove(fullpath.c_str()); //ɾ��δ�ɹ����ļ�
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
    list< tftpd_client* > clients;  //���ڴ����ļ��Ŀͻ���

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
    if (bind(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
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
            check_timeout(); //��ʱ��ʱ���
        }
        //
        int r = recv_packet(buf, 4096, &from);
        if (r < 4) { // TFTP ͷ���� >= 4
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
����TFTP�����ݰ��������Э���ʽ�ɲ��� RFC�ĵ�
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
            c->last_time = time(0); //ˢ�����һ�η���ʱ��
            break;
        }
    }

    if (op_code == 1 || op_code == 2) { // get file �ӷ���������ȡ�ļ� //put file �ϴ��ļ�����������
        if (c) {
            c->close();//���ȹر�ԭ����
            c->op_code = op_code;
        }
        else {
            c = new tftpd_client(op_code, c_ip, c_port);
            clients.push_back(c);
        }

        start_transfile(c, buf + 2, from); // �� 2 ��λ�ÿ�ʼ���ļ�·��,�ɲο�TFTP RFC

        return;
    }

    if (!c) {
        reply_error(1, "client not exists or timeout", from);
        return;
    }
    unsigned short block_id = ntohs(*(unsigned short*)(buf + 2));

    if (op_code == 3) { // DATA
        int data_size = len - 4; //
        if (c->op_code != 2) { //�������д���󣬿϶��������������
            printf("*** Shit OOPS.\n");
        }
        else {
            recv_data_block(c, block_id, buf + 4, data_size, from); //
        }
    }
    else if (op_code == 4) { // ACK
        if (c->op_code != 1) { //������Ƕ����󣬿϶��������������
            printf("---Shit OOPS.\n");
        }
        else if (block_id == c->block_id) { //�ش�İ���ȷ�����ܽ�����һ�����ݿ�Ĵ���
            send_data_block(c, from);
        }
    }
}
void tftpd::reply_error(int err_code, const char* errmsg, sockaddr_in* from)
{
    char buf[1024];
    *(unsigned short*)buf = htons(5); //�ش����
    *(unsigned short*)(buf + 2) = htons(err_code); //������
    strcpy(buf + 4, errmsg);
    ///
    sendto(sock, buf, 4 + strlen(buf + 4) + 1, 0, (sockaddr*)from, sizeof(*from)); //
}
void tftpd::start_transfile(tftpd_client* c, char* filename, sockaddr_in* from)
{
    char path[512]; sprintf(path, "%s/%s", root_path, filename); printf("TRANS: %s\n", path);
    c->fullpath = path;

    FILE* fp = fopen(path, c->op_code == 1 ? "rb" : "wb"); //���ﲻ���Ǵ���ģʽ��������2�����ļ�����
    if (!fp) {
        reply_error(1, "open file error", from);
        return;
    }
    char* buf = c->buf;
    c->block_id = 0;
    c->fp = fp;

    if (c->op_code == 1) { //׼���������ݿ�

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
    if (c->block_id == block_id) { //�����ظ��ϴ������ݿ�,ֱ�ӻظ� ACK
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
    if (data_size < 512) { //�ļ��Ѿ��ϴ����
        c->b_trans_complete = true;
        fclose(c->fp); c->fp = NULL;
    }

    goto L;
}
void tftpd::send_data_block(tftpd_client* c, sockaddr_in* from)
{
    if (c->b_trans_complete) { //�ļ��Ѿ��������
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
    if (cnt != 512) { //������ɣ���ʱ���ر� fp����Ϊ�ȴ��������ACK����,�Լ���ʱ���

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
            if (c->fp && !c->b_trans_complete && abs(cur - c->last_time) >= 15) { //�ط�
                sockaddr_in to; memset(&to, 0, sizeof(to)); to.sin_family = AF_INET;
                to.sin_port = c->c_port;
                memcpy(&to.sin_addr, &c->c_ip, 4);
                sendto(sock, c->buf, c->send_size, 0, (sockaddr*)&to, sizeof(to));
                //
                printf("ReSend [%s] BlockID=%d.\n", c->fullpath.c_str(), c->block_id);
            }
            else if (abs(cur - c->last_time) >= 45) { //����30��
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
