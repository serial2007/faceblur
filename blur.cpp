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
	static int _frame_rows = frame.rows;
	static int _frame_cols = frame.cols;
	static int64_t arrlen;
	static std::vector<cv::Point2i> arr; 
	static std::vector<float> pos;
	static auto _last_time = std::chrono::steady_clock::now();
	auto _now_time 		   = std::chrono::steady_clock::now();
	if(_frame_rows != frame.rows || _frame_cols != frame.cols)
	{
		init = true;
		_frame_rows = frame.rows;
		_frame_cols = frame.cols;
	}
	if(init){
		init = false;
		arrlen = (int64_t)frame.cols * frame.rows * frame.cols * frame.rows / 180 / 1280 / 720;
		srand(time(0));
		arr.clear();
		pos.clear();
		for(int64_t i=0; i<arrlen; i++)
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
	
	int rec_count = 0;
	for(int i=0; i<arr.size()-2; i+=2)
	{
		if(rect.contains(arr[i]) && rect.contains(arr[i+1]))
		{
			rec_count++;
			pos[i] = 0.0f;
		}else
		{
			float us = std::chrono::duration<float, std::micro>(_now_time - _last_time).count();
			pos[i] += us * float(rand() % 1024) / 1024;
			if(pos[i] > 1e10) pos[i] = 1e10;
		}
	}
	if(rec_count < 5)
		return -1;
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