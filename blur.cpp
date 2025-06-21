#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
#include <iostream>
#include <vector>
#include <chrono>

int blur(cv::Mat& frame, cv::Rect rect)
{	
	auto frame_old = frame.clone();
    rect = rect & cv::Rect(0, 0, frame.cols, frame.rows);
    if (rect.width <= 20 || rect.height <= 20) return -1;
	
	static bool init = true;
	static int arrlen = frame.cols * frame.rows / 180;
	static std::vector<cv::Point2i> arr; 
	static std::vector<float> pos;
	static auto _last_time = std::chrono::steady_clock::now();
	auto _now_time 		   = std::chrono::steady_clock::now();
	if(init){
		init = false;
		srand(time(0));
		for(int i=0; i<arrlen; i++)
		{
			arr.push_back(cv::Point2i{rand()%frame.cols, rand()%frame.rows});
			pos.push_back(1e10f);
		}
	}
	else{
		for(auto& i : arr)
		{
			if(rand() % 50 == 0)
			{
				i.x = (i.x + rand() % 3 - 1) % frame.cols;
				i.y = (i.y + rand() % 3 - 1) % frame.rows;
			}
		}
	}
	
	for(int i=0; i<arr.size()-2; i+=2)
	{
		if(rect.contains(arr[i]) && rect.contains(arr[i+1]))
		{
			pos[i] = 0.0f;
		}else
		{
			float us = std::chrono::duration<float, std::micro>(_now_time - _last_time).count();
			pos[i] += us * float(rand() % 1024) / 1024;
			if(pos[i] > 1e10) pos[i] = 1e10;
		}
	}
	// cv::rectangle(frame, rect, color, thickness);
	for(int i=0; i<arr.size()-2; i+=2)
	{
		if(pos[i] < 100000)
		{
			cv::Rect2i r = {arr[i], arr[i+1]};
			auto color = frame_old.at<cv::Vec3b>({(arr[i].x+arr[i+2].x)/2, (arr[i].y+arr[i+2].y)/2});
			cv::rectangle(frame, r, color, -1);
		}
	}
	_last_time = _now_time;
    return 0;
}