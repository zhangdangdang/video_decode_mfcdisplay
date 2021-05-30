
// MFC_displayDlg.cpp: 实现文件
//
#pragma comment(lib ,"SDL2.lib")
#pragma comment(lib ,"SDL2main.lib")
#include "pch.h"
#include "framework.h"
#include "MFC_display.h"
#include "MFC_displayDlg.h"
#include "afxdialogex.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>

#include <atomic>
using namespace std;
#define __STDC_CONSTANT_MACROS
extern "C"
{
#include <SDL.h>
#include <SDL_main.h>
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
};
using namespace std;
#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CMFCdisplayDlg 对话框



CMFCdisplayDlg::CMFCdisplayDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_MFC_DISPLAY_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMFCdisplayDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT1, m_url);
}

BEGIN_MESSAGE_MAP(CMFCdisplayDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_start, &CMFCdisplayDlg::OnBnClickedstart)
	ON_BN_CLICKED(IDC_pause, &CMFCdisplayDlg::OnBnClickedpause)
	ON_BN_CLICKED(IDC_stop, &CMFCdisplayDlg::OnBnClickedstop)
	ON_BN_CLICKED(IDC_openfile, &CMFCdisplayDlg::OnBnClickedopenfile)
	ON_BN_CLICKED(ID_about, &CMFCdisplayDlg::OnBnClickedabout)
END_MESSAGE_MAP()

const int bpp = 12;

int screen_w = 500, screen_h = 500;
const int pixel_w = 320, pixel_h = 180;

unsigned char buffer[pixel_w*pixel_h*bpp / 8];

//Refresh Event
#define REFRESH_EVENT  (SDL_USEREVENT + 1)
//Break
#define BREAK_EVENT  (SDL_USEREVENT + 2)

int thread_exit = 0;
int ch;
atomic<bool> thread_pause(true);
//bool flag = true;
//int zanting_video(void *opaque)
//{
//	while (1) {
//		if (_kbhit)//如果有按键按下
//		{
//			ch = _getch();//使用_getch()函数获取按下的键值
//			if (ch == 32)
//			{
//				flag = !flag;
//				cout << "zanting" << flag << endl;
//				printf("按下");
//			}
//		}
//	}
//}
int refresh_video(void *opaque) {
	thread_exit = 0;
	while (thread_exit == 0) {

		if (thread_pause) {
			//ut << flag << endl;
			SDL_Event event;
			event.type = REFRESH_EVENT;
			SDL_PushEvent(&event);
			SDL_Delay(40);
		}
	}
	thread_exit = 0;
	//Break
	SDL_Event event;
	event.type = BREAK_EVENT;
	SDL_PushEvent(&event); 
	return 0;
}

UINT ffmpeg_display(LPVOID lpParam)
{
	/*...................decode.....................*/
	AVFormatContext	*pFormatCtx = nullptr;
	int				i, videoindex;
	int y_size;
	int ret, got_picture;
	//struct SwsContext *img_convert_ctx;
	//输入文件路径

	CMFCdisplayDlg *dlg = (CMFCdisplayDlg*)lpParam;
	char filepath[500] = { 0 };
	GetWindowTextA(dlg->m_url, (LPSTR)filepath, 500);
	//strcpy(filepath, argv[1]);
	//char filepath[] = "屌丝男士.mov";
	//char *filepath = argv[1];
	int frame_cnt;

	//av_register_all();
	/*加载socket库以及网络加密协议相关的库，为后续使用网络相关提供支持*/
	avformat_network_init();//网络功能的代码
	pFormatCtx = avformat_alloc_context();

	if (avformat_open_input(&pFormatCtx, filepath, NULL, NULL) != 0) {
		printf("Couldn't open input stream.\n");
		return -1;
	}
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
		printf("Couldn't find stream information.\n");
		return -1;
	}
	av_dump_format(pFormatCtx, 0, filepath, 0);
	videoindex = -1;


	//获取视频流 获取视频流的数组编号，判断是否是视频
	videoindex = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
	if (videoindex < 0) {
		//av_log(nullptr, AV_LOG_ERROR, "Can not find video stream!\n");
		avformat_close_input(&pFormatCtx);
		return -1;
	}

	AVCodecParameters* videoParameters = pFormatCtx->streams[videoindex]->codecpar;//一般streams数组有两个，一个视频流一个音频流
	//查找编码器
	const AVCodec* pCodec = avcodec_find_decoder(videoParameters->codec_id);
	if (!pCodec) {
		//av_log(nullptr, AV_LOG_ERROR, "Find video decoder failed!\n");
		avformat_close_input(&pFormatCtx);
		return -1;
	}

	// 创建一个视频编码器上下文
	AVCodecContext* pCodecCtx = avcodec_alloc_context3(pCodec);
	if (!pCodecCtx) {
		//av_log(nullptr, AV_LOG_ERROR, "Create audio_ctx failed!\n");
		avformat_close_input(&pFormatCtx);
		return -1;
	}

	// 将原始的视频参数复制到新创建的上下文中
	ret = avcodec_parameters_to_context(pCodecCtx, videoParameters);
	if (ret < 0) {
		//av_log(nullptr, AV_LOG_ERROR, "videoParameters to context failed!\n");
		avformat_close_input(&pFormatCtx);
		return -1;
	}


	// 打开视频频解码器
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
		printf("Could not open codec.\n");
		return -1;
	}
	/*
	 * 在此处添加输出视频信息的代码
	 * 取自于pFormatCtx，使用fprintf()
	 */
	printf("duration %d\n", pFormatCtx->duration);
	printf("iformat %s\n", pFormatCtx->iformat->name);
	printf("iformat %d*%d\n", pFormatCtx->streams[videoindex]->codecpar->width, pCodecCtx->height);

	AVFrame	*pFrame = av_frame_alloc();//AVFrame装的是解码后的yuv图像数据
	AVFrame	*pFrameYUV = av_frame_alloc();
	uint8_t *out_buffer = (uint8_t *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1));
	av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1);
	AVPacket *packet = av_packet_alloc();//pack装的是h264的数据
	if (packet == NULL) {
		printf("packet Error.\n");
	}
	//av_init_packet(packet);
	//Output Info-----------------------------
	printf("--------------- File Information ----------------\n");
	//av_dump_format(pFormatCtx,0,filepath,0);
	printf("-------------------------------------------------\n");
	struct SwsContext *img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
		pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

	frame_cnt = 0;

	//fstream outfile("t.h264", ios::out| ios::binary);
   //ofstream out("tst.h264", ios::out);

	FILE *fp = nullptr;
	FILE* fp_yuv = nullptr;
	fopen_s(&fp, "tseat.264", "wb+");
	fopen_s(&fp_yuv, "tseat.yuv", "wb+");
	if (fp_yuv == 0) {//检查文件是否正确打开 文件如果被其他文件占用，会打开失败，如果不判断，会造成下面代码运行时报错
		printf("erro point fp_yuv");
		return -1;
	}
	if (fp == 0) {//检查文件是否正确打开 文件如果被其他文件占用，会打开失败，如果不判断，会造成下面代码运行时报错
		printf("erro point fp");
		return -1;
	}
	/*...................SDL.....................*/
	if (SDL_Init(SDL_INIT_VIDEO)) {
		printf("Could not initialize SDL - %s\n", SDL_GetError());
		return -1;
	}

	SDL_Window *screen;
	//SDL 2.0 Support for multiple windows
	screen_w = pCodecCtx->width;
	screen_h = pCodecCtx->height;
	/*screen = SDL_CreateWindow("Simplest Video Play SDL2", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		screen_w, screen_h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);*/
	screen = SDL_CreateWindowFrom(dlg->GetDlgItem(IDC_SCREEN)->GetSafeHwnd());
	if (!screen) {
		printf("SDL: could not create window - exiting:%s\n", SDL_GetError());
		return -1;
	}
	SDL_Renderer* sdlRenderer = SDL_CreateRenderer(screen, -1, 0);

	Uint32 pixformat = 0;
	//IYUV: Y + U + V  (3 planes)
	//YV12: Y + V + U  (3 planes)
	pixformat = SDL_PIXELFORMAT_IYUV;

	SDL_Texture* sdlTexture = SDL_CreateTexture(sdlRenderer, pixformat, SDL_TEXTUREACCESS_STREAMING, pCodecCtx->width, pCodecCtx->height);

	/*FILE *fp = NULL;
	fopen_s(&fp, "test_yuv420p_320x180.yuv", "rb+");

	if (fp == NULL) {
		printf("cannot open this file\n");
		return -1;
	}*/

	SDL_Rect sdlRect;

	SDL_Thread *refresh_thread = SDL_CreateThread(refresh_video, NULL, NULL);
	//SDL_Thread *zanting_thread = SDL_CreateThread(zanting_video, NULL, NULL);
	SDL_Event event;
	while (1) {
		//Wait
		SDL_WaitEvent(&event);
		if (event.type == REFRESH_EVENT) {
			while (1)//不是所有帧都是图像帧
			{
				if (av_read_frame(pFormatCtx, packet) < 0)
					thread_exit = 1;
				if (packet->stream_index == videoindex)
					break;
			}
			if (packet->stream_index == videoindex) {
				//fwrite(packet->data, 1, packet->size, fp);

				if (packet == NULL) {
					printf("packet Error.\n");
				}
				ret = avcodec_send_packet(pCodecCtx, packet);// 发送数据到ffmepg，放到解码队列中
				//将成功的解码队列中取出1个frame
				got_picture = avcodec_receive_frame(pCodecCtx, pFrame); //got_picture = 0 success, a frame was returned
				//fflush(stdout);
				if (ret < 0) {
					printf("Decode Error.\n");
					return -1;
				}
				if (!got_picture) {//md sb瞎jb改api 返回got——picture为0才是对的，以前可能是返回一
					sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
						pFrameYUV->data, pFrameYUV->linesize);
					//printf("Decoded frame index: %d\n", frame_cnt);
					//fwrite(pFrameYUV->data[0], 1, pCodecCtx->width*pCodecCtx->height, fp_yuv);
					//fwrite(pFrameYUV->data[1], 1, pCodecCtx->width*pCodecCtx->height / 4, fp_yuv);//YUV 4:2 : 0采样，每四个Y共用一组UV分量
					//fwrite(pFrameYUV->data[2], 1, pCodecCtx->width*pCodecCtx->height / 4, fp_yuv);
					/*
					 * 在此处添加输出YUV的代码
					 * 取自于pFrameYUV，使用fwrite()
					 */
					 /*..................SDL....................*/

		 //if (fread(buffer, 1, pixel_w*pixel_h*bpp / 8, fp) != pixel_w * pixel_h*bpp / 8) {
		 //	// Loop
		 //	fseek(fp, 0, SEEK_SET);
		 //	fread(buffer, 1, pixel_w*pixel_h*bpp / 8, fp);
		 //}

					SDL_UpdateTexture(sdlTexture, NULL, pFrameYUV->data[0], pFrameYUV->linesize[0]);

					//FIX: If window is resize
					sdlRect.x = 0;
					sdlRect.y = 0;
					sdlRect.w = screen_w;
					sdlRect.h = screen_h;

					SDL_RenderClear(sdlRenderer);
					SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, &sdlRect);
					SDL_RenderPresent(sdlRenderer);

				}


				frame_cnt++;

			}
			av_frame_unref(pFrame);
		}
		else if (event.type == SDL_KEYDOWN) {
			if (event.key.keysym.sym == SDLK_SPACE)
			{
				/*thread_pause = !thread_pause;*/
			}
		}
		else if (event.type == SDL_WINDOWEVENT) {
			//If Resize
			SDL_GetWindowSize(screen, &screen_w, &screen_h);
		}
		else if (event.type == SDL_QUIT) {
			thread_exit = 1;
		}
		else if (event.type == BREAK_EVENT) {
			break;
		}

	}
	SDL_Quit();
	/*SDL的bug 停止后不能重新播放   */
	dlg->GetDlgItem(IDC_SCREEN)->ShowWindow(SW_SHOWNORMAL);
	sws_freeContext(img_convert_ctx);
	fclose(fp);
	fclose(fp_yuv);
	av_frame_free(&pFrameYUV);
	av_frame_free(&pFrame);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);
	
	return 0;
}


// CMFCdisplayDlg 消息处理程序

BOOL CMFCdisplayDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CMFCdisplayDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CMFCdisplayDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		//CDialogEx::OnPaint();
		CPaintDC   dc(this);
		CRect rect;
		GetClientRect(&rect);
		CDC   dcMem;
		dcMem.CreateCompatibleDC(&dc);
		CBitmap   bmpBackground;
		bmpBackground.LoadBitmap(IDB_BITMAP2);  //对话框的背景图片  

		BITMAP   bitmap;
		bmpBackground.GetBitmap(&bitmap);
		CBitmap   *pbmpOld = dcMem.SelectObject(&bmpBackground);
		dc.StretchBlt(0, 0, rect.Width(), rect.Height(), &dcMem, 0, 0, bitmap.bmWidth, bitmap.bmHeight, SRCCOPY);
		
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CMFCdisplayDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


//播放按钮
void CMFCdisplayDlg::OnBnClickedstart()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strl;
	m_url.GetWindowText(strl);
	if(strl.IsEmpty())
	{
		AfxMessageBox("请输入文件");
		return ;
	}
	else
	{
		AfxBeginThread(ffmpeg_display, this);
	}
	//AfxMessageBox(strl);
	//ffmpeg_display();
	//AfxMessageBox(_T("hello world"));
}

//暂停
void CMFCdisplayDlg::OnBnClickedpause()
{
	// TODO: 在此添加控件通知处理程序代码
	thread_pause = !thread_pause;
}

//停止
void CMFCdisplayDlg::OnBnClickedstop()
{
	// TODO: 在此添加控件通知处理程序代码
	thread_exit = 1;
}

//打开文件
void CMFCdisplayDlg::OnBnClickedopenfile()
{
	// TODO: 在此添加控件通知处理程序代码
	//CString strl;
	////strl.Format(_T("%s"), avcodec_configuration());
	//m_url.GetWindowText(strl);
	//AfxMessageBox(strl);
	CString filepathname;
	CFileDialog dlg(TRUE, NULL,NULL,NULL,NULL);
	if (dlg.DoModal() == IDOK)
	{
		filepathname = dlg.GetPathName();
		m_url.SetWindowTextA(filepathname);
	}
}

//关于
void CMFCdisplayDlg::OnBnClickedabout()
{
	// TODO: 在此添加控件通知处理程序代码
	CAboutDlg dlg1;
	dlg1.DoModal();
}
