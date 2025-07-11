#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <random>

extern bool blur_break;

inline bool overlapped(const cv::Rect rect1, const cv::Rect rect2) {
    int x1_start=rect1.x, x1_end=rect1.x + rect1.width;
    int x2_start=rect2.x, x2_end=rect2.x + rect2.width;
    int y1_start=rect1.y, y1_end=rect1.y + rect1.height;
    int y2_start=rect2.y, y2_end=rect2.y + rect2.height;
	return (!(x1_end<x2_start || x2_end<x1_start))
		&& (!(y1_end<y2_start || y2_end<y1_start));
}
template <typename T> inline void _bound(T& val, T min, T max)
{
	if(val < min) val=min;
	else if(val > max) val=max;
}
inline void _debug_cast(bool s)
{
	if(!s)
	{
		std::cout << "debug failed" << std::endl;
		exit(114);
	}
}


int newrand(int min, int max, int mid, int& _strength) {
	int strength = 200;
    if (min > max) return min;
	int res;
	do
	{
		int r=rand() % (strength*2) - strength;
		res = mid + r;
		strength += 5;
	}
	while(res<min || res>max);
	// _strength += 30;
	return res;
}

extern int random_strength;
int local_strength;
int blur(cv::Mat& frame, cv::Rect rect)
{	
	auto frame_old = frame.clone();
    rect = rect & cv::Rect(0, 0, frame.cols-1, frame.rows-1);
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

			int m3 = newrand(0, frame.cols, (arr[i].x+arr[i+1].x)/2, random_strength);
			int n3 = newrand(0, frame.rows, (arr[i].y+arr[i+1].y)/2, random_strength);
			arr.push_back(cv::Point2i{m3, n3});
			pos.push_back(1e10f);
			arr.push_back(cv::Point2i{m3, n3});
			pos.push_back(1e10f);
		}
	}
	{
		for(int64_t i=0; i<arrlen; i++)
		{
			if(i==0)
			{
				if(rect.x < 10) arr[0].x=0;
				else arr[0].x = std::max(std::ceilf(rect.x / 30.0f) * 30, 0.0f);
				if(rect.y < 10) arr[0].y=0;
				else arr[0].y = std::max(std::ceilf(rect.y / 30.0f) * 30, 0.0f);
			}
			else if(i==1)
			{
				if(rect.x+10 > frame.cols) arr[1].x=frame.cols-1;
				else arr[1].x = std::min((int)std::floorf((rect.x+rect.width) / 30.0f) * 30, frame.cols-1);
				if(rect.y+10 > frame.rows) arr[1].y=frame.rows-1;
				else arr[1].y = std::min((int)std::floorf((rect.y+rect.height) / 30.0f) * 30, frame.rows-1);
			}
			else if(i%4 < 3)
			{
				if(rand() % 50 == 0)
				{
					arr[i].x = (arr[i].x + rand() % 3 - 1);
					_bound(arr[i].x, 0, frame.cols-1);
					arr[i].y = (arr[i].y + rand() % 3 - 1);
					_bound(arr[i].y, 0, frame.rows-1);
				}
				if(i%4==1)
				{
					if(arr[i-1].x > arr[i].x) std::swap(arr[i-1].x, arr[i].x);
					if(arr[i-1].y > arr[i].y) std::swap(arr[i-1].y, arr[i].y);
				}
			}else
			{
				int it=0;
				arr[i].x = arr[i-1].x + arr[i-2].x - arr[i-3].x;
				arr[i].y = arr[i-1].y + arr[i-2].y - arr[i-3].y;
				local_strength = 30;
				while(arr[i-1].x==999999 || overlapped(rect, cv::Rect{arr[i-1], arr[i]}) ||
					arr[i].x >= frame.cols || arr[i].y >= frame.rows)
				{
					if(it++ )
					{
						arr[i-1].x=999999;	arr[i-1].y=999999;
						arr[i].x  =999999;  arr[i].y  =999999;
						break;
					}
					
					arr[i-1].x = newrand(0, frame.cols-1, (arr[i-2].x+arr[i-3].x)/2, local_strength);
					arr[i-1].y = newrand(0, frame.rows-1, (arr[i-2].y+arr[i-3].y)/2, local_strength);
					// arr[i-1].x = rand() % frame.cols;
					// arr[i-1].y = rand() % frame.rows;
					arr[i].x = arr[i-1].x + arr[i-2].x - arr[i-3].x;
					arr[i].y = arr[i-1].y + arr[i-2].y - arr[i-3].y;
				}
			}
		}
	}
	
	int rec_count = 0;
	for(int64_t i=0; i<arrlen; i+=2)
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
	for(int64_t i=0; i<arrlen; i+=4)
	{
		if(pos[i] < 100000 || i==0)
		{
			bool bad_flg = false;
			if(j > 18) break;
			cv::Rect2i r = {arr[i], arr[i+1]};
			int startx = arr[i+2].x;
			int starty = arr[i+2].y;
			

			if(startx<0 || startx==999999)
				bad_flg = true;
			auto color = frame_old.at<cv::Vec3b>({(arr[i].x+arr[i+1].x)/2, (arr[i].y+arr[i+1].y)/2});

			if(blur_break && !bad_flg)
			{
				if(startx+r.width < frame.cols && starty+r.height < frame.rows)
				{
					if(!(arr[i+1].x-arr[i].x) == (arr[i+3].x-arr[i+2].x))
					{
						std::cout << arr[i+1].x<<' '<<arr[i].x<<' '<<arr[i+3].x<<' '<<arr[i+2].x<<std::endl;
						// exit(114);
					}

					// _debug_cast((arr[i+1].x-arr[i].x) == (arr[i+3].x-arr[i+2].x));
					frame_old(cv::Rect2i(cv::Point2i{startx, starty}, cv::Point2i{startx+r.width, starty+r.height}))
					.copyTo(frame(r));
					j++;
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