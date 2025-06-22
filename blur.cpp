#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <random>

extern bool blur_break;

inline bool overlapped(const cv::Rect& rect1, const cv::Rect& rect2) {
    bool x_overlap = (rect1.x < rect2.x + rect2.width) && (rect1.x + rect1.width > rect2.x);
    bool y_overlap = (rect1.y < rect2.y + rect2.height) && (rect1.y + rect1.height > rect2.y);
    return x_overlap && y_overlap;
}

int newrand(int min, int max, int mid, float strength = 1.0f) {
    if (min > max) return min;

    // 静态随机数引擎和分布对象
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::normal_distribution<> d(mid, strength);

    int result;
    do {
        result = static_cast<int>(std::round(d(gen)));
    } while (result < min || result > max);

    return result;
}

extern float random_strength;
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
		arrlen = (int64_t)frame.cols * frame.rows * frame.cols * frame.rows * 8 / 180 / 800 / 600;
		srand(time(0));
		arr.clear();
		pos.clear();
		for(int64_t i=0; i<arrlen; i+=4)
		{
			int m1=rand()%frame.cols;
			int n1=rand()%frame.rows;
			arr.push_back(cv::Point2i{m1, n1});
			pos.push_back(1e10f);
			int m2=(m1 + 10 + rand()%300) % frame.cols;
			int n2=(n1 + 10 + rand()%300) % frame.rows;
			arr.push_back(cv::Point2i{m2,n2});
			pos.push_back(1e10f);

			int m3=newrand(0, frame.cols, arr[i].x, random_strength);
			int n3=newrand(0, frame.rows, arr[i].y, random_strength);
			arr.push_back(cv::Point2i{m3, n3});
			pos.push_back(1e10f);
			arr.push_back(cv::Point2i{m3, n3});
			pos.push_back(1e10f);
		}
	}
	else{
		for(int64_t i=0; i<arrlen; i+=4)
		{
			if(i%4 < 3)
			{
				if(rand() % 50 == 0)
				{
					arr[i].x = (arr[i].x + rand() % 3 - 1) % frame.cols;
					arr[i].y = (arr[i].y + rand() % 3 - 1) % frame.rows;
				}
			}else
			{
				int it=0;
				arr[i].x = arr[i-1].x + arr[i-2].x - arr[i-3].x;
				arr[i].y = arr[i-1].y + arr[i-2].y - arr[i-3].y;
				while(arr[i-1].x<0 || overlapped(rect, cv::Rect{arr[i-1], arr[i]}) ||
					arr[i].x >= frame.cols || arr[i].y >= frame.rows)
				{
					if(it++ > 10)
					{
						arr[i-1].x=-1;	arr[i-1].y=-1;
						arr[i].x  =-1;  arr[i-1].y=-1;
					}
					
					arr[i-1].x = newrand(0, frame.cols, arr[i-2].x, random_strength);
					arr[i-1].y = newrand(0, frame.rows, arr[i-2].y, random_strength);


					arr[i].x = arr[i-1].x + arr[i-2].x - arr[i-3].x;
					arr[i].y = arr[i-1].y + arr[i-2].y - arr[i-3].y;
				}
			}
		}
	}
	if(arr.size() >= 2)
	{
		arr[0].x = std::max(std::ceilf(rect.x / 30.0f) * 30, 0.0f);
		arr[0].y = std::max(std::ceilf(rect.y / 30.0f) * 30, 0.0f);
		arr[1].x = std::min(std::floorf((rect.x+rect.width) / 30.0f) * 30, (float)frame.cols);
		arr[1].y = std::min(std::floorf((rect.y+rect.height) / 30.0f) * 30, (float)frame.rows);
		
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

	// cv::rectangle(frame, rect, color, thickness);
	
	int j=0;
	for(int i=0; i<arr.size()-2; i+=4)
	{
		if(pos[i] < 100000 || i==0)
		{
			bool bad_flg = false;
			if(j++ > 20) break;
			cv::Rect2i r = {arr[i], arr[i+1]};
			int startx = arr[i+3].x;
			int starty = arr[i+3].y;
			if(startx<0 || starty < 0)
				bad_flg = true;
			auto color = frame_old.at<cv::Vec3b>({startx, starty});
			if(blur_break)
			{
				if(startx+r.width < frame.cols && starty+r.height < frame.rows)
				{
					frame_old(cv::Rect2i(cv::Point2i{startx, starty}, cv::Point2i{startx+r.width, starty+r.height}))
					.copyTo(frame(r));
				} else
					bad_flg = true;
			}
			if((!blur_break || bad_flg) && i==0)
				cv::rectangle(frame, r, color, -1);
		}
	}
	_last_time = _now_time;
    return 0;
}