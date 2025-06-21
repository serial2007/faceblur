#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
#include <iostream>

int blur(cv::Mat& frame, cv::Rect rect);

int main() {
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "无法打开摄像头 /dev/video0" << std::endl;
        return -1;
    }

    cv::CascadeClassifier face_cascade;
    if (!face_cascade.load("/usr/share/opencv4/haarcascades/haarcascade_frontalface_default.xml")) {
        std::cerr << "无法加载人脸检测器文件" << std::endl;
        return -1;
    }

    const std::string window_name = "Camera";
    cv::namedWindow(window_name, cv::WINDOW_AUTOSIZE);

    cv::Mat frame, gray;
	cv::Mat save;
    std::vector<cv::Rect> faces;

    while (true) {
        cap >> frame;
        if (frame.empty()) break;

        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
        cv::equalizeHist(gray, gray);

        // 检测人脸
        face_cascade.detectMultiScale(
			gray,
			faces,
			1.1,          // scaleFactor：每次图像尺寸减小的比例（越小越精确，但慢）
			14,            // minNeighbors：每个候选矩形保留前需要的相邻矩形数（越大越严格）
			0,            // flags（一般为0即可）
			cv::Size(60, 60)  // minSize：检测的最小人脸尺寸（避免误检小目标）
		);
		bool errflg=false;
        for (const auto& face : faces) {
			// cv::rectangle(frame, face, cv::Scalar(0, 255, 0), 2);
			if(blur(frame, face)<0){
				errflg=true;
			}
        }
		if(faces.size() > 0 && !errflg)
		{
			save = frame.clone();
		}
		if(faces.size() > 0 && !errflg)
        cv::imshow(window_name, frame);
		else if(!save.empty())
		cv::imshow(window_name, save);

        int key = cv::waitKey(1);
        if (key == 27 || cv::getWindowProperty(window_name, cv::WND_PROP_VISIBLE) < 1) {
            break;
        }
    }

    cap.release();
    cv::destroyAllWindows();
    return 0;
}
