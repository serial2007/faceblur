#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
#include <iostream>
#include <string>

int blur(cv::Mat& frame, cv::Rect rect);
cv::Mat captureScreen();

bool blur_break = true;
bool use_screen = false;
bool use_camera = true;
bool single_picture = false;
bool captureWindow_window = false;
bool no_effect = false;
bool no_freeze = false;
bool random_strength = 1000000.0f;
int camera_number = 0;

int main(int argc, char* argv[]) {
	char * pic_path;
	for(int i=1; i<argc; i++)
	{
		if(strcmp(argv[i], "camera=") >= 0 && strcmp(argv[i], "camera>") < 0)
		{
			use_screen = false;
			use_camera = true;
			camera_number = atoi(argv[i] + 7);
		}
		else if(strcmp(argv[i], "picture=") >= 0 && strcmp(argv[i], "picture>") < 0)
		{
			use_screen = false;
			use_camera = false;
			single_picture = true;
			pic_path = argv[i] + 8;
			no_freeze = true;
		}
		else if(strcmp(argv[i], "screen") == 0 || strcmp(argv[i], "s") == 0)
		{
			use_screen = true;
			use_camera = false;
			camera_number = 0;
		}
		else if(strcmp(argv[i], "window") == 0 || strcmp(argv[i], "w") == 0)
		{
			use_screen = true;
			use_camera = false;
			captureWindow_window = true;
			camera_number = 0;
		}
		else if(strcmp(argv[i], "fullscreen") == 0 || strcmp(argv[i], "f") == 0)
		{
			use_screen = true;
			use_camera = false;
			captureWindow_window = false;
			camera_number = 0;
		}
		else if(strcmp(argv[i], "original") == 0 || strcmp(argv[i], "o") == 0)
		{
			no_effect = true;
			no_freeze = true;
		}
		else if(strcmp(argv[i], "unfreeze") == 0 || strcmp(argv[i], "u") == 0)
		{
			no_freeze = true;
		}
		else  {
			// if(strcmp(argv[i], "help")==0 || strcmp(argv[i], "h")==0)
			std::cout << "help:         获得帮助" << std::endl;
			std::cout << "camera=#:     使用摄像机 /dev/video#" << std::endl;
			std::cout << "picture=#:    处理单张图片，路径为#" << std::endl;
			std::cout << "screen:       使用屏幕" << std::endl;
			std::cout << "fullscreen:   使用全屏" << std::endl;
			std::cout << "window:       截取一个窗口" << std::endl;
			std::cout << "original:     不进行特效处理，用于调试目的" << std::endl;
			std::cout << "unfreeze:     在检测人脸失败后不冻结图像" << std::endl;
			return 0;
		}
	}
	
	cv::VideoCapture* cap;
	if(use_camera)
    {
		cap = new cv::VideoCapture(camera_number);
		if (!cap->isOpened()) {
			std::cerr << "无法打开摄像头 /dev/video" << camera_number << std::endl;
			return -1;
    	}
	}

    // cv::CascadeClassifier face_cascade;
    // if (!face_cascade.load("/usr/share/opencv4/haarcascades/haarcascade_frontalface_default.xml")) {
    //     std::cerr << "无法加载人脸检测器文件" << std::endl;
    //     return -1;
    // }

    const std::string window_name = "Camera";
    cv::namedWindow(window_name, cv::WINDOW_NORMAL);
	static int Significance = 50;
	cv::createTrackbar("Significance", window_name, &Significance, 100, nullptr);
	
    cv::Mat frame, gray;
	cv::Mat save;
    std::vector<cv::Rect> faces;
	cv::dnn::Net net = cv::dnn::readNetFromCaffe("deploy.prototxt", "res10_300x300_ssd_iter_140000.caffemodel");
	cv::Mat single_picture_mat;
	if(single_picture)
	{
		single_picture_mat = cv::imread(pic_path);
	}

    while (true) {
        if(use_camera)
		{
			*cap >> frame;
		}
		else if(use_screen)
		{
			frame = captureScreen();
		}
		else if(single_picture)
		{
			frame = single_picture_mat;
		}
        if (frame.empty()) break;

        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
        cv::equalizeHist(gray, gray);

        // 检测人脸
        // face_cascade.detectMultiScale(
		// 	gray,
		// 	faces,
		// 	1.1f + (float)scale_factor * 0.1,          // scaleFactor：每次图像尺寸减小的比例（越小越精确，但慢）
		// 	minNeighbors + 1,            // minNeighbors：每个候选矩形保留前需要的相邻矩形数（越大越严格）
		// 	0,            				// flags（一般为0即可）
		// 	cv::Size(min_size, min_size)  // minSize：检测的最小人脸尺寸（避免误检小目标）
		// );

cv::Mat blob = cv::dnn::blobFromImage(frame, 1.0, cv::Size(300, 300), cv::Scalar(104, 177, 123), false, false);
net.setInput(blob);
cv::Mat detections = net.forward();

cv::Mat detectionMat(detections.size[2], detections.size[3], CV_32F, detections.ptr<float>());
faces.clear();
for (int i = 0; i < detectionMat.rows; i++) {
    float confidence = detectionMat.at<float>(i, 2);
    if (confidence > (float)Significance * 0.01 + 0.01) {
        int x1 = static_cast<int>(detectionMat.at<float>(i, 3) * frame.cols);
        int y1 = static_cast<int>(detectionMat.at<float>(i, 4) * frame.rows);
        int x2 = static_cast<int>(detectionMat.at<float>(i, 5) * frame.cols);
        int y2 = static_cast<int>(detectionMat.at<float>(i, 6) * frame.rows);
        faces.push_back(cv::Rect(cv::Point(x1, y1), cv::Point(x2, y2)));
    }
}




		bool errflg=false;
		if(!no_effect)
        {
			for (const auto& face : faces) {
				// cv::rectangle(frame, face, cv::Scalar(0, 255, 0), 2);
				if(blur(frame, face)<0){
					errflg=true;
				}
        	}
		}
		if((faces.size() > 0 && !errflg) || no_effect)
		{
			save = frame.clone();
		}
		if((faces.size() > 0 && !errflg) || no_effect || no_freeze)
        cv::imshow(window_name, frame);
		else if(!save.empty())
		cv::imshow(window_name, save);

        int key = cv::waitKey(1);
        if (key == 27 || cv::getWindowProperty(window_name, cv::WND_PROP_VISIBLE) < 1) {
            break;
        }
    }
	if(use_camera)
    {
		cap->release();
		delete cap;
	}
    cv::destroyAllWindows();
    return 0;
}
